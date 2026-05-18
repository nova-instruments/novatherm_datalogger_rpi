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
 * @brief Cria e conecta um contexto Modbus RTU
 */
static bool modbus_open_connection(modbus_context_t* mb_ctx) {
    if (!mb_ctx) return false;

    mb_ctx->ctx = (void*)modbus_new_rtu(MODBUS_DEVICE, MODBUS_BAUD_RATE,
                                        MODBUS_PARITY, MODBUS_DATA_BITS, MODBUS_STOP_BITS);
    if (!mb_ctx->ctx) {
        fprintf(stderr, "Erro Modbus: Erro ao criar contexto Modbus: %s\n", modbus_strerror(errno));
        mb_ctx->connected = false;
        return false;
    }

    if (modbus_set_slave((modbus_t*)mb_ctx->ctx, MODBUS_SLAVE_ID) == -1) {
        fprintf(stderr, "Erro Modbus: Erro ao definir slave ID: %s\n", modbus_strerror(errno));
        modbus_free((modbus_t*)mb_ctx->ctx);
        mb_ctx->ctx = NULL;
        mb_ctx->connected = false;
        return false;
    }

    modbus_set_response_timeout((modbus_t*)mb_ctx->ctx, 0, MODBUS_RESPONSE_TIMEOUT_US);
    modbus_set_byte_timeout((modbus_t*)mb_ctx->ctx, 0, MODBUS_BYTE_TIMEOUT_US);

    if (modbus_connect((modbus_t*)mb_ctx->ctx) == -1) {
        fprintf(stderr, "Erro Modbus: Erro na conexão: %s\n", modbus_strerror(errno));
        modbus_free((modbus_t*)mb_ctx->ctx);
        mb_ctx->ctx = NULL;
        mb_ctx->connected = false;
        return false;
    }

    mb_ctx->connected = true;
    return true;
}

/**
 * @brief Reabre conexão Modbus após erro de comunicação
 */
static bool modbus_reconnect(modbus_context_t* mb_ctx) {
    if (!mb_ctx) return false;

    if (mb_ctx->ctx) {
        modbus_close((modbus_t*)mb_ctx->ctx);
        modbus_free((modbus_t*)mb_ctx->ctx);
        mb_ctx->ctx = NULL;
    }
    mb_ctx->connected = false;

    return modbus_open_connection(mb_ctx);
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

    if (!modbus_open_connection(mb_ctx)) {
        free(mb_ctx);
        return NULL;
    }

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
    return modbus_read_register_from_slave(ctx, MODBUS_SLAVE_ID, address, value);
}

bool modbus_read_register_from_slave(modbus_context_t* ctx, uint8_t slave_id,
                                     uint16_t address, uint16_t* value) {
    return modbus_read_registers_from_slave(ctx, slave_id, address, 1, value);
}

bool modbus_read_registers_from_slave(modbus_context_t* ctx, uint8_t slave_id,
                                      uint16_t start_address, int quantity, uint16_t* values) {
    if (!ctx || !ctx->ctx || !ctx->connected || !values) {
        return false;
    }
    if (quantity < 1 || quantity > 125 || !values) {
        return false;
    }

    // Retry curto para lidar com timeouts/interferências no barramento RS-485.
    for (int attempt = 0; attempt < 3; attempt++) {
        if (modbus_set_slave((modbus_t*)ctx->ctx, slave_id) == -1) {
            fprintf(stderr, "Erro ao selecionar slave Modbus %u: %s\n",
                    slave_id, modbus_strerror(errno));
            if (attempt < 2) {
                usleep(60000);
                continue;
            }
            return false;
        }

        int rc = modbus_read_registers((modbus_t*)ctx->ctx, start_address, quantity, values);
        if (rc != -1) {
            return true;
        }

        // Limpar possível resposta pendente no buffer serial antes de nova tentativa
        modbus_flush((modbus_t*)ctx->ctx);

        if (attempt < 2 && errno == ETIMEDOUT) {
            fprintf(stderr, "⚠️  Timeout ao ler slave %u registros 0x%04X..0x%04X, tentando reconectar Modbus...\n",
                    slave_id, start_address, (uint16_t)(start_address + quantity - 1));
            if (!modbus_reconnect(ctx)) {
                break;
            }
            usleep(60000);  // 60ms antes da nova tentativa
        } else if (attempt < 2) {
            usleep(60000);  // 60ms antes da nova tentativa
        }
    }

    fprintf(stderr, "Erro ao ler slave %u registradores Modbus 0x%04X..0x%04X: %s\n",
            slave_id, start_address, (uint16_t)(start_address + quantity - 1), modbus_strerror(errno));
    return false;
}

bool modbus_write_coil(modbus_context_t* ctx, uint16_t address, bool state) {
    if (!ctx || !ctx->ctx || !ctx->connected) {
        return false;
    }

    // Retry curto para lidar com timeouts transitórios no barramento
    for (int attempt = 0; attempt < 2; attempt++) {
        if (modbus_set_slave((modbus_t*)ctx->ctx, MODBUS_SLAVE_ID) == -1) {
            fprintf(stderr, "Erro ao selecionar slave Modbus %d para escrita de coil: %s\n",
                    MODBUS_SLAVE_ID, modbus_strerror(errno));
            if (attempt == 0) {
                usleep(20000);
                continue;
            }
            return false;
        }

        int rc = modbus_write_bit((modbus_t*)ctx->ctx, address, state ? 1 : 0);
        if (rc != -1) {
            return true;
        }

        if (attempt == 0) {
            modbus_flush((modbus_t*)ctx->ctx);
            usleep(20000);  // 20ms antes de tentar novamente
        } else {
            fprintf(stderr, "Erro ao escrever coil 0x%X: %s\n", address, modbus_strerror(errno));
        }
    }

    return false;
}

bool modbus_read_coil(modbus_context_t* ctx, uint16_t address, bool* state) {
    if (!ctx || !ctx->ctx || !ctx->connected || !state) {
        return false;
    }

    // Retry curto para lidar com timeouts transitórios no barramento
    for (int attempt = 0; attempt < 2; attempt++) {
        if (modbus_set_slave((modbus_t*)ctx->ctx, MODBUS_SLAVE_ID) == -1) {
            fprintf(stderr, "Erro ao selecionar slave Modbus %d para leitura de coil: %s\n",
                    MODBUS_SLAVE_ID, modbus_strerror(errno));
            if (attempt == 0) {
                usleep(20000);
                continue;
            }
            return false;
        }

        uint8_t bit = 0;
        int rc = modbus_read_bits((modbus_t*)ctx->ctx, address, 1, &bit);
        if (rc != -1) {
            *state = (bit != 0);
            return true;
        }

        if (attempt == 0) {
            modbus_flush((modbus_t*)ctx->ctx);
            usleep(20000);  // 20ms antes de tentar novamente
        } else {
            fprintf(stderr, "Erro ao ler coil 0x%X: %s\n", address, modbus_strerror(errno));
        }
    }

    return false;
}

bool modbus_read_all(modbus_context_t* ctx, modbus_data_t* data, int num_channels) {
    if (!ctx || !data) {
        return false;
    }

    // Validar número de canais (1 a 6)
    if (num_channels < 1) num_channels = 1;
    if (num_channels > MODBUS_NUM_CHANNELS) num_channels = MODBUS_NUM_CHANNELS;

    // Inicializar estrutura
    memset(data, 0, sizeof(modbus_data_t));

    // Endereços fixos dos registradores Modbus lidos
    const uint16_t channel_addresses[MODBUS_NUM_CHANNELS] = {
        MODBUS_ADDR_MAIN_TEMP,
        MODBUS_ADDR_DEFROST_TEMP,
        MODBUS_ADDR_AC_VOLTAGE,
        MODBUS_ADDR_DC_VOLTAGE,
        MODBUS_ADDR_NTC1_RES,
        MODBUS_ADDR_NTC2_RES
    };

    bool at_least_one_success = false;

    // Ler apenas os canais configurados
    for (int i = 0; i < num_channels; i++) {
        data->ch_valid[i] = modbus_read_register(ctx, channel_addresses[i], &data->ch_raw[i]);

        if (data->ch_valid[i]) {
            at_least_one_success = true;

            // CH1 e CH2: temperaturas (signed 16-bit, escala /10)
            if (i == 0 || i == 1) {
                if (data->ch_raw[i] == MODBUS_SENSOR_ERROR) {
                    data->ch_error[i] = true;
                    data->ch_temp[i] = -273.1f;
                } else {
                    data->ch_error[i] = false;
                    int16_t temp_raw = (int16_t)data->ch_raw[i];
                    data->ch_temp[i] = temp_raw / 10.0f;
                }
            } else if (i == 2 || i == 3) {
                // CH3 e CH4: tensões (unsigned, escala /10)
                data->ch_error[i] = false;
                data->ch_temp[i] = data->ch_raw[i] / 10.0f;
            } else {
                // CH5 e CH6: resistências NTC (unsigned, em ohms sem escala)
                data->ch_error[i] = false;
                data->ch_temp[i] = (float)data->ch_raw[i];
            }
        }

        // Delay de 50ms entre leituras para não sobrecarregar o barramento
        usleep(50000);
    }

    return at_least_one_success;
}

void modbus_print_config(void) {
    printf("Configuração Modbus:\n");
    printf("  Dispositivo: %s\n", MODBUS_DEVICE);
    printf("  Configuração: %d-%c-%d-%d\n", MODBUS_BAUD_RATE, MODBUS_PARITY,
           MODBUS_DATA_BITS, MODBUS_STOP_BITS);
    printf("  Slave ID: %d\n", MODBUS_SLAVE_ID);
    printf("  Registros lidos: %d\n", MODBUS_NUM_CHANNELS);
    printf("  Endereços: 0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X\n",
           MODBUS_ADDR_MAIN_TEMP, MODBUS_ADDR_DEFROST_TEMP,
           MODBUS_ADDR_AC_VOLTAGE, MODBUS_ADDR_DC_VOLTAGE,
           MODBUS_ADDR_NTC1_RES, MODBUS_ADDR_NTC2_RES);
    printf("  Timeout resposta: %d ms\n", MODBUS_RESPONSE_TIMEOUT_US / 1000);
    printf("  Timeout byte: %d ms\n", MODBUS_BYTE_TIMEOUT_US / 1000);
    printf("----------------------------------------\n");
}

void modbus_print_data(const modbus_data_t* data) {
    if (!data) return;

    static const char* labels[MODBUS_NUM_CHANNELS] = {
        "PR1",
        "PR2",
        "AC",
        "DC",
        "NTC1R",
        "NTC2R"
    };

    static const uint16_t addresses[MODBUS_NUM_CHANNELS] = {
        MODBUS_ADDR_MAIN_TEMP,
        MODBUS_ADDR_DEFROST_TEMP,
        MODBUS_ADDR_AC_VOLTAGE,
        MODBUS_ADDR_DC_VOLTAGE,
        MODBUS_ADDR_NTC1_RES,
        MODBUS_ADDR_NTC2_RES
    };

    printf("Dados Modbus (%d registradores):\n", MODBUS_NUM_CHANNELS);

    for (int i = 0; i < MODBUS_NUM_CHANNELS; i++) {
        printf("  %s (0x%04X): ", labels[i], addresses[i]);

        if (!data->ch_valid[i]) {
            printf("ERRO na leitura\n");
            continue;
        }

        if (i < 2) {
            if (data->ch_error[i]) {
                printf("SENSOR DESCONECTADO (0x%04X = -273.1°C)\n", data->ch_raw[i]);
            } else {
                int16_t temp_raw = (int16_t)data->ch_raw[i];
                printf("%d (0x%04X) = %.1f°C\n", temp_raw, data->ch_raw[i], data->ch_temp[i]);
            }
        } else if (i == 2 || i == 3) {
            printf("%u (0x%04X) = %.1fV\n", data->ch_raw[i], data->ch_raw[i], data->ch_temp[i]);
        } else {
            printf("%u (0x%04X) = %.0f ohms\n", data->ch_raw[i], data->ch_raw[i], data->ch_temp[i]);
        }
    }
}

bool modbus_value_to_binary(uint16_t value) {
    return (value != 0);
}
