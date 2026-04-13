/**
 * @file ili9341_spi.h
 * @brief ILI9341 240x320 TFT Display Driver for Raspberry Pi via SPI
 * @author Nova Instruments
 * 
 * Driver for ILI9341 2.4" TFT display using SPI interface
 * GPIO Configuration:
 *   - DC (Data/Command): GPIO 22
 *   - RST (Reset): GPIO 27
 *   - SPI: Default SPI0 (MOSI=GPIO10, MISO=GPIO9, SCLK=GPIO11, CE0=GPIO8)
 */

#ifndef ILI9341_SPI_H
#define ILI9341_SPI_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Configurações do display
#define ILI9341_WIDTH       240
#define ILI9341_HEIGHT      320
#define ILI9341_ROTATION    0       // 0, 1, 2, 3 (0=portrait, 1=landscape, etc)

// Configuração GPIO
#define ILI9341_GPIO_DC     22      // Data/Command pin
#define ILI9341_GPIO_RST    27      // Reset pin
#define ILI9341_SPI_DEVICE  "/dev/spidev0.0"
#define ILI9341_SPI_SPEED   16000000  // 16 MHz

// Comandos ILI9341
#define ILI9341_NOP         0x00
#define ILI9341_SWRESET     0x01
#define ILI9341_RDDID       0x04
#define ILI9341_RDDST       0x09

#define ILI9341_SLPIN       0x10
#define ILI9341_SLPOUT      0x11
#define ILI9341_PTLON       0x12
#define ILI9341_NORON       0x13

#define ILI9341_RDMODE      0x0A
#define ILI9341_RDMADCTL    0x0B
#define ILI9341_RDPIXFMT    0x0C
#define ILI9341_RDIMGFMT    0x0D
#define ILI9341_RDSELFDIAG  0x0F

#define ILI9341_INVOFF      0x20
#define ILI9341_INVON       0x21
#define ILI9341_GAMMASET    0x26
#define ILI9341_DISPOFF     0x28
#define ILI9341_DISPON      0x29

#define ILI9341_CASET       0x2A
#define ILI9341_PASET       0x2B
#define ILI9341_RAMWR       0x2C
#define ILI9341_RAMRD       0x2E

#define ILI9341_PTLAR       0x30
#define ILI9341_MADCTL      0x36
#define ILI9341_PIXFMT      0x3A

#define ILI9341_FRMCTR1     0xB1
#define ILI9341_FRMCTR2     0xB2
#define ILI9341_FRMCTR3     0xB3
#define ILI9341_INVCTR      0xB4
#define ILI9341_DFUNCTR     0xB6

#define ILI9341_PWCTR1      0xC0
#define ILI9341_PWCTR2      0xC1
#define ILI9341_PWCTR3      0xC2
#define ILI9341_PWCTR4      0xC3
#define ILI9341_PWCTR5      0xC4
#define ILI9341_VMCTR1      0xC5
#define ILI9341_VMCTR2      0xC7

#define ILI9341_RDID1       0xDA
#define ILI9341_RDID2       0xDB
#define ILI9341_RDID3       0xDC
#define ILI9341_RDID4       0xDD

#define ILI9341_GMCTRP1     0xE0
#define ILI9341_GMCTRN1     0xE1

// Forward declaration para gpiod
struct gpiod_line;

// Contexto do driver
typedef struct {
    int spi_fd;                  // File descriptor do SPI
    struct gpiod_line *dc_line;  // Linha GPIO para DC
    struct gpiod_line *rst_line; // Linha GPIO para RST
    uint16_t width;              // Largura do display
    uint16_t height;             // Altura do display
    uint8_t rotation;            // Rotação (0-3)
    bool initialized;            // Flag de inicialização
} ili9341_context_t;

/**
 * @brief Inicializa o display ILI9341
 * @return Ponteiro para o contexto ou NULL em caso de erro
 */
ili9341_context_t* ili9341_init(void);

/**
 * @brief Finaliza e libera recursos
 * @param ctx Contexto do display
 */
void ili9341_cleanup(ili9341_context_t* ctx);

/**
 * @brief Define a orientação do display
 * @param ctx Contexto do display
 * @param rotation Rotação (0=portrait, 1=landscape 90°, 2=portrait invertido, 3=landscape 270°)
 */
void ili9341_set_rotation(ili9341_context_t* ctx, uint8_t rotation);

/**
 * @brief Limpa a tela com uma cor
 * @param ctx Contexto do display
 * @param color Cor RGB565
 */
void ili9341_fill_screen(ili9341_context_t* ctx, uint16_t color);

/**
 * @brief Define a janela de desenho
 * @param ctx Contexto do display
 * @param x0 Coordenada X inicial
 * @param y0 Coordenada Y inicial
 * @param x1 Coordenada X final
 * @param y1 Coordenada Y final
 */
void ili9341_set_window(ili9341_context_t* ctx, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

/**
 * @brief Envia dados de pixels para o display
 * @param ctx Contexto do display
 * @param data Buffer de dados (RGB565)
 * @param len Número de pixels
 */
void ili9341_write_pixels(ili9341_context_t* ctx, const uint16_t* data, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif // ILI9341_SPI_H
