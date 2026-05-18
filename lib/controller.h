/**
 * @file controller.h
 * @brief NovaTherm DataLogger - Temperature Controller Library
 * @author Nova Instruments
 *
 * Controlador de temperatura ON/OFF com histerese e ciclo de degelo automático
 */

#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// ⚙️ CONFIGURAÇÕES DO CONTROLADOR
#define CONTROLLER_HYSTERESIS 0.6f          // Histerese em °C
#define DEFROST_INTERVAL_HOURS 24           // Intervalo entre ciclos de degelo (horas)
#define DEFROST_MAX_DURATION_SEC 300        // Duração máxima do degelo (5 minutos)
#define DEFROST_TEMP_THRESHOLD -12.0f       // Temperatura para iniciar degelo (°C)
#define DEFROST_STOP_TEMP 0.0f              // Temperatura para parar degelo (°C)

// Estados do controlador
typedef enum {
    CONTROLLER_STATE_IDLE = 0,              // Inativo
    CONTROLLER_STATE_COOLING,               // Resfriando (compressor ligado)
    CONTROLLER_STATE_WAITING,               // Aguardando (compressor desligado)
    CONTROLLER_STATE_DEFROSTING             // Em processo de degelo
} controller_state_t;

// Estrutura de contexto do controlador
typedef struct controller_context_s {
    // Configurações
    float setpoint;                         // Setpoint de temperatura (°C)
    float hysteresis;                       // Histerese (°C)
    
    // Estado atual
    controller_state_t state;               // Estado do controlador
    bool compressor_on;                     // Estado do compressor
    bool heater_on;                         // Estado da resistência de degelo
    
    // Controle de degelo
    time_t last_defrost_time;               // Timestamp do último degelo
    time_t defrost_start_time;              // Timestamp do início do degelo atual
    bool defrost_needed;                    // Flag indicando necessidade de degelo
    
    // Estatísticas
    uint32_t cooling_cycles;                // Contador de ciclos de resfriamento
    uint32_t defrost_cycles;                // Contador de ciclos de degelo
    uint32_t defrost_interrupted;           // Contador de degelos interrompidos por temperatura
} controller_context_t;

/**
 * @brief Inicializa o controlador de temperatura
 * @param setpoint Setpoint inicial de temperatura (°C)
 * @return Ponteiro para o contexto do controlador, ou NULL em caso de erro
 */
controller_context_t* controller_init(float setpoint);

/**
 * @brief Finaliza o controlador e libera recursos
 * @param ctx Contexto do controlador
 */
void controller_cleanup(controller_context_t* ctx);

/**
 * @brief Atualiza o setpoint do controlador
 * @param ctx Contexto do controlador
 * @param setpoint Novo setpoint (°C)
 */
void controller_set_setpoint(controller_context_t* ctx, float setpoint);
void controller_set_hysteresis(controller_context_t* ctx, float hysteresis);

/**
 * @brief Obtém o setpoint atual
 * @param ctx Contexto do controlador
 * @return Setpoint atual (°C)
 */
float controller_get_setpoint(controller_context_t* ctx);
float controller_get_hysteresis(controller_context_t* ctx);

/**
 * @brief Atualiza o controlador com novas leituras de temperatura
 * @param ctx Contexto do controlador
 * @param temp_ch1 Temperatura do sensor CH1 (controle principal) em °C
 * @param temp_ch2 Temperatura do sensor CH2 (controle de degelo) em °C
 * @param ch1_valid Flag indicando se CH1 é válido
 * @param ch2_valid Flag indicando se CH2 é válido
 * @return true se atualização foi bem-sucedida, false caso contrário
 */
bool controller_update(controller_context_t* ctx, float temp_ch1, float temp_ch2,
                      bool ch1_valid, bool ch2_valid);

/**
 * @brief Obtém o estado atual do controlador
 * @param ctx Contexto do controlador
 * @return Estado atual
 */
controller_state_t controller_get_state(controller_context_t* ctx);

/**
 * @brief Obtém o nome do estado atual (para debug/display)
 * @param state Estado do controlador
 * @return String com o nome do estado
 */
const char* controller_get_state_name(controller_state_t state);

/**
 * @brief Verifica se o compressor deve estar ligado
 * @param ctx Contexto do controlador
 * @return true se compressor deve estar ligado, false caso contrário
 */
bool controller_should_compressor_be_on(controller_context_t* ctx);

/**
 * @brief Verifica se a resistência de degelo deve estar ligada
 * @param ctx Contexto do controlador
 * @return true se resistência deve estar ligada, false caso contrário
 */
bool controller_should_heater_be_on(controller_context_t* ctx);

/**
 * @brief Força um ciclo de degelo imediato (para testes)
 * @param ctx Contexto do controlador
 */
void controller_force_defrost(controller_context_t* ctx);

/**
 * @brief Obtém tempo restante até próximo degelo (em segundos)
 * @param ctx Contexto do controlador
 * @return Segundos até próximo degelo, ou 0 se já passou do horário
 */
uint32_t controller_get_time_to_next_defrost(controller_context_t* ctx);

#ifdef __cplusplus
}
#endif

#endif // CONTROLLER_H
