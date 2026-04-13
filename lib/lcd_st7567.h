/**
 * @file lcd_st7567.h
 * @brief NovaTherm DataLogger - ST7567 128x64 COG LCD Display Library
 * @author Nova Instruments
 *
 * Biblioteca para controle de display LCD 128x64 com controlador ST7567
 * Model: SLG12864Q (2.2" IIC COG)
 * Interface: I2C
 * Resolution: 128x64 pixels
 * Outline: 56.00 x 44.60 mm
 */

#ifndef LCD_ST7567_H
#define LCD_ST7567_H

#include <stdint.h>
#include <stdbool.h>
#include "modbus.h"

#ifdef __cplusplus
extern "C" {
#endif

// Configurações do display ST7567
#define ST7567_I2C_ADDRESS_PRIMARY    0x3F    // Endereço I2C primário
#define ST7567_I2C_ADDRESS_SECONDARY  0x3C    // Endereço I2C alternativo
#define ST7567_I2C_ADDRESS_TERTIARY   0x3D    // Endereço I2C alternativo 2
#define ST7567_WIDTH        128     // Largura em pixels
#define ST7567_HEIGHT       64      // Altura em pixels
#define ST7567_I2C_BUS      1       // Barramento I2C (/dev/i2c-1)
#define ST7567_PAGES        8       // 64 pixels / 8 = 8 páginas

// Comandos ST7567
#define ST7567_CMD_DISPLAY_OFF          0xAE    // Display OFF
#define ST7567_CMD_DISPLAY_ON           0xAF    // Display ON
#define ST7567_CMD_SET_START_LINE       0x40    // Set display start line (0x40-0x7F)
#define ST7567_CMD_SET_PAGE_ADDR        0xB0    // Set page address (0xB0-0xB7)
#define ST7567_CMD_SET_COLUMN_ADDR_MSB  0x10    // Set column address MSB (0x10-0x1F)
#define ST7567_CMD_SET_COLUMN_ADDR_LSB  0x00    // Set column address LSB (0x00-0x0F)
#define ST7567_CMD_SEG_NORMAL           0xA0    // SEG normal direction
#define ST7567_CMD_SEG_REVERSE          0xA1    // SEG reverse direction
#define ST7567_CMD_DISPLAY_NORMAL       0xA6    // Normal display
#define ST7567_CMD_DISPLAY_REVERSE      0xA7    // Reverse display
#define ST7567_CMD_ALL_PIXEL_ON         0xA5    // All pixel ON
#define ST7567_CMD_ALL_PIXEL_NORMAL     0xA4    // Resume to RAM content display
#define ST7567_CMD_LCD_BIAS_1_9         0xA2    // LCD bias 1/9
#define ST7567_CMD_LCD_BIAS_1_7         0xA3    // LCD bias 1/7
#define ST7567_CMD_RESET                0xE2    // Internal reset
#define ST7567_CMD_COM_NORMAL           0xC0    // COM output normal direction
#define ST7567_CMD_COM_REVERSE          0xC8    // COM output reverse direction
#define ST7567_CMD_POWER_CTRL           0x28    // Power control (0x28-0x2F)
#define ST7567_CMD_VOLTAGE_REGULATOR    0x20    // Voltage regulator internal resistor ratio (0x20-0x27)
#define ST7567_CMD_CONTRAST             0x81    // Electronic volume mode set
#define ST7567_CMD_BOOSTER_SET          0xF8    // Booster ratio set
#define ST7567_CMD_NOP                  0xE3    // NOP

// Prefixos de controle I2C
#define ST7567_CONTROL_CMD_SINGLE       0x80    // Co=1, D/C=0 - Single command byte
#define ST7567_CONTROL_CMD_STREAM       0x00    // Co=0, D/C=0 - Command stream
#define ST7567_CONTROL_DATA_STREAM      0x40    // Co=0, D/C=1 - Data stream

// Enumeração das telas disponíveis
typedef enum {
    ST7567_SCREEN_MAIN_TEMP = 0,    // Tela 0: Temperatura principal
    ST7567_SCREEN_INFO,             // Tela 1: Informações (todos os canais)
    ST7567_SCREEN_DIAGNOSTICS,      // Tela 2: Status dos relés
    ST7567_SCREEN_COUNT             // Total de telas
} st7567_screen_t;

// Contexto do display ST7567
typedef struct st7567_context_s {
    int i2c_fd;                     // File descriptor do I2C
    uint8_t address;                // Endereço I2C do display
    bool initialized;               // Flag de inicialização
    uint8_t buffer[ST7567_WIDTH * ST7567_PAGES]; // Buffer de vídeo (128x64 pixels = 1024 bytes)
    st7567_screen_t current_screen; // Tela atual
    float setpoint;                 // Setpoint ajustável
    float temp_max;                 // Temperatura máxima registrada
    float temp_min;                 // Temperatura mínima registrada
    bool wifi_connected;            // Status da conexão WiFi
} st7567_context_t;

/**
 * @brief Inicializa o display ST7567 128x64
 * @return Ponteiro para o contexto do display, ou NULL em caso de erro
 */
st7567_context_t* st7567_init(void);

/**
 * @brief Finaliza o display ST7567 e libera recursos
 * @param ctx Contexto do display
 */
void st7567_cleanup(st7567_context_t* ctx);

/**
 * @brief Limpa o buffer do display (preenche com zeros)
 * @param ctx Contexto do display
 */
void st7567_clear(st7567_context_t* ctx);

/**
 * @brief Atualiza o display com o conteúdo do buffer
 * @param ctx Contexto do display
 * @return true se bem sucedido, false caso contrário
 */
bool st7567_display(st7567_context_t* ctx);

/**
 * @brief Define um pixel no buffer
 * @param ctx Contexto do display
 * @param x Coordenada X (0-127)
 * @param y Coordenada Y (0-63)
 * @param color 1 = pixel ligado, 0 = pixel desligado
 */
void st7567_set_pixel(st7567_context_t* ctx, uint8_t x, uint8_t y, uint8_t color);

/**
 * @brief Desenha um caractere no buffer
 * @param ctx Contexto do display
 * @param x Coordenada X (0-127)
 * @param y Coordenada Y (0-63)
 * @param c Caractere a desenhar
 */
void st7567_draw_char(st7567_context_t* ctx, uint8_t x, uint8_t y, char c);

/**
 * @brief Desenha uma string no buffer
 * @param ctx Contexto do display
 * @param x Coordenada X (0-127)
 * @param y Coordenada Y (0-63)
 * @param str String a desenhar
 */
void st7567_draw_string(st7567_context_t* ctx, uint8_t x, uint8_t y, const char* str);

/**
 * @brief Exibe tela de splash/inicialização
 * @param ctx Contexto do display
 * @param device_name Nome do dispositivo
 */
void st7567_display_splash(st7567_context_t* ctx, const char* device_name);

/**
 * @brief Navega para a próxima tela
 * @param ctx Contexto do display
 */
void st7567_next_screen(st7567_context_t* ctx);

/**
 * @brief Navega para a tela anterior
 * @param ctx Contexto do display
 */
void st7567_previous_screen(st7567_context_t* ctx);

/**
 * @brief Incrementa o setpoint
 * @param ctx Contexto do display
 */
void st7567_increment_setpoint(st7567_context_t* ctx);

/**
 * @brief Decrementa o setpoint
 * @param ctx Contexto do display
 */
void st7567_decrement_setpoint(st7567_context_t* ctx);

/**
 * @brief Obtém o valor atual do setpoint
 * @param ctx Contexto do display
 * @return Valor do setpoint em °C
 */
float st7567_get_setpoint(st7567_context_t* ctx);

/**
 * @brief Atualiza o display com a tela atual
 * @param ctx Contexto do display
 * @param device_name Nome do dispositivo
 * @param data Dados Modbus
 * @param record_count Número de registros
 * @param lamp_on Estado do relé da lâmpada
 * @param dialer_on Estado do relé da discadora
 * @param compressor_on Estado do relé do compressor
 * @param heater_on Estado do relé da resistência
 * @param door_open Estado da porta (true = aberta, false = fechada)
 */
void st7567_update_current_screen(st7567_context_t* ctx, const char* device_name,
                                  const modbus_data_t* data, uint32_t record_count,
                                  bool lamp_on, bool dialer_on, bool compressor_on, bool heater_on,
                                  bool door_open);

/**
 * @brief Desenha uma linha horizontal
 * @param ctx Contexto do display
 * @param x Coordenada X inicial
 * @param y Coordenada Y
 * @param width Largura da linha
 */
void st7567_draw_hline(st7567_context_t* ctx, uint8_t x, uint8_t y, uint8_t width);

/**
 * @brief Desenha uma linha vertical
 * @param ctx Contexto do display
 * @param x Coordenada X
 * @param y Coordenada Y inicial
 * @param height Altura da linha
 */
void st7567_draw_vline(st7567_context_t* ctx, uint8_t x, uint8_t y, uint8_t height);

/**
 * @brief Desenha um retângulo
 * @param ctx Contexto do display
 * @param x Coordenada X do canto superior esquerdo
 * @param y Coordenada Y do canto superior esquerdo
 * @param width Largura do retângulo
 * @param height Altura do retângulo
 * @param filled true para retângulo preenchido, false para apenas contorno
 */
void st7567_draw_rect(st7567_context_t* ctx, uint8_t x, uint8_t y, uint8_t width, uint8_t height, bool filled);

#ifdef __cplusplus
}
#endif

#endif // LCD_ST7567_H
