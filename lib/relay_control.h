/**
 * @file relay_control.h
 * @brief NovaTherm DataLogger - Relay Control Library
 * @author Nova Instruments
 *
 * Controla 4 relés:
 * - GPIO 24 (pino físico 18): Lâmpada
 * - GPIO 25 (pino físico 22): Discadora
 * - GPIO 4  (pino físico 7):  Compressor (lógica inversa)
 * - GPIO 17 (pino físico 11): Resistência (lógica inversa)
 */

#ifndef RELAY_CONTROL_H
#define RELAY_CONTROL_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Definições dos GPIOs dos relés
#define RELAY_LAMP_GPIO         24  // Relé da lâmpada
#define RELAY_DIALER_GPIO       25  // Relé da discadora
#define RELAY_COMPRESSOR_GPIO   4   // Relé do compressor (lógica inversa)
#define RELAY_HEATER_GPIO       17  // Relé da resistência (lógica inversa)

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
 * @brief Liga a lâmpada (relé GPIO 24)
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int relay_lamp_on(void);

/**
 * @brief Desliga a lâmpada (relé GPIO 24)
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int relay_lamp_off(void);

/**
 * @brief Liga a discadora (relé GPIO 25)
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int relay_dialer_on(void);

/**
 * @brief Desliga a discadora (relé GPIO 25)
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int relay_dialer_off(void);

/**
 * @brief Liga o compressor (GPIO 4) - Lógica inversa
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int relay_compressor_on(void);

/**
 * @brief Desliga o compressor (GPIO 4) - Lógica inversa
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int relay_compressor_off(void);

/**
 * @brief Liga a resistência (GPIO 17) - Lógica inversa
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int relay_heater_on(void);

/**
 * @brief Desliga a resistência (GPIO 17) - Lógica inversa
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int relay_heater_off(void);

/**
 * @brief Controla a lâmpada baseado no estado da porta
 * @param door_open true se porta aberta (1), false se fechada (0)
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int relay_control_lamp_by_door(bool door_open);

#ifdef __cplusplus
}
#endif

#endif // RELAY_CONTROL_H

