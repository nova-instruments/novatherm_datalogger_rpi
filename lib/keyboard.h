/**
 * @file keyboard.h
 * @brief Controle do teclado de 5 botões
 * @author Nova Instruments
 * 
 * Teclado com 5 botões:
 * - GPIO 5:  SEL (botão de seleção/reset)
 * - GPIO 6:  DEC (decrementar)
 * - GPIO 13: INC (incrementar)
 * - GPIO 19: BEFORE (anterior)
 * - GPIO 26: NEXT (próximo)
 */

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdbool.h>

// Definições dos GPIOs dos botões
#define BUTTON_SEL_GPIO     5   // Botão SEL (pino físico 29)
#define BUTTON_DEC_GPIO     6   // Botão DEC (pino físico 31)
#define BUTTON_INC_GPIO     13  // Botão INC (pino físico 33)
#define BUTTON_BEFORE_GPIO  19  // Botão BEFORE (pino físico 35)
#define BUTTON_NEXT_GPIO    26  // Botão NEXT (pino físico 37)

/**
 * @brief Inicializa o monitoramento do teclado
 * @return 0 em sucesso, -1 em erro
 */
int keyboard_init(void);

/**
 * @brief Verifica se o botão SEL foi pressionado
 * @return true se pressionado, false caso contrário
 */
bool keyboard_sel_is_pressed(void);

/**
 * @brief Verifica se o botão DEC foi pressionado
 * @return true se pressionado, false caso contrário
 */
bool keyboard_dec_is_pressed(void);

/**
 * @brief Verifica se o botão INC foi pressionado
 * @return true se pressionado, false caso contrário
 */
bool keyboard_inc_is_pressed(void);

/**
 * @brief Verifica se o botão BEFORE foi pressionado
 * @return true se pressionado, false caso contrário
 */
bool keyboard_before_is_pressed(void);

/**
 * @brief Verifica se o botão NEXT foi pressionado
 * @return true se pressionado, false caso contrário
 */
bool keyboard_next_is_pressed(void);

/**
 * @brief Finaliza o monitoramento do teclado
 */
void keyboard_cleanup(void);

/**
 * @brief Apaga todos os arquivos de datalogger
 * @param log_dir Diretório onde estão os logs
 * @return true em sucesso, false em erro
 */
bool keyboard_delete_all_logs(const char* log_dir);

#endif // KEYBOARD_H

