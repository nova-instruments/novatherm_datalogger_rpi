/**
 * @file ili9341_spi.c
 * @brief ILI9341 Display Driver Implementation
 */

#include "ili9341_spi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <gpiod.h>

// Funções privadas de baixo nível

static void ili9341_gpio_init(ili9341_context_t* ctx) {
    struct gpiod_chip *chip;
    
    // Abrir GPIO chip
    chip = gpiod_chip_open("/dev/gpiochip0");
    if (!chip) {
        fprintf(stderr, "❌ Erro ao abrir GPIO chip\n");
        return;
    }
    
    // Configurar pino DC como saída
    ctx->dc_line = gpiod_chip_get_line(chip, ILI9341_GPIO_DC);
    if (ctx->dc_line) {
        gpiod_line_request_output(ctx->dc_line, "ili9341_dc", 0);
    }
    
    // Configurar pino RST como saída
    ctx->rst_line = gpiod_chip_get_line(chip, ILI9341_GPIO_RST);
    if (ctx->rst_line) {
        gpiod_line_request_output(ctx->rst_line, "ili9341_rst", 1);
    }
}

static void ili9341_reset(ili9341_context_t* ctx) {
    if (ctx->rst_line) {
        gpiod_line_set_value(ctx->rst_line, 0);
        usleep(10000);  // 10ms
        gpiod_line_set_value(ctx->rst_line, 1);
        usleep(120000); // 120ms
    }
}

static void ili9341_write_command(ili9341_context_t* ctx, uint8_t cmd) {
    // DC = 0 para comando
    if (ctx->dc_line) {
        gpiod_line_set_value(ctx->dc_line, 0);
    }
    
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)&cmd,
        .len = 1,
        .speed_hz = ILI9341_SPI_SPEED,
        .bits_per_word = 8,
    };
    
    ioctl(ctx->spi_fd, SPI_IOC_MESSAGE(1), &tr);
}

static void ili9341_write_data(ili9341_context_t* ctx, const uint8_t* data, size_t len) {
    // DC = 1 para dados
    if (ctx->dc_line) {
        gpiod_line_set_value(ctx->dc_line, 1);
    }
    
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)data,
        .len = len,
        .speed_hz = ILI9341_SPI_SPEED,
        .bits_per_word = 8,
    };
    
    ioctl(ctx->spi_fd, SPI_IOC_MESSAGE(1), &tr);
}

static void ili9341_write_data_byte(ili9341_context_t* ctx, uint8_t data) {
    ili9341_write_data(ctx, &data, 1);
}

// Funções públicas

ili9341_context_t* ili9341_init(void) {
    ili9341_context_t* ctx = (ili9341_context_t*)malloc(sizeof(ili9341_context_t));
    if (!ctx) {
        fprintf(stderr, "❌ Erro ao alocar memória para ILI9341\n");
        return NULL;
    }
    
    memset(ctx, 0, sizeof(ili9341_context_t));
    
    // Abrir dispositivo SPI
    ctx->spi_fd = open(ILI9341_SPI_DEVICE, O_RDWR);
    if (ctx->spi_fd < 0) {
        fprintf(stderr, "❌ Erro ao abrir %s\n", ILI9341_SPI_DEVICE);
        free(ctx);
        return NULL;
    }
    
    // Configurar SPI mode e velocidade
    uint8_t mode = SPI_MODE_0;
    uint8_t bits = 8;
    uint32_t speed = ILI9341_SPI_SPEED;
    
    ioctl(ctx->spi_fd, SPI_IOC_WR_MODE, &mode);
    ioctl(ctx->spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
    ioctl(ctx->spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
    
    // Inicializar GPIO
    ili9341_gpio_init(ctx);
    
    // Reset do display
    ili9341_reset(ctx);
    
    // Sequência de inicialização ILI9341
    ili9341_write_command(ctx, 0xEF);
    ili9341_write_data_byte(ctx, 0x03);
    ili9341_write_data_byte(ctx, 0x80);
    ili9341_write_data_byte(ctx, 0x02);
    
    ili9341_write_command(ctx, 0xCF);
    ili9341_write_data_byte(ctx, 0x00);
    ili9341_write_data_byte(ctx, 0xC1);
    ili9341_write_data_byte(ctx, 0x30);
    
    ili9341_write_command(ctx, 0xED);
    ili9341_write_data_byte(ctx, 0x64);
    ili9341_write_data_byte(ctx, 0x03);
    ili9341_write_data_byte(ctx, 0x12);
    ili9341_write_data_byte(ctx, 0x81);
    
    ili9341_write_command(ctx, 0xE8);
    ili9341_write_data_byte(ctx, 0x85);
    ili9341_write_data_byte(ctx, 0x00);
    ili9341_write_data_byte(ctx, 0x78);
    
    // Continuar inicialização
    ili9341_write_command(ctx, 0xCB);
    ili9341_write_data_byte(ctx, 0x39);
    ili9341_write_data_byte(ctx, 0x2C);
    ili9341_write_data_byte(ctx, 0x00);
    ili9341_write_data_byte(ctx, 0x34);
    ili9341_write_data_byte(ctx, 0x02);

    ili9341_write_command(ctx, 0xF7);
    ili9341_write_data_byte(ctx, 0x20);

    ili9341_write_command(ctx, 0xEA);
    ili9341_write_data_byte(ctx, 0x00);
    ili9341_write_data_byte(ctx, 0x00);

    ili9341_write_command(ctx, ILI9341_PWCTR1);    // Power control
    ili9341_write_data_byte(ctx, 0x23);             // VRH[5:0]

    ili9341_write_command(ctx, ILI9341_PWCTR2);    // Power control
    ili9341_write_data_byte(ctx, 0x10);             // SAP[2:0];BT[3:0]

    ili9341_write_command(ctx, ILI9341_VMCTR1);    // VCM control
    ili9341_write_data_byte(ctx, 0x3e);
    ili9341_write_data_byte(ctx, 0x28);

    ili9341_write_command(ctx, ILI9341_VMCTR2);    // VCM control2
    ili9341_write_data_byte(ctx, 0x86);

    ili9341_write_command(ctx, ILI9341_MADCTL);    // Memory Access Control
    ili9341_write_data_byte(ctx, 0x48);

    ili9341_write_command(ctx, ILI9341_PIXFMT);
    ili9341_write_data_byte(ctx, 0x55);

    ili9341_write_command(ctx, ILI9341_FRMCTR1);
    ili9341_write_data_byte(ctx, 0x00);
    ili9341_write_data_byte(ctx, 0x18);

    ili9341_write_command(ctx, ILI9341_DFUNCTR);    // Display Function Control
    ili9341_write_data_byte(ctx, 0x08);
    ili9341_write_data_byte(ctx, 0x82);
    ili9341_write_data_byte(ctx, 0x27);

    ili9341_write_command(ctx, 0xF2);              // 3Gamma Function Disable
    ili9341_write_data_byte(ctx, 0x00);

    ili9341_write_command(ctx, ILI9341_GAMMASET);   // Gamma curve selected
    ili9341_write_data_byte(ctx, 0x01);

    ili9341_write_command(ctx, ILI9341_GMCTRP1);    // Set Gamma
    uint8_t gamma_p[] = {0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1,
                         0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00};
    ili9341_write_data(ctx, gamma_p, 15);

    ili9341_write_command(ctx, ILI9341_GMCTRN1);    // Set Gamma
    uint8_t gamma_n[] = {0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1,
                         0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F};
    ili9341_write_data(ctx, gamma_n, 15);

    ili9341_write_command(ctx, ILI9341_SLPOUT);    // Exit Sleep
    usleep(120000);

    ili9341_write_command(ctx, ILI9341_DISPON);    // Display on
    usleep(120000);

    ctx->width = ILI9341_WIDTH;
    ctx->height = ILI9341_HEIGHT;
    ctx->rotation = ILI9341_ROTATION;
    ctx->initialized = true;

    printf("✅ ILI9341 240x320 inicializado (SPI: DC=GPIO%d, RST=GPIO%d)\n",
           ILI9341_GPIO_DC, ILI9341_GPIO_RST);

    return ctx;
}

void ili9341_cleanup(ili9341_context_t* ctx) {
    if (ctx) {
        if (ctx->spi_fd >= 0) {
            close(ctx->spi_fd);
        }
        if (ctx->dc_line) {
            gpiod_line_release(ctx->dc_line);
        }
        if (ctx->rst_line) {
            gpiod_line_release(ctx->rst_line);
        }
        free(ctx);
    }
}

void ili9341_set_rotation(ili9341_context_t* ctx, uint8_t rotation) {
    if (!ctx || !ctx->initialized) return;

    ctx->rotation = rotation % 4;

    uint8_t madctl = 0;
    switch (ctx->rotation) {
        case 0:  // Portrait
            madctl = 0x48;
            ctx->width = ILI9341_WIDTH;
            ctx->height = ILI9341_HEIGHT;
            break;
        case 1:  // Landscape 90°
            madctl = 0x28;
            ctx->width = ILI9341_HEIGHT;
            ctx->height = ILI9341_WIDTH;
            break;
        case 2:  // Portrait 180°
            madctl = 0x88;
            ctx->width = ILI9341_WIDTH;
            ctx->height = ILI9341_HEIGHT;
            break;
        case 3:  // Landscape 270°
            madctl = 0xE8;
            ctx->width = ILI9341_HEIGHT;
            ctx->height = ILI9341_WIDTH;
            break;
    }

    ili9341_write_command(ctx, ILI9341_MADCTL);
    ili9341_write_data_byte(ctx, madctl);
}

void ili9341_set_window(ili9341_context_t* ctx, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    if (!ctx || !ctx->initialized) return;

    // Column address set
    ili9341_write_command(ctx, ILI9341_CASET);
    uint8_t col[] = {x0 >> 8, x0 & 0xFF, x1 >> 8, x1 & 0xFF};
    ili9341_write_data(ctx, col, 4);

    // Page address set
    ili9341_write_command(ctx, ILI9341_PASET);
    uint8_t page[] = {y0 >> 8, y0 & 0xFF, y1 >> 8, y1 & 0xFF};
    ili9341_write_data(ctx, page, 4);

    // Write to RAM
    ili9341_write_command(ctx, ILI9341_RAMWR);
}

void ili9341_write_pixels(ili9341_context_t* ctx, const uint16_t* data, uint32_t len) {
    if (!ctx || !ctx->initialized || !data) return;

    // Converter RGB565 para big-endian se necessário e enviar
    ili9341_write_data(ctx, (const uint8_t*)data, len * 2);
}

void ili9341_fill_screen(ili9341_context_t* ctx, uint16_t color) {
    if (!ctx || !ctx->initialized) return;

    ili9341_set_window(ctx, 0, 0, ctx->width - 1, ctx->height - 1);

    uint32_t total_pixels = ctx->width * ctx->height;
    uint16_t buffer[64];

    // Preencher buffer com a cor
    for (int i = 0; i < 64; i++) {
        buffer[i] = color;
    }

    // Enviar em blocos
    for (uint32_t i = 0; i < total_pixels; i += 64) {
        uint32_t count = (i + 64 > total_pixels) ? (total_pixels - i) : 64;
        ili9341_write_pixels(ctx, buffer, count);
    }
}
