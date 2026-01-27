/**
 * @file modbus.h
 * @brief NovaTherm DataLogger - Modbus RTU Library
 * @author Nova Instruments
 */

#ifndef MODBUS_H
#define MODBUS_H

#include <stdint.h>
#include <stdbool.h>

// Forward declaration para evitar dependência circular
typedef struct modbus_t modbus_t;

// Configurações Modbus
#define MODBUS_DEVICE     "/dev/serial0"
#define MODBUS_BAUD_RATE  9600
#define MODBUS_PARITY     'N'
#define MODBUS_DATA_BITS  8
#define MODBUS_STOP_BITS  1
#define MODBUS_SLAVE_ID   1

// Endereços Modbus - NT18B07 (7 canais de temperatura)
#define MODBUS_ADDR_CH1 0x0000  // Canal 1 - Temperatura
#define MODBUS_ADDR_CH2 0x0001  // Canal 2 - Temperatura
#define MODBUS_ADDR_CH3 0x0002  // Canal 3 - Temperatura
#define MODBUS_ADDR_CH4 0x0003  // Canal 4 - Temperatura
#define MODBUS_ADDR_CH5 0x0004  // Canal 5 - Temperatura
#define MODBUS_ADDR_CH6 0x0005  // Canal 6 - Temperatura
#define MODBUS_ADDR_CH7 0x0006  // Canal 7 - Temperatura

// Número de canais
#define MODBUS_NUM_CHANNELS 7

// Valor de erro do sensor (sensor desconectado)
#define MODBUS_SENSOR_ERROR 0xF555  // -273.1°C indica erro

// Timeouts (em microssegundos)
#define MODBUS_RESPONSE_TIMEOUT_US 500000  // 500ms
#define MODBUS_BYTE_TIMEOUT_US     200000  // 200ms

// Estrutura para dados lidos do NT18B07
typedef struct {
    uint16_t ch_raw[MODBUS_NUM_CHANNELS];     // Valores brutos dos 7 canais
    float ch_temp[MODBUS_NUM_CHANNELS];       // Temperaturas convertidas (°C)
    bool ch_valid[MODBUS_NUM_CHANNELS];       // Flags de validade de cada canal
    bool ch_error[MODBUS_NUM_CHANNELS];       // Flags de erro de sensor (desconectado)
} modbus_data_t;

// Handle opaco para contexto Modbus
typedef struct modbus_context_s modbus_context_t;

/**
 * @brief Inicializa conexão Modbus
 * @return Ponteiro para contexto Modbus ou NULL em caso de erro
 */
modbus_context_t* modbus_init(void);

/**
 * @brief Finaliza conexão Modbus e libera recursos
 * @param ctx Contexto Modbus
 */
void modbus_cleanup(modbus_context_t* ctx);

/**
 * @brief Lê todos os registradores configurados
 * @param ctx Contexto Modbus
 * @param data Estrutura para armazenar os dados lidos
 * @return true se pelo menos uma leitura foi bem-sucedida, false caso contrário
 */
bool modbus_read_all(modbus_context_t* ctx, modbus_data_t* data);

/**
 * @brief Lê um registrador específico
 * @param ctx Contexto Modbus
 * @param address Endereço do registrador
 * @param value Ponteiro para armazenar o valor lido
 * @return true se leitura foi bem-sucedida, false caso contrário
 */
bool modbus_read_register(modbus_context_t* ctx, uint16_t address, uint16_t* value);

/**
 * @brief Imprime informações de configuração Modbus
 */
void modbus_print_config(void);

/**
 * @brief Imprime dados lidos de forma formatada
 * @param data Estrutura com os dados lidos
 */
void modbus_print_data(const modbus_data_t* data);

/**
 * @brief Converte valor para representação binária (0 ou 1)
 * @param value Valor a ser convertido
 * @return true se valor != 0, false se valor == 0
 */
bool modbus_value_to_binary(uint16_t value);

#endif // MODBUS_H
