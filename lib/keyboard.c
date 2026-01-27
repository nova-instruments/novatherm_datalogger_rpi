/**
 * @file keyboard.c
 * @brief Implementação do controle do teclado de 5 botões
 * @author Nova Instruments
 */

#include "keyboard.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gpiod.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>

#define GPIO_CHIP "gpiochip0"

static struct gpiod_chip *chip = NULL;
static struct gpiod_line *sel_line = NULL;
static struct gpiod_line *dec_line = NULL;
static struct gpiod_line *inc_line = NULL;
static struct gpiod_line *before_line = NULL;
static struct gpiod_line *next_line = NULL;

// Estados anteriores dos botões (para detecção de borda)
static int last_dec_state = 1;
static int last_inc_state = 1;
static int last_before_state = 1;
static int last_next_state = 1;

/**
 * @brief Inicializa o monitoramento do teclado
 */
int keyboard_init(void) {
    // Abrir chip GPIO
    chip = gpiod_chip_open_by_name(GPIO_CHIP);
    if (!chip) {
        fprintf(stderr, "❌ Erro ao abrir GPIO chip para teclado\n");
        return -1;
    }

    // Obter linha GPIO 5 (SEL)
    sel_line = gpiod_chip_get_line(chip, BUTTON_SEL_GPIO);
    if (!sel_line) {
        fprintf(stderr, "❌ Erro ao obter linha GPIO %d (SEL)\n", BUTTON_SEL_GPIO);
        gpiod_chip_close(chip);
        chip = NULL;
        return -1;
    }

    // Obter linha GPIO 6 (DEC)
    dec_line = gpiod_chip_get_line(chip, BUTTON_DEC_GPIO);
    if (!dec_line) {
        fprintf(stderr, "❌ Erro ao obter linha GPIO %d (DEC)\n", BUTTON_DEC_GPIO);
        gpiod_chip_close(chip);
        chip = NULL;
        sel_line = NULL;
        return -1;
    }

    // Obter linha GPIO 13 (INC)
    inc_line = gpiod_chip_get_line(chip, BUTTON_INC_GPIO);
    if (!inc_line) {
        fprintf(stderr, "❌ Erro ao obter linha GPIO %d (INC)\n", BUTTON_INC_GPIO);
        gpiod_chip_close(chip);
        chip = NULL;
        sel_line = NULL;
        dec_line = NULL;
        return -1;
    }

    // Obter linha GPIO 19 (BEFORE)
    before_line = gpiod_chip_get_line(chip, BUTTON_BEFORE_GPIO);
    if (!before_line) {
        fprintf(stderr, "❌ Erro ao obter linha GPIO %d (BEFORE)\n", BUTTON_BEFORE_GPIO);
        gpiod_chip_close(chip);
        chip = NULL;
        sel_line = NULL;
        dec_line = NULL;
        inc_line = NULL;
        return -1;
    }

    // Obter linha GPIO 26 (NEXT)
    next_line = gpiod_chip_get_line(chip, BUTTON_NEXT_GPIO);
    if (!next_line) {
        fprintf(stderr, "❌ Erro ao obter linha GPIO %d (NEXT)\n", BUTTON_NEXT_GPIO);
        gpiod_chip_close(chip);
        chip = NULL;
        sel_line = NULL;
        dec_line = NULL;
        inc_line = NULL;
        before_line = NULL;
        return -1;
    }

    // Configurar todos como entrada com pull-up (botões conectam ao GND)
    if (gpiod_line_request_input_flags(sel_line, "button_sel", GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP) < 0) {
        fprintf(stderr, "❌ Erro ao configurar GPIO %d como entrada\n", BUTTON_SEL_GPIO);
        keyboard_cleanup();
        return -1;
    }

    if (gpiod_line_request_input_flags(dec_line, "button_dec", GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP) < 0) {
        fprintf(stderr, "❌ Erro ao configurar GPIO %d como entrada\n", BUTTON_DEC_GPIO);
        keyboard_cleanup();
        return -1;
    }

    if (gpiod_line_request_input_flags(inc_line, "button_inc", GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP) < 0) {
        fprintf(stderr, "❌ Erro ao configurar GPIO %d como entrada\n", BUTTON_INC_GPIO);
        keyboard_cleanup();
        return -1;
    }

    if (gpiod_line_request_input_flags(before_line, "button_before", GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP) < 0) {
        fprintf(stderr, "❌ Erro ao configurar GPIO %d como entrada\n", BUTTON_BEFORE_GPIO);
        keyboard_cleanup();
        return -1;
    }

    if (gpiod_line_request_input_flags(next_line, "button_next", GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP) < 0) {
        fprintf(stderr, "❌ Erro ao configurar GPIO %d como entrada\n", BUTTON_NEXT_GPIO);
        keyboard_cleanup();
        return -1;
    }

    printf("⌨️  Teclado inicializado:\n");
    printf("   - SEL (GPIO %d, pino 29): Reset de logs\n", BUTTON_SEL_GPIO);
    printf("   - DEC (GPIO %d, pino 31): Decrementar\n", BUTTON_DEC_GPIO);
    printf("   - INC (GPIO %d, pino 33): Incrementar\n", BUTTON_INC_GPIO);
    printf("   - BEFORE (GPIO %d, pino 35): Anterior\n", BUTTON_BEFORE_GPIO);
    printf("   - NEXT (GPIO %d, pino 37): Próximo\n", BUTTON_NEXT_GPIO);

    return 0;
}

/**
 * @brief Verifica se o botão SEL foi pressionado
 */
bool keyboard_sel_is_pressed(void) {
    if (!sel_line) return false;
    int value = gpiod_line_get_value(sel_line);
    return (value == 0);  // 0 = pressionado (conecta ao GND)
}

/**
 * @brief Verifica se o botão DEC foi pressionado (detecção de borda)
 */
bool keyboard_dec_is_pressed(void) {
    if (!dec_line) return false;
    int value = gpiod_line_get_value(dec_line);

    // Detectar borda de descida (transição de 1 para 0)
    if (value == 0 && last_dec_state == 1) {
        last_dec_state = value;
        return true;
    }

    last_dec_state = value;
    return false;
}

/**
 * @brief Verifica se o botão INC foi pressionado (detecção de borda)
 */
bool keyboard_inc_is_pressed(void) {
    if (!inc_line) return false;
    int value = gpiod_line_get_value(inc_line);

    // Detectar borda de descida (transição de 1 para 0)
    if (value == 0 && last_inc_state == 1) {
        last_inc_state = value;
        return true;
    }

    last_inc_state = value;
    return false;
}

/**
 * @brief Verifica se o botão BEFORE foi pressionado (detecção de borda)
 */
bool keyboard_before_is_pressed(void) {
    if (!before_line) return false;
    int value = gpiod_line_get_value(before_line);

    // Detectar borda de descida (transição de 1 para 0)
    if (value == 0 && last_before_state == 1) {
        last_before_state = value;
        return true;
    }

    last_before_state = value;
    return false;
}

/**
 * @brief Verifica se o botão NEXT foi pressionado (detecção de borda)
 */
bool keyboard_next_is_pressed(void) {
    if (!next_line) return false;
    int value = gpiod_line_get_value(next_line);

    // Detectar borda de descida (transição de 1 para 0)
    if (value == 0 && last_next_state == 1) {
        last_next_state = value;
        return true;
    }

    last_next_state = value;
    return false;
}

/**
 * @brief Finaliza o monitoramento do teclado
 */
void keyboard_cleanup(void) {
    if (sel_line) {
        gpiod_line_release(sel_line);
        sel_line = NULL;
    }
    if (dec_line) {
        gpiod_line_release(dec_line);
        dec_line = NULL;
    }
    if (inc_line) {
        gpiod_line_release(inc_line);
        inc_line = NULL;
    }
    if (before_line) {
        gpiod_line_release(before_line);
        before_line = NULL;
    }
    if (next_line) {
        gpiod_line_release(next_line);
        next_line = NULL;
    }

    if (chip) {
        gpiod_chip_close(chip);
        chip = NULL;
    }

    printf("⌨️  Teclado finalizado\n");
}

/**
 * @brief Apaga todos os arquivos de datalogger
 */
bool keyboard_delete_all_logs(const char* log_dir) {
    DIR *dir;
    struct dirent *entry;
    char filepath[512];
    int deleted_count = 0;
    int error_count = 0;

    printf("\n🗑️  INICIANDO LIMPEZA DE LOGS...\n");
    printf("📁 Diretório: %s\n", log_dir);

    // Abrir diretório
    dir = opendir(log_dir);
    if (!dir) {
        fprintf(stderr, "❌ Erro ao abrir diretório: %s\n", log_dir);
        return false;
    }

    // Percorrer todos os arquivos
    while ((entry = readdir(dir)) != NULL) {
        // Ignorar . e ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Verificar se é arquivo .db ou .txt (começando com NI)
        size_t len = strlen(entry->d_name);
        bool is_db = (len > 3 && strcmp(entry->d_name + len - 3, ".db") == 0);
        bool is_txt = (len > 4 && strcmp(entry->d_name + len - 4, ".txt") == 0);
        bool starts_with_ni = (len > 2 && entry->d_name[0] == 'N' && entry->d_name[1] == 'I');

        if ((is_db || is_txt) && starts_with_ni) {
            // Construir caminho completo
            snprintf(filepath, sizeof(filepath), "%s/%s", log_dir, entry->d_name);

            // Tentar apagar arquivo
            if (remove(filepath) == 0) {
                printf("  ✅ Removido: %s\n", entry->d_name);
                deleted_count++;
            } else {
                fprintf(stderr, "  ❌ Erro ao remover: %s\n", entry->d_name);
                error_count++;
            }
        }
    }

    closedir(dir);

    printf("\n📊 Resultado da limpeza:\n");
    printf("  ✅ Arquivos removidos: %d\n", deleted_count);
    if (error_count > 0) {
        printf("  ❌ Erros: %d\n", error_count);
    }
    printf("🗑️  LIMPEZA CONCLUÍDA!\n\n");

    return (error_count == 0);
}

