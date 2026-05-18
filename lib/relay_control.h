/**
 * @file relay_control.h
 * @brief NovaTherm DataLogger - Relay Control Library
 * @author Nova Instruments
 *
 * Controla 4 relés:
 * - Modbus Coil 0: Lâmpada
 * - Modbus Coil 1: Discadora
 * - GPIO 24 (pino físico 18): Compressor (nível ativo configurável)
 * - GPIO 25 (pino físico 22): Resistência (nível ativo configurável)
 */

#ifndef RELAY_CONTROL_H
#define RELAY_CONTROL_H

#include <stdbool.h>
#include "modbus.h"

#ifdef __cplusplus
extern "C" {
#endif

// Definições dos relés via Modbus
#define RELAY_LAMP_COIL         MODBUS_COIL_LAMP
#define RELAY_DIALER_COIL       MODBUS_COIL_DIALER

// Definições dos GPIOs dos relés locais
#define RELAY_COMPRESSOR_GPIO   24  // Relé do compressor
#define RELAY_HEATER_GPIO       25  // Relé da resistência

// Nível lógico ativo dos relés locais:
// 0 = ativo em LOW (módulos de relé com lógica inversa)
// 1 = ativo em HIGH (módulos de relé com lógica direta)
#define RELAY_GPIO_ACTIVE_LEVEL   1
#define RELAY_GPIO_INACTIVE_LEVEL (RELAY_GPIO_ACTIVE_LEVEL ? 0 : 1)

/**
 * @brief Define o contexto Modbus para operação dos relés em coil
 * @param ctx Contexto Modbus
 */
void relay_set_modbus_context(modbus_context_t* ctx);

/**
 * @brief Inicializa o controle dos relés
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int relay_init(void);

/**
 * @brief Finaliza o controle dos relés e libera recursos
 */
void relay_cleanup(void);

/**
 * @brief Liga a lâmpada (coil Modbus 0)
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int relay_lamp_on(void);

/**
 * @brief Desliga a lâmpada (coil Modbus 0)
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int relay_lamp_off(void);

/**
 * @brief Liga a discadora (coil Modbus 1)
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int relay_dialer_on(void);

/**
 * @brief Desliga a discadora (coil Modbus 1)
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int relay_dialer_off(void);

/**
 * @brief Liga o compressor (GPIO 24)
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int relay_compressor_on(void);

/**
 * @brief Desliga o compressor (GPIO 24)
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int relay_compressor_off(void);

/**
 * @brief Liga a resistência (GPIO 25)
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int relay_heater_on(void);

/**
 * @brief Desliga a resistência (GPIO 25)
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int relay_heater_off(void);

/**
 * @brief Controla a lâmpada baseado no estado da porta
 * @param door_open true se porta aberta (1), false se fechada (0)
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int relay_control_lamp_by_door(bool door_open);

/**
 * @brief Obtém o estado atual da lâmpada
 * @return true se ligada, false se desligada
 */
bool relay_lamp_is_on(void);

/**
 * @brief Obtém o estado atual da discadora
 * @return true se ligada, false se desligada
 */
bool relay_dialer_is_on(void);

/**
 * @brief Obtém o estado atual do compressor
 * @return true se ligado, false se desligado
 */
bool relay_compressor_is_on(void);

/**
 * @brief Obtém o estado atual da resistência
 * @return true se ligada, false se desligada
 */
bool relay_heater_is_on(void);

#ifdef __cplusplus
}
#endif

#endif // RELAY_CONTROL_H
