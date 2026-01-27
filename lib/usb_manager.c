#include "usb_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <errno.h>
#include <time.h>
#include <libudev.h>
#include <mntent.h>
#include <sys/mount.h>
#include <gpiod.h>

// Definir MNT_FORCE se não estiver definido
#ifndef MNT_FORCE
#define MNT_FORCE 1
#endif

// Configurações do buzzer
#define BUZZER_GPIO 23
#define GPIO_CHIP_NAME "gpiochip0"

// Estrutura usb_device_info_t definida no header

// Variáveis globais
static struct udev *udev_context = NULL;
static usb_device_info_t current_usb = {0};
static struct gpiod_chip *gpio_chip = NULL;
static struct gpiod_line *buzzer_line = NULL;

// Função para inicializar o contexto udev
int usb_manager_init(void) {
    if (udev_context) {
        printf("USB Manager já inicializado\n");
        return 0;
    }

    udev_context = udev_new();
    if (!udev_context) {
        printf("Erro: Não foi possível criar contexto udev\n");
        return -1;
    }

    // Inicializar buzzer
    if (buzzer_init() != 0) {
        printf("⚠️  Aviso: Falha ao inicializar buzzer (continuando sem sinalização sonora)\n");
    }

    printf("USB Manager inicializado com sucesso\n");
    return 0;
}

// Função para finalizar o contexto udev
void usb_manager_cleanup(void) {
    // Finalizar buzzer
    buzzer_cleanup();

    if (udev_context) {
        udev_unref(udev_context);
        udev_context = NULL;
        printf("USB Manager finalizado\n");
    }
}

// Função para verificar se um dispositivo está montado
static bool is_device_mounted(const char* device_path, char* mount_point, size_t mount_point_size) {
    FILE *fp = fopen("/proc/mounts", "r");
    if (!fp) {
        printf("Erro ao abrir /proc/mounts\n");
        return false;
    }

    struct mntent *entry;
    while ((entry = getmntent(fp)) != NULL) {
        if (strcmp(entry->mnt_fsname, device_path) == 0) {
            if (mount_point && mount_point_size > 0) {
                strncpy(mount_point, entry->mnt_dir, mount_point_size - 1);
                mount_point[mount_point_size - 1] = '\0';
            }
            fclose(fp);
            return true;
        }
    }

    fclose(fp);
    return false;
}

// Função para criar ponto de montagem
static int create_mount_point(const char* mount_point) {
    struct stat st;
    
    if (stat(mount_point, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            printf("Ponto de montagem já existe: %s\n", mount_point);
            return 0;
        } else {
            printf("Erro: %s existe mas não é um diretório\n", mount_point);
            return -1;
        }
    }

    if (mkdir(mount_point, 0755) != 0) {
        printf("Erro ao criar ponto de montagem %s: %s\n", mount_point, strerror(errno));
        return -1;
    }

    printf("Ponto de montagem criado: %s\n", mount_point);
    return 0;
}

// Função para montar dispositivo USB
static int mount_usb_device(const char* device_path, const char* mount_point) {
    printf("Tentando montar %s em %s\n", device_path, mount_point);

    // Criar ponto de montagem se não existir
    if (create_mount_point(mount_point) != 0) {
        return -1;
    }

    // Tentar montagem com diferentes sistemas de arquivos
    const char* fs_types[] = {"vfat", "exfat", "ntfs", "ext4", "ext3", "ext2", NULL};
    
    for (int i = 0; fs_types[i] != NULL; i++) {
        printf("Tentando montar como %s...\n", fs_types[i]);
        
        if (mount(device_path, mount_point, fs_types[i], MS_NOATIME, NULL) == 0) {
            printf("USB montado com sucesso como %s\n", fs_types[i]);
            strncpy(current_usb.fs_type, fs_types[i], sizeof(current_usb.fs_type) - 1);
            current_usb.fs_type[sizeof(current_usb.fs_type) - 1] = '\0';
            return 0;
        }
        
        printf("Falha ao montar como %s: %s\n", fs_types[i], strerror(errno));
    }

    printf("Erro: Não foi possível montar o dispositivo USB\n");
    return -1;
}

// Função para obter informações do dispositivo usando udev
static int get_device_info(const char* device_path, usb_device_info_t* info) {
    if (!udev_context || !device_path || !info) {
        return -1;
    }

    struct stat st;
    if (stat(device_path, &st) != 0) {
        printf("Erro: Dispositivo %s não encontrado\n", device_path);
        return -1;
    }

    struct udev_device *dev = udev_device_new_from_devnum(udev_context, 'b', st.st_rdev);
    if (!dev) {
        printf("Erro: Não foi possível obter informações do dispositivo\n");
        return -1;
    }

    // Obter informações do dispositivo pai (USB)
    struct udev_device *parent = udev_device_get_parent_with_subsystem_devtype(dev, "usb", "usb_device");
    if (parent) {
        const char* vendor = udev_device_get_sysattr_value(parent, "manufacturer");
        const char* model = udev_device_get_sysattr_value(parent, "product");
        
        if (vendor) {
            strncpy(info->vendor, vendor, sizeof(info->vendor) - 1);
            info->vendor[sizeof(info->vendor) - 1] = '\0';
        } else {
            strcpy(info->vendor, "Desconhecido");
        }
        
        if (model) {
            strncpy(info->model, model, sizeof(info->model) - 1);
            info->model[sizeof(info->model) - 1] = '\0';
        } else {
            strcpy(info->model, "Desconhecido");
        }
    }

    // Obter tamanho do dispositivo
    const char* size_str = udev_device_get_sysattr_value(dev, "size");
    if (size_str) {
        unsigned long long size_sectors = strtoull(size_str, NULL, 10);
        info->size_mb = (size_sectors * 512) / (1024 * 1024); // Converter para MB
    }

    udev_device_unref(dev);
    return 0;
}

// Função para detectar dispositivos USB removíveis
int detect_usb_devices(usb_device_info_t* devices, int max_devices) {
    if (!udev_context) {
        printf("Erro: USB Manager não inicializado\n");
        return -1;
    }

    struct udev_enumerate *enumerate = udev_enumerate_new(udev_context);
    if (!enumerate) {
        printf("Erro: Não foi possível criar enumerador udev\n");
        return -1;
    }

    // Filtrar apenas dispositivos de bloco
    udev_enumerate_add_match_subsystem(enumerate, "block");
    udev_enumerate_add_match_property(enumerate, "DEVTYPE", "partition");
    udev_enumerate_scan_devices(enumerate);

    struct udev_list_entry *devices_list = udev_enumerate_get_list_entry(enumerate);
    struct udev_list_entry *entry;
    int device_count = 0;

    udev_list_entry_foreach(entry, devices_list) {
        if (device_count >= max_devices) {
            break;
        }

        const char *path = udev_list_entry_get_name(entry);
        struct udev_device *dev = udev_device_new_from_syspath(udev_context, path);
        
        if (!dev) {
            continue;
        }

        // Verificar se é um dispositivo USB removível
        struct udev_device *parent = udev_device_get_parent_with_subsystem_devtype(dev, "usb", "usb_device");
        if (!parent) {
            udev_device_unref(dev);
            continue;
        }

        // Verificar se é removível
        const char *removable = udev_device_get_sysattr_value(dev, "removable");
        if (!removable || strcmp(removable, "1") != 0) {
            // Verificar no dispositivo pai
            struct udev_device *block_parent = udev_device_get_parent_with_subsystem_devtype(dev, "block", "disk");
            if (block_parent) {
                removable = udev_device_get_sysattr_value(block_parent, "removable");
            }
        }

        if (removable && strcmp(removable, "1") == 0) {
            const char *devnode = udev_device_get_devnode(dev);
            if (devnode) {
                usb_device_info_t *info = &devices[device_count];
                memset(info, 0, sizeof(usb_device_info_t));
                
                strncpy(info->device_path, devnode, sizeof(info->device_path) - 1);
                info->device_path[sizeof(info->device_path) - 1] = '\0';
                
                // Verificar se já está montado
                info->is_mounted = is_device_mounted(devnode, info->mount_point, sizeof(info->mount_point));
                
                // Obter informações adicionais
                get_device_info(devnode, info);
                
                printf("USB encontrado: %s (%s %s, %lu MB)\n", 
                       devnode, info->vendor, info->model, info->size_mb);
                
                device_count++;
            }
        }

        udev_device_unref(dev);
    }

    udev_enumerate_unref(enumerate);
    return device_count;
}

// Função para montar dispositivo USB automaticamente
int mount_usb_device_auto(usb_device_info_t* device_info) {
    if (!device_info) {
        return USB_ERROR_INVALID_PARAM;
    }

    // Se já está montado, verificar se ainda é acessível
    if (device_info->is_mounted) {
        if (access(device_info->mount_point, W_OK) == 0) {
            printf("USB já montado e acessível: %s\n", device_info->mount_point);
            return USB_SUCCESS;
        } else {
            printf("USB montado mas não acessível, tentando remontar...\n");
            device_info->is_mounted = false;
        }
    }

    // Definir ponto de montagem padrão
    if (strlen(device_info->mount_point) == 0) {
        snprintf(device_info->mount_point, sizeof(device_info->mount_point), "/media/usb_%s",
                 strrchr(device_info->device_path, '/') ? strrchr(device_info->device_path, '/') + 1 : "unknown");
    }

    // Tentar montar
    if (mount_usb_device(device_info->device_path, device_info->mount_point) == 0) {
        device_info->is_mounted = true;

        // Verificar espaço disponível
        struct statvfs stat;
        if (statvfs(device_info->mount_point, &stat) == 0) {
            unsigned long available_mb = (stat.f_bavail * stat.f_frsize) / (1024 * 1024);
            printf("USB montado com %lu MB livres\n", available_mb);
        }

        return USB_SUCCESS;
    }

    return USB_ERROR_MOUNT_FAILED;
}

// Função para desmontar dispositivo USB
int unmount_usb_device(const char* mount_point) {
    if (!mount_point) {
        return USB_ERROR_INVALID_PARAM;
    }

    printf("Desmontando USB: %s\n", mount_point);

    // Forçar sincronização antes da desmontagem
    sync();
    usleep(100000); // 100ms

    // Tentar desmontagem normal primeiro
    if (umount(mount_point) == 0) {
        printf("USB desmontado com sucesso\n");

        // Tentar remover o diretório se estiver vazio
        if (rmdir(mount_point) == 0) {
            printf("Diretório de montagem removido: %s\n", mount_point);
        } else if (errno != ENOENT && errno != ENOTEMPTY) {
            printf("Aviso: Não foi possível remover diretório %s: %s\n", mount_point, strerror(errno));
        }

        return USB_SUCCESS;
    } else {
        printf("Desmontagem normal falhou (%s), tentando desmontagem forçada...\n", strerror(errno));

        // Tentar desmontagem forçada
        if (umount2(mount_point, MNT_FORCE) == 0) {
            printf("USB desmontado com desmontagem forçada\n");

            // Tentar remover o diretório
            if (rmdir(mount_point) == 0) {
                printf("Diretório de montagem removido: %s\n", mount_point);
            }

            return USB_SUCCESS;
        } else {
            printf("Erro ao desmontar USB (forçado): %s\n", strerror(errno));
            return USB_ERROR_MOUNT_FAILED;
        }
    }
}





// Função para obter tamanho do arquivo
static long get_file_size(const char* file_path) {
    FILE *file = fopen(file_path, "rb");
    if (!file) {
        return -1;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fclose(file);

    return size;
}

// Função para gerar nome único do arquivo de destino
static void generate_dest_filename(char* dest_path, size_t dest_size, const char* mount_point, const char* source_file) {
    // Usar valores fixos para máquina
    const char* machine_name = "NI";
    const char* serial_number = "000000";

    // Obter timestamp atual
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);

    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", tm_info);

    // Gerar nome do arquivo
    snprintf(dest_path, dest_size, "%s/%s_%s_%s.txt",
             mount_point, machine_name, serial_number, timestamp);
}

// Função para copiar arquivo com callback de progresso
int copy_log_to_usb(const usb_device_info_t* usb_device, const char* log_file_path, const usb_callbacks_t* callbacks) {
    if (!usb_device || !log_file_path) {
        if (callbacks && callbacks->on_error) {
            callbacks->on_error(USB_ERROR_INVALID_PARAM, "Parâmetros inválidos");
        }
        return USB_ERROR_INVALID_PARAM;
    }

    if (callbacks && callbacks->on_progress) {
        callbacks->on_progress(0, "Verificando arquivo de origem...");
    }

    // Verificar se arquivo de origem existe
    if (access(log_file_path, R_OK) != 0) {
        const char* error_msg = "Arquivo de log não encontrado ou não legível";
        printf("Erro: %s - %s\n", error_msg, log_file_path);
        if (callbacks && callbacks->on_error) {
            callbacks->on_error(USB_ERROR_NOT_FOUND, error_msg);
        }
        return USB_ERROR_NOT_FOUND;
    }

    // Obter tamanho do arquivo
    long file_size = get_file_size(log_file_path);
    if (file_size < 0) {
        const char* error_msg = "Erro ao obter tamanho do arquivo";
        printf("Erro: %s\n", error_msg);
        if (callbacks && callbacks->on_error) {
            callbacks->on_error(USB_ERROR_COPY_FAILED, error_msg);
        }
        return USB_ERROR_COPY_FAILED;
    }

    if (callbacks && callbacks->on_progress) {
        callbacks->on_progress(10, "Preparando cópia...");
    }

    if (callbacks && callbacks->on_progress) {
        callbacks->on_progress(20, "Preparando cópia...");
    }

    // Gerar nome do arquivo de destino
    char dest_file_path[512];
    generate_dest_filename(dest_file_path, sizeof(dest_file_path), usb_device->mount_point, log_file_path);

    printf("Copiando %s para %s\n", log_file_path, dest_file_path);

    // Abrir arquivos
    FILE *src = fopen(log_file_path, "rb");
    if (!src) {
        const char* error_msg = "Erro ao abrir arquivo de origem";
        printf("Erro: %s\n", error_msg);
        if (callbacks && callbacks->on_error) {
            callbacks->on_error(USB_ERROR_COPY_FAILED, error_msg);
        }
        return USB_ERROR_COPY_FAILED;
    }

    FILE *dst = fopen(dest_file_path, "wb");
    if (!dst) {
        const char* error_msg = "Erro ao criar arquivo de destino";
        printf("Erro: %s\n", error_msg);
        fclose(src);
        if (callbacks && callbacks->on_error) {
            callbacks->on_error(USB_ERROR_COPY_FAILED, error_msg);
        }
        return USB_ERROR_COPY_FAILED;
    }

    // Copiar arquivo com progresso
    char buffer[8192];
    size_t bytes_read;
    size_t total_copied = 0;
    int last_progress = 20;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        if (fwrite(buffer, 1, bytes_read, dst) != bytes_read) {
            const char* error_msg = "Erro durante a escrita no USB";
            printf("Erro: %s\n", error_msg);
            fclose(src);
            fclose(dst);
            unlink(dest_file_path); // Remover arquivo parcial
            if (callbacks && callbacks->on_error) {
                callbacks->on_error(USB_ERROR_COPY_FAILED, error_msg);
            }
            return USB_ERROR_COPY_FAILED;
        }

        total_copied += bytes_read;

        // Atualizar progresso
        if (callbacks && callbacks->on_progress) {
            int progress = 20 + (int)((total_copied * 70) / file_size);
            if (progress != last_progress && progress % 5 == 0) {
                char progress_msg[64];
                snprintf(progress_msg, sizeof(progress_msg), "Copiando arquivo... %d%%", progress - 20);
                callbacks->on_progress(progress, progress_msg);
                last_progress = progress;
            }
        }
    }

    // Fechar arquivos
    fclose(src);
    if (fclose(dst) != 0) {
        const char* error_msg = "Erro ao finalizar arquivo no USB";
        printf("Erro: %s\n", error_msg);
        unlink(dest_file_path);
        if (callbacks && callbacks->on_error) {
            callbacks->on_error(USB_ERROR_COPY_FAILED, error_msg);
        }
        return USB_ERROR_COPY_FAILED;
    }

    if (callbacks && callbacks->on_progress) {
        callbacks->on_progress(95, "Verificando arquivo copiado...");
    }

    // Verificar se o arquivo foi copiado corretamente
    long dest_size = get_file_size(dest_file_path);
    if (dest_size != file_size) {
        const char* error_msg = "Arquivo copiado com tamanho incorreto";
        printf("Erro: %s (origem: %ld bytes, destino: %ld bytes)\n", error_msg, file_size, dest_size);
        unlink(dest_file_path);
        if (callbacks && callbacks->on_error) {
            callbacks->on_error(USB_ERROR_COPY_FAILED, error_msg);
        }
        return USB_ERROR_COPY_FAILED;
    }

    printf("Arquivo copiado com sucesso: %s (%ld bytes)\n", dest_file_path, file_size);

    if (callbacks && callbacks->on_complete) {
        char success_msg[256];
        snprintf(success_msg, sizeof(success_msg), "Arquivo salvo como: %s", strrchr(dest_file_path, '/') + 1);
        callbacks->on_complete(USB_SUCCESS, success_msg);
    }

    return USB_SUCCESS;
}

// Função principal para extrair logs para USB
int extract_log_to_usb(const usb_callbacks_t* callbacks) {
    if (callbacks && callbacks->on_progress) {
        callbacks->on_progress(0, "Inicializando detecção USB...");
    }

    // Detectar dispositivos USB
    usb_device_info_t devices[5];
    int device_count = detect_usb_devices(devices, 5);

    if (device_count <= 0) {
        const char* error_msg = "Nenhum dispositivo USB encontrado";
        printf("Erro: %s\n", error_msg);
        if (callbacks && callbacks->on_error) {
            callbacks->on_error(USB_ERROR_NOT_FOUND, error_msg);
        }
        return USB_ERROR_NOT_FOUND;
    }

    if (callbacks && callbacks->on_progress) {
        callbacks->on_progress(20, "USB detectado, preparando montagem...");
    }

    // Usar o primeiro dispositivo encontrado
    usb_device_info_t* usb_device = &devices[0];

    printf("Usando USB: %s (%s %s)\n", usb_device->device_path, usb_device->vendor, usb_device->model);

    // Montar dispositivo se necessário
    if (!usb_device->is_mounted) {
        if (callbacks && callbacks->on_progress) {
            callbacks->on_progress(30, "Montando dispositivo USB...");
        }

        int mount_result = mount_usb_device_auto(usb_device);
        if (mount_result != USB_SUCCESS) {
            const char* error_msg = "Erro ao montar dispositivo USB";
            printf("Erro: %s\n", error_msg);
            if (callbacks && callbacks->on_error) {
                callbacks->on_error(mount_result, error_msg);
            }
            return mount_result;
        }
    }

    if (callbacks && callbacks->on_progress) {
        callbacks->on_progress(40, "USB montado, localizando arquivo de log...");
    }

    // Construir caminho do arquivo de log
    const char* machine_name = "NI";
    const char* serial_number = "000000";

    char log_file_path[256];
    snprintf(log_file_path, sizeof(log_file_path), "/home/nova/%s*.txt", machine_name);

    printf("Procurando arquivo de log: %s\n", log_file_path);

    // Verificar se arquivo de log existe
    if (access(log_file_path, R_OK) != 0) {
        const char* error_msg = "Arquivo de log não encontrado";
        printf("Erro: %s - %s\n", error_msg, log_file_path);
        if (callbacks && callbacks->on_error) {
            callbacks->on_error(USB_ERROR_NOT_FOUND, error_msg);
        }
        return USB_ERROR_NOT_FOUND;
    }

    if (callbacks && callbacks->on_progress) {
        callbacks->on_progress(50, "Arquivo de log encontrado, iniciando cópia...");
    }

    // Copiar arquivo para USB
    int copy_result = copy_log_to_usb(usb_device, log_file_path, callbacks);

    if (copy_result == USB_SUCCESS) {
        printf("Extração para USB concluída com sucesso!\n");
    } else {
        printf("Erro durante a extração para USB\n");
    }

    // Desmontagem automática após extração (opcional)
    if (copy_result == USB_SUCCESS) {
        if (callbacks && callbacks->on_progress) {
            callbacks->on_progress(100, "Desmontando USB...");
        }

        // Aguardar um pouco para garantir que a escrita foi finalizada
        sync(); // Força sincronização dos dados
        usleep(500000); // 500ms

        // Desmontar o dispositivo
        int unmount_result = unmount_usb_device(usb_device->mount_point);
        if (unmount_result == USB_SUCCESS) {
            printf("USB desmontado com sucesso após extração\n");
        } else {
            printf("Aviso: Não foi possível desmontar o USB automaticamente\n");
        }
    }

    return copy_result;
}

// Função para limpar pontos de montagem órfãos
int cleanup_orphaned_mount_points(void) {
    printf("Limpando pontos de montagem órfãos...\n");

    const char* media_paths[] = {
        "/media/usb",
        "/media/usb0",
        "/media/usb1",
        "/media/usb2",
        "/media/usb3",
        "/media/usb4",
        "/media/usb5",
        "/media/usb_sda1",
        "/media/usb_sdb1",
        "/media/usb_sdc1",
        "/media/usb_sdd1",
        "/media/usb_sde1",
        "/media/usb_sdf1",
        "/media/usb_sdg1",
        "/media/usb_sdh1",
        NULL
    };

    int cleaned_count = 0;

    for (int i = 0; media_paths[i] != NULL; i++) {
        const char* mount_point = media_paths[i];

        // Verificar se o diretório existe
        if (access(mount_point, F_OK) != 0) {
            continue; // Diretório não existe
        }

        // Verificar se está montado
        bool is_mounted = false;
        FILE *fp = fopen("/proc/mounts", "r");
        if (fp) {
            char line[512];
            while (fgets(line, sizeof(line), fp)) {
                if (strstr(line, mount_point)) {
                    is_mounted = true;
                    break;
                }
            }
            fclose(fp);
        }

        if (is_mounted) {
            // Tentar desmontar
            printf("Desmontando %s...\n", mount_point);
            if (umount(mount_point) == 0) {
                printf("Desmontado com sucesso: %s\n", mount_point);
                cleaned_count++;
            } else {
                printf("Erro ao desmontar %s: %s\n", mount_point, strerror(errno));
            }
        }

        // Verificar se o diretório está vazio e removê-lo
        if (!is_mounted || umount(mount_point) == 0) {
            if (rmdir(mount_point) == 0) {
                printf("Diretório removido: %s\n", mount_point);
            } else if (errno != ENOENT) {
                printf("Aviso: Não foi possível remover diretório %s: %s\n", mount_point, strerror(errno));
            }
        }
    }

    printf("Limpeza concluída. %d pontos de montagem processados.\n", cleaned_count);
    return cleaned_count;
}

// Função para forçar desmontagem de todos os USBs
int force_unmount_all_usb(void) {
    printf("Forçando desmontagem de todos os dispositivos USB...\n");

    int unmounted_count = 0;
    FILE *fp = fopen("/proc/mounts", "r");
    if (!fp) {
        printf("Erro ao abrir /proc/mounts\n");
        return -1;
    }

    char mount_points[20][256]; // Máximo 20 pontos de montagem
    int mount_count = 0;

    // Primeiro, coletar todos os pontos de montagem USB
    char line[512];
    while (fgets(line, sizeof(line), fp) && mount_count < 20) {
        char device[256], mount_point[256], fs_type[32];

        if (sscanf(line, "%255s %255s %31s", device, mount_point, fs_type) == 3) {
            // Verificar se é um ponto de montagem USB típico
            if (strstr(mount_point, "/media/usb") ||
                (strstr(device, "/dev/sd") && strstr(mount_point, "/media/"))) {
                strncpy(mount_points[mount_count], mount_point, sizeof(mount_points[mount_count]) - 1);
                mount_points[mount_count][sizeof(mount_points[mount_count]) - 1] = '\0';
                mount_count++;
                printf("Encontrado ponto de montagem USB: %s\n", mount_point);
            }
        }
    }
    fclose(fp);

    // Agora desmontar todos os pontos encontrados
    for (int i = 0; i < mount_count; i++) {
        printf("Desmontando %s...\n", mount_points[i]);

        // Tentar desmontagem normal primeiro
        if (umount(mount_points[i]) == 0) {
            printf("Desmontado com sucesso: %s\n", mount_points[i]);
            unmounted_count++;
        } else {
            printf("Desmontagem normal falhou, tentando desmontagem forçada...\n");

            // Tentar desmontagem forçada
            if (umount2(mount_points[i], MNT_FORCE) == 0) {
                printf("Desmontagem forçada bem-sucedida: %s\n", mount_points[i]);
                unmounted_count++;
            } else {
                printf("Erro ao desmontar %s: %s\n", mount_points[i], strerror(errno));
            }
        }

        // Tentar remover o diretório se estiver vazio
        if (rmdir(mount_points[i]) == 0) {
            printf("Diretório removido: %s\n", mount_points[i]);
        }
    }

    printf("Desmontagem forçada concluída. %d de %d pontos desmontados.\n", unmounted_count, mount_count);
    return unmounted_count;
}

/**
 * @brief Extração automática completa de todos os logs para USB
 */
int usb_auto_extract_all_logs(const char* source_dir, const usb_callbacks_t* callbacks) {
    if (!source_dir) {
        if (callbacks && callbacks->on_error) {
            callbacks->on_error(USB_ERROR_INVALID_PARAM, "Diretório de origem inválido");
        }
        return USB_ERROR_INVALID_PARAM;
    }

    if (callbacks && callbacks->on_progress) {
        callbacks->on_progress(10, "Detectando dispositivos USB...");
    }

    // Detectar dispositivos USB
    usb_device_info_t devices[5];
    int device_count = detect_usb_devices(devices, 5);

    if (device_count <= 0) {
        if (callbacks && callbacks->on_error) {
            callbacks->on_error(USB_ERROR_NOT_FOUND, "Nenhum dispositivo USB encontrado");
        }
        return USB_ERROR_NOT_FOUND;
    }

    // Usar o primeiro dispositivo encontrado
    usb_device_info_t* usb_device = &devices[0];

    if (callbacks && callbacks->on_progress) {
        callbacks->on_progress(20, "Montando dispositivo USB...");
    }

    // Montar dispositivo se necessário
    if (!usb_device->is_mounted) {
        int mount_result = mount_usb_device_auto(usb_device);
        if (mount_result != 0) {
            if (callbacks && callbacks->on_error) {
                callbacks->on_error(USB_ERROR_MOUNT_FAILED, "Falha ao montar dispositivo USB");
            }
            return USB_ERROR_MOUNT_FAILED;
        }
    }

    if (callbacks && callbacks->on_progress) {
        callbacks->on_progress(30, "Copiando bancos de dados...");
    }

    // Copiar apenas arquivos de banco do DataLogger (padrão: NI*.db)
    char command[1024];
    snprintf(command, sizeof(command),
             "find \"%s\" -name \"NI*.db\" -type f -exec cp {} \"%s/\" \\; 2>/dev/null",
             source_dir, usb_device->mount_point);

    int copy_result = system(command);

    // Verificar quantos arquivos foram copiados
    char count_command[1024];
    snprintf(count_command, sizeof(count_command),
             "find \"%s\" -name \"NI*.db\" -type f | wc -l",
             usb_device->mount_point);

    FILE* count_fp = popen(count_command, "r");
    int file_count = 0;
    if (count_fp) {
        fscanf(count_fp, "%d", &file_count);
        pclose(count_fp);
    }

    if (callbacks && callbacks->on_progress) {
        char progress_msg[256];
        snprintf(progress_msg, sizeof(progress_msg),
                 "Sincronizando dados... (%d bancos copiados)", file_count);
        callbacks->on_progress(80, progress_msg);
    }

    // Sincronizar dados
    sync();
    sleep(1);

    if (callbacks && callbacks->on_progress) {
        callbacks->on_progress(90, "Desmontando dispositivo USB...");
    }

    // Desmontar dispositivo
    int unmount_result = unmount_usb_device(usb_device->mount_point);

    if (callbacks && callbacks->on_progress) {
        callbacks->on_progress(100, "Extração concluída com sucesso!");
    }

    if (copy_result == 0 && unmount_result == 0) {
        if (callbacks && callbacks->on_complete) {
            char complete_msg[256];
            snprintf(complete_msg, sizeof(complete_msg),
                     "%d bancos de dados extraídos com sucesso para USB", file_count);
            callbacks->on_complete(USB_SUCCESS, complete_msg);
        }

        // Sinalizar sucesso com buzzer
        buzzer_signal_extraction_complete();

        return USB_SUCCESS;
    } else {
        if (callbacks && callbacks->on_error) {
            callbacks->on_error(USB_ERROR_COPY_FAILED, "Erro durante cópia ou desmontagem");
        }
        return USB_ERROR_COPY_FAILED;
    }
}

/**
 * @brief Monitora continuamente inserção de pen drives para extração automática
 */
void usb_monitor_and_extract(const char* source_dir, volatile bool* running, const usb_callbacks_t* callbacks) {
    if (!source_dir || !running) {
        if (callbacks && callbacks->on_error) {
            callbacks->on_error(USB_ERROR_INVALID_PARAM, "Parâmetros inválidos para monitoramento");
        }
        return;
    }

    printf("🔍 Iniciando monitoramento de pen drives para extração automática...\n");
    printf("📁 Diretório de logs: %s\n", source_dir);
    printf("💡 Insira um pen drive para iniciar extração automática\n");

    time_t last_check = 0;
    bool last_usb_detected = false;

    while (*running) {
        time_t current_time = time(NULL);

        // Verificar a cada 3 segundos
        if (current_time - last_check >= 3) {
            last_check = current_time;

            // Detectar dispositivos USB
            usb_device_info_t devices[5];
            int device_count = detect_usb_devices(devices, 5);

            bool usb_detected = (device_count > 0);

            // Se USB foi inserido (transição de não detectado para detectado)
            if (usb_detected && !last_usb_detected) {
                printf("\n🔌 Pen drive detectado! Iniciando extração automática...\n");

                // Aguardar um pouco para estabilizar
                sleep(2);

                // Executar extração automática
                int result = usb_auto_extract_all_logs(source_dir, callbacks);

                if (result == USB_SUCCESS) {
                    printf("✅ Extração concluída com sucesso!\n");
                    printf("💡 Pen drive pode ser removido com segurança\n");
                } else {
                    printf("❌ Erro durante extração (código: %d)\n", result);
                }

                printf("💡 Aguardando próximo pen drive...\n");
            }

            last_usb_detected = usb_detected;
        }

        // Aguardar 1 segundo antes da próxima verificação
        sleep(1);
    }

    printf("🛑 Monitoramento de pen drives finalizado\n");
}

/**
 * @brief Inicializa o buzzer no GPIO23 usando libgpiod
 */
int buzzer_init(void) {
    if (gpio_chip && buzzer_line) {
        printf("Buzzer já inicializado\n");
        return 0;
    }

    // Abrir chip GPIO
    gpio_chip = gpiod_chip_open_by_name(GPIO_CHIP_NAME);
    if (!gpio_chip) {
        printf("Erro ao abrir chip GPIO: %s\n", GPIO_CHIP_NAME);
        return -1;
    }

    // Obter linha do GPIO23
    buzzer_line = gpiod_chip_get_line(gpio_chip, BUZZER_GPIO);
    if (!buzzer_line) {
        printf("Erro ao obter linha GPIO %d\n", BUZZER_GPIO);
        gpiod_chip_close(gpio_chip);
        gpio_chip = NULL;
        return -1;
    }

    // Configurar linha como saída
    int ret = gpiod_line_request_output(buzzer_line, "buzzer", 0);
    if (ret < 0) {
        printf("Erro ao configurar GPIO %d como saída\n", BUZZER_GPIO);
        gpiod_chip_close(gpio_chip);
        gpio_chip = NULL;
        buzzer_line = NULL;
        return -1;
    }

    printf("🔊 Buzzer inicializado no GPIO %d\n", BUZZER_GPIO);
    return 0;
}

/**
 * @brief Finaliza o buzzer e libera recursos
 */
void buzzer_cleanup(void) {
    if (buzzer_line) {
        // Garantir que o buzzer está desligado
        gpiod_line_set_value(buzzer_line, 0);
        gpiod_line_release(buzzer_line);
        buzzer_line = NULL;
    }

    if (gpio_chip) {
        gpiod_chip_close(gpio_chip);
        gpio_chip = NULL;
    }

    printf("🔊 Buzzer finalizado\n");
}

/**
 * @brief Toca o buzzer para sinalizar sucesso na extração
 * Executa sequência de 3 beeps curtos
 */
void buzzer_signal_extraction_complete(void) {
    if (!buzzer_line) {
        printf("⚠️  Buzzer não inicializado, pulando sinalização sonora\n");
        return;
    }

    printf("🔊 Sinalizando extração concluída...\n");

    // Sequência de 3 beeps curtos
    for (int i = 0; i < 3; i++) {
        // Ligar buzzer
        gpiod_line_set_value(buzzer_line, 1);
        usleep(200000); // 200ms ligado

        // Desligar buzzer
        gpiod_line_set_value(buzzer_line, 0);
        usleep(200000); // 200ms desligado
    }

    printf("🔊 Sinalização sonora concluída\n");
}

/**
 * @brief Toca o buzzer para sinalizar erro de comunicação Modbus
 * Executa 1 beep longo (500ms)
 */
void buzzer_signal_modbus_error(void) {
    if (!buzzer_line) {
        // Não exibir mensagem para não poluir o log
        return;
    }

    // 1 beep longo para indicar erro
    gpiod_line_set_value(buzzer_line, 1);
    usleep(500000); // 500ms ligado

    gpiod_line_set_value(buzzer_line, 0);
}
