/**
 * @file lcd_i2c.h
 * @brief NovaTherm DataLogger - LCD 20x4 I2C Display Library
 * @author Nova Instruments
 *
 * Biblioteca para controle de display LCD 20x4 com interface I2C PCF8574
 * Interface: I2C
 * Endereço I2C: 0x27
 * Dimensões: 20 colunas x 4 linhas
 */

#ifndef LCD_I2C_H
#define LCD_I2C_H

#include <stdint.h>
#include <stdbool.h>
#include "modbus.h"

#ifdef __cplusplus
extern "C" {
#endif

// Configurações do display LCD
#define LCD_I2C_ADDRESS     0x27    // Endereço I2C do PCF8574
#define LCD_COLS            20      // Número de colunas
#define LCD_ROWS            4       // Número de linhas
#define LCD_I2C_BUS         1       // Barramento I2C (/dev/i2c-1)

// Comandos LCD HD44780
#define LCD_CMD_CLEAR           0x01
#define LCD_CMD_HOME            0x02
#define LCD_CMD_ENTRY_MODE      0x04
#define LCD_CMD_DISPLAY_CTRL    0x08
#define LCD_CMD_CURSOR_SHIFT    0x10
#define LCD_CMD_FUNCTION_SET    0x20
#define LCD_CMD_SET_CGRAM       0x40
#define LCD_CMD_SET_DDRAM       0x80

// Flags para modos de entrada
#define LCD_ENTRY_RIGHT         0x00
#define LCD_ENTRY_LEFT          0x02
#define LCD_ENTRY_SHIFT_INC     0x01
#define LCD_ENTRY_SHIFT_DEC     0x00

// Flags para controle de display
#define LCD_DISPLAY_ON          0x04
#define LCD_DISPLAY_OFF         0x00
#define LCD_CURSOR_ON           0x02
#define LCD_CURSOR_OFF          0x00
#define LCD_BLINK_ON            0x01
#define LCD_BLINK_OFF           0x00

// Flags para função set
#define LCD_8BIT_MODE           0x10
#define LCD_4BIT_MODE           0x00
#define LCD_2LINE               0x08
#define LCD_1LINE               0x00
#define LCD_5x10_DOTS           0x04
#define LCD_5x8_DOTS            0x00

// Flags de backlight
#define LCD_BACKLIGHT_ON        0x08
#define LCD_BACKLIGHT_OFF       0x00

// Flags de enable
#define LCD_ENABLE              0x04

// Flags de read/write
#define LCD_RW_WRITE            0x00
#define LCD_RW_READ             0x02

// Flags de register select
#define LCD_RS_COMMAND          0x00
#define LCD_RS_DATA             0x01

// Enumeração das telas disponíveis
typedef enum {
    LCD_SCREEN_MAIN_TEMP = 0,       // Tela 0: Temperatura principal (BigFont)
    LCD_SCREEN_INFO,                // Tela 1: Informações (PR2, AC, DC, Porta, SP)
    LCD_SCREEN_DIAGNOSTICS,         // Tela 2: Status dos relés
    LCD_SCREEN_RELAY_TEST,          // Tela 3: Teste temporário de relés Modbus
    LCD_SCREEN_SLAVE2_TEMPS,        // Tela 4: T1/T2 do slave Modbus 2
    LCD_SCREEN_COUNT                // Total de telas
} lcd_screen_t;

typedef enum {
    LCD_RELAY_TEST_TARGET_LAMP = 0,
    LCD_RELAY_TEST_TARGET_DIALER
} lcd_relay_test_target_t;

// Contexto do display LCD
typedef struct lcd_context_s {
    int i2c_fd;                     // File descriptor do I2C
    uint8_t address;                // Endereço I2C do display
    uint8_t backlight;              // Estado do backlight
    bool initialized;               // Flag de inicialização
    lcd_screen_t current_screen;    // Tela atual
    float setpoint;                 // Setpoint ajustável
    float temp_max;                 // Temperatura máxima registrada
    float temp_min;                 // Temperatura mínima registrada
    bool wifi_connected;            // Status da conexão WiFi
    float filtered_temp_ch1;        // Temperatura PR1 filtrada (filtro passa-baixas)
    bool first_temp_reading;        // Flag para primeira leitura (inicialização do filtro)
    float last_displayed_temp;      // Última temperatura exibida (para evitar redesenho desnecessário)
    float last_displayed_max;       // Último Max exibido
    float last_displayed_min;       // Último Min exibido
    bool screen_needs_redraw;       // Flag para forçar redesenho completo
    lcd_relay_test_target_t relay_test_target;  // Relé selecionado na tela de teste
} lcd_context_t;

/**
 * @brief Inicializa o display LCD 20x4
 * @return Ponteiro para o contexto do LCD, ou NULL em caso de erro
 */
lcd_context_t* lcd_init(void);

/**
 * @brief Finaliza o display LCD e libera recursos
 * @param ctx Contexto do LCD
 */
void lcd_cleanup(lcd_context_t* ctx);

/**
 * @brief Limpa o display
 * @param ctx Contexto do LCD
 */
void lcd_clear(lcd_context_t* ctx);

/**
 * @brief Posiciona o cursor
 * @param ctx Contexto do LCD
 * @param col Coluna (0-19)
 * @param row Linha (0-3)
 */
void lcd_set_cursor(lcd_context_t* ctx, uint8_t col, uint8_t row);

/**
 * @brief Escreve uma string no display
 * @param ctx Contexto do LCD
 * @param str String a ser escrita
 */
void lcd_print(lcd_context_t* ctx, const char* str);

/**
 * @brief Liga/desliga o backlight
 * @param ctx Contexto do LCD
 * @param on true para ligar, false para desligar
 */
void lcd_backlight(lcd_context_t* ctx, bool on);

/**
 * @brief Exibe tela de splash/inicialização
 * @param ctx Contexto do LCD
 * @param device_name Nome do dispositivo
 */
void lcd_display_splash(lcd_context_t* ctx, const char* device_name);

/**
 * @brief Navega para a próxima tela
 * @param ctx Contexto do LCD
 */
void lcd_next_screen(lcd_context_t* ctx);

/**
 * @brief Navega para a tela anterior
 * @param ctx Contexto do LCD
 */
void lcd_previous_screen(lcd_context_t* ctx);

/**
 * @brief Incrementa o setpoint
 * @param ctx Contexto do LCD
 */
void lcd_increment_setpoint(lcd_context_t* ctx);

/**
 * @brief Decrementa o setpoint
 * @param ctx Contexto do LCD
 */
void lcd_decrement_setpoint(lcd_context_t* ctx);

/**
 * @brief Obtém o valor atual do setpoint
 * @param ctx Contexto do LCD
 * @return Valor do setpoint em °C
 */
float lcd_get_setpoint(lcd_context_t* ctx);

/**
 * @brief Atualiza o display com a tela atual
 * @param ctx Contexto do LCD
 * @param device_name Nome do dispositivo
 * @param data Dados Modbus
 * @param record_count Número de registros
 * @param lamp_on Estado do relé da lâmpada
 * @param dialer_on Estado do relé da discadora
 * @param compressor_on Estado do relé do compressor
 * @param heater_on Estado do relé da resistência
 * @param door_open Estado da porta (true = aberta, false = fechada)
 */
void lcd_update_current_screen(lcd_context_t* ctx, const char* device_name,
                               const modbus_data_t* data, uint32_t record_count,
                               bool lamp_on, bool dialer_on, bool compressor_on, bool heater_on,
                               bool door_open,
                               bool slave2_t1_valid, float slave2_t1,
                               bool slave2_t2_valid, float slave2_t2);

/**
 * @brief Cria caracteres customizados para números grandes (BigFont)
 * @param ctx Contexto do LCD
 *
 * Baseado em https://coeleveld.com/bigfont/ (BigFont02)
 * Cria 8 caracteres customizados (0-7) para formar dígitos grandes:
 * - Char 0: Top Left Corner
 * - Char 1: Top Right Corner
 * - Char 2: Bottom Left Corner
 * - Char 3: Bottom Right Corner
 * - Char 4: Full Block
 * - Char 5: Bottom Bar
 * - Char 6: Degree symbol (°)
 * - Char 7: Decimal point (.)
 *
 * Cada dígito ocupa 3 colunas x 2 linhas
 */
void lcd_create_bargraph_chars(lcd_context_t* ctx);

/**
 * @brief Desenha temperatura grande no centro do display (BigFont style)
 * @param ctx Contexto do LCD
 * @param col Não usado (mantido para compatibilidade)
 * @param value Temperatura a exibir (-9.9 a 99.9°C)
 * @param min_value Não usado (mantido para compatibilidade)
 * @param max_value Não usado (mantido para compatibilidade)
 *
 * Desenha a temperatura em formato "XX.X°C" usando dígitos grandes
 * ocupando as linhas 1 e 2 do display, centralizado automaticamente.
 * Cada dígito ocupa 3 colunas x 2 linhas.
 */
void lcd_draw_bargraph(lcd_context_t* ctx, uint8_t col, float value, float min_value, float max_value);

/**
 * @brief Atualiza os valores de temperatura máxima e mínima
 * @param ctx Contexto do LCD
 * @param current_temp Temperatura atual
 */
void lcd_update_temp_minmax(lcd_context_t* ctx, float current_temp);

/**
 * @brief Define o status da conexão WiFi
 * @param ctx Contexto do LCD
 * @param connected true se conectado, false caso contrário
 */
void lcd_set_wifi_status(lcd_context_t* ctx, bool connected);

/**
 * @brief Verifica se a tela atual é a tela de teste de relés
 * @param ctx Contexto do LCD
 * @return true se tela de teste está ativa
 */
bool lcd_is_relay_test_screen(lcd_context_t* ctx);

/**
 * @brief Alterna o relé selecionado na tela de teste
 * @param ctx Contexto do LCD
 */
void lcd_relay_test_toggle_target(lcd_context_t* ctx);

/**
 * @brief Obtém o relé selecionado na tela de teste
 * @param ctx Contexto do LCD
 * @return Alvo selecionado (lâmpada ou discadora)
 */
lcd_relay_test_target_t lcd_relay_test_get_target(lcd_context_t* ctx);

#ifdef __cplusplus
}
#endif

#endif // LCD_I2C_H
