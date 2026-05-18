/**
 * @file modbus.h
 * @brief NovaTherm DataLogger - Modbus RTU Library
 * @author Nova Instruments
 */

#ifndef NOVATHERM_MODBUS_H
#define NOVATHERM_MODBUS_H

#include <stdint.h>
#include <stdbool.h>

// Configurações Modbus
#define MODBUS_DEVICE     "/dev/serial0"
#define MODBUS_BAUD_RATE  9600
#define MODBUS_PARITY     'N'
#define MODBUS_DATA_BITS  8
#define MODBUS_STOP_BITS  1
#define MODBUS_SLAVE_ID   1

// Endereços Modbus utilizados atualmente
#define MODBUS_ADDR_MAIN_TEMP     0x0000  // Temperatura principal (°C)
#define MODBUS_ADDR_DEFROST_TEMP  0x0001  // Temperatura degelo (°C)
#define MODBUS_ADDR_AC_VOLTAGE    0x0002  // Tensão AC (V)
#define MODBUS_ADDR_DC_VOLTAGE    0x0003  // Tensão DC (V)
#define MODBUS_ADDR_NTC1_RES      0x0004  // Resistência NTC1 (ohms)
#define MODBUS_ADDR_NTC2_RES      0x0005  // Resistência NTC2 (ohms)

// Endereços de coils Modbus
#define MODBUS_COIL_LAMP          0x0000  // Lâmpada
#define MODBUS_COIL_DIALER        0x0001  // Discadora

// Aliases de compatibilidade com o código legado (CH1-CH6)
#define MODBUS_ADDR_CH1 MODBUS_ADDR_MAIN_TEMP
#define MODBUS_ADDR_CH2 MODBUS_ADDR_DEFROST_TEMP
#define MODBUS_ADDR_CH3 MODBUS_ADDR_AC_VOLTAGE
#define MODBUS_ADDR_CH4 MODBUS_ADDR_DC_VOLTAGE
#define MODBUS_ADDR_CH5 MODBUS_ADDR_NTC1_RES
#define MODBUS_ADDR_CH6 MODBUS_ADDR_NTC2_RES

// Número total de registradores lidos
#define MODBUS_NUM_CHANNELS 6

// Valor de erro do sensor (sensor desconectado)
#define MODBUS_SENSOR_ERROR 0xF555  // -273.1°C indica erro

// Timeouts (em microssegundos)
#define MODBUS_RESPONSE_TIMEOUT_US 800000  // 800ms
#define MODBUS_BYTE_TIMEOUT_US     300000  // 300ms

// Estrutura para dados lidos do Modbus
typedef struct {
    uint16_t ch_raw[MODBUS_NUM_CHANNELS];     // Valores brutos dos registradores
    float ch_temp[MODBUS_NUM_CHANNELS];       // Valores escalados:
                                              // CH1/CH2 em °C, CH3/CH4 em V, CH5/CH6 em ohms
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
 * @param num_channels Número de canais a serem lidos (1 a 6)
 * @return true se pelo menos uma leitura foi bem-sucedida, false caso contrário
 */
bool modbus_read_all(modbus_context_t* ctx, modbus_data_t* data, int num_channels);

/**
 * @brief Lê um registrador específico
 * @param ctx Contexto Modbus
 * @param address Endereço do registrador
 * @param value Ponteiro para armazenar o valor lido
 * @return true se leitura foi bem-sucedida, false caso contrário
 */
bool modbus_read_register(modbus_context_t* ctx, uint16_t address, uint16_t* value);
/**
 * @brief Lê um registrador específico de um slave Modbus informado
 * @param ctx Contexto Modbus
 * @param slave_id ID do slave Modbus
 * @param address Endereço do registrador
 * @param value Ponteiro para armazenar o valor lido
 * @return true se leitura foi bem-sucedida, false caso contrário
 */
bool modbus_read_register_from_slave(modbus_context_t* ctx, uint8_t slave_id,
                                     uint16_t address, uint16_t* value);
/**
 * @brief Lê múltiplos registradores contíguos de um slave Modbus informado
 * @param ctx Contexto Modbus
 * @param slave_id ID do slave Modbus
 * @param start_address Endereço inicial
 * @param quantity Quantidade de registradores
 * @param values Buffer para armazenar valores lidos
 * @return true se leitura foi bem-sucedida, false caso contrário
 */
bool modbus_read_registers_from_slave(modbus_context_t* ctx, uint8_t slave_id,
                                      uint16_t start_address, int quantity, uint16_t* values);

/**
 * @brief Escreve um coil específico
 * @param ctx Contexto Modbus
 * @param address Endereço do coil
 * @param state Estado desejado (true = ON, false = OFF)
 * @return true se escrita foi bem-sucedida, false caso contrário
 */
bool modbus_write_coil(modbus_context_t* ctx, uint16_t address, bool state);

/**
 * @brief Lê um coil específico
 * @param ctx Contexto Modbus
 * @param address Endereço do coil
 * @param state Ponteiro para armazenar estado lido (true = ON, false = OFF)
 * @return true se leitura foi bem-sucedida, false caso contrário
 */
bool modbus_read_coil(modbus_context_t* ctx, uint16_t address, bool* state);

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

#endif // NOVATHERM_MODBUS_H
