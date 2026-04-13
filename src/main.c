/*
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
#include "keyboard.h"
#include "input.h"
#include "display.h"
#include "controller.h"

// Configurações da aplicação
#define LOOP_INTERVAL_SECONDS 60  // 5 minutos = 300 segundos
#define DEFAULT_DEVICE_NAME "NI00002"  // Nome padrão do dispositivo
#define CONFIG_FILE "/boot/firmware/config.txt"  // Arquivo de configuração do sistema

// ⚙️ CONFIGURAÇÃO: Número de sensores NTC a serem lidos do NT18B07
// Valores válidos: 1 a 7 (NT18B07 suporta até 7 canais)
// Reduzir este valor acelera a leitura Modbus (cada canal leva ~50-100ms)
static const int NUM_SENSORS_TO_READ = 2;

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

    // Inicializar DataLogger com número de canais configurado
    datalogger_context_t* datalogger_ctx = datalogger_init(device_name, NUM_SENSORS_TO_READ);
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
        printf("ℹ️  Compressor e resistência serão controlados pelo controller\n");
    }

    // Inicializar teclado (5 botões)
    if (keyboard_init() != 0) {
        printf("⚠️  Aviso: Falha ao inicializar teclado (continuando sem esta funcionalidade)\n");
    } else {
        printf("✅ Teclado ativo\n");
    }

    // Inicializar sensor de porta (GPIO 10)
    if (input_init() != 0) {
        printf("⚠️  Aviso: Falha ao inicializar sensor de porta (continuando sem esta funcionalidade)\n");
    } else {
        printf("✅ Sensor de porta ativo\n");
    }

    // Inicializar display
    display_context_t* display_ctx = display_init();
    if (!display_ctx) {
        printf("⚠️  Aviso: Falha ao inicializar display (continuando sem display)\n");
    } else {
        printf("✅ Display ativo\n");
        // Exibir tela de splash
        display_splash(display_ctx, device_name);
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

    // 🎛️ Inicializar controlador de temperatura
    float initial_setpoint = display_ctx ? display_get_setpoint(display_ctx) : 5.0f;
    controller_context_t* controller_ctx = controller_init(initial_setpoint);
    if (!controller_ctx) {
        fprintf(stderr, "❌ Erro ao inicializar controlador de temperatura\n");
        running = false;
    } else {
        printf("✅ Controlador de temperatura inicializado\n");
    }

    printf("\nIniciando loop de aquisição de dados (intervalo: %d segundos = %d minutos)\n",
           LOOP_INTERVAL_SECONDS, LOOP_INTERVAL_SECONDS / 60);
    printf("Pressione Ctrl+C para finalizar\n");
    printf("🔘 Pressione o botão de reset (GPIO5 → GND) para apagar todos os logs\n\n");

    // Loop principal de aquisição e logging
    // Estado anterior da porta (para detecção de mudança)
    bool previous_door_state_valid = false;
    bool previous_door_state = false;
    uint32_t door_change_logs = 0;

    // Estado anterior do alarme (código legado - não usado no NT18B07)
    bool previous_alarm_state_valid = false;
    uint16_t previous_alarm_state = 0;
    uint32_t alarm_change_logs = 0;

    // Controle de tempo para log periódico
    time_t last_periodic_log = time(NULL);

    // Variáveis para cache de dados Modbus (para atualização rápida do display)
    modbus_data_t last_data = {0};
    bool last_data_valid = false;
    uint32_t last_total_logs = 0;

    // Variáveis para controle de logs de relés (evitar repetições)
    bool last_compressor_state = false;
    bool last_heater_state = false;
    bool relay_states_initialized = false;

    while (running) {
        // Verificar botões do teclado

        // Botão SEL - Reset de logs
        if (keyboard_sel_is_pressed()) {
            printf("\n🔘 BOTÃO SEL PRESSIONADO!\n");
            printf("⚠️  Aguarde 3 segundos para confirmar reset de logs...\n");

            // Aguardar 3 segundos para confirmar (evitar acionamento acidental)
            sleep(3);

            // Verificar novamente se ainda está pressionado
            if (keyboard_sel_is_pressed()) {
                printf("🗑️  CONFIRMADO! Apagando todos os logs...\n\n");

                // Apagar todos os logs
                if (keyboard_delete_all_logs("/home/nova")) {
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
                    datalogger_ctx = datalogger_init(device_name, NUM_SENSORS_TO_READ);
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
                printf("💡 Solte o botão SEL...\n");
                while (keyboard_sel_is_pressed() && running) {
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

        // printf("Lendo registradores Modbus...\n");

        if (modbus_read_all(modbus_ctx, &data, NUM_SENSORS_TO_READ)) {
            // ✅ LEITURA BEM-SUCEDIDA - Processar dados normalmente

            // Exibir dados na tela
            // modbus_print_data(&data);

            // 🚪 VERIFICAR MUDANÇA DE ESTADO DA PORTA (GPIO 10)
            bool current_door_state = false;
            if (input_door_state_changed(&current_door_state)) {
                // Houve mudança no estado da porta
                is_door_change = true;
                should_log = true;  // Forçar log imediato
                printf("🚪 Porta %s - registrando no log imediatamente\n",
                       current_door_state ? "ABERTA" : "FECHADA");
                previous_door_state = current_door_state;
                previous_door_state_valid = true;
            } else if (!previous_door_state_valid) {
                // Primeira leitura - inicializar estado
                current_door_state = input_door_is_open();
                previous_door_state = current_door_state;
                previous_door_state_valid = true;
            }

            // Verificar se é hora do log periódico (60 segundos)
            time_t current_time = time(NULL);
            if (!should_log && (current_time - last_periodic_log) >= LOOP_INTERVAL_SECONDS) {
                should_log = true;
                last_periodic_log = current_time;
                // printf("⏰ Log periódico (60 segundos)\n");
            }

            // ✅ GRAVAR NO DATALOGGER (apenas quando leitura foi bem-sucedida)
            if (should_log) {
                // Obter status dos relés do controlador
                bool compressor_status = controller_should_compressor_be_on(controller_ctx);
                bool heater_status = controller_should_heater_be_on(controller_ctx);

                if (datalogger_log_data(datalogger_ctx, &data, compressor_status, heater_status)) {
                    if (is_door_change) {
                        // printf("✅ Mudança de porta registrada imediatamente no log\n");
                        door_change_logs++;
                    } else if (is_alarm_change) {
                        // printf("✅ Mudança de alarme registrada imediatamente no log\n");
                        alarm_change_logs++;
                    } else {
                        // printf("✅ Dados registrados no log (periódico)\n");
                    }
                } else {
                    // printf("❌ Erro ao registrar dados no log\n");
                }
            }

            // Salvar dados válidos no cache
            last_data = data;
            last_data_valid = true;
            last_total_logs = datalogger_ctx->record_counter;

            // 🎛️ ATUALIZAR CONTROLADOR DE TEMPERATURA
            if (controller_ctx) {
                // Atualizar controlador com temperaturas CH1 e CH2
                controller_update(controller_ctx,
                                data.ch_temp[0],  // CH1 - Controle principal
                                data.ch_temp[1],  // CH2 - Controle de degelo
                                data.ch_valid[0], // CH1 válido?
                                data.ch_valid[1]); // CH2 válido?

                // Aplicar saídas do controlador aos relés
                bool compressor_should_be_on = controller_should_compressor_be_on(controller_ctx);
                bool heater_should_be_on = controller_should_heater_be_on(controller_ctx);

                // Compressor - só imprime se mudou de estado (após inicialização)
                if (relay_states_initialized && compressor_should_be_on != last_compressor_state) {
                    if (compressor_should_be_on) {
                        printf("❄️  Compressor LIGADO (GPIO 4)\n");
                    } else {
                        printf("❄️  Compressor DESLIGADO (GPIO 4)\n");
                    }
                }

                // Aplicar estado ao relé
                if (compressor_should_be_on) {
                    relay_compressor_on();
                } else {
                    relay_compressor_off();
                }
                last_compressor_state = compressor_should_be_on;

                // Resistência - só imprime se mudou de estado (após inicialização)
                if (relay_states_initialized && heater_should_be_on != last_heater_state) {
                    if (heater_should_be_on) {
                        printf("🔥 Resistência LIGADA (GPIO 17)\n");
                    } else {
                        printf("🔥 Resistência DESLIGADA (GPIO 17)\n");
                    }
                }

                // Aplicar estado ao relé
                if (heater_should_be_on) {
                    relay_heater_on();
                } else {
                    relay_heater_off();
                }
                last_heater_state = heater_should_be_on;

                relay_states_initialized = true;
            }

            // 📺 Atualizar display com a tela atual
            // (botões são verificados no loop de espera e atualizam imediatamente)
            if (display_ctx) {
                // Obter estado atual da porta para exibir no display
                bool door_state_for_display = input_door_is_open();

                display_update_current_screen(display_ctx, device_name, &data, last_total_logs,
                                             relay_lamp_is_on(), relay_dialer_is_on(),
                                             relay_compressor_is_on(), relay_heater_is_on(),
                                             door_state_for_display);
            }

        } else {
            // ❌ ERRO NA LEITURA MODBUS
            // printf("❌ Erro: Falha na leitura de todos os registradores Modbus\n");
            // printf("⚠️  NÃO será gravado no datalogger (dados inválidos)\n");

            // 🔊 Emitir alarme sonoro de erro (1 beep longo) - se habilitado
            if (ENABLE_MODBUS_ERROR_ALARM) {
                buzzer_signal_modbus_error();
            }

            // 📺 Display continua mostrando última tela válida em caso de erro
        }

        // Aguardar próxima leitura com verificação contínua de botões
        // Total: 2 segundos, mas verifica botões a cada 50ms
        for (int i = 0; i < 40 && running; i++) {
            usleep(50000);  // 50ms

            // Verificar botões durante a espera para resposta instantânea
            bool button_pressed = false;

            if (keyboard_next_is_pressed() && display_ctx) {
                display_next_screen(display_ctx);
                button_pressed = true;
            }

            if (keyboard_before_is_pressed() && display_ctx) {
                display_previous_screen(display_ctx);
                button_pressed = true;
            }

            if (keyboard_inc_is_pressed() && display_ctx) {
                display_increment_setpoint(display_ctx);
                // Sincronizar setpoint com o controlador
                if (controller_ctx) {
                    controller_set_setpoint(controller_ctx, display_get_setpoint(display_ctx));
                }
                button_pressed = true;
            }

            if (keyboard_dec_is_pressed() && display_ctx) {
                display_decrement_setpoint(display_ctx);
                // Sincronizar setpoint com o controlador
                if (controller_ctx) {
                    controller_set_setpoint(controller_ctx, display_get_setpoint(display_ctx));
                }
                button_pressed = true;
            }

            // Atualizar display imediatamente se botão foi pressionado
            if (button_pressed && display_ctx) {
                // Obter estado atual da porta para exibir no display
                bool door_state_for_display = input_door_is_open();

                display_update_current_screen(display_ctx, device_name, &last_data, last_total_logs,
                                             relay_lamp_is_on(), relay_dialer_is_on(),
                                             relay_compressor_is_on(), relay_heater_is_on(),
                                             door_state_for_display);
            }
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
    keyboard_cleanup();
    input_cleanup();
    relay_cleanup();
    if (display_ctx) {
        display_cleanup(display_ctx);
    }
    if (controller_ctx) {
        controller_cleanup(controller_ctx);
    }
    datalogger_cleanup(datalogger_ctx);
    modbus_cleanup(modbus_ctx);

    printf("Aplicação finalizada com sucesso.\n");
    return EXIT_SUCCESS;
}
