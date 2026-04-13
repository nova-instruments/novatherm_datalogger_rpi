/**
 * @file display.h
 * @brief NovaTherm DataLogger - Display Abstraction Layer
 * @author Nova Instruments
 *
 * Camada de abstração para suportar múltiplos tipos de display
 * Compila com ST7567, LCD 20x4 I2C ou OLED SSD1306
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>
#include <stdbool.h>
#include "modbus.h"

#ifdef __cplusplus
extern "C" {
#endif

// Incluir o header apropriado baseado na configuração
#ifdef USE_LVGL_ILI9341
    #include "display_lvgl.h"

    // Aliases para LVGL + ILI9341
    typedef lvgl_context_t display_context_t;
    typedef lvgl_screen_t display_screen_t;

    #define DISPLAY_SCREEN_MAIN_TEMP    LVGL_SCREEN_MAIN_TEMP
    #define DISPLAY_SCREEN_INFO         LVGL_SCREEN_INFO
    #define DISPLAY_SCREEN_DIAGNOSTICS  LVGL_SCREEN_DIAGNOSTICS
    #define DISPLAY_SCREEN_COUNT        LVGL_SCREEN_COUNT

    #define display_init()              lvgl_display_init()
    #define display_cleanup(ctx)        lvgl_display_cleanup(ctx)
    #define display_clear(ctx)          lvgl_display_clear(ctx)
    #define display_splash(ctx, name)   lvgl_display_splash(ctx, name)
    #define display_next_screen(ctx)    lvgl_next_screen(ctx)
    #define display_previous_screen(ctx) lvgl_previous_screen(ctx)
    #define display_increment_setpoint(ctx) lvgl_increment_setpoint(ctx)
    #define display_decrement_setpoint(ctx) lvgl_decrement_setpoint(ctx)
    #define display_get_setpoint(ctx)   lvgl_get_setpoint(ctx)
    #define display_update_current_screen(ctx, name, data, count, lamp, dial, comp, heat, door) \
        lvgl_update_current_screen(ctx, name, data, count, lamp, dial, comp, heat, door)
    #define display_task_handler(ctx)   lvgl_task_handler(ctx)

#elif defined(USE_ST7567)
    #include "lcd_st7567.h"

    // Aliases para ST7567
    typedef st7567_context_t display_context_t;
    typedef st7567_screen_t display_screen_t;

    #define DISPLAY_SCREEN_MAIN_TEMP    ST7567_SCREEN_MAIN_TEMP
    #define DISPLAY_SCREEN_INFO         ST7567_SCREEN_INFO
    #define DISPLAY_SCREEN_DIAGNOSTICS  ST7567_SCREEN_DIAGNOSTICS
    #define DISPLAY_SCREEN_COUNT        ST7567_SCREEN_COUNT

    #define display_init()              st7567_init()
    #define display_cleanup(ctx)        st7567_cleanup(ctx)
    #define display_clear(ctx)          st7567_clear(ctx)
    #define display_splash(ctx, name)   st7567_display_splash(ctx, name)
    #define display_next_screen(ctx)    st7567_next_screen(ctx)
    #define display_previous_screen(ctx) st7567_previous_screen(ctx)
    #define display_increment_setpoint(ctx) st7567_increment_setpoint(ctx)
    #define display_decrement_setpoint(ctx) st7567_decrement_setpoint(ctx)
    #define display_get_setpoint(ctx)   st7567_get_setpoint(ctx)
    #define display_update_current_screen(ctx, name, data, count, lamp, dial, comp, heat, door) \
        st7567_update_current_screen(ctx, name, data, count, lamp, dial, comp, heat, door)

#elif defined(USE_LCD_I2C)
    #include "lcd_i2c.h"

    // Aliases para LCD
    typedef lcd_context_t display_context_t;
    typedef lcd_screen_t display_screen_t;

    #define DISPLAY_SCREEN_MAIN_TEMP    LCD_SCREEN_MAIN_TEMP
    #define DISPLAY_SCREEN_INFO         LCD_SCREEN_INFO
    #define DISPLAY_SCREEN_DIAGNOSTICS  LCD_SCREEN_DIAGNOSTICS
    #define DISPLAY_SCREEN_COUNT        LCD_SCREEN_COUNT

    #define display_init()              lcd_init()
    #define display_cleanup(ctx)        lcd_cleanup(ctx)
    #define display_clear(ctx)          lcd_clear(ctx)
    #define display_splash(ctx, name)   lcd_display_splash(ctx, name)
    #define display_next_screen(ctx)    lcd_next_screen(ctx)
    #define display_previous_screen(ctx) lcd_previous_screen(ctx)
    #define display_increment_setpoint(ctx) lcd_increment_setpoint(ctx)
    #define display_decrement_setpoint(ctx) lcd_decrement_setpoint(ctx)
    #define display_get_setpoint(ctx)   lcd_get_setpoint(ctx)
    #define display_update_current_screen(ctx, name, data, count, lamp, dial, comp, heat, door) \
        lcd_update_current_screen(ctx, name, data, count, lamp, dial, comp, heat, door)

#else
    #include "oled_ssd1306.h"

    // Aliases para OLED
    typedef oled_context_t display_context_t;
    typedef oled_screen_t display_screen_t;

    #define DISPLAY_SCREEN_MAIN_TEMP    SCREEN_TEMPERATURES
    #define DISPLAY_SCREEN_INFO         SCREEN_SETPOINT
    #define DISPLAY_SCREEN_DIAGNOSTICS  SCREEN_DIAGNOSTICS
    #define DISPLAY_SCREEN_COUNT        SCREEN_COUNT

    #define display_init()              oled_init()
    #define display_cleanup(ctx)        oled_cleanup(ctx)
    #define display_clear(ctx)          oled_clear(ctx)
    #define display_splash(ctx, name)   oled_display_splash(ctx, name)
    #define display_next_screen(ctx)    oled_next_screen(ctx)
    #define display_previous_screen(ctx) oled_previous_screen(ctx)
    #define display_increment_setpoint(ctx) oled_increment_setpoint(ctx)
    #define display_decrement_setpoint(ctx) oled_decrement_setpoint(ctx)
    #define display_get_setpoint(ctx)   oled_get_setpoint(ctx)
    #define display_update_current_screen(ctx, name, data, count, lamp, dial, comp, heat, door) \
        oled_update_current_screen(ctx, name, data, count, lamp, dial, comp, heat, door)

#endif

#ifdef __cplusplus
}
#endif

#endif // DISPLAY_H

