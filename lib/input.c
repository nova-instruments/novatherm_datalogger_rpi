/**
 * @file input.c
 * @brief Implementação do controle de entradas digitais
 * @author Nova Instruments
 */

#include "input.h"
#include <stdio.h>
#include <stdlib.h>
#include <gpiod.h>

#define GPIO_CHIP "gpiochip0"

static struct gpiod_chip *chip = NULL;
static struct gpiod_line *door_line = NULL;

// Estado anterior da porta (para detecção de mudança)
static int last_door_state = -1;  // -1 = não inicializado

/**
 * @brief Inicializa o monitoramento das entradas digitais
 */
int input_init(void) {
    // Abrir chip GPIO
    chip = gpiod_chip_open_by_name(GPIO_CHIP);
    if (!chip) {
        fprintf(stderr, "❌ Erro ao abrir GPIO chip para entradas digitais\n");
        return -1;
    }

    // Obter linha GPIO 10 (Sensor de Porta)
    door_line = gpiod_chip_get_line(chip, INPUT_DOOR_GPIO);
    if (!door_line) {
        fprintf(stderr, "❌ Erro ao obter linha GPIO %d (Sensor de Porta)\n", INPUT_DOOR_GPIO);
        gpiod_chip_close(chip);
        chip = NULL;
        return -1;
    }

    // Configurar como entrada com pull-up
    // Lógica: 1 = porta aberta (pull-up), 0 = porta fechada (conectado ao GND)
    if (gpiod_line_request_input_flags(door_line, "door_sensor", 
                                       GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP) < 0) {
        fprintf(stderr, "❌ Erro ao configurar GPIO %d como entrada\n", INPUT_DOOR_GPIO);
        input_cleanup();
        return -1;
    }

    // Ler estado inicial
    last_door_state = gpiod_line_get_value(door_line);

    printf("🚪 Sensor de porta inicializado:\n");
    printf("   - GPIO %d (pino físico 19)\n", INPUT_DOOR_GPIO);
    printf("   - Estado inicial: %s\n", last_door_state ? "ABERTA" : "FECHADA");

    return 0;
}

/**
 * @brief Lê o estado atual do sensor de porta
 */
bool input_door_is_open(void) {
    if (!door_line) return false;
    
    int value = gpiod_line_get_value(door_line);
    
    // Lógica: 1 = porta aberta (pull-up), 0 = porta fechada (conectado ao GND)
    return (value == 1);
}

/**
 * @brief Verifica se houve mudança no estado da porta
 */
bool input_door_state_changed(bool* current_state) {
    if (!door_line) return false;
    
    int value = gpiod_line_get_value(door_line);
    
    // Detectar mudança de estado
    bool changed = (value != last_door_state && last_door_state != -1);
    
    if (changed) {
        printf("🚪 Mudança detectada: %s → %s\n",
               last_door_state ? "ABERTA" : "FECHADA",
               value ? "ABERTA" : "FECHADA");
    }
    
    last_door_state = value;
    
    if (current_state) {
        *current_state = (value == 1);  // true = aberta, false = fechada
    }
    
    return changed;
}

/**
 * @brief Obtém o estado da porta como uint16_t (compatível com código legado)
 */
uint16_t input_door_get_state_uint16(void) {
    if (!door_line) return 0;
    
    int value = gpiod_line_get_value(door_line);
    return (uint16_t)value;  // 1 = aberta, 0 = fechada
}

/**
 * @brief Finaliza o monitoramento das entradas digitais
 */
void input_cleanup(void) {
    if (door_line) {
        gpiod_line_release(door_line);
        door_line = NULL;
    }

    if (chip) {
        gpiod_chip_close(chip);
        chip = NULL;
    }

    printf("🚪 Entradas digitais finalizadas\n");
}

