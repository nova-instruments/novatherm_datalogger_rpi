/**
 * @file input.h
 * @brief Controle de entradas digitais (sensor de porta)
 * @author Nova Instruments
 * 
 * Entradas digitais:
 * - GPIO 17: Sensor de porta (pino físico 11)
 */

#ifndef INPUT_H
#define INPUT_H

#include <stdbool.h>
#include <stdint.h>

// Definições dos GPIOs de entrada
#define INPUT_DOOR_GPIO  17  // Sensor de porta (pino físico 11)
// Nível lógico que representa PORTA ABERTA
// 0 = ativa em nível baixo (open = LOW), 1 = ativa em nível alto (open = HIGH)
#define INPUT_DOOR_OPEN_LEVEL 0

/**
 * @brief Inicializa o monitoramento das entradas digitais
 * @return 0 em sucesso, -1 em erro
 */
int input_init(void);

/**
 * @brief Lê o estado atual do sensor de porta
 * @return true se porta aberta, false se porta fechada
 */
bool input_door_is_open(void);

/**
 * @brief Verifica se houve mudança no estado da porta
 * @param current_state Ponteiro para armazenar estado atual (true = aberta, false = fechada)
 * @return true se houve mudança, false caso contrário
 */
bool input_door_state_changed(bool* current_state);

/**
 * @brief Obtém o estado da porta como uint16_t (compatível com código legado)
 * @return 1 se porta aberta, 0 se porta fechada
 */
uint16_t input_door_get_state_uint16(void);

/**
 * @brief Finaliza o monitoramento das entradas digitais
 */
void input_cleanup(void);

#endif // INPUT_H
