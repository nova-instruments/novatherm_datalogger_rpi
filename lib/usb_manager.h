#ifndef USB_MANAGER_H
#define USB_MANAGER_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Estrutura para informações do dispositivo USB (declaração pública)
typedef struct {
    char device_path[256];
    char mount_point[256];
    char fs_type[32];
    unsigned long size_mb;
    bool is_mounted;
    char vendor[64];
    char model[64];
} usb_device_info_t;

// Códigos de retorno
typedef enum {
    USB_SUCCESS = 0,
    USB_ERROR_INIT = -1,
    USB_ERROR_NOT_FOUND = -2,
    USB_ERROR_MOUNT_FAILED = -3,
    USB_ERROR_COPY_FAILED = -4,
    USB_ERROR_INVALID_PARAM = -5
} usb_result_t;

// Estrutura para callback de progresso
typedef struct {
    void (*on_progress)(int percentage, const char* message);
    void (*on_complete)(usb_result_t result, const char* message);
    void (*on_error)(usb_result_t error, const char* message);
} usb_callbacks_t;

/**
 * @brief Inicializa o gerenciador USB
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int usb_manager_init(void);

/**
 * @brief Finaliza o gerenciador USB e libera recursos
 */
void usb_manager_cleanup(void);

/**
 * @brief Detecta dispositivos USB removíveis conectados
 * @param devices Array para armazenar informações dos dispositivos encontrados
 * @param max_devices Número máximo de dispositivos a serem detectados
 * @return Número de dispositivos encontrados, ou -1 em caso de erro
 */
int detect_usb_devices(usb_device_info_t* devices, int max_devices);

/**
 * @brief Monta um dispositivo USB automaticamente
 * @param device_info Informações do dispositivo a ser montado
 * @return 0 em caso de sucesso, código de erro negativo caso contrário
 */
int mount_usb_device_auto(usb_device_info_t* device_info);

/**
 * @brief Desmonta um dispositivo USB
 * @param mount_point Ponto de montagem do dispositivo
 * @return 0 em caso de sucesso, código de erro negativo caso contrário
 */
int unmount_usb_device(const char* mount_point);

/**
 * @brief Extração automática completa de todos os logs para USB
 * Detecta USB, monta, copia todos os arquivos .txt, desmonta e ejeta
 * @param source_dir Diretório com arquivos de log (ex: "/home/nova")
 * @param callbacks Callbacks para notificação de progresso (pode ser NULL)
 * @return 0 em caso de sucesso, código de erro negativo caso contrário
 */
int usb_auto_extract_all_logs(const char* source_dir, const usb_callbacks_t* callbacks);

/**
 * @brief Monitora continuamente inserção de pen drives para extração automática
 * @param source_dir Diretório com arquivos de log
 * @param running Ponteiro para flag de controle do loop
 * @param callbacks Callbacks para notificação de progresso (pode ser NULL)
 */
void usb_monitor_and_extract(const char* source_dir, volatile bool* running, const usb_callbacks_t* callbacks);

/**
 * @brief Inicializa o buzzer no GPIO23
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int buzzer_init(void);

/**
 * @brief Finaliza o buzzer e libera recursos
 */
void buzzer_cleanup(void);

/**
 * @brief Toca o buzzer para sinalizar sucesso na extração
 * Executa sequência de 3 beeps curtos
 */
void buzzer_signal_extraction_complete(void);

/**
 * @brief Toca o buzzer para sinalizar erro de comunicação Modbus
 * Executa 1 beep longo (500ms)
 */
void buzzer_signal_modbus_error(void);

#ifdef __cplusplus
}
#endif

#endif // USB_MANAGER_H
