/**
 * @file oled_ssd1306.h
 * @brief NovaTherm DataLogger - OLED SSD1306 Display Library
 * @author Nova Instruments
 *
 * Biblioteca para controle de display OLED I2C com driver SSD1306
 * Resolução: 128x64 pixels
 * Interface: I2C
 * Endereço I2C padrão: 0x3C
 */

#ifndef OLED_SSD1306_H
#define OLED_SSD1306_H

#include <stdint.h>
#include <stdbool.h>
#include "modbus.h"

#ifdef __cplusplus
extern "C" {
#endif

// Configurações do display
#define OLED_I2C_ADDRESS    0x3C    // Endereço I2C padrão do SSD1306
#define OLED_WIDTH          128     // Largura em pixels
#define OLED_HEIGHT         64      // Altura em pixels
#define OLED_I2C_BUS        1       // Barramento I2C (/dev/i2c-1)

// Comandos SSD1306
#define OLED_CMD_DISPLAY_OFF            0xAE
#define OLED_CMD_DISPLAY_ON             0xAF
#define OLED_CMD_SET_CONTRAST           0x81
#define OLED_CMD_NORMAL_DISPLAY         0xA6
#define OLED_CMD_INVERSE_DISPLAY        0xA7
#define OLED_CMD_SET_DISPLAY_OFFSET     0xD3
#define OLED_CMD_SET_COM_PINS           0xDA
#define OLED_CMD_SET_VCOM_DETECT        0xDB
#define OLED_CMD_SET_DISPLAY_CLK_DIV    0xD5
#define OLED_CMD_SET_PRECHARGE          0xD9
#define OLED_CMD_SET_MULTIPLEX          0xA8
#define OLED_CMD_SET_LOW_COLUMN         0x00
#define OLED_CMD_SET_HIGH_COLUMN        0x10
#define OLED_CMD_SET_START_LINE         0x40
#define OLED_CMD_MEMORY_MODE            0x20
#define OLED_CMD_COLUMN_ADDR            0x21
#define OLED_CMD_PAGE_ADDR              0x22
#define OLED_CMD_COM_SCAN_DEC           0xC8
#define OLED_CMD_SEG_REMAP              0xA1
#define OLED_CMD_CHARGE_PUMP            0x8D
#define OLED_CMD_ACTIVATE_SCROLL        0x2F
#define OLED_CMD_DEACTIVATE_SCROLL      0x2E

// Enumeração das telas disponíveis
typedef enum {
    SCREEN_TEMPERATURES = 0,    // Tela 1: Temperaturas dos 7 canais
    SCREEN_SETPOINT,            // Tela 2: Ajuste de setpoint
    SCREEN_DIAGNOSTICS,         // Tela 3: Status dos relés
    SCREEN_COUNT                // Total de telas
} oled_screen_t;

// Contexto do display OLED
typedef struct oled_context_s {
    int i2c_fd;                     // File descriptor do I2C
    uint8_t address;                // Endereço I2C do display
    uint8_t buffer[OLED_WIDTH * OLED_HEIGHT / 8];  // Buffer de vídeo (1024 bytes)
    bool initialized;               // Flag de inicialização
    oled_screen_t current_screen;   // Tela atual
    float setpoint;                 // Setpoint ajustável (0.0 a 10.0°C)
} oled_context_t;

/**
 * @brief Inicializa o display OLED SSD1306
 * @return Ponteiro para o contexto do OLED, ou NULL em caso de erro
 */
oled_context_t* oled_init(void);

/**
 * @brief Finaliza o display OLED e libera recursos
 * @param ctx Contexto do OLED
 */
void oled_cleanup(oled_context_t* ctx);

/**
 * @brief Limpa o buffer do display (preenche com zeros)
 * @param ctx Contexto do OLED
 */
void oled_clear(oled_context_t* ctx);

/**
 * @brief Atualiza o display com o conteúdo do buffer
 * @param ctx Contexto do OLED
 * @return true se sucesso, false se erro
 */
bool oled_display(oled_context_t* ctx);

/**
 * @brief Define um pixel no buffer
 * @param ctx Contexto do OLED
 * @param x Coordenada X (0-127)
 * @param y Coordenada Y (0-63)
 * @param color 1 = aceso, 0 = apagado
 */
void oled_set_pixel(oled_context_t* ctx, uint8_t x, uint8_t y, uint8_t color);

/**
 * @brief Desenha um caractere no buffer (fonte 8x8)
 * @param ctx Contexto do OLED
 * @param x Coordenada X (0-127)
 * @param y Coordenada Y (0-63)
 * @param c Caractere ASCII
 */
void oled_draw_char(oled_context_t* ctx, uint8_t x, uint8_t y, char c);

/**
 * @brief Desenha uma string no buffer (fonte 8x8)
 * @param ctx Contexto do OLED
 * @param x Coordenada X (0-127)
 * @param y Coordenada Y (0-63)
 * @param str String a ser desenhada
 */
void oled_draw_string(oled_context_t* ctx, uint8_t x, uint8_t y, const char* str);

/**
 * @brief Desenha uma linha horizontal
 * @param ctx Contexto do OLED
 * @param x Coordenada X inicial
 * @param y Coordenada Y
 * @param width Largura da linha
 */
void oled_draw_hline(oled_context_t* ctx, uint8_t x, uint8_t y, uint8_t width);

/**
 * @brief Exibe informações do datalogger no display
 * @param ctx Contexto do OLED
 * @param device_name Nome do dispositivo
 * @param data Dados Modbus lidos
 * @param record_count Número de registros gravados
 */
void oled_display_datalogger_info(oled_context_t* ctx, const char* device_name, 
                                   const modbus_data_t* data, uint32_t record_count);

/**
 * @brief Exibe mensagem de erro no display
 * @param ctx Contexto do OLED
 * @param error_msg Mensagem de erro
 */
void oled_display_error(oled_context_t* ctx, const char* error_msg);

/**
 * @brief Exibe tela de inicialização
 * @param ctx Contexto do OLED
 * @param device_name Nome do dispositivo
 */
void oled_display_splash(oled_context_t* ctx, const char* device_name);

/**
 * @brief Navega para a próxima tela
 * @param ctx Contexto do OLED
 */
void oled_next_screen(oled_context_t* ctx);

/**
 * @brief Navega para a tela anterior
 * @param ctx Contexto do OLED
 */
void oled_previous_screen(oled_context_t* ctx);

/**
 * @brief Incrementa o setpoint em 0.5°C
 * @param ctx Contexto do OLED
 */
void oled_increment_setpoint(oled_context_t* ctx);

/**
 * @brief Decrementa o setpoint em 0.5°C
 * @param ctx Contexto do OLED
 */
void oled_decrement_setpoint(oled_context_t* ctx);

/**
 * @brief Obtém o valor atual do setpoint
 * @param ctx Contexto do OLED
 * @return Valor do setpoint em °C
 */
float oled_get_setpoint(oled_context_t* ctx);

/**
 * @brief Exibe a tela de ajuste de setpoint
 * @param ctx Contexto do OLED
 */
void oled_display_setpoint_screen(oled_context_t* ctx);

/**
 * @brief Exibe a tela de diagnóstico dos relés
 * @param ctx Contexto do OLED
 * @param lamp_on Estado do relé da lâmpada
 * @param dialer_on Estado do relé da discadora
 * @param compressor_on Estado do relé do compressor
 * @param heater_on Estado do relé da resistência
 */
void oled_display_diagnostics_screen(oled_context_t* ctx, bool lamp_on, bool dialer_on,
                                     bool compressor_on, bool heater_on);

/**
 * @brief Atualiza o display com a tela atual
 * @param ctx Contexto do OLED
 * @param device_name Nome do dispositivo
 * @param data Dados Modbus (para tela de temperaturas)
 * @param record_count Número de registros
 * @param lamp_on Estado do relé da lâmpada
 * @param dialer_on Estado do relé da discadora
 * @param compressor_on Estado do relé do compressor
 * @param heater_on Estado do relé da resistência
 * @param door_open Estado da porta (true = aberta, false = fechada)
 */
void oled_update_current_screen(oled_context_t* ctx, const char* device_name,
                                const modbus_data_t* data, uint32_t record_count,
                                bool lamp_on, bool dialer_on, bool compressor_on, bool heater_on,
                                bool door_open,
                                bool slave2_t1_valid, float slave2_t1,
                                bool slave2_t2_valid, float slave2_t2);

#ifdef __cplusplus
}
#endif

#endif // OLED_SSD1306_H
