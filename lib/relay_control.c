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
static struct gpiod_line *lamp_line = NULL;
static struct gpiod_line *dialer_line = NULL;
static struct gpiod_line *compressor_line = NULL;
static struct gpiod_line *heater_line = NULL;

/**
 * @brief Inicializa o controle dos relés
 */
int relay_init(void) {
    if (gpio_chip && lamp_line && dialer_line && compressor_line && heater_line) {
        printf("Relés já inicializados\n");
        return 0;
    }

    // Abrir chip GPIO
    gpio_chip = gpiod_chip_open_by_name(GPIO_CHIP_NAME);
    if (!gpio_chip) {
        fprintf(stderr, "Erro ao abrir chip GPIO: %s\n", GPIO_CHIP_NAME);
        return -1;
    }

    // Obter linha do GPIO 24 (Lâmpada)
    lamp_line = gpiod_chip_get_line(gpio_chip, RELAY_LAMP_GPIO);
    if (!lamp_line) {
        fprintf(stderr, "Erro ao obter linha GPIO %d (Lâmpada)\n", RELAY_LAMP_GPIO);
        gpiod_chip_close(gpio_chip);
        gpio_chip = NULL;
        return -1;
    }

    // Obter linha do GPIO 25 (Discadora)
    dialer_line = gpiod_chip_get_line(gpio_chip, RELAY_DIALER_GPIO);
    if (!dialer_line) {
        fprintf(stderr, "Erro ao obter linha GPIO %d (Discadora)\n", RELAY_DIALER_GPIO);
        gpiod_chip_close(gpio_chip);
        gpio_chip = NULL;
        lamp_line = NULL;
        return -1;
    }

    // Configurar GPIO 24 como saída (Lâmpada) - iniciar desligado
    int ret = gpiod_line_request_output(lamp_line, "relay_lamp", 0);
    if (ret < 0) {
        fprintf(stderr, "Erro ao configurar GPIO %d como saída\n", RELAY_LAMP_GPIO);
        gpiod_chip_close(gpio_chip);
        gpio_chip = NULL;
        lamp_line = NULL;
        dialer_line = NULL;
        return -1;
    }

    // Configurar GPIO 25 como saída (Discadora) - iniciar desligado
    ret = gpiod_line_request_output(dialer_line, "relay_dialer", 0);
    if (ret < 0) {
        fprintf(stderr, "Erro ao configurar GPIO %d como saída\n", RELAY_DIALER_GPIO);
        gpiod_line_release(lamp_line);
        gpiod_chip_close(gpio_chip);
        gpio_chip = NULL;
        lamp_line = NULL;
        dialer_line = NULL;
        return -1;
    }

    // Obter linha do GPIO 4 (Compressor)
    compressor_line = gpiod_chip_get_line(gpio_chip, RELAY_COMPRESSOR_GPIO);
    if (!compressor_line) {
        fprintf(stderr, "Erro ao obter linha GPIO %d (Compressor)\n", RELAY_COMPRESSOR_GPIO);
        gpiod_line_release(lamp_line);
        gpiod_line_release(dialer_line);
        gpiod_chip_close(gpio_chip);
        gpio_chip = NULL;
        lamp_line = NULL;
        dialer_line = NULL;
        return -1;
    }

    // Configurar GPIO 4 como saída (Compressor) - iniciar desligado (lógica inversa: HIGH = desligado)
    ret = gpiod_line_request_output(compressor_line, "relay_compressor", 1);
    if (ret < 0) {
        fprintf(stderr, "Erro ao configurar GPIO %d como saída\n", RELAY_COMPRESSOR_GPIO);
        gpiod_line_release(lamp_line);
        gpiod_line_release(dialer_line);
        gpiod_chip_close(gpio_chip);
        gpio_chip = NULL;
        lamp_line = NULL;
        dialer_line = NULL;
        compressor_line = NULL;
        return -1;
    }

    // Obter linha do GPIO 17 (Resistência)
    heater_line = gpiod_chip_get_line(gpio_chip, RELAY_HEATER_GPIO);
    if (!heater_line) {
        fprintf(stderr, "Erro ao obter linha GPIO %d (Resistência)\n", RELAY_HEATER_GPIO);
        gpiod_line_release(lamp_line);
        gpiod_line_release(dialer_line);
        gpiod_line_release(compressor_line);
        gpiod_chip_close(gpio_chip);
        gpio_chip = NULL;
        lamp_line = NULL;
        dialer_line = NULL;
        compressor_line = NULL;
        return -1;
    }

    // Configurar GPIO 17 como saída (Resistência) - iniciar desligado (lógica inversa: HIGH = desligado)
    ret = gpiod_line_request_output(heater_line, "relay_heater", 1);
    if (ret < 0) {
        fprintf(stderr, "Erro ao configurar GPIO %d como saída\n", RELAY_HEATER_GPIO);
        gpiod_line_release(lamp_line);
        gpiod_line_release(dialer_line);
        gpiod_line_release(compressor_line);
        gpiod_chip_close(gpio_chip);
        gpio_chip = NULL;
        lamp_line = NULL;
        dialer_line = NULL;
        compressor_line = NULL;
        heater_line = NULL;
        return -1;
    }

    printf("💡 Relés inicializados:\n");
    printf("   - Lâmpada:     GPIO %d (pino físico 18)\n", RELAY_LAMP_GPIO);
    printf("   - Discadora:   GPIO %d (pino físico 22)\n", RELAY_DIALER_GPIO);
    printf("   - Compressor:  GPIO %d (pino físico 7) [lógica inversa]\n", RELAY_COMPRESSOR_GPIO);
    printf("   - Resistência: GPIO %d (pino físico 11) [lógica inversa]\n", RELAY_HEATER_GPIO);

    return 0;
}

/**
 * @brief Finaliza o controle dos relés e libera recursos
 */
void relay_cleanup(void) {
    if (lamp_line) {
        // Garantir que a lâmpada está desligada
        gpiod_line_set_value(lamp_line, 0);
        gpiod_line_release(lamp_line);
        lamp_line = NULL;
    }

    if (dialer_line) {
        // Garantir que a discadora está desligada
        gpiod_line_set_value(dialer_line, 0);
        gpiod_line_release(dialer_line);
        dialer_line = NULL;
    }

    if (compressor_line) {
        // Garantir que o compressor está desligado (lógica inversa: HIGH = desligado)
        gpiod_line_set_value(compressor_line, 1);
        gpiod_line_release(compressor_line);
        compressor_line = NULL;
    }

    if (heater_line) {
        // Garantir que a resistência está desligada (lógica inversa: HIGH = desligado)
        gpiod_line_set_value(heater_line, 1);
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
 * @brief Liga a lâmpada (relé GPIO 24)
 */
int relay_lamp_on(void) {
    if (!lamp_line) {
        fprintf(stderr, "Erro: Relé da lâmpada não inicializado\n");
        return -1;
    }

    int ret = gpiod_line_set_value(lamp_line, 1);
    if (ret < 0) {
        fprintf(stderr, "Erro ao ligar lâmpada (GPIO %d)\n", RELAY_LAMP_GPIO);
        return -1;
    }

    printf("💡 Lâmpada LIGADA (GPIO %d)\n", RELAY_LAMP_GPIO);
    return 0;
}

/**
 * @brief Desliga a lâmpada (relé GPIO 24)
 */
int relay_lamp_off(void) {
    if (!lamp_line) {
        fprintf(stderr, "Erro: Relé da lâmpada não inicializado\n");
        return -1;
    }

    int ret = gpiod_line_set_value(lamp_line, 0);
    if (ret < 0) {
        fprintf(stderr, "Erro ao desligar lâmpada (GPIO %d)\n", RELAY_LAMP_GPIO);
        return -1;
    }

    printf("💡 Lâmpada DESLIGADA (GPIO %d)\n", RELAY_LAMP_GPIO);
    return 0;
}

/**
 * @brief Liga a discadora (relé GPIO 25)
 */
int relay_dialer_on(void) {
    if (!dialer_line) {
        fprintf(stderr, "Erro: Relé da discadora não inicializado\n");
        return -1;
    }

    int ret = gpiod_line_set_value(dialer_line, 1);
    if (ret < 0) {
        fprintf(stderr, "Erro ao ligar discadora (GPIO %d)\n", RELAY_DIALER_GPIO);
        return -1;
    }

    printf("📞 Discadora LIGADA (GPIO %d)\n", RELAY_DIALER_GPIO);
    return 0;
}

/**
 * @brief Desliga a discadora (relé GPIO 25)
 */
int relay_dialer_off(void) {
    if (!dialer_line) {
        fprintf(stderr, "Erro: Relé da discadora não inicializado\n");
        return -1;
    }

    int ret = gpiod_line_set_value(dialer_line, 0);
    if (ret < 0) {
        fprintf(stderr, "Erro ao desligar discadora (GPIO %d)\n", RELAY_DIALER_GPIO);
        return -1;
    }

    printf("📞 Discadora DESLIGADA (GPIO %d)\n", RELAY_DIALER_GPIO);
    return 0;
}

/**
 * @brief Liga o compressor (GPIO 4) - Lógica inversa: LOW = ligado
 */
int relay_compressor_on(void) {
    if (!compressor_line) {
        fprintf(stderr, "Erro: Compressor não inicializado\n");
        return -1;
    }

    int ret = gpiod_line_set_value(compressor_line, 0);  // LOW = ligado
    if (ret < 0) {
        fprintf(stderr, "Erro ao ligar compressor (GPIO %d)\n", RELAY_COMPRESSOR_GPIO);
        return -1;
    }

    printf("❄️  Compressor LIGADO (GPIO %d)\n", RELAY_COMPRESSOR_GPIO);
    return 0;
}

/**
 * @brief Desliga o compressor (GPIO 4) - Lógica inversa: HIGH = desligado
 */
int relay_compressor_off(void) {
    if (!compressor_line) {
        fprintf(stderr, "Erro: Compressor não inicializado\n");
        return -1;
    }

    int ret = gpiod_line_set_value(compressor_line, 1);  // HIGH = desligado
    if (ret < 0) {
        fprintf(stderr, "Erro ao desligar compressor (GPIO %d)\n", RELAY_COMPRESSOR_GPIO);
        return -1;
    }

    printf("❄️  Compressor DESLIGADO (GPIO %d)\n", RELAY_COMPRESSOR_GPIO);
    return 0;
}

/**
 * @brief Liga a resistência (GPIO 17) - Lógica inversa: LOW = ligado
 */
int relay_heater_on(void) {
    if (!heater_line) {
        fprintf(stderr, "Erro: Resistência não inicializada\n");
        return -1;
    }

    int ret = gpiod_line_set_value(heater_line, 0);  // LOW = ligado
    if (ret < 0) {
        fprintf(stderr, "Erro ao ligar resistência (GPIO %d)\n", RELAY_HEATER_GPIO);
        return -1;
    }

    printf("🔥 Resistência LIGADA (GPIO %d)\n", RELAY_HEATER_GPIO);
    return 0;
}

/**
 * @brief Desliga a resistência (GPIO 17) - Lógica inversa: HIGH = desligado
 */
int relay_heater_off(void) {
    if (!heater_line) {
        fprintf(stderr, "Erro: Resistência não inicializada\n");
        return -1;
    }

    int ret = gpiod_line_set_value(heater_line, 1);  // HIGH = desligado
    if (ret < 0) {
        fprintf(stderr, "Erro ao desligar resistência (GPIO %d)\n", RELAY_HEATER_GPIO);
        return -1;
    }

    printf("🔥 Resistência DESLIGADA (GPIO %d)\n", RELAY_HEATER_GPIO);
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

