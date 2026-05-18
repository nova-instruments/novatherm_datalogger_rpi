/**
 * @file relay_control.c
 * @brief NovaTherm DataLogger - Relay Control Implementation
 * @author Nova Instruments
 */

#include "relay_control.h"
#include <stdio.h>
#include <gpiod.h>

#define GPIO_CHIP_NAME "gpiochip0"

// Variáveis globais para controle dos GPIOs
static struct gpiod_chip *gpio_chip = NULL;
static struct gpiod_line *compressor_line = NULL;
static struct gpiod_line *heater_line = NULL;
static modbus_context_t* relay_modbus_ctx = NULL;
static bool lamp_state_cache = false;
static bool dialer_state_cache = false;

void relay_set_modbus_context(modbus_context_t* ctx) {
    relay_modbus_ctx = ctx;
}

/**
 * @brief Inicializa o controle dos relés
 */
int relay_init(void) {
    int ret;

    if (gpio_chip && compressor_line && heater_line) {
        printf("Relés já inicializados\n");
        return 0;
    }

    // Abrir chip GPIO
    gpio_chip = gpiod_chip_open_by_name(GPIO_CHIP_NAME);
    if (!gpio_chip) {
        fprintf(stderr, "Erro ao abrir chip GPIO: %s\n", GPIO_CHIP_NAME);
        return -1;
    }

    // Obter linha do GPIO do compressor
    compressor_line = gpiod_chip_get_line(gpio_chip, RELAY_COMPRESSOR_GPIO);
    if (!compressor_line) {
        fprintf(stderr, "Erro ao obter linha GPIO %d (Compressor)\n", RELAY_COMPRESSOR_GPIO);
        gpiod_chip_close(gpio_chip);
        gpio_chip = NULL;
        return -1;
    }

    // Configurar GPIO do compressor como saída (iniciar desligado)
    ret = gpiod_line_request_output(compressor_line, "relay_compressor", RELAY_GPIO_INACTIVE_LEVEL);
    if (ret < 0) {
        fprintf(stderr, "Erro ao configurar GPIO %d como saída\n", RELAY_COMPRESSOR_GPIO);
        gpiod_chip_close(gpio_chip);
        gpio_chip = NULL;
        compressor_line = NULL;
        return -1;
    }

    // Obter linha do GPIO da resistência
    heater_line = gpiod_chip_get_line(gpio_chip, RELAY_HEATER_GPIO);
    if (!heater_line) {
        fprintf(stderr, "Erro ao obter linha GPIO %d (Resistência)\n", RELAY_HEATER_GPIO);
        gpiod_line_release(compressor_line);
        gpiod_chip_close(gpio_chip);
        gpio_chip = NULL;
        compressor_line = NULL;
        return -1;
    }

    // Configurar GPIO da resistência como saída (iniciar desligada)
    ret = gpiod_line_request_output(heater_line, "relay_heater", RELAY_GPIO_INACTIVE_LEVEL);
    if (ret < 0) {
        fprintf(stderr, "Erro ao configurar GPIO %d como saída\n", RELAY_HEATER_GPIO);
        gpiod_line_release(compressor_line);
        gpiod_chip_close(gpio_chip);
        gpio_chip = NULL;
        compressor_line = NULL;
        heater_line = NULL;
        return -1;
    }

    printf("💡 Relés inicializados:\n");
    printf("   - Lâmpada:     Coil Modbus %d\n", RELAY_LAMP_COIL);
    printf("   - Discadora:   Coil Modbus %d\n", RELAY_DIALER_COIL);
    printf("   - Compressor:  GPIO %d (pino físico 18) [ativo em %s]\n",
           RELAY_COMPRESSOR_GPIO, RELAY_GPIO_ACTIVE_LEVEL ? "HIGH" : "LOW");
    printf("   - Resistência: GPIO %d (pino físico 22) [ativo em %s]\n",
           RELAY_HEATER_GPIO, RELAY_GPIO_ACTIVE_LEVEL ? "HIGH" : "LOW");

    return 0;
}

/**
 * @brief Finaliza o controle dos relés e libera recursos
 */
void relay_cleanup(void) {
    if (compressor_line) {
        // Garantir que o compressor está desligado
        gpiod_line_set_value(compressor_line, RELAY_GPIO_INACTIVE_LEVEL);
        gpiod_line_release(compressor_line);
        compressor_line = NULL;
    }

    if (heater_line) {
        // Garantir que a resistência está desligada
        gpiod_line_set_value(heater_line, RELAY_GPIO_INACTIVE_LEVEL);
        gpiod_line_release(heater_line);
        heater_line = NULL;
    }

    if (gpio_chip) {
        gpiod_chip_close(gpio_chip);
        gpio_chip = NULL;
    }

    printf("💡 Relés finalizados\n");
}

/**
 * @brief Liga a lâmpada (coil Modbus 0)
 */
int relay_lamp_on(void) {
    if (!relay_modbus_ctx) {
        fprintf(stderr, "Erro: Contexto Modbus não definido para lâmpada\n");
        return -1;
    }

    if (!modbus_write_coil(relay_modbus_ctx, RELAY_LAMP_COIL, true)) {
        fprintf(stderr, "Erro ao ligar lâmpada (coil %d)\n", RELAY_LAMP_COIL);
        return -1;
    }

    lamp_state_cache = true;
    printf("💡 Lâmpada LIGADA (coil %d)\n", RELAY_LAMP_COIL);
    return 0;
}

/**
 * @brief Desliga a lâmpada (coil Modbus 0)
 */
int relay_lamp_off(void) {
    if (!relay_modbus_ctx) {
        fprintf(stderr, "Erro: Contexto Modbus não definido para lâmpada\n");
        return -1;
    }

    if (!modbus_write_coil(relay_modbus_ctx, RELAY_LAMP_COIL, false)) {
        fprintf(stderr, "Erro ao desligar lâmpada (coil %d)\n", RELAY_LAMP_COIL);
        return -1;
    }

    lamp_state_cache = false;
    printf("💡 Lâmpada DESLIGADA (coil %d)\n", RELAY_LAMP_COIL);
    return 0;
}

/**
 * @brief Liga a discadora (coil Modbus 1)
 */
int relay_dialer_on(void) {
    if (!relay_modbus_ctx) {
        fprintf(stderr, "Erro: Contexto Modbus não definido para discadora\n");
        return -1;
    }

    if (!modbus_write_coil(relay_modbus_ctx, RELAY_DIALER_COIL, true)) {
        fprintf(stderr, "Erro ao ligar discadora (coil %d)\n", RELAY_DIALER_COIL);
        return -1;
    }

    dialer_state_cache = true;
    printf("📞 Discadora LIGADA (coil %d)\n", RELAY_DIALER_COIL);
    return 0;
}

/**
 * @brief Desliga a discadora (coil Modbus 1)
 */
int relay_dialer_off(void) {
    if (!relay_modbus_ctx) {
        fprintf(stderr, "Erro: Contexto Modbus não definido para discadora\n");
        return -1;
    }

    if (!modbus_write_coil(relay_modbus_ctx, RELAY_DIALER_COIL, false)) {
        fprintf(stderr, "Erro ao desligar discadora (coil %d)\n", RELAY_DIALER_COIL);
        return -1;
    }

    dialer_state_cache = false;
    printf("📞 Discadora DESLIGADA (coil %d)\n", RELAY_DIALER_COIL);
    return 0;
}

/**
 * @brief Liga o compressor (GPIO 24)
 */
int relay_compressor_on(void) {
    if (!compressor_line) {
        fprintf(stderr, "Erro: Compressor não inicializado\n");
        return -1;
    }

    int ret = gpiod_line_set_value(compressor_line, RELAY_GPIO_ACTIVE_LEVEL);
    if (ret < 0) {
        fprintf(stderr, "Erro ao ligar compressor (GPIO %d)\n", RELAY_COMPRESSOR_GPIO);
        return -1;
    }

    return 0;
}

/**
 * @brief Desliga o compressor (GPIO 24)
 */
int relay_compressor_off(void) {
    if (!compressor_line) {
        fprintf(stderr, "Erro: Compressor não inicializado\n");
        return -1;
    }

    int ret = gpiod_line_set_value(compressor_line, RELAY_GPIO_INACTIVE_LEVEL);
    if (ret < 0) {
        fprintf(stderr, "Erro ao desligar compressor (GPIO %d)\n", RELAY_COMPRESSOR_GPIO);
        return -1;
    }

    return 0;
}

/**
 * @brief Liga a resistência (GPIO 25)
 */
int relay_heater_on(void) {
    if (!heater_line) {
        fprintf(stderr, "Erro: Resistência não inicializada\n");
        return -1;
    }

    int ret = gpiod_line_set_value(heater_line, RELAY_GPIO_ACTIVE_LEVEL);
    if (ret < 0) {
        fprintf(stderr, "Erro ao ligar resistência (GPIO %d)\n", RELAY_HEATER_GPIO);
        return -1;
    }

    return 0;
}

/**
 * @brief Desliga a resistência (GPIO 25)
 */
int relay_heater_off(void) {
    if (!heater_line) {
        fprintf(stderr, "Erro: Resistência não inicializada\n");
        return -1;
    }

    int ret = gpiod_line_set_value(heater_line, RELAY_GPIO_INACTIVE_LEVEL);
    if (ret < 0) {
        fprintf(stderr, "Erro ao desligar resistência (GPIO %d)\n", RELAY_HEATER_GPIO);
        return -1;
    }

    return 0;
}

/**
 * @brief Controla a lâmpada baseado no estado da porta
 * @param door_open true se porta aberta (1), false se fechada (0)
 */
int relay_control_lamp_by_door(bool door_open) {
    if (door_open) {
        printf("🚪 Porta ABERTA → Ligando lâmpada\n");
        return relay_lamp_on();
    } else {
        printf("🚪 Porta FECHADA → Desligando lâmpada\n");
        return relay_lamp_off();
    }
}

/**
 * @brief Obtém o estado atual da lâmpada
 */
bool relay_lamp_is_on(void) {
    // Retorna cache local para evitar polling contínuo de coil a cada refresh de tela
    return lamp_state_cache;
}

/**
 * @brief Obtém o estado atual da discadora
 */
bool relay_dialer_is_on(void) {
    // Retorna cache local para evitar polling contínuo de coil a cada refresh de tela
    return dialer_state_cache;
}

/**
 * @brief Obtém o estado atual do compressor
 */
bool relay_compressor_is_on(void) {
    if (!compressor_line) return false;
    int value = gpiod_line_get_value(compressor_line);
    return (value == RELAY_GPIO_ACTIVE_LEVEL);
}

/**
 * @brief Obtém o estado atual da resistência
 */
bool relay_heater_is_on(void) {
    if (!heater_line) return false;
    int value = gpiod_line_get_value(heater_line);
    return (value == RELAY_GPIO_ACTIVE_LEVEL);
}
