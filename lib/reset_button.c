/**
 * @file reset_button.c
 * @brief Implementação do controle do botão de reset
 * @author Nova Instruments
 */

#include "reset_button.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gpiod.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>

#define RESET_BUTTON_GPIO 5  // GPIO 5 (pino físico 29)
#define GPIO_CHIP "gpiochip0"

static struct gpiod_chip *chip = NULL;
static struct gpiod_line *line = NULL;

/**
 * @brief Inicializa o monitoramento do botão de reset
 */
int reset_button_init(void) {
    // Abrir chip GPIO
    chip = gpiod_chip_open_by_name(GPIO_CHIP);
    if (!chip) {
        fprintf(stderr, "❌ Erro ao abrir GPIO chip para botão de reset\n");
        return -1;
    }

    // Obter linha GPIO
    line = gpiod_chip_get_line(chip, RESET_BUTTON_GPIO);
    if (!line) {
        fprintf(stderr, "❌ Erro ao obter linha GPIO %d para botão de reset\n", RESET_BUTTON_GPIO);
        gpiod_chip_close(chip);
        chip = NULL;
        return -1;
    }

    // Configurar como entrada com pull-up (botão conecta ao GND)
    if (gpiod_line_request_input(line, "reset_button") < 0) {
        fprintf(stderr, "❌ Erro ao configurar GPIO %d como entrada\n", RESET_BUTTON_GPIO);
        gpiod_chip_close(chip);
        chip = NULL;
        line = NULL;
        return -1;
    }

    printf("🔘 Botão de reset inicializado no GPIO %d (pino físico 29)\n", RESET_BUTTON_GPIO);
    printf("💡 Pressione o botão (GPIO5 → GND) para apagar todos os logs\n");
    
    return 0;
}

/**
 * @brief Verifica se o botão de reset foi pressionado
 */
bool reset_button_is_pressed(void) {
    if (!line) {
        return false;
    }

    // Ler valor do GPIO (0 = pressionado, pois conecta ao GND)
    int value = gpiod_line_get_value(line);
    return (value == 0);
}

/**
 * @brief Finaliza o monitoramento do botão de reset
 */
void reset_button_cleanup(void) {
    if (line) {
        gpiod_line_release(line);
        line = NULL;
    }
    
    if (chip) {
        gpiod_chip_close(chip);
        chip = NULL;
    }
    
    printf("🔘 Botão de reset finalizado\n");
}

/**
 * @brief Apaga todos os arquivos de datalogger
 */
bool reset_delete_all_logs(const char* log_dir) {
    DIR *dir;
    struct dirent *entry;
    char filepath[512];
    int deleted_count = 0;
    int error_count = 0;

    printf("\n🗑️  INICIANDO LIMPEZA DE LOGS...\n");
    printf("📁 Diretório: %s\n", log_dir);

    // Abrir diretório
    dir = opendir(log_dir);
    if (!dir) {
        fprintf(stderr, "❌ Erro ao abrir diretório: %s\n", log_dir);
        return false;
    }

    // Percorrer todos os arquivos
    while ((entry = readdir(dir)) != NULL) {
        // Ignorar . e ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Verificar se é arquivo .db ou .txt (começando com NI)
        size_t len = strlen(entry->d_name);
        bool is_db = (len > 3 && strcmp(entry->d_name + len - 3, ".db") == 0);
        bool is_txt = (len > 4 && strcmp(entry->d_name + len - 4, ".txt") == 0);
        bool starts_with_ni = (len > 2 && entry->d_name[0] == 'N' && entry->d_name[1] == 'I');

        if ((is_db || is_txt) && starts_with_ni) {
            // Construir caminho completo
            snprintf(filepath, sizeof(filepath), "%s/%s", log_dir, entry->d_name);

            // Tentar apagar arquivo
            if (remove(filepath) == 0) {
                printf("  ✅ Removido: %s\n", entry->d_name);
                deleted_count++;
            } else {
                fprintf(stderr, "  ❌ Erro ao remover: %s\n", entry->d_name);
                error_count++;
            }
        }
    }

    closedir(dir);

    printf("\n📊 Resultado da limpeza:\n");
    printf("  ✅ Arquivos removidos: %d\n", deleted_count);
    if (error_count > 0) {
        printf("  ❌ Erros: %d\n", error_count);
    }
    printf("🗑️  LIMPEZA CONCLUÍDA!\n\n");

    return (error_count == 0);
}

