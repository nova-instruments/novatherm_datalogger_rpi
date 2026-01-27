/**
 * @file main.c
 * @brief COEL E33 DataLogger RPi - Main Application
 * @author Nova Instruments
 */

/* Upgrades

- Oled
- Botao p/ temporização de lampada
- Minima e maxima no datalogger
- 

*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include "modbus.h"
#include "datalogger.h"
#include "usb_manager.h"
#include "relay_control.h"
#include "reset_button.h"
#include "oled_ssd1306.h"

// Configurações da aplicação
#define LOOP_INTERVAL_SECONDS 300  // 5 minutos = 300 segundos
#define DEFAULT_DEVICE_NAME "NI00002"  // Nome padrão do dispositivo
#define CONFIG_FILE "/boot/firmware/config.txt"  // Arquivo de configuração do sistema

// ⚙️ CONFIGURAÇÃO: Alarme de falta de comunicação Modbus
// true  = Buzzer emite alarme quando há erro de comunicação com COEL
// false = Buzzer desabilitado para erros Modbus (silencioso)
static const bool ENABLE_MODBUS_ERROR_ALARM = false;

// Variável global para controle do loop principal
static volatile bool running = true;

// Estrutura para passar dados para thread USB
typedef struct {
    const char* source_dir;
    volatile bool* running;
} usb_thread_data_t;

/**
 * @brief Lê o nome do dispositivo do final do arquivo /boot/firmware/config.txt
 * Procura por uma linha começando com "DEVICE_NAME=" no final do arquivo
 * @param device_name Buffer para armazenar o nome do dispositivo
 * @param buffer_size Tamanho do buffer
 * @return true se leu com sucesso, false caso contrário
 */
static bool read_device_name_from_config(char* device_name, size_t buffer_size) {
    FILE* config_file = fopen(CONFIG_FILE, "r");
    if (!config_file) {
        fprintf(stderr, "⚠️  Arquivo de configuração '%s' não encontrado\n", CONFIG_FILE);
        fprintf(stderr, "⚠️  Usando nome padrão: %s\n", DEFAULT_DEVICE_NAME);
        return false;
    }

    char line[256];
    bool found = false;
    char last_device_name[256] = {0};

    // Ler todo o arquivo procurando por "DEVICE_NAME="
    // Se houver múltiplas ocorrências, a última prevalece
    while (fgets(line, sizeof(line), config_file)) {
        // Remover espaços em branco no início
        char* start = line;
        while (*start == ' ' || *start == '\t') start++;

        // Verificar se a linha começa com "DEVICE_NAME="
        if (strncmp(start, "DEVICE_NAME=", 12) == 0) {
            char* value = start + 12;

            // Remover espaços em branco no início do valor
            while (*value == ' ' || *value == '\t') value++;

            // Remover quebra de linha e espaços no final
            size_t len = strlen(value);
            while (len > 0 && (value[len-1] == '\n' || value[len-1] == '\r' ||
                               value[len-1] == ' ' || value[len-1] == '\t')) {
                value[len-1] = '\0';
                len--;
            }

            // Armazenar valor se não estiver vazio
            if (len > 0 && len < sizeof(last_device_name)) {
                strncpy(last_device_name, value, sizeof(last_device_name) - 1);
                last_device_name[sizeof(last_device_name) - 1] = '\0';
                found = true;
            }
        }
    }

    fclose(config_file);

    // Copiar o último valor encontrado para o buffer de saída
    if (found && strlen(last_device_name) < buffer_size) {
        strncpy(device_name, last_device_name, buffer_size - 1);
        device_name[buffer_size - 1] = '\0';
        return true;
    }

    return false;
}

/**
 * @brief Handler para sinais (SIGINT, SIGTERM)
 */
static void signal_handler(int sig) {
    printf("\nSinal %d recebido. Finalizando aplicação...\n", sig);
    running = false;
}

/**
 * @brief Callbacks para operações USB
 */
void usb_on_progress(int percentage, const char* message) {
    printf("📦 USB [%d%%]: %s\n", percentage, message);
}

void usb_on_complete(usb_result_t result, const char* message) {
    printf("✅ USB: %s\n", message);
}

void usb_on_error(usb_result_t error, const char* message) {
    printf("❌ USB Erro [%d]: %s\n", error, message);
}

/**
 * @brief Thread para monitoramento de pen drives
 */
void* usb_monitor_thread(void* arg) {
    usb_thread_data_t* data = (usb_thread_data_t*)arg;

    // Configurar callbacks
    usb_callbacks_t callbacks = {
        .on_progress = usb_on_progress,
        .on_complete = usb_on_complete,
        .on_error = usb_on_error
    };

    // Inicializar USB Manager
    if (usb_manager_init() != 0) {
        printf("❌ Erro ao inicializar USB Manager\n");
        return NULL;
    }

    // Monitorar pen drives
    usb_monitor_and_extract(data->source_dir, data->running, &callbacks);

    // Cleanup
    usb_manager_cleanup();

    return NULL;
}

/**
 * @brief Configura handlers de sinais para saída graceful
 */
static void setup_signal_handlers(void) {
    signal(SIGINT, signal_handler);   // Ctrl+C
    signal(SIGTERM, signal_handler);  // Termination signal
}

/**
 * @brief Função principal da aplicação
 */
int main(void) {
    printf("=== COEL E33 DataLogger RPi ===\n");
    printf("Nova Instruments\n\n");

    // Ler nome do dispositivo do arquivo de configuração
    char device_name[32];
    if (read_device_name_from_config(device_name, sizeof(device_name))) {
        printf("✅ Nome do dispositivo lido do arquivo '%s': %s\n", CONFIG_FILE, device_name);
    } else {
        printf("⚠️  Usando nome padrão do dispositivo: %s\n", DEFAULT_DEVICE_NAME);
        strncpy(device_name, DEFAULT_DEVICE_NAME, sizeof(device_name) - 1);
        device_name[sizeof(device_name) - 1] = '\0';
    }
    printf("Dispositivo: %s\n\n", device_name);

    // Configurar handlers de sinais
    setup_signal_handlers();

    // Inicializar conexão Modbus
    modbus_context_t* modbus_ctx = modbus_init();
    if (!modbus_ctx) {
        fprintf(stderr, "Erro: Falha ao inicializar Modbus\n");
        return EXIT_FAILURE;
    }

    // Inicializar DataLogger
    datalogger_context_t* datalogger_ctx = datalogger_init(device_name);
    if (!datalogger_ctx) {
        fprintf(stderr, "Erro: Falha ao inicializar DataLogger\n");
        modbus_cleanup(modbus_ctx);
        return EXIT_FAILURE;
    }

    // Inicializar controle de relés
    if (relay_init() != 0) {
        fprintf(stderr, "⚠️  Aviso: Falha ao inicializar relés (continuando sem controle de relés)\n");
    } else {
        printf("✅ Controle de relés ativo\n");

        // Acionar compressor e resistência na inicialização
        relay_compressor_on();
        relay_heater_on();
        printf("⚡ Compressor e resistência acionados automaticamente\n");
    }

    // Inicializar botão de reset
    if (reset_button_init() != 0) {
        printf("⚠️  Aviso: Falha ao inicializar botão de reset (continuando sem esta funcionalidade)\n");
    } else {
        printf("✅ Botão de reset ativo\n");
    }

    // Inicializar display OLED
    oled_context_t* oled_ctx = oled_init();
    if (!oled_ctx) {
        printf("⚠️  Aviso: Falha ao inicializar display OLED (continuando sem display)\n");
    } else {
        printf("✅ Display OLED ativo\n");
        // Exibir tela de splash
        oled_display_splash(oled_ctx, device_name);
        sleep(2);  // Mostrar splash por 2 segundos
    }

    // Inicializar thread de monitoramento USB
    pthread_t usb_thread;
    usb_thread_data_t usb_data = {
        .source_dir = "/home/nova",
        .running = &running
    };

    printf("🔌 Iniciando monitoramento de pen drives para extração automática...\n");
    if (pthread_create(&usb_thread, NULL, usb_monitor_thread, &usb_data) != 0) {
        printf("⚠️  Aviso: Falha ao iniciar monitoramento USB (continuando sem esta funcionalidade)\n");
    } else {
        printf("✅ Monitoramento USB ativo\n");
    }

    printf("\nIniciando loop de aquisição de dados (intervalo: %d segundos = %d minutos)\n",
           LOOP_INTERVAL_SECONDS, LOOP_INTERVAL_SECONDS / 60);
    printf("Pressione Ctrl+C para finalizar\n");
    printf("🔘 Pressione o botão de reset (GPIO5 → GND) para apagar todos os logs\n\n");

    // Loop principal de aquisição e logging
    // Estado anterior da porta (inicializar com valor inválido)
    bool previous_door_state_valid = false;
    uint16_t previous_door_state = 0;
    uint32_t door_change_logs = 0;

    // Estado anterior do alarme (inicializar com valor inválido)
    bool previous_alarm_state_valid = false;
    uint16_t previous_alarm_state = 0;
    uint32_t alarm_change_logs = 0;

    // Controle de tempo para log periódico
    time_t last_periodic_log = time(NULL);

    while (running) {
        // Verificar botão de reset
        if (reset_button_is_pressed()) {
            printf("\n🔘 BOTÃO DE RESET PRESSIONADO!\n");
            printf("⚠️  Aguarde 3 segundos para confirmar...\n");

            // Aguardar 3 segundos para confirmar (evitar acionamento acidental)
            sleep(3);

            // Verificar novamente se ainda está pressionado
            if (reset_button_is_pressed()) {
                printf("🗑️  CONFIRMADO! Apagando todos os logs...\n\n");

                // Apagar todos os logs
                if (reset_delete_all_logs("/home/nova")) {
                    printf("✅ Todos os logs foram apagados com sucesso!\n");

                    // Sinalizar com buzzer (5 beeps longos)
                    printf("🔊 Sinalizando limpeza de logs...\n");
                    buzzer_signal_extraction_complete();
                    sleep(1);
                    buzzer_signal_extraction_complete();
                    printf("🔊 Sinalização concluída\n\n");

                    printf("🔄 Reiniciando sistema de logging...\n\n");

                    // Reiniciar datalogger para criar novos arquivos
                    datalogger_cleanup(datalogger_ctx);
                    datalogger_ctx = datalogger_init(device_name);
                    if (!datalogger_ctx) {
                        fprintf(stderr, "❌ Erro ao reiniciar DataLogger\n");
                        running = false;
                        break;
                    }

                    // Resetar contadores
                    door_change_logs = 0;
                    alarm_change_logs = 0;
                    last_periodic_log = time(NULL);

                    printf("✅ Sistema de logging reiniciado!\n\n");
                } else {
                    fprintf(stderr, "❌ Erro ao apagar logs\n\n");
                }

                // Aguardar soltar o botão
                printf("💡 Solte o botão de reset...\n");
                while (reset_button_is_pressed() && running) {
                    sleep(1);
                }
                printf("✅ Botão liberado. Continuando operação normal.\n\n");
            } else {
                printf("❌ Cancelado (botão não mantido pressionado)\n\n");
            }
        }

        modbus_data_t data;
        bool should_log = false;
        bool is_door_change = false;
        bool is_alarm_change = false;

        printf("Lendo registradores Modbus...\n");

        if (modbus_read_all(modbus_ctx, &data)) {
            // ✅ LEITURA BEM-SUCEDIDA - Processar dados normalmente

            // Exibir dados na tela
            modbus_print_data(&data);

            // NT18B07: Não há porta nem alarme, apenas 7 canais de temperatura
            // Log periódico será feito a cada 5 minutos

            // Verificar se é hora do log periódico (5 minutos)
            time_t current_time = time(NULL);
            if (!should_log && (current_time - last_periodic_log) >= LOOP_INTERVAL_SECONDS) {
                should_log = true;
                last_periodic_log = current_time;
                printf("⏰ Log periódico (5 minutos)\n");
            }

            // ✅ GRAVAR NO DATALOGGER (apenas quando leitura foi bem-sucedida)
            if (should_log) {
                if (datalogger_log_data(datalogger_ctx, &data)) {
                    if (is_door_change) {
                        printf("✅ Mudança de porta registrada imediatamente no log\n");
                        door_change_logs++;
                    } else if (is_alarm_change) {
                        printf("✅ Mudança de alarme registrada imediatamente no log\n");
                        alarm_change_logs++;
                    } else {
                        printf("✅ Dados registrados no log (periódico)\n");
                    }
                } else {
                    printf("❌ Erro ao registrar dados no log\n");
                }
            }

            // 📺 Atualizar display OLED com dados válidos
            if (oled_ctx) {
                uint32_t total_logs = datalogger_ctx->record_counter;
                oled_display_datalogger_info(oled_ctx, device_name, &data, total_logs);
            }

        } else {
            // ❌ ERRO NA LEITURA MODBUS
            printf("❌ Erro: Falha na leitura de todos os registradores Modbus\n");
            printf("⚠️  NÃO será gravado no datalogger (dados inválidos)\n");

            // 🔊 Emitir alarme sonoro de erro (1 beep longo) - se habilitado
            if (ENABLE_MODBUS_ERROR_ALARM) {
                buzzer_signal_modbus_error();
            }

            // 📺 Exibir erro no display OLED
            if (oled_ctx) {
                oled_display_error(oled_ctx, "Erro Modbus");
            }
        }

        printf("----------------------------------------\n");

        // Aguardar próxima leitura (verificação mais frequente para detectar mudanças)
        // Verificar a cada 2 segundos em vez de 5 minutos
        for (int i = 0; i < 2 && running; i++) {
            sleep(1);
        }
    }

    // Cleanup
    printf("\nFinalizando aplicação...\n");

    // Aguardar thread USB finalizar
    printf("🔌 Finalizando monitoramento USB...\n");
    pthread_join(usb_thread, NULL);

    // Mostrar estatísticas finais
    datalogger_print_stats(datalogger_ctx);
    printf("Mudanças de porta registradas: %u\n", door_change_logs);
    printf("Mudanças de alarme registradas: %u\n", alarm_change_logs);

    // Limpar recursos
    reset_button_cleanup();
    relay_cleanup();
    if (oled_ctx) {
        oled_cleanup(oled_ctx);
    }
    datalogger_cleanup(datalogger_ctx);
    modbus_cleanup(modbus_ctx);

    printf("Aplicação finalizada com sucesso.\n");
    return EXIT_SUCCESS;
}
