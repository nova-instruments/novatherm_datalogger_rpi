/**
 * @file lcd_i2c.c
 * @brief NovaTherm DataLogger - LCD 20x4 I2C Display Implementation
 * @author Nova Instruments
 */

#include "lcd_i2c.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <time.h>
#include <math.h>  // Para fabsf()

// Funções privadas de baixo nível

/**
 * @brief Escreve um byte no I2C
 */
static bool lcd_write_byte(lcd_context_t* ctx, uint8_t data) {
    if (!ctx || ctx->i2c_fd < 0) return false;
    
    if (write(ctx->i2c_fd, &data, 1) != 1) {
        return false;
    }
    return true;
}

/**
 * @brief Envia um nibble (4 bits) para o LCD
 */
static void lcd_write_nibble(lcd_context_t* ctx, uint8_t nibble) {
    uint8_t data = nibble | ctx->backlight;
    
    // Pulso de enable
    lcd_write_byte(ctx, data | LCD_ENABLE);
    usleep(1);
    lcd_write_byte(ctx, data & ~LCD_ENABLE);
    usleep(50);
}

/**
 * @brief Envia um byte para o LCD (modo 4 bits)
 */
static void lcd_send_byte(lcd_context_t* ctx, uint8_t data, uint8_t mode) {
    uint8_t high_nibble = (data & 0xF0);
    uint8_t low_nibble = ((data << 4) & 0xF0);
    
    lcd_write_nibble(ctx, high_nibble | mode);
    lcd_write_nibble(ctx, low_nibble | mode);
}

/**
 * @brief Envia um comando para o LCD
 */
static void lcd_command(lcd_context_t* ctx, uint8_t cmd) {
    lcd_send_byte(ctx, cmd, LCD_RS_COMMAND);
    usleep(2000);
}

/**
 * @brief Envia um caractere para o LCD
 */
static void lcd_write_char(lcd_context_t* ctx, char c) {
    lcd_send_byte(ctx, (uint8_t)c, LCD_RS_DATA);
}

// Funções públicas

lcd_context_t* lcd_init(void) {
    lcd_context_t* ctx = (lcd_context_t*)malloc(sizeof(lcd_context_t));
    if (!ctx) {
        fprintf(stderr, "❌ Erro ao alocar memória para LCD\n");
        return NULL;
    }
    
    memset(ctx, 0, sizeof(lcd_context_t));
    
    // Abrir barramento I2C
    char i2c_device[20];
    snprintf(i2c_device, sizeof(i2c_device), "/dev/i2c-%d", LCD_I2C_BUS);
    
    ctx->i2c_fd = open(i2c_device, O_RDWR);
    if (ctx->i2c_fd < 0) {
        fprintf(stderr, "❌ Erro ao abrir %s\n", i2c_device);
        free(ctx);
        return NULL;
    }
    
    // Configurar endereço I2C
    if (ioctl(ctx->i2c_fd, I2C_SLAVE, LCD_I2C_ADDRESS) < 0) {
        fprintf(stderr, "❌ Erro ao configurar endereço I2C 0x%02X\n", LCD_I2C_ADDRESS);
        close(ctx->i2c_fd);
        free(ctx);
        return NULL;
    }
    
    ctx->address = LCD_I2C_ADDRESS;
    ctx->backlight = LCD_BACKLIGHT_ON;
    ctx->current_screen = LCD_SCREEN_MAIN_TEMP;  // Iniciar com tela de temperatura principal
    ctx->setpoint = 5.0f;
    ctx->temp_max = -999.0f;  // Inicializar com valor muito baixo
    ctx->temp_min = 999.0f;   // Inicializar com valor muito alto
    ctx->wifi_connected = false;
    ctx->filtered_temp_ch1 = 0.0f;  // Inicializar filtro
    ctx->first_temp_reading = true;  // Primeira leitura será direta
    ctx->last_displayed_temp = -999.0f;  // Valor inválido para forçar primeiro desenho
    ctx->last_displayed_max = -999.0f;
    ctx->last_displayed_min = 999.0f;
    ctx->screen_needs_redraw = true;  // Forçar desenho inicial
    ctx->relay_test_target = LCD_RELAY_TEST_TARGET_LAMP;
    
    // Sequência de inicialização do LCD (modo 4 bits)
    usleep(50000);  // Aguardar 50ms após power-on
    
    // Inicialização em modo 8 bits (3 vezes)
    lcd_write_nibble(ctx, 0x30);
    usleep(5000);
    lcd_write_nibble(ctx, 0x30);
    usleep(200);
    lcd_write_nibble(ctx, 0x30);
    usleep(200);
    
    // Mudar para modo 4 bits
    lcd_write_nibble(ctx, 0x20);
    usleep(200);
    
    // Configurar display: 4 bits, 2 linhas, fonte 5x8
    lcd_command(ctx, LCD_CMD_FUNCTION_SET | LCD_4BIT_MODE | LCD_2LINE | LCD_5x8_DOTS);
    
    // Display on, cursor off, blink off
    lcd_command(ctx, LCD_CMD_DISPLAY_CTRL | LCD_DISPLAY_ON | LCD_CURSOR_OFF | LCD_BLINK_OFF);
    
    // Limpar display
    lcd_command(ctx, LCD_CMD_CLEAR);
    usleep(2000);
    
    // Entry mode: incrementar cursor, sem shift
    lcd_command(ctx, LCD_CMD_ENTRY_MODE | LCD_ENTRY_LEFT | LCD_ENTRY_SHIFT_DEC);
    
    ctx->initialized = true;

    // Criar caracteres customizados para barras verticais
    lcd_create_bargraph_chars(ctx);

    printf("✅ LCD 20x4 inicializado (I2C 0x%02X)\n", LCD_I2C_ADDRESS);

    return ctx;
}

void lcd_cleanup(lcd_context_t* ctx) {
    if (ctx) {
        if (ctx->i2c_fd >= 0) {
            lcd_clear(ctx);
            lcd_backlight(ctx, false);
            close(ctx->i2c_fd);
        }
        free(ctx);
    }
}

void lcd_clear(lcd_context_t* ctx) {
    if (!ctx || !ctx->initialized) return;
    lcd_command(ctx, LCD_CMD_CLEAR);
    usleep(2000);
}

void lcd_set_cursor(lcd_context_t* ctx, uint8_t col, uint8_t row) {
    if (!ctx || !ctx->initialized) return;
    if (col >= LCD_COLS || row >= LCD_ROWS) return;

    // Endereços de memória para cada linha do LCD 20x4
    static const uint8_t row_offsets[] = {0x00, 0x40, 0x14, 0x54};

    lcd_command(ctx, LCD_CMD_SET_DDRAM | (col + row_offsets[row]));
}

void lcd_print(lcd_context_t* ctx, const char* str) {
    if (!ctx || !ctx->initialized || !str) return;

    while (*str) {
        lcd_write_char(ctx, *str++);
    }
}

void lcd_backlight(lcd_context_t* ctx, bool on) {
    if (!ctx || !ctx->initialized) return;

    ctx->backlight = on ? LCD_BACKLIGHT_ON : LCD_BACKLIGHT_OFF;
    lcd_write_byte(ctx, ctx->backlight);
}

void lcd_display_splash(lcd_context_t* ctx, const char* device_name) {
    if (!ctx || !ctx->initialized) return;

    lcd_clear(ctx);

    // Linha 0: Nome do dispositivo centralizado
    lcd_set_cursor(ctx, 0, 0);
    lcd_print(ctx, "  NovaTherm Logger  ");

    // Linha 1: Versão
    lcd_set_cursor(ctx, 0, 1);
    lcd_print(ctx, "   LCD 20x4 I2C     ");

    // Linha 2: Dispositivo
    lcd_set_cursor(ctx, 0, 2);
    char line[21];
    snprintf(line, sizeof(line), "%-20s", device_name ? device_name : "NT18B07");
    lcd_print(ctx, line);

    // Linha 3: Nova Instruments
    lcd_set_cursor(ctx, 0, 3);
    lcd_print(ctx, " Nova Instruments   ");
}

void lcd_next_screen(lcd_context_t* ctx) {
    if (!ctx) return;
    ctx->current_screen = (ctx->current_screen + 1) % LCD_SCREEN_COUNT;
    ctx->screen_needs_redraw = true;  // Forçar redesenho ao mudar de tela
}

void lcd_previous_screen(lcd_context_t* ctx) {
    if (!ctx) return;
    if (ctx->current_screen == 0) {
        ctx->current_screen = LCD_SCREEN_COUNT - 1;
    } else {
        ctx->current_screen--;
    }
    ctx->screen_needs_redraw = true;  // Forçar redesenho ao mudar de tela
}

void lcd_increment_setpoint(lcd_context_t* ctx) {
    if (!ctx) return;
    ctx->setpoint += 0.5f;
    if (ctx->setpoint > 10.0f) {
        ctx->setpoint = 10.0f;
    }
}

void lcd_decrement_setpoint(lcd_context_t* ctx) {
    if (!ctx) return;
    ctx->setpoint -= 0.5f;
    if (ctx->setpoint < -10.0f) {
        ctx->setpoint = -10.0f;
    }
}

float lcd_get_setpoint(lcd_context_t* ctx) {
    return ctx ? ctx->setpoint : 0.0f;
}

/**
 * @brief Cria caracteres customizados para símbolos de porta
 *
 * Redefine temporariamente os caracteres 6 e 7 para exibir porta
 * (usado apenas na Tela 1 - Info Screen)
 */
static void lcd_create_door_chars(lcd_context_t* ctx) {
    if (!ctx || !ctx->initialized) return;

    // Caractere 6: Porta FECHADA (bloco cheio centralizado)
    uint8_t char6[8] = {
        0b01110,  // ┌──┐
        0b01110,  // │██│
        0b01110,  // │██│
        0b01110,  // │██│
        0b01110,  // │██│
        0b01110,  // │██│
        0b01110,  // │██│
        0b01110   // └──┘
    };

    // Caractere 7: Porta ABERTA (bloco deslocado para direita)
    uint8_t char7[8] = {
        0b00111,  //  ┌─┐
        0b00111,  //  │█│
        0b00111,  //  │█│
        0b00111,  //  │█│
        0b00111,  //  │█│
        0b00111,  //  │█│
        0b00111,  //  │█│
        0b00111   //  └─┘
    };

    // Gravar apenas caracteres 6 e 7 no CGRAM
    lcd_command(ctx, LCD_CMD_SET_CGRAM | (6 * 8));
    for (int j = 0; j < 8; j++) {
        lcd_write_char(ctx, char6[j]);
    }

    lcd_command(ctx, LCD_CMD_SET_CGRAM | (7 * 8));
    for (int j = 0; j < 8; j++) {
        lcd_write_char(ctx, char7[j]);
    }

    // Retornar ao modo DDRAM (display normal)
    lcd_command(ctx, LCD_CMD_SET_DDRAM);
}

/**
 * @brief Exibe tela 0: Temperatura principal em BigFont (tela inteira)
 *
 * Layout:
 * ┌────────────────────┐
 * │ Max:15.2° Min: 4.8°│  ← Linha 0: Máximo e Mínimo
 * │                    │  ← Linha 1: Vazia
 * │      12.3°C        │  ← Linhas 2-3: Temperatura grande centralizada
 * │ F    12.3°C  SP:5.0│  ← Linha 3: Porta + Temp (inferior) + Setpoint
 * └────────────────────┘
 */
static void lcd_display_main_temperature(lcd_context_t* ctx, const modbus_data_t* data,
                                         uint32_t record_count, bool door_open) {
    if (!ctx || !data) return;

    (void)record_count;  // Não usado nesta versão

    // Restaurar caracteres customizados para números grandes (caso tenham sido alterados)
    lcd_create_bargraph_chars(ctx);

    float temp_ch1 = data->ch_valid[0] && !data->ch_error[0] ? data->ch_temp[0] : 0.0f;
    bool ch1_valid = data->ch_valid[0] && !data->ch_error[0];

    if (ch1_valid) {
        // ===== APLICAR FILTRO EXPONENCIAL PASSA-BAIXAS (EMA) =====
        // Alpha = 0.2 (20% nova leitura, 80% histórico)
        // Suaviza oscilações mantendo responsividade para mudanças reais
        const float ALPHA = 0.2f;

        if (ctx->first_temp_reading) {
            // Primeira leitura: usar valor direto (sem filtro)
            ctx->filtered_temp_ch1 = temp_ch1;
            ctx->first_temp_reading = false;
            ctx->screen_needs_redraw = true;  // Forçar redesenho na primeira leitura
        } else {
            // Aplicar filtro: S(t) = α × Y(t) + (1 - α) × S(t-1)
            ctx->filtered_temp_ch1 = (ALPHA * temp_ch1) + ((1.0f - ALPHA) * ctx->filtered_temp_ch1);
        }

        // Atualizar min/max com temperatura FILTRADA
        lcd_update_temp_minmax(ctx, ctx->filtered_temp_ch1);

        // ===== VERIFICAR SE PRECISA REDESENHAR (evitar piscar) =====
        // Só redesenha se houver mudança de 0.1°C ou mais
        const float REDRAW_THRESHOLD = 0.1f;

        bool temp_changed = fabsf(ctx->filtered_temp_ch1 - ctx->last_displayed_temp) >= REDRAW_THRESHOLD;
        bool max_changed = fabsf(ctx->temp_max - ctx->last_displayed_max) >= REDRAW_THRESHOLD;
        bool min_changed = fabsf(ctx->temp_min - ctx->last_displayed_min) >= REDRAW_THRESHOLD;

        if (temp_changed || max_changed || min_changed || ctx->screen_needs_redraw) {
            // Limpar display apenas quando necessário
            lcd_clear(ctx);

            // Desenhar temperatura grande centralizada com Max, Min
            // Formato: "XX.X°C" nas linhas 2 e 3 (centralizadas)
            // Usar temperatura FILTRADA para exibição suave
            lcd_draw_bargraph(ctx, 0, ctx->filtered_temp_ch1, ctx->temp_min, ctx->temp_max);

            // Atualizar valores exibidos
            ctx->last_displayed_temp = ctx->filtered_temp_ch1;
            ctx->last_displayed_max = ctx->temp_max;
            ctx->last_displayed_min = ctx->temp_min;
            ctx->screen_needs_redraw = false;
        }

        // Exibir indicador de porta e setpoint na linha 3
        // Usa caracteres ASCII para não conflitar com caracteres customizados do BigFont
        lcd_set_cursor(ctx, 0, 3);  // Primeira coluna, linha 3
        lcd_print(ctx, door_open ? "A" : "F");  // A = Aberta, F = Fechada

        // Exibir setpoint no lado direito da linha 3
        lcd_set_cursor(ctx, 14, 3);  // Coluna 14, linha 3 (formato: "SP:XX.X")
        char sp_line[10];
        snprintf(sp_line, sizeof(sp_line), "SP:%.1f", ctx->setpoint);
        lcd_print(ctx, sp_line);

        // Se não mudou nada, não faz nada (evita piscar)

    } else {
        // Sensor inválido - só redesenha se necessário
        if (ctx->screen_needs_redraw || ctx->last_displayed_temp > -900.0f) {
            lcd_clear(ctx);

            // Mostrar "----" se sensor inválido
            lcd_set_cursor(ctx, 7, 2);
            lcd_print(ctx, "----");
            lcd_set_cursor(ctx, 7, 3);
            lcd_print(ctx, "----");

            ctx->last_displayed_temp = -999.0f;  // Marcar como inválido
            ctx->screen_needs_redraw = false;
        }

        // Resetar flag de primeira leitura quando sensor fica inválido
        ctx->first_temp_reading = true;
    }
}

/**
 * @brief Exibe tela 1: Informações detalhadas
 *
 * Layout:
 * ┌────────────────────┐
 * │ PR2:  8.5°C        │  ← Linha 0: PR2
 * │ AC:  10.2V         │  ← Linha 1: AC
 * │ DC:  12.8V         │  ← Linha 2: DC
 * │ 🚪 SP: 5.0         │  ← Linha 3: Porta + Setpoint
 * └────────────────────┘
 */
static void lcd_display_info_screen(lcd_context_t* ctx, const modbus_data_t* data,
                                    uint32_t record_count, bool door_open) {
    if (!ctx || !data) return;

    (void)record_count;  // Não usado nesta versão

    char line[21];

    // Criar caracteres customizados para porta (redefine chars 6 e 7)
    lcd_create_door_chars(ctx);

    // Limpar display
    lcd_clear(ctx);

    // Linha 0: PR2
    lcd_set_cursor(ctx, 0, 0);
    if (data->ch_valid[1] && !data->ch_error[1]) {
        snprintf(line, sizeof(line), "PR2: %5.1f%cC", data->ch_temp[1], 0xDF);
    } else {
        snprintf(line, sizeof(line), "PR2: ----%cC", 0xDF);
    }
    lcd_print(ctx, line);

    // Linha 1: AC
    lcd_set_cursor(ctx, 0, 1);
    if (data->ch_valid[2] && !data->ch_error[2]) {
        snprintf(line, sizeof(line), "AC:  %6.1fV", data->ch_temp[2]);
    } else {
        snprintf(line, sizeof(line), "AC:  ------V");
    }
    lcd_print(ctx, line);

    // Linha 2: DC
    lcd_set_cursor(ctx, 0, 2);
    if (data->ch_valid[3] && !data->ch_error[3]) {
        snprintf(line, sizeof(line), "DC:  %6.1fV", data->ch_temp[3]);
    } else {
        snprintf(line, sizeof(line), "DC:  ------V");
    }
    lcd_print(ctx, line);

    // Linha 3: Porta + Setpoint
    lcd_set_cursor(ctx, 0, 3);

    // Exibir símbolo de porta (caractere customizado 6 ou 7)
    lcd_write_char(ctx, door_open ? 7 : 6);  // 7 = aberta (deslocado), 6 = fechada (centralizado)
    lcd_print(ctx, " SP:");

    // Exibir setpoint (sem espaço extra)
    snprintf(line, sizeof(line), "%.1f", ctx->setpoint);
    lcd_print(ctx, line);
}

/**
 * @brief Exibe tela de ajuste de setpoint
 */
static void lcd_display_setpoint_screen(lcd_context_t* ctx) {
    if (!ctx) return;

    lcd_clear(ctx);

    char line[21];

    // Linha 0: Título
    lcd_set_cursor(ctx, 0, 0);
    lcd_print(ctx, "  AJUSTE SETPOINT   ");

    // Linha 1: Vazio
    lcd_set_cursor(ctx, 0, 1);
    lcd_print(ctx, "                    ");

    // Linha 2: Valor do setpoint (grande e centralizado)
    lcd_set_cursor(ctx, 0, 2);
    snprintf(line, sizeof(line), "    %+6.1f%cC      ", ctx->setpoint, 0xDF);
    lcd_print(ctx, line);

    // Linha 3: Instruções
    lcd_set_cursor(ctx, 0, 3);
    lcd_print(ctx, "  UP/DOWN p/ ajustar");
}

/**
 * @brief Exibe tela de diagnóstico
 */
static void lcd_display_diagnostics(lcd_context_t* ctx, bool lamp_on, bool dialer_on,
                                   bool compressor_on, bool heater_on) {
    if (!ctx) return;

    lcd_clear(ctx);

    char line[21];

    // Linha 0: Título
    lcd_set_cursor(ctx, 0, 0);
    lcd_print(ctx, "   DIAGNOSTICO      ");

    // Linha 1: Compressor e Resistência
    lcd_set_cursor(ctx, 0, 1);
    snprintf(line, sizeof(line), "Comp:%s Heat:%s",
             compressor_on ? "ON " : "OFF",
             heater_on ? "ON " : "OFF");
    lcd_print(ctx, line);

    // Linha 2: Lâmpada e Discadora
    lcd_set_cursor(ctx, 0, 2);
    snprintf(line, sizeof(line), "Lamp:%s Dial:%s",
             lamp_on ? "ON " : "OFF",
             dialer_on ? "ON " : "OFF");
    lcd_print(ctx, line);

    // Linha 3: Status geral
    lcd_set_cursor(ctx, 0, 3);
    int active_count = (lamp_on ? 1 : 0) + (dialer_on ? 1 : 0) +
                       (compressor_on ? 1 : 0) + (heater_on ? 1 : 0);
    snprintf(line, sizeof(line), " %d reles ativos     ", active_count);
    lcd_print(ctx, line);
}

/**
 * @brief Exibe tela 3: Teste temporário de relés Modbus
 *
 * Controle esperado:
 * - DEC: alterna seleção (Lâmpada / Discadora)
 * - INC: alterna ON/OFF do relé selecionado
 */
static void lcd_display_relay_test_screen(lcd_context_t* ctx, bool lamp_on, bool dialer_on) {
    if (!ctx) return;

    lcd_clear(ctx);

    char line[21];

    // Linha 0: Título
    lcd_set_cursor(ctx, 0, 0);
    lcd_print(ctx, " TESTE RELES MODBUS ");

    // Linha 1: Lâmpada
    lcd_set_cursor(ctx, 0, 1);
    snprintf(line, sizeof(line), "%c Lampada: %s      ",
             (ctx->relay_test_target == LCD_RELAY_TEST_TARGET_LAMP) ? '>' : ' ',
             lamp_on ? "ON " : "OFF");
    lcd_print(ctx, line);

    // Linha 2: Discadora
    lcd_set_cursor(ctx, 0, 2);
    snprintf(line, sizeof(line), "%c Discadora: %s    ",
             (ctx->relay_test_target == LCD_RELAY_TEST_TARGET_DIALER) ? '>' : ' ',
             dialer_on ? "ON " : "OFF");
    lcd_print(ctx, line);

    // Linha 3: Instruções
    lcd_set_cursor(ctx, 0, 3);
    lcd_print(ctx, "DEC=SEL  INC=TOGGLE");
}

static void lcd_display_slave2_temps_screen(lcd_context_t* ctx, const modbus_data_t* data,
                                            bool t1_valid, float t1,
                                            bool t2_valid, float t2) {
    if (!ctx) return;

    lcd_clear(ctx);

    char line[21];

    lcd_set_cursor(ctx, 0, 0);
    lcd_print(ctx, " S2 T1/T2 + S1 NTCs ");

    lcd_set_cursor(ctx, 0, 1);
    if (t1_valid) {
        snprintf(line, sizeof(line), "T1 (0x200): %5.1fC", t1);
    } else {
        snprintf(line, sizeof(line), "T1 (0x200):   ERR ");
    }
    lcd_print(ctx, line);

    lcd_set_cursor(ctx, 0, 2);
    if (t2_valid) {
        snprintf(line, sizeof(line), "T2 (0x201): %5.1fC", t2);
    } else {
        snprintf(line, sizeof(line), "T2 (0x201):   ERR ");
    }
    lcd_print(ctx, line);

    lcd_set_cursor(ctx, 0, 3);
    if (data && data->ch_valid[4] && data->ch_valid[5]) {
        snprintf(line, sizeof(line), "N1:%4.0f N2:%4.0f", data->ch_temp[4], data->ch_temp[5]);
    } else if (data && data->ch_valid[4]) {
        snprintf(line, sizeof(line), "N1:%4.0f N2: ERR", data->ch_temp[4]);
    } else if (data && data->ch_valid[5]) {
        snprintf(line, sizeof(line), "N1: ERR N2:%4.0f", data->ch_temp[5]);
    } else {
        snprintf(line, sizeof(line), "N1: ERR N2: ERR");
    }
    lcd_print(ctx, line);
}

void lcd_update_current_screen(lcd_context_t* ctx, const char* device_name,
                               const modbus_data_t* data, uint32_t record_count,
                               bool lamp_on, bool dialer_on, bool compressor_on, bool heater_on,
                               bool door_open,
                               bool slave2_t1_valid, float slave2_t1,
                               bool slave2_t2_valid, float slave2_t2) {
    if (!ctx || !ctx->initialized) return;

    switch (ctx->current_screen) {
        case LCD_SCREEN_MAIN_TEMP:
            // Tela 0: Temperatura principal em BigFont + Porta no canto
            lcd_display_main_temperature(ctx, data, record_count, door_open);
            break;

        case LCD_SCREEN_INFO:
            // Tela 1: Informações (CH2, CH3, CH4, Porta, SP)
            lcd_display_info_screen(ctx, data, record_count, door_open);
            break;

        case LCD_SCREEN_DIAGNOSTICS:
            // Tela 2: Status dos relés
            lcd_display_diagnostics(ctx, lamp_on, dialer_on, compressor_on, heater_on);
            break;

        case LCD_SCREEN_RELAY_TEST:
            // Tela 3: Teste temporário de relés Modbus
            lcd_display_relay_test_screen(ctx, lamp_on, dialer_on);
            break;

        case LCD_SCREEN_SLAVE2_TEMPS:
            // Tela 4: Temperaturas T1/T2 do slave 2
            lcd_display_slave2_temps_screen(ctx, data, slave2_t1_valid, slave2_t1,
                                            slave2_t2_valid, slave2_t2);
            break;

        default:
            break;
    }
}

bool lcd_is_relay_test_screen(lcd_context_t* ctx) {
    return (ctx && ctx->current_screen == LCD_SCREEN_RELAY_TEST);
}

void lcd_relay_test_toggle_target(lcd_context_t* ctx) {
    if (!ctx) return;

    if (ctx->relay_test_target == LCD_RELAY_TEST_TARGET_LAMP) {
        ctx->relay_test_target = LCD_RELAY_TEST_TARGET_DIALER;
    } else {
        ctx->relay_test_target = LCD_RELAY_TEST_TARGET_LAMP;
    }
}

lcd_relay_test_target_t lcd_relay_test_get_target(lcd_context_t* ctx) {
    if (!ctx) return LCD_RELAY_TEST_TARGET_LAMP;
    return ctx->relay_test_target;
}

/**
 * @brief Cria caracteres customizados para números grandes (BigFont02)
 *
 * Baseado em https://coeleveld.com/bigfont/ (BigFont02)
 * Cada dígito ocupa 3 colunas x 2 linhas
 *
 * Caracteres customizados (exatamente como BigFont02):
 * 0: LT (Left Top) - Canto superior esquerdo
 * 1: RT (Right Top) - Canto superior direito
 * 2: LB (Left Bottom) - Canto inferior esquerdo
 * 3: RB (Right Bottom) - Canto inferior direito
 * 4: UB (Upper Block) - Bloco superior
 * 5: LMB (Lower Mid Block) - Bloco meio inferior
 * 6: MB (Middle Block) - Bloco do meio
 * 7: Degree symbol (°)
 */
void lcd_create_bargraph_chars(lcd_context_t* ctx) {
    if (!ctx || !ctx->initialized) return;

    // Caractere 0: LT (Left Top) - Canto superior esquerdo
    uint8_t char0[8] = {
        0b00111,
        0b01111,
        0b11111,
        0b11111,
        0b11111,
        0b11111,
        0b11111,
        0b11111
    };

    // Caractere 1: UB (Upper Block) - Bloco superior
    uint8_t char1[8] = {
        0b11111,
        0b11111,
        0b11111,
        0b00000,
        0b00000,
        0b00000,
        0b00000,
        0b00000
    };

    // Caractere 2: RT (Right Top) - Canto superior direito
    uint8_t char2[8] = {
        0b11100,
        0b11110,
        0b11111,
        0b11111,
        0b11111,
        0b11111,
        0b11111,
        0b11111
    };

    // Caractere 3: LL (Left Lower) - Canto inferior esquerdo
    uint8_t char3[8] = {
        0b11111,
        0b11111,
        0b11111,
        0b11111,
        0b11111,
        0b11111,
        0b01111,
        0b00111
    };

    // Caractere 4: LB (Lower Block) - Bloco inferior
    uint8_t char4[8] = {
        0b00000,
        0b00000,
        0b00000,
        0b00000,
        0b00000,
        0b11111,
        0b11111,
        0b11111
    };

    // Caractere 5: LR (Lower Right) - Canto inferior direito
    uint8_t char5[8] = {
        0b11111,
        0b11111,
        0b11111,
        0b11111,
        0b11111,
        0b11111,
        0b11110,
        0b11100
    };

    // Caractere 6: MB (Middle Block) - Bloco do meio (barra horizontal dupla)
    uint8_t char6[8] = {
        0b11111,
        0b11111,
        0b11111,
        0b00000,
        0b00000,
        0b11111,
        0b11111,
        0b11111
    };

    // Caractere 7: Degree symbol (símbolo de grau °)
    uint8_t char7[8] = {
        0b00110,
        0b01001,
        0b01001,
        0b00110,
        0b00000,
        0b00000,
        0b00000,
        0b00000
    };

    // Array de ponteiros para facilitar iteração
    uint8_t* chars[8] = {char0, char1, char2, char3, char4, char5, char6, char7};

    // Gravar caracteres no CGRAM
    for (int i = 0; i < 8; i++) {
        lcd_command(ctx, LCD_CMD_SET_CGRAM | (i * 8));
        for (int j = 0; j < 8; j++) {
            lcd_write_char(ctx, chars[i][j]);
        }
    }

    // Retornar ao modo DDRAM (display normal)
    lcd_command(ctx, LCD_CMD_SET_DDRAM);
}

/**
 * @brief Desenha um dígito GRANDE (3 colunas x 2 linhas)
 *
 * Baseado no exemplo fornecido com caracteres otimizados
 * Usa 2 linhas do display 20x4
 *
 * @param ctx Contexto do LCD
 * @param col Coluna inicial (0-17 para caber 3 colunas)
 * @param row Linha inicial (0-2 para caber 2 linhas)
 * @param digit Dígito a desenhar (0-9)
 */
static void lcd_draw_big_digit(lcd_context_t* ctx, uint8_t col, uint8_t row, uint8_t digit) {
    if (!ctx || !ctx->initialized) return;
    if (col + 2 >= LCD_COLS || row + 1 >= LCD_ROWS) return;
    if (digit > 9) return;

    // Lookup table para dígitos GRANDES (2 linhas x 3 colunas)
    // Baseado no exemplo fornecido
    // Formato: [dígito][6 posições] = {linha0_col0, linha0_col1, linha0_col2, linha1_col0, linha1_col1, linha1_col2}
    // Caracteres: 0=LT, 1=UB, 2=RT, 3=LL, 4=LB, 5=LR, 6=MB, 7=°, 255=espaço
    static const uint8_t digit_patterns[10][6] = {
        {0,1,2, 3,4,5},         // 0
        {1,2,255,255,5,255},     // 1
        {6,6,2, 3,4,4},         // 2
        {6,6,2, 4,4,5},         // 3
        {3,4,5, 255,255,5},     // 4
        {3,6,6, 4,4,5},         // 5
        {3,6,6, 3,4,5},         // 6
        {1,1,2, 255,255,5},     // 7
        {3,6,2, 3,4,5},         // 8
        {3,6,2, 4,4,5}          // 9
    };

    // Desenhar linha superior (3 caracteres)
    lcd_set_cursor(ctx, col, row);
    for (int i = 0; i < 3; i++) {
        uint8_t ch = digit_patterns[digit][i];
        if (ch == 255) {
            lcd_print(ctx, " ");
        } else {
            lcd_write_char(ctx, ch);
        }
    }

    // Desenhar linha inferior (3 caracteres)
    lcd_set_cursor(ctx, col, row + 1);
    for (int i = 0; i < 3; i++) {
        uint8_t ch = digit_patterns[digit][i + 3];
        if (ch == 255) {
            lcd_print(ctx, " ");
        } else {
            lcd_write_char(ctx, ch);
        }
    }
}

/**
 * @brief Desenha temperatura GRANDE no display com Max e Min
 *
 * Layout:
 * Linha 0: "Max:XX.X°  Min:XX.X°"
 * Linha 1: (vazia)
 * Linhas 2-3: Temperatura grande centralizada "XX.X°C"
 *
 * @param ctx Contexto do LCD
 * @param col Coluna (não usado)
 * @param value Temperatura atual
 * @param min_value Temperatura mínima
 * @param max_value Temperatura máxima
 */
void lcd_draw_bargraph(lcd_context_t* ctx, uint8_t col, float value, float min_value, float max_value) {
    (void)col;

    if (!ctx || !ctx->initialized) return;

    float temperature = value;

    // Limitar temperatura ao range -9.9 a 99.9
    if (temperature < -9.9f) temperature = -9.9f;
    if (temperature > 99.9f) temperature = 99.9f;

    // Extrair dígitos da temperatura principal
    int temp_int = (int)(temperature * 10.0f);
    bool negative = temp_int < 0;
    if (negative) temp_int = -temp_int;

    int digit1 = (temp_int / 100) % 10;  // Dezena
    int digit2 = (temp_int / 10) % 10;   // Unidade
    int digit3 = temp_int % 10;          // Decimal

    // Limpar display inteiro
    lcd_clear(ctx);

    // ===== LINHA 0: Max e Min =====
    lcd_set_cursor(ctx, 0, 0);
    lcd_print(ctx, "Ma:");
    char temp_str[8];

    // Exibir Max (usar valor do contexto se disponível)
    if (ctx->temp_max > -900.0f) {
        snprintf(temp_str, sizeof(temp_str), "%.1f", ctx->temp_max);
    } else {
        snprintf(temp_str, sizeof(temp_str), "--.-");
    }
    lcd_print(ctx, temp_str);
    lcd_write_char(ctx, 7);  // Símbolo de grau

    lcd_set_cursor(ctx, 13, 0);
    lcd_print(ctx, "Mi:");

    // Exibir Min (usar valor do contexto se disponível)
    if (ctx->temp_min < 900.0f) {
        snprintf(temp_str, sizeof(temp_str), "%.1f", ctx->temp_min);
    } else {
        snprintf(temp_str, sizeof(temp_str), "--.-");
    }
    lcd_print(ctx, temp_str);
    lcd_write_char(ctx, 7);  // Símbolo de grau

    // ===== LINHAS 2-3: Temperatura grande (2 colunas à esquerda) =====
    uint8_t start_col = 1;  // Movido de 3 para 1 (2 colunas à esquerda)

    // Sempre desenhar dezena para manter centralização
    if (negative) {
        // Desenhar sinal de menos (centralizado na linha 2)
        lcd_set_cursor(ctx, start_col + 1, 2);
        lcd_print(ctx, "-");
    } else if (digit1 > 0) {
        // Desenhar dezena se for maior que 0
        lcd_draw_big_digit(ctx, start_col, 2, digit1);
    } else {
        // Desenhar espaços vazios para manter posição (temperatura < 10)
        lcd_set_cursor(ctx, start_col, 2);
        lcd_print(ctx, "   ");
        lcd_set_cursor(ctx, start_col, 3);
        lcd_print(ctx, "   ");
    }
    start_col += 3;

    // Desenhar unidade
    lcd_draw_big_digit(ctx, start_col, 2, digit2);
    start_col += 3;

    // Desenhar ponto decimal (na linha 3)
    lcd_set_cursor(ctx, start_col, 3);
    lcd_print(ctx, ".");
    start_col += 1;

    // Desenhar decimal
    lcd_draw_big_digit(ctx, start_col, 2, digit3);
    start_col += 3;

    // Desenhar °C (na linha 2)
    lcd_set_cursor(ctx, start_col, 2);
    lcd_write_char(ctx, 7);  // Símbolo de grau
    lcd_print(ctx, "C");
}

/**
 * @brief Atualiza os valores de temperatura máxima e mínima
 *
 * Mantém registro das temperaturas máxima e mínima observadas
 *
 * @param ctx Contexto do LCD
 * @param current_temp Temperatura atual
 */
void lcd_update_temp_minmax(lcd_context_t* ctx, float current_temp) {
    if (!ctx) return;

    // Atualizar máximo
    if (current_temp > ctx->temp_max || ctx->temp_max < -900.0f) {
        ctx->temp_max = current_temp;
    }

    // Atualizar mínimo
    if (current_temp < ctx->temp_min || ctx->temp_min > 900.0f) {
        ctx->temp_min = current_temp;
    }
}

/**
 * @brief Define o status da conexão WiFi
 *
 * @param ctx Contexto do LCD
 * @param connected true se conectado, false caso contrário
 */
void lcd_set_wifi_status(lcd_context_t* ctx, bool connected) {
    if (!ctx) return;
    ctx->wifi_connected = connected;
}
