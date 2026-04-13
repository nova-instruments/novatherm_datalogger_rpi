/**
 * @file display_lvgl.h
 * @brief LVGL Display Driver for NovaTherm DataLogger
 * @author Nova Instruments
 * 
 * Integração do LVGL v9.5 com ILI9341 para interface gráfica
 */

#ifndef DISPLAY_LVGL_H
#define DISPLAY_LVGL_H

#include <stdint.h>
#include <stdbool.h>
#include "modbus.h"

#ifdef __cplusplus
extern "C" {
#endif

// Enumeração das telas disponíveis
typedef enum {
    LVGL_SCREEN_MAIN_TEMP = 0,      // Tela 0: Temperatura principal
    LVGL_SCREEN_INFO,               // Tela 1: Informações (todos os canais)
    LVGL_SCREEN_DIAGNOSTICS,        // Tela 2: Status dos relés
    LVGL_SCREEN_COUNT               // Total de telas
} lvgl_screen_t;

// Contexto do display LVGL
typedef struct lvgl_context_s {
    void* ili9341_ctx;              // Contexto do driver ILI9341
    void* lv_disp;                  // Display LVGL
    lvgl_screen_t current_screen;   // Tela atual
    float setpoint;                 // Setpoint ajustável
    float temp_max;                 // Temperatura máxima registrada
    float temp_min;                 // Temperatura mínima registrada
    bool initialized;               // Flag de inicialização
} lvgl_context_t;

/**
 * @brief Inicializa o sistema LVGL com ILI9341
 * @return Ponteiro para o contexto do display, ou NULL em caso de erro
 */
lvgl_context_t* lvgl_display_init(void);

/**
 * @brief Finaliza o display LVGL e libera recursos
 * @param ctx Contexto do display
 */
void lvgl_display_cleanup(lvgl_context_t* ctx);

/**
 * @brief Limpa a tela
 * @param ctx Contexto do display
 */
void lvgl_display_clear(lvgl_context_t* ctx);

/**
 * @brief Exibe tela de splash/inicialização
 * @param ctx Contexto do display
 * @param device_name Nome do dispositivo
 */
void lvgl_display_splash(lvgl_context_t* ctx, const char* device_name);

/**
 * @brief Navega para a próxima tela
 * @param ctx Contexto do display
 */
void lvgl_next_screen(lvgl_context_t* ctx);

/**
 * @brief Navega para a tela anterior
 * @param ctx Contexto do display
 */
void lvgl_previous_screen(lvgl_context_t* ctx);

/**
 * @brief Incrementa o setpoint
 * @param ctx Contexto do display
 */
void lvgl_increment_setpoint(lvgl_context_t* ctx);

/**
 * @brief Decrementa o setpoint
 * @param ctx Contexto do display
 */
void lvgl_decrement_setpoint(lvgl_context_t* ctx);

/**
 * @brief Obtém o valor atual do setpoint
 * @param ctx Contexto do display
 * @return Valor do setpoint em °C
 */
float lvgl_get_setpoint(lvgl_context_t* ctx);

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
void lvgl_update_current_screen(lvgl_context_t* ctx, const char* device_name,
                                const modbus_data_t* data, uint32_t record_count,
                                bool lamp_on, bool dialer_on, bool compressor_on, bool heater_on,
                                bool door_open);

/**
 * @brief Processa tasks do LVGL (deve ser chamado periodicamente)
 * @param ctx Contexto do display
 */
void lvgl_task_handler(lvgl_context_t* ctx);

#ifdef __cplusplus
}
#endif

#endif // DISPLAY_LVGL_H
