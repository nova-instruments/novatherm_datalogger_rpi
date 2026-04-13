/**
 * @file display_lvgl.c
 * @brief LVGL Display Driver Implementation
 * @author Nova Instruments
 */

#include "display_lvgl.h"
#include "ili9341_spi.h"
#include "lvgl/lvgl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Buffer para LVGL (1/10 da tela)
#define DISP_BUF_SIZE (ILI9341_WIDTH * ILI9341_HEIGHT / 10)

// Variáveis globais LVGL
static lv_display_t *lv_disp = NULL;
static lv_color_t *buf1 = NULL;
static lv_color_t *buf2 = NULL;

// Objetos da UI
static lv_obj_t *screen_main = NULL;
static lv_obj_t *screen_info = NULL;
static lv_obj_t *screen_diagnostics = NULL;
static lv_obj_t *label_temp_ch1 = NULL;
static lv_obj_t *label_setpoint = NULL;
static lv_obj_t *label_door = NULL;

// Contexto global
static lvgl_context_t *g_ctx = NULL;

/**
 * @brief Callback de flush para o display (envia dados para ILI9341)
 */
static void lvgl_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    if (!g_ctx || !g_ctx->ili9341_ctx) {
        lv_display_flush_ready(disp);
        return;
    }
    
    ili9341_context_t *ili = (ili9341_context_t*)g_ctx->ili9341_ctx;
    
    // Configurar janela de desenho
    ili9341_set_window(ili, area->x1, area->y1, area->x2, area->y2);
    
    // Calcular número de pixels
    uint32_t width = area->x2 - area->x1 + 1;
    uint32_t height = area->y2 - area->y1 + 1;
    uint32_t size = width * height;
    
    // Enviar dados
    ili9341_write_pixels(ili, (uint16_t*)px_map, size);
    
    // Informar LVGL que o flush terminou
    lv_display_flush_ready(disp);
}

/**
 * @brief Cria as telas da aplicação
 */
static void lvgl_create_screens(lvgl_context_t *ctx) {
    // Tela 0: Temperatura Principal
    screen_main = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_main, lv_color_hex(0x000000), 0);
    
    // Título
    lv_obj_t *title = lv_label_create(screen_main);
    lv_label_set_text(title, "NovaTherm Logger");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    
    // Temperatura CH1 (grande)
    label_temp_ch1 = lv_label_create(screen_main);
    lv_label_set_text(label_temp_ch1, "CH1: --.- C");
    lv_obj_set_style_text_color(label_temp_ch1, lv_color_hex(0x00FF00), 0);
    lv_obj_align(label_temp_ch1, LV_ALIGN_CENTER, 0, -20);
    
    // Setpoint
    label_setpoint = lv_label_create(screen_main);
    lv_label_set_text(label_setpoint, "SP: 5.0 C");
    lv_obj_set_style_text_color(label_setpoint, lv_color_hex(0xFFFF00), 0);
    lv_obj_align(label_setpoint, LV_ALIGN_CENTER, 0, 40);
    
    // Status da porta
    label_door = lv_label_create(screen_main);
    lv_label_set_text(label_door, "Porta: FECHADA");
    lv_obj_set_style_text_color(label_door, lv_color_hex(0x00FFFF), 0);
    lv_obj_align(label_door, LV_ALIGN_BOTTOM_MID, 0, -20);
    
    // Tela 1: Informações (todos os canais)
    screen_info = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_info, lv_color_hex(0x000000), 0);
    
    lv_obj_t *title_info = lv_label_create(screen_info);
    lv_label_set_text(title_info, "Canais de Temperatura");
    lv_obj_set_style_text_color(title_info, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title_info, LV_ALIGN_TOP_MID, 0, 10);
    
    // Tela 2: Diagnósticos
    screen_diagnostics = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_diagnostics, lv_color_hex(0x000000), 0);
    
    lv_obj_t *title_diag = lv_label_create(screen_diagnostics);
    lv_label_set_text(title_diag, "Status dos Reles");
    lv_obj_set_style_text_color(title_diag, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title_diag, LV_ALIGN_TOP_MID, 0, 10);
    
    // Carregar tela inicial
    lv_scr_load(screen_main);
}

lvgl_context_t* lvgl_display_init(void) {
    lvgl_context_t* ctx = (lvgl_context_t*)malloc(sizeof(lvgl_context_t));
    if (!ctx) {
        fprintf(stderr, "❌ Erro ao alocar memória para LVGL context\n");
        return NULL;
    }
    
    memset(ctx, 0, sizeof(lvgl_context_t));
    g_ctx = ctx;
    
    // Inicializar LVGL
    lv_init();
    
    // Inicializar driver ILI9341
    ctx->ili9341_ctx = ili9341_init();
    if (!ctx->ili9341_ctx) {
        fprintf(stderr, "❌ Erro ao inicializar ILI9341\n");
        free(ctx);
        g_ctx = NULL;
        return NULL;
    }
    
    // Alocar buffers de display
    buf1 = (lv_color_t*)malloc(DISP_BUF_SIZE * sizeof(lv_color_t));
    buf2 = (lv_color_t*)malloc(DISP_BUF_SIZE * sizeof(lv_color_t));
    
    if (!buf1 || !buf2) {
        fprintf(stderr, "❌ Erro ao alocar buffers LVGL\n");
        ili9341_cleanup(ctx->ili9341_ctx);
        free(ctx);
        g_ctx = NULL;
        return NULL;
    }
    
    // Criar display LVGL
    lv_disp = lv_display_create(ILI9341_WIDTH, ILI9341_HEIGHT);
    lv_display_set_buffers(lv_disp, buf1, buf2, DISP_BUF_SIZE * sizeof(lv_color_t), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(lv_disp, lvgl_disp_flush);
    
    ctx->lv_disp = lv_disp;
    ctx->current_screen = LVGL_SCREEN_MAIN_TEMP;
    ctx->setpoint = 5.0f;
    ctx->temp_max = -999.0f;
    ctx->temp_min = 999.0f;
    ctx->initialized = true;
    
    // Criar telas da aplicação
    lvgl_create_screens(ctx);
    
    printf("✅ LVGL v9.x inicializado com ILI9341 240x320\n");

    return ctx;
}

void lvgl_display_cleanup(lvgl_context_t* ctx) {
    if (ctx) {
        if (ctx->ili9341_ctx) {
            ili9341_cleanup(ctx->ili9341_ctx);
        }
        if (buf1) {
            free(buf1);
            buf1 = NULL;
        }
        if (buf2) {
            free(buf2);
            buf2 = NULL;
        }
        lv_deinit();
        free(ctx);
        g_ctx = NULL;
    }
}

void lvgl_display_clear(lvgl_context_t* ctx) {
    if (!ctx || !ctx->initialized) return;

    ili9341_context_t *ili = (ili9341_context_t*)ctx->ili9341_ctx;
    ili9341_fill_screen(ili, 0x0000);  // Preto
}

void lvgl_display_splash(lvgl_context_t* ctx, const char* device_name) {
    if (!ctx || !ctx->initialized) return;

    // Criar tela de splash
    lv_obj_t *splash = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(splash, lv_color_hex(0x001F3F), 0);

    lv_obj_t *title = lv_label_create(splash);
    lv_label_set_text(title, "NovaTherm Logger");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, -60);

    lv_obj_t *subtitle = lv_label_create(splash);
    lv_label_set_text(subtitle, "ILI9341 240x320");
    lv_obj_set_style_text_color(subtitle, lv_color_hex(0xAAAAAA), 0);
    lv_obj_align(subtitle, LV_ALIGN_CENTER, 0, -20);

    if (device_name) {
        lv_obj_t *device = lv_label_create(splash);
        lv_label_set_text(device, device_name);
        lv_obj_set_style_text_color(device, lv_color_hex(0x00FF00), 0);
        lv_obj_align(device, LV_ALIGN_CENTER, 0, 20);
    }

    lv_obj_t *footer = lv_label_create(splash);
    lv_label_set_text(footer, "Nova Instruments");
    lv_obj_set_style_text_color(footer, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, -20);

    lv_scr_load(splash);
    lv_task_handler();
    usleep(2000000);  // 2 segundos
}

void lvgl_next_screen(lvgl_context_t* ctx) {
    if (!ctx) return;

    ctx->current_screen = (lvgl_screen_t)((ctx->current_screen + 1) % LVGL_SCREEN_COUNT);

    switch (ctx->current_screen) {
        case LVGL_SCREEN_MAIN_TEMP:
            lv_scr_load(screen_main);
            break;
        case LVGL_SCREEN_INFO:
            lv_scr_load(screen_info);
            break;
        case LVGL_SCREEN_DIAGNOSTICS:
            lv_scr_load(screen_diagnostics);
            break;
        default:
            break;
    }
}

void lvgl_previous_screen(lvgl_context_t* ctx) {
    if (!ctx) return;

    if (ctx->current_screen == 0) {
        ctx->current_screen = (lvgl_screen_t)(LVGL_SCREEN_COUNT - 1);
    } else {
        ctx->current_screen = (lvgl_screen_t)(ctx->current_screen - 1);
    }

    switch (ctx->current_screen) {
        case LVGL_SCREEN_MAIN_TEMP:
            lv_scr_load(screen_main);
            break;
        case LVGL_SCREEN_INFO:
            lv_scr_load(screen_info);
            break;
        case LVGL_SCREEN_DIAGNOSTICS:
            lv_scr_load(screen_diagnostics);
            break;
        default:
            break;
    }
}

void lvgl_increment_setpoint(lvgl_context_t* ctx) {
    if (!ctx) return;

    ctx->setpoint += 0.5f;
    if (ctx->setpoint > 30.0f) {
        ctx->setpoint = 30.0f;
    }
}

void lvgl_decrement_setpoint(lvgl_context_t* ctx) {
    if (!ctx) return;

    ctx->setpoint -= 0.5f;
    if (ctx->setpoint < -20.0f) {
        ctx->setpoint = -20.0f;
    }
}

float lvgl_get_setpoint(lvgl_context_t* ctx) {
    if (!ctx) return 0.0f;
    return ctx->setpoint;
}

void lvgl_update_current_screen(lvgl_context_t* ctx, const char* device_name,
                                const modbus_data_t* data, uint32_t record_count,
                                bool lamp_on, bool dialer_on, bool compressor_on, bool heater_on,
                                bool door_open) {
    if (!ctx || !ctx->initialized || !data) return;

    char buffer[64];

    // Atualizar tela principal
    if (label_temp_ch1) {
        snprintf(buffer, sizeof(buffer), "CH1: %.1f C", data->ch_temp[0]);
        lv_label_set_text(label_temp_ch1, buffer);
    }

    if (label_setpoint) {
        snprintf(buffer, sizeof(buffer), "SP: %.1f C", ctx->setpoint);
        lv_label_set_text(label_setpoint, buffer);
    }

    if (label_door) {
        snprintf(buffer, sizeof(buffer), "Porta: %s", door_open ? "ABERTA" : "FECHADA");
        lv_label_set_text(label_door, buffer);
        lv_obj_set_style_text_color(label_door,
            door_open ? lv_color_hex(0xFF0000) : lv_color_hex(0x00FF00), 0);
    }
}

void lvgl_task_handler(lvgl_context_t* ctx) {
    if (!ctx || !ctx->initialized) return;

    lv_task_handler();
}
