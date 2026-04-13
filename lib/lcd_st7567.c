/**
 * @file lcd_st7567.c
 * @brief NovaTherm DataLogger - ST7567 128x64 COG LCD Display Implementation
 * @author Nova Instruments
 */

#include "lcd_st7567.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <time.h>

// Fonte 5x7 para caracteres ASCII 32-127
static const uint8_t font5x7[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // 32 ' '
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // 33 !
    {0x00, 0x07, 0x00, 0x07, 0x00}, // 34 "
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // 35 #
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // 36 $
    {0x23, 0x13, 0x08, 0x64, 0x62}, // 37 %
    {0x36, 0x49, 0x55, 0x22, 0x50}, // 38 &
    {0x00, 0x05, 0x03, 0x00, 0x00}, // 39 '
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // 40 (
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // 41 )
    {0x14, 0x08, 0x3E, 0x08, 0x14}, // 42 *
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // 43 +
    {0x00, 0x50, 0x30, 0x00, 0x00}, // 44 ,
    {0x08, 0x08, 0x08, 0x08, 0x08}, // 45 -
    {0x00, 0x60, 0x60, 0x00, 0x00}, // 46 .
    {0x20, 0x10, 0x08, 0x04, 0x02}, // 47 /
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 48 0
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 49 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 50 2
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 51 3
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 52 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 53 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 54 6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 55 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 56 8
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // 57 9
    {0x00, 0x36, 0x36, 0x00, 0x00}, // 58 :
    {0x00, 0x56, 0x36, 0x00, 0x00}, // 59 ;
    {0x08, 0x14, 0x22, 0x41, 0x00}, // 60 <
    {0x14, 0x14, 0x14, 0x14, 0x14}, // 61 =
    {0x00, 0x41, 0x22, 0x14, 0x08}, // 62 >
    {0x02, 0x01, 0x51, 0x09, 0x06}, // 63 ?
    {0x32, 0x49, 0x79, 0x41, 0x3E}, // 64 @
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // 65 A
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // 66 B
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // 67 C
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // 68 D
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // 69 E
    {0x7F, 0x09, 0x09, 0x09, 0x01}, // 70 F
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, // 71 G
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // 72 H
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // 73 I
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // 74 J
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // 75 K
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // 76 L
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // 77 M
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // 78 N
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // 79 O
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // 80 P
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // 81 Q
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // 82 R
    {0x46, 0x49, 0x49, 0x49, 0x31}, // 83 S
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // 84 T
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // 85 U
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // 86 V
    {0x3F, 0x40, 0x38, 0x40, 0x3F}, // 87 W
    {0x63, 0x14, 0x08, 0x14, 0x63}, // 88 X
    {0x07, 0x08, 0x70, 0x08, 0x07}, // 89 Y
    {0x61, 0x51, 0x49, 0x45, 0x43}, // 90 Z
    {0x00, 0x7F, 0x41, 0x41, 0x00}, // 91 [
    {0x02, 0x04, 0x08, 0x10, 0x20}, // 92 '\'
    {0x00, 0x41, 0x41, 0x7F, 0x00}, // 93 ]
    {0x04, 0x02, 0x01, 0x02, 0x04}, // 94 ^
    {0x40, 0x40, 0x40, 0x40, 0x40}, // 95 _
};

// Funções privadas de baixo nível

/**
 * @brief Envia um comando para o display
 */
static bool st7567_send_command(st7567_context_t* ctx, uint8_t cmd) {
    if (!ctx || ctx->i2c_fd < 0) return false;
    
    uint8_t buffer[2];
    buffer[0] = ST7567_CONTROL_CMD_SINGLE;  // Control byte: command
    buffer[1] = cmd;
    
    if (write(ctx->i2c_fd, buffer, 2) != 2) {
        return false;
    }
    return true;
}

/**
 * @brief Envia dados para o display
 */
static bool st7567_send_data(st7567_context_t* ctx, const uint8_t* data, size_t len) {
    if (!ctx || ctx->i2c_fd < 0 || !data || len == 0) return false;
    
    // Enviar em blocos (I2C tem limite)
    const size_t chunk_size = 16;
    for (size_t i = 0; i < len; i += chunk_size) {
        size_t current_len = (i + chunk_size > len) ? (len - i) : chunk_size;
        
        uint8_t buffer[chunk_size + 1];
        buffer[0] = ST7567_CONTROL_DATA_STREAM;  // Control byte: data stream
        memcpy(&buffer[1], &data[i], current_len);
        
        if (write(ctx->i2c_fd, buffer, current_len + 1) != (ssize_t)(current_len + 1)) {
            return false;
        }
    }
    
    return true;
}

// Funções públicas

st7567_context_t* st7567_init(void) {
    st7567_context_t* ctx = (st7567_context_t*)malloc(sizeof(st7567_context_t));
    if (!ctx) {
        fprintf(stderr, "❌ Erro ao alocar memória para ST7567\n");
        return NULL;
    }
    
    memset(ctx, 0, sizeof(st7567_context_t));
    
    // Tentar abrir barramento I2C (tentar I2C-1 primeiro, depois I2C-0)
    char i2c_device[20];
    int bus_numbers[] = {ST7567_I2C_BUS, 0};  // Tentar I2C-1 e I2C-0

    for (int b = 0; b < 2; b++) {
        snprintf(i2c_device, sizeof(i2c_device), "/dev/i2c-%d", bus_numbers[b]);
        ctx->i2c_fd = open(i2c_device, O_RDWR);
        if (ctx->i2c_fd >= 0) {
            printf("📡 Barramento I2C aberto: %s\n", i2c_device);
            break;
        }
    }

    if (ctx->i2c_fd < 0) {
        fprintf(stderr, "❌ Erro ao abrir barramentos I2C (/dev/i2c-0 e /dev/i2c-1)\n");
        free(ctx);
        return NULL;
    }

    // Tentar múltiplos endereços I2C
    uint8_t addresses[] = {
        ST7567_I2C_ADDRESS_PRIMARY,
        ST7567_I2C_ADDRESS_SECONDARY,
        ST7567_I2C_ADDRESS_TERTIARY
    };

    bool address_found = false;
    for (int i = 0; i < 3; i++) {
        if (ioctl(ctx->i2c_fd, I2C_SLAVE, addresses[i]) == 0) {
            // Tentar enviar um comando de teste (NOP)
            uint8_t test_buf[2] = {ST7567_CONTROL_CMD_SINGLE, ST7567_CMD_NOP};
            if (write(ctx->i2c_fd, test_buf, 2) == 2) {
                ctx->address = addresses[i];
                address_found = true;
                printf("✅ ST7567 encontrado no endereço I2C 0x%02X\n", addresses[i]);
                break;
            }
        }
    }

    if (!address_found) {
        fprintf(stderr, "❌ ST7567 não encontrado nos endereços 0x3F, 0x3C ou 0x3D\n");
        fprintf(stderr, "   Execute 'sudo i2cdetect -y 1' para verificar endereços disponíveis\n");
        close(ctx->i2c_fd);
        free(ctx);
        return NULL;
    }
    ctx->current_screen = ST7567_SCREEN_MAIN_TEMP;
    ctx->setpoint = 5.0f;
    ctx->temp_max = -999.0f;
    ctx->temp_min = 999.0f;
    ctx->wifi_connected = false;
    
    // Sequência de inicialização do ST7567
    usleep(100000);  // Aguardar 100ms após power-on
    
    // Reset interno
    if (!st7567_send_command(ctx, ST7567_CMD_RESET)) {
        fprintf(stderr, "❌ Erro ao enviar comando de reset\n");
        close(ctx->i2c_fd);
        free(ctx);
        return NULL;
    }
    usleep(10000);  // Aguardar reset
    
    // Configuração básica
    st7567_send_command(ctx, ST7567_CMD_LCD_BIAS_1_9);      // LCD bias 1/9
    st7567_send_command(ctx, ST7567_CMD_SEG_NORMAL);        // SEG normal
    st7567_send_command(ctx, ST7567_CMD_COM_REVERSE);       // COM reverse (ajustar conforme necessário)
    st7567_send_command(ctx, ST7567_CMD_SET_START_LINE | 0); // Start line = 0
    
    // Power control
    st7567_send_command(ctx, ST7567_CMD_POWER_CTRL | 0x07); // Booster, regulator, follower ON
    
    // Voltage regulator
    st7567_send_command(ctx, ST7567_CMD_VOLTAGE_REGULATOR | 0x05); // Resistor ratio
    
    // Contraste
    st7567_send_command(ctx, ST7567_CMD_CONTRAST);
    st7567_send_command(ctx, 0x20);  // Valor de contraste (0x00-0x3F)
    
    // Display normal
    st7567_send_command(ctx, ST7567_CMD_ALL_PIXEL_NORMAL);
    st7567_send_command(ctx, ST7567_CMD_DISPLAY_NORMAL);
    
    // Ligar display
    st7567_send_command(ctx, ST7567_CMD_DISPLAY_ON);
    
    ctx->initialized = true;
    
    // Limpar buffer
    st7567_clear(ctx);
    st7567_display(ctx);

    printf("✅ ST7567 128x64 inicializado com sucesso (I2C 0x%02X)\n", ctx->address);

    return ctx;
}

void st7567_cleanup(st7567_context_t* ctx) {
    if (ctx) {
        if (ctx->i2c_fd >= 0) {
            st7567_clear(ctx);
            st7567_send_command(ctx, ST7567_CMD_DISPLAY_OFF);
            close(ctx->i2c_fd);
        }
        free(ctx);
    }
}

void st7567_clear(st7567_context_t* ctx) {
    if (!ctx) return;
    memset(ctx->buffer, 0, sizeof(ctx->buffer));
}

bool st7567_display(st7567_context_t* ctx) {
    if (!ctx || !ctx->initialized) return false;

    // Enviar buffer página por página
    for (uint8_t page = 0; page < ST7567_PAGES; page++) {
        // Set page address
        if (!st7567_send_command(ctx, ST7567_CMD_SET_PAGE_ADDR | page)) {
            return false;
        }

        // Set column address (start at column 0)
        if (!st7567_send_command(ctx, ST7567_CMD_SET_COLUMN_ADDR_MSB | 0)) {
            return false;
        }
        if (!st7567_send_command(ctx, ST7567_CMD_SET_COLUMN_ADDR_LSB | 0)) {
            return false;
        }

        // Enviar dados da página
        if (!st7567_send_data(ctx, &ctx->buffer[page * ST7567_WIDTH], ST7567_WIDTH)) {
            return false;
        }
    }

    return true;
}

void st7567_set_pixel(st7567_context_t* ctx, uint8_t x, uint8_t y, uint8_t color) {
    if (!ctx || x >= ST7567_WIDTH || y >= ST7567_HEIGHT) return;

    uint16_t index = x + (y / 8) * ST7567_WIDTH;
    uint8_t bit = y % 8;

    if (color) {
        ctx->buffer[index] |= (1 << bit);
    } else {
        ctx->buffer[index] &= ~(1 << bit);
    }
}

void st7567_draw_char(st7567_context_t* ctx, uint8_t x, uint8_t y, char c) {
    if (!ctx || c < 32 || c > 95) return;

    int index = c - 32;
    if (index >= 0 && index < 64) {  // Caracteres de espaço (32) até _ (95)
        for (int i = 0; i < 5; i++) {
            for (int j = 0; j < 8; j++) {
                if (font5x7[index][i] & (1 << j)) {
                    st7567_set_pixel(ctx, x + i, y + j, 1);
                }
            }
        }
    }
}

void st7567_draw_string(st7567_context_t* ctx, uint8_t x, uint8_t y, const char* str) {
    if (!ctx || !str) return;

    uint8_t cur_x = x;
    while (*str) {
        st7567_draw_char(ctx, cur_x, y, *str);
        cur_x += 6;  // 5 pixels + 1 de espaçamento
        if (cur_x >= ST7567_WIDTH) break;
        str++;
    }
}

void st7567_draw_hline(st7567_context_t* ctx, uint8_t x, uint8_t y, uint8_t width) {
    if (!ctx) return;

    for (uint8_t i = 0; i < width && (x + i) < ST7567_WIDTH; i++) {
        st7567_set_pixel(ctx, x + i, y, 1);
    }
}

void st7567_draw_vline(st7567_context_t* ctx, uint8_t x, uint8_t y, uint8_t height) {
    if (!ctx) return;

    for (uint8_t i = 0; i < height && (y + i) < ST7567_HEIGHT; i++) {
        st7567_set_pixel(ctx, x, y + i, 1);
    }
}

void st7567_draw_rect(st7567_context_t* ctx, uint8_t x, uint8_t y, uint8_t width, uint8_t height, bool filled) {
    if (!ctx) return;

    if (filled) {
        for (uint8_t i = 0; i < height && (y + i) < ST7567_HEIGHT; i++) {
            st7567_draw_hline(ctx, x, y + i, width);
        }
    } else {
        st7567_draw_hline(ctx, x, y, width);                 // Top
        st7567_draw_hline(ctx, x, y + height - 1, width);    // Bottom
        st7567_draw_vline(ctx, x, y, height);                // Left
        st7567_draw_vline(ctx, x + width - 1, y, height);    // Right
    }
}

void st7567_display_splash(st7567_context_t* ctx, const char* device_name) {
    if (!ctx || !ctx->initialized) return;

    st7567_clear(ctx);

    // Desenhar borda
    st7567_draw_rect(ctx, 0, 0, ST7567_WIDTH, ST7567_HEIGHT, false);

    // Título
    st7567_draw_string(ctx, 10, 10, "NovaTherm Logger");

    // Linha separadora
    st7567_draw_hline(ctx, 5, 25, ST7567_WIDTH - 10);

    // Modelo do display
    st7567_draw_string(ctx, 15, 30, "ST7567 128x64");

    // Nome do dispositivo
    if (device_name) {
        st7567_draw_string(ctx, 15, 40, device_name);
    }

    // Nova Instruments
    st7567_draw_string(ctx, 10, 52, "Nova Instruments");

    st7567_display(ctx);
}

void st7567_next_screen(st7567_context_t* ctx) {
    if (!ctx) return;
    ctx->current_screen = (st7567_screen_t)((ctx->current_screen + 1) % ST7567_SCREEN_COUNT);
}

void st7567_previous_screen(st7567_context_t* ctx) {
    if (!ctx) return;

    if (ctx->current_screen == 0) {
        ctx->current_screen = (st7567_screen_t)(ST7567_SCREEN_COUNT - 1);
    } else {
        ctx->current_screen = (st7567_screen_t)(ctx->current_screen - 1);
    }
}

void st7567_increment_setpoint(st7567_context_t* ctx) {
    if (!ctx) return;

    ctx->setpoint += 0.5f;
    if (ctx->setpoint > 30.0f) {
        ctx->setpoint = 30.0f;
    }
}

void st7567_decrement_setpoint(st7567_context_t* ctx) {
    if (!ctx) return;

    ctx->setpoint -= 0.5f;
    if (ctx->setpoint < -20.0f) {
        ctx->setpoint = -20.0f;
    }
}

float st7567_get_setpoint(st7567_context_t* ctx) {
    if (!ctx) return 0.0f;
    return ctx->setpoint;
}

void st7567_update_current_screen(st7567_context_t* ctx, const char* device_name,
                                  const modbus_data_t* data, uint32_t record_count,
                                  bool lamp_on, bool dialer_on, bool compressor_on, bool heater_on,
                                  bool door_open) {
    if (!ctx || !ctx->initialized) return;

    st7567_clear(ctx);

    char line[32];

    // Desenhar borda
    st7567_draw_rect(ctx, 0, 0, ST7567_WIDTH, ST7567_HEIGHT, false);

    switch (ctx->current_screen) {
        case ST7567_SCREEN_MAIN_TEMP:
            // Tela 0: Temperatura principal
            st7567_draw_string(ctx, 5, 3, "TEMP PRINCIPAL");
            st7567_draw_hline(ctx, 2, 13, ST7567_WIDTH - 4);

            if (data) {
                // CH1 - Temperatura principal (grande)
                snprintf(line, sizeof(line), "CH1: %.1f C", data->ch_temp[0]);
                st7567_draw_string(ctx, 10, 20, line);

                // Setpoint
                snprintf(line, sizeof(line), "SP:  %.1f C", ctx->setpoint);
                st7567_draw_string(ctx, 10, 33, line);

                // Status da porta
                snprintf(line, sizeof(line), "Porta: %s", door_open ? "ABERTA" : "FECHADA");
                st7567_draw_string(ctx, 10, 46, line);
            }
            break;

        case ST7567_SCREEN_INFO:
            // Tela 1: Informações de todos os canais
            st7567_draw_string(ctx, 5, 3, "CANAIS TEMP");
            st7567_draw_hline(ctx, 2, 13, ST7567_WIDTH - 4);

            if (data) {
                // CH1
                snprintf(line, sizeof(line), "CH1: %.1fC", data->ch_temp[0]);
                st7567_draw_string(ctx, 5, 18, line);

                // CH2
                snprintf(line, sizeof(line), "CH2: %.1fC", data->ch_temp[1]);
                st7567_draw_string(ctx, 5, 28, line);

                // CH3
                snprintf(line, sizeof(line), "CH3: %.1fC", data->ch_temp[2]);
                st7567_draw_string(ctx, 5, 38, line);

                // CH4
                snprintf(line, sizeof(line), "CH4: %.1fC", data->ch_temp[3]);
                st7567_draw_string(ctx, 5, 48, line);
            }
            break;

        case ST7567_SCREEN_DIAGNOSTICS:
            // Tela 2: Status dos relés
            st7567_draw_string(ctx, 5, 3, "STATUS RELES");
            st7567_draw_hline(ctx, 2, 13, ST7567_WIDTH - 4);

            // Lamp
            snprintf(line, sizeof(line), "Lamp: %s", lamp_on ? "ON " : "OFF");
            st7567_draw_string(ctx, 5, 20, line);

            // Discadora
            snprintf(line, sizeof(line), "Disc: %s", dialer_on ? "ON " : "OFF");
            st7567_draw_string(ctx, 5, 30, line);

            // Compressor
            snprintf(line, sizeof(line), "Comp: %s", compressor_on ? "ON " : "OFF");
            st7567_draw_string(ctx, 5, 40, line);

            // Heater
            snprintf(line, sizeof(line), "Heat: %s", heater_on ? "ON " : "OFF");
            st7567_draw_string(ctx, 5, 50, line);
            break;

        default:
            break;
    }

    st7567_display(ctx);
}
