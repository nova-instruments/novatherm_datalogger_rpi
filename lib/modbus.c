/**
 * @file modbus.c
 * @brief NovaTherm DataLogger - Modbus RTU Library Implementation
 * @author Nova Instruments
 */

#include "modbus.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <modbus/modbus.h>

// Estrutura interna do contexto Modbus
struct modbus_context_s {
    void* ctx;  // modbus_t* - usando void* para evitar dependência circular
    bool connected;
};

/**
 * @brief Função auxiliar para tratamento de erros
 */
static void modbus_error(modbus_context_t* mb_ctx, const char* msg) {
    fprintf(stderr, "Erro Modbus: %s: %s\n", msg, modbus_strerror(errno));
    if (mb_ctx && mb_ctx->ctx) {
        modbus_close((modbus_t*)mb_ctx->ctx);
        modbus_free((modbus_t*)mb_ctx->ctx);
        mb_ctx->ctx = NULL;
        mb_ctx->connected = false;
    }
}

modbus_context_t* modbus_init(void) {
    modbus_context_t* mb_ctx = malloc(sizeof(struct modbus_context_s));
    if (!mb_ctx) {
        fprintf(stderr, "Erro: Falha ao alocar memória para contexto Modbus\n");
        return NULL;
    }

    mb_ctx->ctx = NULL;
    mb_ctx->connected = false;

    printf("Iniciando conexão Modbus...\n");
    modbus_print_config();

    // Criar contexto RTU
    mb_ctx->ctx = (void*)modbus_new_rtu(MODBUS_DEVICE, MODBUS_BAUD_RATE,
                                        MODBUS_PARITY, MODBUS_DATA_BITS, MODBUS_STOP_BITS);
    if (!mb_ctx->ctx) {
        modbus_error(mb_ctx, "Erro ao criar contexto Modbus");
        free(mb_ctx);
        return NULL;
    }

    // Definir ID do escravo
    if (modbus_set_slave((modbus_t*)mb_ctx->ctx, MODBUS_SLAVE_ID) == -1) {
        modbus_error(mb_ctx, "Erro ao definir slave ID");
        free(mb_ctx);
        return NULL;
    }

    // Configurar timeouts
    modbus_set_response_timeout((modbus_t*)mb_ctx->ctx, 0, MODBUS_RESPONSE_TIMEOUT_US);
    modbus_set_byte_timeout((modbus_t*)mb_ctx->ctx, 0, MODBUS_BYTE_TIMEOUT_US);

    // Abrir conexão
    if (modbus_connect((modbus_t*)mb_ctx->ctx) == -1) {
        modbus_error(mb_ctx, "Erro na conexão");
        free(mb_ctx);
        return NULL;
    }

    mb_ctx->connected = true;
    printf("Conexão Modbus estabelecida com sucesso!\n\n");

    return mb_ctx;
}

void modbus_cleanup(modbus_context_t* ctx) {
    if (!ctx) return;

    if (ctx->ctx) {
        if (ctx->connected) {
            modbus_close((modbus_t*)ctx->ctx);
        }
        modbus_free((modbus_t*)ctx->ctx);
    }

    free(ctx);
}

bool modbus_read_register(modbus_context_t* ctx, uint16_t address, uint16_t* value) {
    if (!ctx || !ctx->ctx || !ctx->connected || !value) {
        return false;
    }

    int rc = modbus_read_registers((modbus_t*)ctx->ctx, address, 1, value);
    if (rc == -1) {
        fprintf(stderr, "Erro ao ler endereço 0x%X: %s\n", address, modbus_strerror(errno));
        return false;
    }

    return true;
}

bool modbus_read_all(modbus_context_t* ctx, modbus_data_t* data, int num_channels) {
    if (!ctx || !data) {
        return false;
    }

    // Validar número de canais (1 a 7)
    if (num_channels < 1) num_channels = 1;
    if (num_channels > MODBUS_NUM_CHANNELS) num_channels = MODBUS_NUM_CHANNELS;

    // Inicializar estrutura
    memset(data, 0, sizeof(modbus_data_t));

    // Endereços dos 7 canais do NT18B07
    const uint16_t channel_addresses[MODBUS_NUM_CHANNELS] = {
        MODBUS_ADDR_CH1, MODBUS_ADDR_CH2, MODBUS_ADDR_CH3, MODBUS_ADDR_CH4,
        MODBUS_ADDR_CH5, MODBUS_ADDR_CH6, MODBUS_ADDR_CH7
    };

    bool at_least_one_success = false;

    // Ler apenas os canais configurados
    for (int i = 0; i < num_channels; i++) {
        data->ch_valid[i] = modbus_read_register(ctx, channel_addresses[i], &data->ch_raw[i]);

        if (data->ch_valid[i]) {
            at_least_one_success = true;

            // Verificar se é erro de sensor (0xF555 = -273.1°C)
            if (data->ch_raw[i] == MODBUS_SENSOR_ERROR) {
                data->ch_error[i] = true;
                data->ch_temp[i] = -273.1f;
            } else {
                data->ch_error[i] = false;

                // Converter valor signed 16-bit para temperatura
                // Valores negativos: subtrair 65536 e dividir por 10
                int16_t temp_raw = (int16_t)data->ch_raw[i];
                data->ch_temp[i] = temp_raw / 10.0f;
            }
        }

        // Delay de 50ms entre leituras para não sobrecarregar o barramento
        usleep(50000);
    }

    return at_least_one_success;
}

void modbus_print_config(void) {
    printf("Configuração Modbus NT18B07:\n");
    printf("  Dispositivo: %s\n", MODBUS_DEVICE);
    printf("  Configuração: %d-%c-%d-%d\n", MODBUS_BAUD_RATE, MODBUS_PARITY,
           MODBUS_DATA_BITS, MODBUS_STOP_BITS);
    printf("  Slave ID: %d\n", MODBUS_SLAVE_ID);
    printf("  Canais de temperatura: %d (CH1-CH7)\n", MODBUS_NUM_CHANNELS);
    printf("  Endereços: 0x%04X a 0x%04X\n", MODBUS_ADDR_CH1, MODBUS_ADDR_CH7);
    printf("  Timeout resposta: %d ms\n", MODBUS_RESPONSE_TIMEOUT_US / 1000);
    printf("  Timeout byte: %d ms\n", MODBUS_BYTE_TIMEOUT_US / 1000);
    printf("----------------------------------------\n");
}

void modbus_print_data(const modbus_data_t* data) {
    if (!data) return;

    printf("Dados NT18B07 (7 canais):\n");

    for (int i = 0; i < MODBUS_NUM_CHANNELS; i++) {
        printf("  CH%d (0x%04X): ", i + 1, MODBUS_ADDR_CH1 + i);

        if (data->ch_valid[i]) {
            if (data->ch_error[i]) {
                printf("SENSOR DESCONECTADO (0x%04X = -273.1°C)\n", data->ch_raw[i]);
            } else {
                int16_t temp_raw = (int16_t)data->ch_raw[i];
                printf("%d (0x%04X) = %.1f°C\n", temp_raw, data->ch_raw[i], data->ch_temp[i]);
            }
        } else {
            printf("ERRO na leitura\n");
        }
    }
}

bool modbus_value_to_binary(uint16_t value) {
    return (value != 0);
}
