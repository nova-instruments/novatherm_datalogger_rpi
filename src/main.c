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

// ⚙️ CONFIGURAÇÃO: Número de registradores a serem lidos do Modbus
// Valores válidos: 1 a 6 (Principal, Degelo, AC, DC, NTC1R, NTC2R)
// Reduzir este valor acelera a leitura Modbus (cada registrador leva ~50-100ms)
static const int NUM_SENSORS_TO_READ = MODBUS_NUM_CHANNELS;

// ⚙️ CONFIGURAÇÃO: Alarme de falta de comunicação Modbus
// true  = Buzzer emite alarme quando há erro de comunicação com COEL
// false = Buzzer desabilitado para erros Modbus (silencioso)
static const bool ENABLE_MODBUS_ERROR_ALARM = false;

// ⚙️ CONFIGURAÇÃO: Leitura de segundo slave Modbus (sem impactar fluxo principal)
static const bool ENABLE_SECONDARY_SLAVE_READ = true;
static const uint8_t SECONDARY_SLAVE_ID = 2;
static const uint16_t SECONDARY_SLAVE_T1_ADDR = 0x0200;
static const uint16_t SECONDARY_SLAVE_T2_ADDR = 0x0201;
static const useconds_t SECONDARY_SLAVE_PRE_READ_DELAY_US = 100000;  // 100ms

// Variável global para controle do loop principal
static volatile bool running = true;
static pthread_mutex_t usb_ui_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    bool visible;
    int progress;
    bool is_error;
    bool is_done;
    time_t done_at;
    char message[128];
} usb_ui_state_t;

static usb_ui_state_t g_usb_ui = {
    .visible = false,
    .progress = 0,
    .is_error = false,
    .is_done = false,
    .done_at = 0,
    .message = ""
};

static uint64_t monotonic_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ((uint64_t)ts.tv_sec * 1000ULL) + ((uint64_t)ts.tv_nsec / 1000000ULL);
}

// Estrutura para passar dados para thread USB
typedef struct {
    const char* source_dir;
    volatile bool* running;
} usb_thread_data_t;

typedef enum {
    ADMIN_ACTION_NONE = 0,
    ADMIN_ACTION_HYSTERESIS_EDIT,
    ADMIN_ACTION_OFFSET_PRINCIPAL_EDIT,
    ADMIN_ACTION_OFFSET_DEGELO_EDIT,
    ADMIN_ACTION_DIAG_HEATER_TOGGLE
} admin_action_t;

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
    if (percentage < 0) percentage = 0;
    if (percentage > 100) percentage = 100;

    pthread_mutex_lock(&usb_ui_mutex);
    g_usb_ui.visible = true;
    g_usb_ui.progress = percentage;
    g_usb_ui.is_error = false;
    g_usb_ui.is_done = false;
    g_usb_ui.done_at = 0;
    snprintf(g_usb_ui.message, sizeof(g_usb_ui.message), "%s",
             (message && message[0]) ? message : "Extraindo dados...");
    pthread_mutex_unlock(&usb_ui_mutex);
}

void usb_on_complete(usb_result_t result, const char* message) {
    printf("✅ USB: %s\n", message);
    pthread_mutex_lock(&usb_ui_mutex);
    g_usb_ui.visible = true;
    g_usb_ui.progress = (result == USB_SUCCESS) ? 100 : g_usb_ui.progress;
    g_usb_ui.is_error = (result != USB_SUCCESS);
    g_usb_ui.is_done = true;
    g_usb_ui.done_at = time(NULL);
    snprintf(g_usb_ui.message, sizeof(g_usb_ui.message), "%s",
             (message && message[0]) ? message : "Extracao finalizada");
    pthread_mutex_unlock(&usb_ui_mutex);
}

void usb_on_error(usb_result_t error, const char* message) {
    printf("❌ USB Erro [%d]: %s\n", error, message);
    pthread_mutex_lock(&usb_ui_mutex);
    g_usb_ui.visible = true;
    g_usb_ui.is_error = true;
    g_usb_ui.is_done = true;
    g_usb_ui.done_at = time(NULL);
    snprintf(g_usb_ui.message, sizeof(g_usb_ui.message), "%s",
             (message && message[0]) ? message : "Falha na extracao USB");
    pthread_mutex_unlock(&usb_ui_mutex);
}

static void update_usb_extraction_ui(display_context_t* display_ctx) {
    if (!display_ctx) {
        return;
    }

    usb_ui_state_t ui_copy;
    pthread_mutex_lock(&usb_ui_mutex);
    ui_copy = g_usb_ui;
    pthread_mutex_unlock(&usb_ui_mutex);

    if (ui_copy.visible && ui_copy.is_done) {
        time_t now = time(NULL);
        if ((now - ui_copy.done_at) >= 2) {
            pthread_mutex_lock(&usb_ui_mutex);
            g_usb_ui.visible = false;
            g_usb_ui.progress = 0;
            g_usb_ui.is_error = false;
            g_usb_ui.is_done = false;
            g_usb_ui.done_at = 0;
            g_usb_ui.message[0] = '\0';
            ui_copy = g_usb_ui;
            pthread_mutex_unlock(&usb_ui_mutex);
        }
    }

    display_set_usb_extraction_screen(display_ctx, ui_copy.visible, ui_copy.progress,
                                      ui_copy.message, ui_copy.is_error, ui_copy.is_done);
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

    // Disponibilizar contexto Modbus para relés em coil (lâmpada/discadora)
    relay_set_modbus_context(modbus_ctx);

    // Inicializar DataLogger com número de canais configurado
    datalogger_context_t* datalogger_ctx = datalogger_init(device_name, NUM_SENSORS_TO_READ);
    if (!datalogger_ctx) {
        fprintf(stderr, "Erro: Falha ao inicializar DataLogger\n");
        modbus_cleanup(modbus_ctx);
        return EXIT_FAILURE;
    }

    // Inicializar controle de relés
    bool relays_available = false;
    if (relay_init() != 0) {
        fprintf(stderr, "⚠️  Aviso: Falha ao inicializar relés (continuando sem controle de relés)\n");
    } else {
        relays_available = true;
        printf("✅ Controle de relés ativo\n");
        printf("ℹ️  Compressor e resistência serão controlados pelo controller\n");
    }

#ifdef USE_ST7789_EC11
    printf("ℹ️  Perfil ST7789 ativo: teclado físico desabilitado (navegação via encoder)\n");
#else
    // Inicializar teclado (5 botões)
    if (keyboard_init() != 0) {
        printf("⚠️  Aviso: Falha ao inicializar teclado (continuando sem esta funcionalidade)\n");
    } else {
        printf("✅ Teclado ativo\n");
    }
#endif

    // Inicializar sensor de porta (GPIO 17)
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
    float initial_setpoint = display_ctx ? display_get_setpoint(display_ctx) : 4.5f;
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
#ifndef USE_ST7789_EC11
    printf("🔘 Pressione o botão de reset (GPIO5 → GND) para apagar todos os logs\n");
#endif
    printf("\n");

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
    bool st7789_logs_hold_active = false;
    uint64_t st7789_logs_hold_start_ms = 0;
    bool st7789_main_minmax_hold_active = false;
    uint64_t st7789_main_minmax_hold_start_ms = 0;
    bool st7789_exit_hold_active = false;
    bool st7789_exit_hold_triggered = false;
    bool st7789_lamp_timer_active = false;
    bool st7789_lamp_timer_done_by_feature = false;
    uint64_t st7789_exit_hold_start_ms = 0;
    uint64_t st7789_lamp_timer_end_ms = 0;
    char st7789_top_status_last[64] = "";
    admin_action_t pending_admin_action = ADMIN_ACTION_NONE;

    // Variáveis para controle de logs de relés (evitar repetições)
    bool last_compressor_state = false;
    bool last_heater_state = false;
    bool relay_states_initialized = false;
    bool secondary_slave_read_error_logged = false;
    bool secondary_t1_valid = false;
    bool secondary_t2_valid = false;
    float secondary_t1_c = 0.0f;
    float secondary_t2_c = 0.0f;

    while (running) {
        // Verificar botões do teclado

#ifndef USE_ST7789_EC11
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
#endif

        modbus_data_t data;
        bool should_log = false;
        bool is_door_change = false;
        bool is_alarm_change = false;

        // printf("Lendo registradores Modbus...\n");

        if (modbus_read_all(modbus_ctx, &data, NUM_SENSORS_TO_READ)) {
            // ✅ LEITURA BEM-SUCEDIDA - Processar dados normalmente

            if (data.ch_valid[0] && !data.ch_error[0]) {
                data.ch_temp[0] += display_get_offset_principal(display_ctx);
            }
            if (data.ch_valid[1] && !data.ch_error[1]) {
                data.ch_temp[1] += display_get_offset_degelo(display_ctx);
            }

            if (ENABLE_SECONDARY_SLAVE_READ) {
                usleep(SECONDARY_SLAVE_PRE_READ_DELAY_US);
                uint16_t t_raw[2] = {0};
                bool pair_ok = modbus_read_registers_from_slave(
                    modbus_ctx, SECONDARY_SLAVE_ID, SECONDARY_SLAVE_T1_ADDR, 2, t_raw);

                secondary_t1_valid = pair_ok;
                secondary_t2_valid = pair_ok;
                if (pair_ok) {
                    secondary_t1_c = ((int16_t)t_raw[0]) / 10.0f;
                    secondary_t2_c = ((int16_t)t_raw[1]) / 10.0f;
                }

                if (!pair_ok) {
                    if (!secondary_slave_read_error_logged) {
                        fprintf(stderr,
                                "⚠️  Falha ao ler T1/T2 do slave %u (0x%04X/0x%04X). "
                                "Fluxo principal (slave %d) segue normal.\n",
                                SECONDARY_SLAVE_ID, SECONDARY_SLAVE_T1_ADDR,
                                SECONDARY_SLAVE_T2_ADDR, MODBUS_SLAVE_ID);
                        secondary_slave_read_error_logged = true;
                    }
                } else if (secondary_slave_read_error_logged) {
                    printf("✅ Leitura do slave %u (T1/T2) restabelecida.\n", SECONDARY_SLAVE_ID);
                    secondary_slave_read_error_logged = false;
                }
            }

            // Exibir dados na tela
            // modbus_print_data(&data);

            // 🚪 VERIFICAR MUDANÇA DE ESTADO DA PORTA (GPIO 17)
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
                bool compressor_status = relays_available
                                         ? relay_compressor_is_on()
                                         : false;
                bool heater_status = relays_available
                                     ? relay_heater_is_on()
                                     : false;

                if (datalogger_log_data(datalogger_ctx, &data, compressor_status, heater_status)) {
                    if (!datalogger_log_secondary_data(
                            datalogger_ctx,
                            data.ch_valid[4], data.ch_temp[4],
                            data.ch_valid[5], data.ch_temp[5],
                            secondary_t1_valid, secondary_t1_c,
                            secondary_t2_valid, secondary_t2_c)) {
                        // Banco secundário é auxiliar; não interrompe o fluxo principal.
                        // printf("⚠️  Falha ao registrar no banco secundário\n");
                    }

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
                // Atualizar controlador com temperaturas PR1 e PR2
                controller_update(controller_ctx,
                                data.ch_temp[0],  // PR1 - Controle principal
                                data.ch_temp[1],  // PR2 - Controle de degelo
                                data.ch_valid[0], // PR1 válido?
                                data.ch_valid[1]); // PR2 válido?

                // Aplicar saídas do controlador aos relés
                bool compressor_should_be_on = controller_should_compressor_be_on(controller_ctx);
                bool heater_should_be_on = controller_should_heater_be_on(controller_ctx);

                if (relays_available) {
                    // Compressor - só imprime se mudou de estado (após inicialização)
                    if (relay_states_initialized && compressor_should_be_on != last_compressor_state) {
                        if (compressor_should_be_on) {
                            printf("❄️  Compressor LIGADO (GPIO 24)\n");
                        } else {
                            printf("❄️  Compressor DESLIGADO (GPIO 24)\n");
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
                            printf("🔥 Resistência LIGADA (GPIO 25)\n");
                        } else {
                            printf("🔥 Resistência DESLIGADA (GPIO 25)\n");
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
            }

            // 📺 Atualizar display com a tela atual
            // (botões são verificados no loop de espera e atualizam imediatamente)
            if (display_ctx) {
                // Obter estado atual da porta para exibir no display
                bool door_state_for_display = input_door_is_open();

                display_update_current_screen(display_ctx, device_name, &data, last_total_logs,
                                             relay_lamp_is_on(), relay_dialer_is_on(),
                                             relay_compressor_is_on(), relay_heater_is_on(),
                                             door_state_for_display,
                                             secondary_t1_valid, secondary_t1_c,
                                             secondary_t2_valid, secondary_t2_c);
                update_usb_extraction_ui(display_ctx);
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

            // Verificar entradas durante a espera para resposta instantânea
#ifndef USE_ST7789_EC11
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
#ifdef USE_LCD_I2C
                if (lcd_is_relay_test_screen(display_ctx)) {
                    if (lcd_relay_test_get_target(display_ctx) == LCD_RELAY_TEST_TARGET_LAMP) {
                        if (relay_lamp_is_on()) {
                            relay_lamp_off();
                        } else {
                            relay_lamp_on();
                        }
                    } else {
                        if (relay_dialer_is_on()) {
                            relay_dialer_off();
                        } else {
                            relay_dialer_on();
                        }
                    }
                    button_pressed = true;
                } else
#endif
                {
                display_increment_setpoint(display_ctx);
                // Sincronizar setpoint com o controlador
                if (controller_ctx) {
                    controller_set_setpoint(controller_ctx, display_get_setpoint(display_ctx));
                }
                button_pressed = true;
                }
            }

            if (keyboard_dec_is_pressed() && display_ctx) {
#ifdef USE_LCD_I2C
                if (lcd_is_relay_test_screen(display_ctx)) {
                    lcd_relay_test_toggle_target(display_ctx);
                    button_pressed = true;
                } else
#endif
                {
                display_decrement_setpoint(display_ctx);
                // Sincronizar setpoint com o controlador
                if (controller_ctx) {
                    controller_set_setpoint(controller_ctx, display_get_setpoint(display_ctx));
                }
                button_pressed = true;
                }
            }

            // Atualizar display imediatamente se botão foi pressionado
            if (button_pressed && display_ctx) {
                // Obter estado atual da porta para exibir no display
                bool door_state_for_display = input_door_is_open();

                display_update_current_screen(display_ctx, device_name, &last_data, last_total_logs,
                                             relay_lamp_is_on(), relay_dialer_is_on(),
                                             relay_compressor_is_on(), relay_heater_is_on(),
                                             door_state_for_display,
                                             secondary_t1_valid, secondary_t1_c,
                                             secondary_t2_valid, secondary_t2_c);
                update_usb_extraction_ui(display_ctx);
            }
#else
            // Navegação por encoder (ST7789): rotação alterna entre telas
            if (display_ctx) {
                bool button_clicked = display_consume_nav_click(display_ctx);
                bool exit_edit_clicked = display_consume_exit_edit_click(display_ctx);
                bool screen_changed = false;
                bool setpoint_changed = false;
                bool exit_pressed = display_is_exit_button_pressed(display_ctx);
                uint64_t now_ms = monotonic_ms();
                char top_status_line[64] = "";

                if (exit_edit_clicked) {
                    display_cancel_edit_modes(display_ctx);
                    pending_admin_action = ADMIN_ACTION_NONE;
                    st7789_logs_hold_active = false;
                    st7789_logs_hold_start_ms = 0;
                    st7789_main_minmax_hold_active = false;
                    st7789_main_minmax_hold_start_ms = 0;
                    setpoint_changed = true;
                }

                if (button_clicked) {
                    if (display_is_offset_editor_active(display_ctx)) {
                        if (display_confirm_offset_editor_digit(display_ctx)) {
                            pending_admin_action = ADMIN_ACTION_NONE;
                        }
                        setpoint_changed = true;
                    } else if (display_is_setpoint_edit_active(display_ctx)) {
                        display_set_setpoint_edit_active(display_ctx, false);
                    } else if (display_get_current_screen(display_ctx) == DISPLAY_SCREEN_MAIN_TEMP) {
                        if (display_is_hysteresis_password_active(display_ctx)) {
                            display_confirm_hysteresis_password_digit(display_ctx);
                        } else if (display_is_main_minmax_reset_dialog_visible(display_ctx)) {
                            // hold de 3s executa o reset
                        } else if (!display_is_main_nav_active(display_ctx)) {
                            display_set_main_nav_active(display_ctx, true);
                        } else {
                            switch (display_get_main_selection(display_ctx)) {
                                case ST7789_MAIN_ITEM_SETPOINT:
                                    display_set_main_nav_active(display_ctx, false);
                                    display_set_setpoint_edit_active(display_ctx, true);
                                    break;
                                case ST7789_MAIN_ITEM_OFFSET_PRINCIPAL:
                                    pending_admin_action = ADMIN_ACTION_OFFSET_PRINCIPAL_EDIT;
                                    display_start_hysteresis_password(display_ctx);
                                    break;
                                case ST7789_MAIN_ITEM_OFFSET_DEGELO:
                                    pending_admin_action = ADMIN_ACTION_OFFSET_DEGELO_EDIT;
                                    display_start_hysteresis_password(display_ctx);
                                    break;
                                case ST7789_MAIN_ITEM_MIN:
                                case ST7789_MAIN_ITEM_MAX:
                                default:
                                    display_set_main_minmax_reset_dialog(display_ctx, true,
                                                                         "Pressione o botao por 3s");
                                    st7789_main_minmax_hold_active = false;
                                    st7789_main_minmax_hold_start_ms = 0;
                                    break;
                            }
                        }
                    } else if (display_get_current_screen(display_ctx) == DISPLAY_SCREEN_INFO) {
                        if (display_is_logs_reset_dialog_visible(display_ctx)) {
                            // Ação por hold de 3s; clique não faz nada aqui.
                        } else if (display_is_hysteresis_password_active(display_ctx)) {
                            display_confirm_hysteresis_password_digit(display_ctx);
                        } else if (!display_is_info_nav_active(display_ctx)) {
                            display_set_info_nav_active(display_ctx, true);
                        } else {
                            switch (display_get_info_selection(display_ctx)) {
                                case ST7789_INFO_ITEM_HYSTERESIS:
                                    pending_admin_action = ADMIN_ACTION_HYSTERESIS_EDIT;
                                    display_start_hysteresis_password(display_ctx);
                                    break;
                                case ST7789_INFO_ITEM_LOGS:
                                    display_set_logs_reset_dialog(display_ctx, true,
                                                                  "Pressione o botao por 3s");
                                    st7789_logs_hold_active = false;
                                    st7789_logs_hold_start_ms = 0;
                                    break;
                                case ST7789_INFO_ITEM_AC:
                                case ST7789_INFO_ITEM_DC:
                                default:
                                    break;
                            }
                        }
                    } else if (display_get_current_screen(display_ctx) == DISPLAY_SCREEN_DIAGNOSTICS) {
                        if (display_is_hysteresis_password_active(display_ctx)) {
                            display_confirm_hysteresis_password_digit(display_ctx);
                            setpoint_changed = true;
                        } else if (display_is_diag_compressor_avg_dialog_visible(display_ctx)) {
                            display_set_diag_compressor_avg_dialog(display_ctx, false);
                            setpoint_changed = true;
                        } else if (!display_is_diag_nav_active(display_ctx)) {
                            display_set_diag_nav_active(display_ctx, true);
                            setpoint_changed = true;
                        } else {
                            switch (display_get_diag_selection(display_ctx)) {
                                case ST7789_DIAG_ITEM_COMPRESSOR:
                                    display_set_diag_compressor_avg_dialog(display_ctx, true);
                                    setpoint_changed = true;
                                    break;
                                case ST7789_DIAG_ITEM_LAMP:
                                    if (relay_lamp_is_on()) {
                                        relay_lamp_off();
                                    } else {
                                        relay_lamp_on();
                                    }
                                    setpoint_changed = true;
                                    break;
                                case ST7789_DIAG_ITEM_DIALER:
                                    if (relay_dialer_is_on()) {
                                        relay_dialer_off();
                                    } else {
                                        relay_dialer_on();
                                    }
                                    setpoint_changed = true;
                                    break;
                                case ST7789_DIAG_ITEM_HEATER:
                                    pending_admin_action = ADMIN_ACTION_DIAG_HEATER_TOGGLE;
                                    display_start_hysteresis_password(display_ctx);
                                    setpoint_changed = true;
                                    break;
                                default:
                                    break;
                            }
                        }
                    }
                }

                if (exit_pressed) {
                    if (st7789_exit_hold_triggered) {
                        // Aguarda soltar para permitir novo ciclo de 3s.
                    } else if (!st7789_exit_hold_active) {
                        st7789_exit_hold_active = true;
                        st7789_exit_hold_start_ms = now_ms;
                    } else if ((now_ms - st7789_exit_hold_start_ms) >= 3000ULL) {
                        if (relay_lamp_on() == 0) {
                            st7789_lamp_timer_active = true;
                            st7789_lamp_timer_done_by_feature = true;
                            st7789_lamp_timer_end_ms = now_ms + 30000ULL;
                            printf("💡 Temporização de lâmpada: ligada por 30s\n");
                        } else {
                            fprintf(stderr, "❌ Falha ao acionar coil da lâmpada para temporização\n");
                        }
                        st7789_exit_hold_active = false;
                        st7789_exit_hold_triggered = true;
                    }
                } else {
                    st7789_exit_hold_active = false;
                    st7789_exit_hold_triggered = false;
                }

                if (st7789_lamp_timer_active && now_ms >= st7789_lamp_timer_end_ms) {
                    if (relay_lamp_off() != 0) {
                        fprintf(stderr, "❌ Falha ao desligar lâmpada após temporização\n");
                    } else if (st7789_lamp_timer_done_by_feature) {
                        printf("💡 Temporização de lâmpada concluída (30s)\n");
                    }
                    st7789_lamp_timer_active = false;
                    st7789_lamp_timer_done_by_feature = false;
                }

                if (st7789_exit_hold_active && !st7789_exit_hold_triggered) {
                    uint64_t elapsed = now_ms - st7789_exit_hold_start_ms;
                    int rem = (int)((3000ULL - (elapsed > 3000ULL ? 3000ULL : elapsed) + 999ULL) / 1000ULL);
                    snprintf(top_status_line, sizeof(top_status_line), "Lamp %ds", rem);
                } else if (st7789_lamp_timer_active) {
                    uint64_t rem_ms = (st7789_lamp_timer_end_ms > now_ms) ? (st7789_lamp_timer_end_ms - now_ms) : 0;
                    int rem_s = (int)((rem_ms + 999ULL) / 1000ULL);
                    snprintf(top_status_line, sizeof(top_status_line), "LampON %ds", rem_s);
                }
                display_set_top_status(display_ctx, top_status_line[0] ? top_status_line : NULL);
                if (strcmp(st7789_top_status_last, top_status_line) != 0) {
                    snprintf(st7789_top_status_last, sizeof(st7789_top_status_last), "%s", top_status_line);
                    setpoint_changed = true;
                }

                int32_t nav_diff = display_consume_nav_diff(display_ctx);

                if (display_is_offset_editor_active(display_ctx)) {
                    while (nav_diff > 0) {
                        display_increment_offset_editor_digit(display_ctx);
                        nav_diff--;
                        setpoint_changed = true;
                    }
                    while (nav_diff < 0) {
                        display_decrement_offset_editor_digit(display_ctx);
                        nav_diff++;
                        setpoint_changed = true;
                    }
                } else if (display_is_setpoint_edit_active(display_ctx)) {
                    if (display_get_current_screen(display_ctx) == DISPLAY_SCREEN_INFO) {
                        while (nav_diff > 0) {
                            display_increment_hysteresis(display_ctx);
                            nav_diff--;
                            setpoint_changed = true;
                        }

                        while (nav_diff < 0) {
                            display_decrement_hysteresis(display_ctx);
                            nav_diff++;
                            setpoint_changed = true;
                        }
                    } else if (display_get_current_screen(display_ctx) == DISPLAY_SCREEN_MAIN_TEMP) {
                        while (nav_diff > 0) {
                            display_increment_main_selected_value(display_ctx);
                            nav_diff--;
                            setpoint_changed = true;
                        }
                        while (nav_diff < 0) {
                            display_decrement_main_selected_value(display_ctx);
                            nav_diff++;
                            setpoint_changed = true;
                        }
                    } else {
                        while (nav_diff > 0) {
                            display_increment_setpoint(display_ctx);
                            nav_diff--;
                            setpoint_changed = true;
                        }

                        while (nav_diff < 0) {
                            display_decrement_setpoint(display_ctx);
                            nav_diff++;
                            setpoint_changed = true;
                        }
                    }
                } else if ((display_get_current_screen(display_ctx) == DISPLAY_SCREEN_INFO ||
                            display_get_current_screen(display_ctx) == DISPLAY_SCREEN_MAIN_TEMP ||
                            display_get_current_screen(display_ctx) == DISPLAY_SCREEN_DIAGNOSTICS) &&
                           display_is_hysteresis_password_active(display_ctx)) {
                    while (nav_diff > 0) {
                        display_increment_hysteresis_password_digit(display_ctx);
                        nav_diff--;
                        setpoint_changed = true;
                    }
                    while (nav_diff < 0) {
                        display_decrement_hysteresis_password_digit(display_ctx);
                        nav_diff++;
                        setpoint_changed = true;
                    }
                } else if (display_get_current_screen(display_ctx) == DISPLAY_SCREEN_MAIN_TEMP &&
                           display_is_main_nav_active(display_ctx)) {
                    if (nav_diff != 0) {
                        display_move_main_selection(display_ctx, nav_diff);
                        setpoint_changed = true;
                    }
                } else if (display_get_current_screen(display_ctx) == DISPLAY_SCREEN_INFO &&
                           display_is_info_nav_active(display_ctx)) {
                    if (nav_diff != 0) {
                        display_move_info_selection(display_ctx, nav_diff);
                        setpoint_changed = true;
                    }
                } else if (display_get_current_screen(display_ctx) == DISPLAY_SCREEN_DIAGNOSTICS &&
                           display_is_diag_nav_active(display_ctx)) {
                    if (nav_diff != 0) {
                        if (display_is_diag_compressor_avg_dialog_visible(display_ctx)) {
                            display_set_diag_compressor_avg_dialog(display_ctx, false);
                        }
                        display_move_diag_selection(display_ctx, nav_diff);
                        setpoint_changed = true;
                    }
                } else {
                    while (nav_diff > 0) {
                        display_next_screen(display_ctx);
                        nav_diff--;
                        screen_changed = true;
                    }

                    while (nav_diff < 0) {
                        display_previous_screen(display_ctx);
                        nav_diff++;
                        screen_changed = true;
                    }
                }

                if (setpoint_changed && controller_ctx) {
                    if (display_get_current_screen(display_ctx) == DISPLAY_SCREEN_INFO &&
                        display_is_setpoint_edit_active(display_ctx)) {
                        controller_set_hysteresis(controller_ctx, display_get_hysteresis(display_ctx));
                    } else if (display_get_current_screen(display_ctx) == DISPLAY_SCREEN_MAIN_TEMP &&
                               display_is_setpoint_edit_active(display_ctx) &&
                               display_get_main_selection(display_ctx) == ST7789_MAIN_ITEM_SETPOINT) {
                        controller_set_setpoint(controller_ctx, display_get_setpoint(display_ctx));
                    } else {
                        // Sem sincronização de controlador para offsets.
                    }
                }

                if (display_get_current_screen(display_ctx) == DISPLAY_SCREEN_INFO &&
                    display_is_hysteresis_password_result_visible(display_ctx)) {
                    bool auth_ok = display_consume_hysteresis_password_result(display_ctx);
                    bool still_visible = display_is_hysteresis_password_result_visible(display_ctx);
                    if (auth_ok) {
                        if (pending_admin_action == ADMIN_ACTION_HYSTERESIS_EDIT) {
                            display_set_info_nav_active(display_ctx, false);
                            display_set_setpoint_edit_active(display_ctx, true);
                        } else if (pending_admin_action == ADMIN_ACTION_OFFSET_PRINCIPAL_EDIT ||
                                   pending_admin_action == ADMIN_ACTION_OFFSET_DEGELO_EDIT) {
                            display_set_main_nav_active(display_ctx, false);
                            display_start_offset_editor(
                                display_ctx,
                                pending_admin_action == ADMIN_ACTION_OFFSET_PRINCIPAL_EDIT
                                    ? ST7789_OFFSET_TARGET_PRINCIPAL
                                    : ST7789_OFFSET_TARGET_DEGELO);
                        }
                    }
                    if (!still_visible) {
                        pending_admin_action = ADMIN_ACTION_NONE;
                    }
                    setpoint_changed = true;
                }

                if (display_get_current_screen(display_ctx) == DISPLAY_SCREEN_MAIN_TEMP &&
                    display_is_hysteresis_password_result_visible(display_ctx)) {
                    bool auth_ok = display_consume_hysteresis_password_result(display_ctx);
                    bool still_visible = display_is_hysteresis_password_result_visible(display_ctx);
                    if (auth_ok) {
                        if (pending_admin_action == ADMIN_ACTION_OFFSET_PRINCIPAL_EDIT ||
                            pending_admin_action == ADMIN_ACTION_OFFSET_DEGELO_EDIT) {
                            display_set_main_nav_active(display_ctx, false);
                            display_start_offset_editor(
                                display_ctx,
                                pending_admin_action == ADMIN_ACTION_OFFSET_PRINCIPAL_EDIT
                                    ? ST7789_OFFSET_TARGET_PRINCIPAL
                                    : ST7789_OFFSET_TARGET_DEGELO);
                        }
                    }
                    if (!still_visible) {
                        pending_admin_action = ADMIN_ACTION_NONE;
                    }
                    setpoint_changed = true;
                }

                if (display_get_current_screen(display_ctx) == DISPLAY_SCREEN_DIAGNOSTICS &&
                    display_is_hysteresis_password_result_visible(display_ctx)) {
                    bool auth_ok = display_consume_hysteresis_password_result(display_ctx);
                    bool still_visible = display_is_hysteresis_password_result_visible(display_ctx);
                    if (auth_ok && pending_admin_action == ADMIN_ACTION_DIAG_HEATER_TOGGLE) {
                        if (relay_heater_is_on()) {
                            relay_heater_off();
                        } else {
                            relay_heater_on();
                        }
                    }
                    if (!still_visible) {
                        pending_admin_action = ADMIN_ACTION_NONE;
                    }
                    setpoint_changed = true;
                }

                if (display_get_current_screen(display_ctx) == DISPLAY_SCREEN_MAIN_TEMP &&
                    display_is_main_minmax_reset_dialog_visible(display_ctx)) {
                    bool pressed = display_is_nav_pressed(display_ctx);
                    uint64_t now_ms_mm = monotonic_ms();

                    if (pressed) {
                        if (!st7789_main_minmax_hold_active) {
                            st7789_main_minmax_hold_active = true;
                            st7789_main_minmax_hold_start_ms = now_ms_mm;
                        }

                        uint64_t elapsed = now_ms_mm - st7789_main_minmax_hold_start_ms;
                        if (elapsed >= 3000ULL) {
                            bool ok = display_reset_main_minmax_from_current_temp(display_ctx);
                            if (ok) {
                                display_set_main_minmax_reset_dialog(display_ctx, true, "Min/Max resetados");
                            } else {
                                display_set_main_minmax_reset_dialog(display_ctx, true, "Sem PR1 valido");
                            }
                            st7789_main_minmax_hold_active = false;
                            st7789_main_minmax_hold_start_ms = 0;
                            usleep(700000);
                            display_set_main_minmax_reset_dialog(display_ctx, false, NULL);
                            display_set_main_nav_active(display_ctx, false);
                            setpoint_changed = true;
                        } else {
                            char msg[64];
                            int rem = (int)((3000ULL - elapsed + 999ULL) / 1000ULL);
                            snprintf(msg, sizeof(msg), "Segure por %ds...", rem);
                            display_set_main_minmax_reset_dialog(display_ctx, true, msg);
                            setpoint_changed = true;
                        }
                    } else {
                        st7789_main_minmax_hold_active = false;
                        st7789_main_minmax_hold_start_ms = 0;
                        display_set_main_minmax_reset_dialog(display_ctx, true, "Pressione o botao por 3s");
                    }
                } else {
                    st7789_main_minmax_hold_active = false;
                    st7789_main_minmax_hold_start_ms = 0;
                }

                if (display_get_current_screen(display_ctx) == DISPLAY_SCREEN_INFO &&
                    display_is_logs_reset_dialog_visible(display_ctx)) {
                    bool pressed = display_is_nav_pressed(display_ctx);
                    uint64_t now_ms = monotonic_ms();

                    if (pressed) {
                        if (!st7789_logs_hold_active) {
                            st7789_logs_hold_active = true;
                            st7789_logs_hold_start_ms = now_ms;
                        }

                        uint64_t elapsed = now_ms - st7789_logs_hold_start_ms;
                        if (elapsed >= 3000ULL) {
                            bool ok = keyboard_delete_all_logs("/home/nova");
                            if (ok) {
                                printf("✅ Todos os logs foram apagados com sucesso!\n");
                                printf("🔊 Sinalizando limpeza de logs...\n");
                                buzzer_signal_extraction_complete();
                                sleep(1);
                                buzzer_signal_extraction_complete();
                                printf("🔊 Sinalização concluída\n");
                                printf("🔄 Reiniciando sistema de logging...\n");

                                datalogger_cleanup(datalogger_ctx);
                                datalogger_ctx = datalogger_init(device_name, NUM_SENSORS_TO_READ);
                                if (!datalogger_ctx) {
                                    fprintf(stderr, "❌ Erro ao reiniciar DataLogger\n");
                                    running = false;
                                    break;
                                }

                                if (datalogger_ctx) {
                                    datalogger_ctx->record_counter = 0;
                                }
                                last_total_logs = 0;
                                door_change_logs = 0;
                                alarm_change_logs = 0;
                                last_periodic_log = time(NULL);
                                display_set_logs_reset_dialog(display_ctx, true, "Registros resetados");
                            } else {
                                display_set_logs_reset_dialog(display_ctx, true, "Falha ao resetar");
                            }

                            st7789_logs_hold_active = false;
                            st7789_logs_hold_start_ms = 0;
                            usleep(700000);
                            display_set_logs_reset_dialog(display_ctx, false, NULL);
                            display_set_info_nav_active(display_ctx, false);
                            setpoint_changed = true;
                        } else {
                            char msg[64];
                            int rem = (int)((3000ULL - elapsed + 999ULL) / 1000ULL);
                            snprintf(msg, sizeof(msg), "Segure por %ds...", rem);
                            display_set_logs_reset_dialog(display_ctx, true, msg);
                            setpoint_changed = true;
                        }
                    } else {
                        st7789_logs_hold_active = false;
                        st7789_logs_hold_start_ms = 0;
                        display_set_logs_reset_dialog(display_ctx, true, "Pressione o botao por 3s");
                    }
                } else {
                    st7789_logs_hold_active = false;
                    st7789_logs_hold_start_ms = 0;
                }

                if (screen_changed || setpoint_changed || button_clicked || exit_edit_clicked) {
                    bool door_state_for_display = input_door_is_open();

                    display_update_current_screen(display_ctx, device_name, &last_data, last_total_logs,
                                                 relay_lamp_is_on(), relay_dialer_is_on(),
                                                 relay_compressor_is_on(), relay_heater_is_on(),
                                                 door_state_for_display,
                                                 secondary_t1_valid, secondary_t1_c,
                                                 secondary_t2_valid, secondary_t2_c);
                    update_usb_extraction_ui(display_ctx);
                }
            }
#endif

            // Processamento periódico do backend de display (necessário para LVGL/ST7789)
            if (display_ctx) {
                update_usb_extraction_ui(display_ctx);
                display_process(display_ctx);
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
#ifndef USE_ST7789_EC11
    keyboard_cleanup();
#endif
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
