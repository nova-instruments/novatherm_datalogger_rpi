/**
 * @file reset_button.h
 * @brief Controle do botão de reset para apagar logs
 * @author Nova Instruments
 */

#ifndef RESET_BUTTON_H
#define RESET_BUTTON_H

#include <stdbool.h>

/**
 * @brief Inicializa o monitoramento do botão de reset
 * @return 0 em sucesso, -1 em erro
 */
int reset_button_init(void);

/**
 * @brief Verifica se o botão de reset foi pressionado
 * @return true se pressionado, false caso contrário
 */
bool reset_button_is_pressed(void);

/**
 * @brief Finaliza o monitoramento do botão de reset
 */
void reset_button_cleanup(void);

/**
 * @brief Apaga todos os arquivos de datalogger
 * @param log_dir Diretório onde estão os logs
 * @return true em sucesso, false em erro
 */
bool reset_delete_all_logs(const char* log_dir);

#endif // RESET_BUTTON_H

