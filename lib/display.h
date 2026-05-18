/**
 * @file display.h
 * @brief NovaTherm DataLogger - Display Abstraction Layer
 * @author Nova Instruments
 *
 * Camada de abstração para suportar múltiplos tipos de display
 * Compila com ST7789+LVGL, LCD 20x4 I2C ou OLED SSD1306
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>
#include <stdbool.h>
#include "modbus.h"
#include "controller.h"

#ifdef __cplusplus
extern "C" {
#endif

// Incluir o header apropriado baseado na configuração
#ifdef USE_ST7789_EC11
    #include "display_st7789_lvgl.h"

    // Aliases para ST7789 + LVGL
    typedef st7789_context_t display_context_t;
    typedef st7789_screen_t display_screen_t;

    #define DISPLAY_SCREEN_MAIN_TEMP    ST7789_SCREEN_MAIN_TEMP
    #define DISPLAY_SCREEN_INFO         ST7789_SCREEN_INFO
    #define DISPLAY_SCREEN_DIAGNOSTICS  ST7789_SCREEN_DIAGNOSTICS
    #define DISPLAY_SCREEN_COUNT        ST7789_SCREEN_COUNT

    #define display_init()              st7789_init()
    #define display_cleanup(ctx)        st7789_cleanup(ctx)
    #define display_clear(ctx)          st7789_clear(ctx)
    #define display_splash(ctx, name)   st7789_display_splash(ctx, name)
    #define display_next_screen(ctx)    st7789_next_screen(ctx)
    #define display_previous_screen(ctx) st7789_previous_screen(ctx)
    #define display_increment_setpoint(ctx) st7789_increment_setpoint(ctx)
    #define display_decrement_setpoint(ctx) st7789_decrement_setpoint(ctx)
    #define display_get_setpoint(ctx)   st7789_get_setpoint(ctx)
    #define display_increment_hysteresis(ctx) st7789_increment_hysteresis(ctx)
    #define display_decrement_hysteresis(ctx) st7789_decrement_hysteresis(ctx)
    #define display_get_hysteresis(ctx) st7789_get_hysteresis(ctx)
    #define display_is_hysteresis_password_active(ctx) st7789_is_hysteresis_password_active(ctx)
    #define display_is_hysteresis_password_result_visible(ctx) st7789_is_hysteresis_password_result_visible(ctx)
    #define display_consume_hysteresis_password_result(ctx) st7789_consume_hysteresis_password_result(ctx)
    #define display_start_hysteresis_password(ctx) st7789_start_hysteresis_password(ctx)
    #define display_increment_hysteresis_password_digit(ctx) st7789_increment_hysteresis_password_digit(ctx)
    #define display_decrement_hysteresis_password_digit(ctx) st7789_decrement_hysteresis_password_digit(ctx)
    #define display_confirm_hysteresis_password_digit(ctx) st7789_confirm_hysteresis_password_digit(ctx)
    #define display_is_nav_pressed(ctx) st7789_is_encoder_button_pressed(ctx)
    #define display_is_info_nav_active(ctx) st7789_is_info_nav_active(ctx)
    #define display_set_info_nav_active(ctx, active) st7789_set_info_nav_active(ctx, active)
    #define display_move_info_selection(ctx, diff) st7789_move_info_selection(ctx, diff)
    #define display_get_info_selection(ctx) st7789_get_info_selection(ctx)
    #define display_set_logs_reset_dialog(ctx, visible, message) st7789_set_logs_reset_dialog(ctx, visible, message)
    #define display_is_logs_reset_dialog_visible(ctx) st7789_is_logs_reset_dialog_visible(ctx)
    #define display_is_main_nav_active(ctx) st7789_is_main_nav_active(ctx)
    #define display_set_main_nav_active(ctx, active) st7789_set_main_nav_active(ctx, active)
    #define display_move_main_selection(ctx, diff) st7789_move_main_selection(ctx, diff)
    #define display_get_main_selection(ctx) st7789_get_main_selection(ctx)
    #define display_is_diag_nav_active(ctx) st7789_is_diag_nav_active(ctx)
    #define display_set_diag_nav_active(ctx, active) st7789_set_diag_nav_active(ctx, active)
    #define display_move_diag_selection(ctx, diff) st7789_move_diag_selection(ctx, diff)
    #define display_get_diag_selection(ctx) st7789_get_diag_selection(ctx)
    #define display_set_diag_compressor_avg_dialog(ctx, visible) st7789_set_diag_compressor_avg_dialog(ctx, visible)
    #define display_is_diag_compressor_avg_dialog_visible(ctx) st7789_is_diag_compressor_avg_dialog_visible(ctx)
    #define display_set_main_minmax_reset_dialog(ctx, visible, message) st7789_set_main_minmax_reset_dialog(ctx, visible, message)
    #define display_is_main_minmax_reset_dialog_visible(ctx) st7789_is_main_minmax_reset_dialog_visible(ctx)
    #define display_reset_main_minmax_from_current_temp(ctx) st7789_reset_main_minmax_from_current_temp(ctx)
    #define display_increment_main_selected_value(ctx) st7789_increment_main_selected_value(ctx)
    #define display_decrement_main_selected_value(ctx) st7789_decrement_main_selected_value(ctx)
    #define display_get_offset_principal(ctx) st7789_get_offset_principal(ctx)
    #define display_get_offset_degelo(ctx) st7789_get_offset_degelo(ctx)
    #define display_start_offset_editor(ctx, target) st7789_start_offset_editor(ctx, target)
    #define display_is_offset_editor_active(ctx) st7789_is_offset_editor_active(ctx)
    #define display_increment_offset_editor_digit(ctx) st7789_increment_offset_editor_digit(ctx)
    #define display_decrement_offset_editor_digit(ctx) st7789_decrement_offset_editor_digit(ctx)
    #define display_confirm_offset_editor_digit(ctx) st7789_confirm_offset_editor_digit(ctx)
    #define display_update_current_screen(ctx, name, data, count, lamp, dial, comp, heat, door, s2t1v, s2t1, s2t2v, s2t2) \
        st7789_update_current_screen(ctx, name, data, count, lamp, dial, comp, heat, door, s2t1v, s2t1, s2t2v, s2t2)
    #define display_set_usb_extraction_screen(ctx, visible, progress, message, is_error, is_done) \
        st7789_set_usb_extraction_screen(ctx, visible, progress, message, is_error, is_done)
    #define display_process(ctx)        st7789_process(ctx)
    #define display_consume_nav_diff(ctx) st7789_consume_encoder_diff(ctx)
    #define display_consume_nav_click(ctx) st7789_consume_encoder_click(ctx)
    #define display_consume_exit_edit_click(ctx) st7789_consume_exit_button_click(ctx)
    #define display_is_exit_button_pressed(ctx) st7789_is_exit_button_pressed(ctx)
    #define display_is_setpoint_edit_active(ctx) st7789_is_setpoint_edit_active(ctx)
    #define display_set_setpoint_edit_active(ctx, active) st7789_set_setpoint_edit_active(ctx, active)
    #define display_get_current_screen(ctx) st7789_get_current_screen(ctx)
    #define display_cancel_edit_modes(ctx) st7789_cancel_edit_modes(ctx)
    #define display_set_top_status(ctx, text) st7789_set_top_status(ctx, text)

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
    #define display_increment_hysteresis(ctx) ((void)0)
    #define display_decrement_hysteresis(ctx) ((void)0)
    #define display_get_hysteresis(ctx) CONTROLLER_HYSTERESIS
    #define display_is_hysteresis_password_active(ctx) false
    #define display_is_hysteresis_password_result_visible(ctx) false
    #define display_consume_hysteresis_password_result(ctx) false
    #define display_start_hysteresis_password(ctx) ((void)0)
    #define display_increment_hysteresis_password_digit(ctx) ((void)0)
    #define display_decrement_hysteresis_password_digit(ctx) ((void)0)
    #define display_confirm_hysteresis_password_digit(ctx) false
    #define display_is_nav_pressed(ctx) false
    #define display_is_info_nav_active(ctx) false
    #define display_set_info_nav_active(ctx, active) ((void)0)
    #define display_move_info_selection(ctx, diff) ((void)0)
    #define display_get_info_selection(ctx) 0
    #define display_set_logs_reset_dialog(ctx, visible, message) ((void)0)
    #define display_is_logs_reset_dialog_visible(ctx) false
    #define display_is_main_nav_active(ctx) false
    #define display_set_main_nav_active(ctx, active) ((void)0)
    #define display_move_main_selection(ctx, diff) ((void)0)
    #define display_get_main_selection(ctx) 0
    #define display_is_diag_nav_active(ctx) false
    #define display_set_diag_nav_active(ctx, active) ((void)0)
    #define display_move_diag_selection(ctx, diff) ((void)0)
    #define display_get_diag_selection(ctx) 0
    #define display_set_diag_compressor_avg_dialog(ctx, visible) ((void)0)
    #define display_is_diag_compressor_avg_dialog_visible(ctx) false
    #define display_set_main_minmax_reset_dialog(ctx, visible, message) ((void)0)
    #define display_is_main_minmax_reset_dialog_visible(ctx) false
    #define display_reset_main_minmax_from_current_temp(ctx) false
    #define display_increment_main_selected_value(ctx) ((void)0)
    #define display_decrement_main_selected_value(ctx) ((void)0)
    #define display_get_offset_principal(ctx) 0.0f
    #define display_get_offset_degelo(ctx) 0.0f
    #define display_start_offset_editor(ctx, target) ((void)0)
    #define display_is_offset_editor_active(ctx) false
    #define display_increment_offset_editor_digit(ctx) ((void)0)
    #define display_decrement_offset_editor_digit(ctx) ((void)0)
    #define display_confirm_offset_editor_digit(ctx) false
    #define display_update_current_screen(ctx, name, data, count, lamp, dial, comp, heat, door, s2t1v, s2t1, s2t2v, s2t2) \
        lcd_update_current_screen(ctx, name, data, count, lamp, dial, comp, heat, door, s2t1v, s2t1, s2t2v, s2t2)
    #define display_set_usb_extraction_screen(ctx, visible, progress, message, is_error, is_done) ((void)0)
    #define display_process(ctx)        ((void)0)
    #define display_consume_nav_diff(ctx) 0
    #define display_consume_nav_click(ctx) false
    #define display_consume_exit_edit_click(ctx) false
    #define display_is_exit_button_pressed(ctx) false
    #define display_is_setpoint_edit_active(ctx) false
    #define display_set_setpoint_edit_active(ctx, active) ((void)0)
    #define display_get_current_screen(ctx) DISPLAY_SCREEN_MAIN_TEMP
    #define display_cancel_edit_modes(ctx) ((void)0)
    #define display_set_top_status(ctx, text) ((void)0)

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
    #define display_increment_hysteresis(ctx) ((void)0)
    #define display_decrement_hysteresis(ctx) ((void)0)
    #define display_get_hysteresis(ctx) CONTROLLER_HYSTERESIS
    #define display_is_hysteresis_password_active(ctx) false
    #define display_is_hysteresis_password_result_visible(ctx) false
    #define display_consume_hysteresis_password_result(ctx) false
    #define display_start_hysteresis_password(ctx) ((void)0)
    #define display_increment_hysteresis_password_digit(ctx) ((void)0)
    #define display_decrement_hysteresis_password_digit(ctx) ((void)0)
    #define display_confirm_hysteresis_password_digit(ctx) false
    #define display_is_nav_pressed(ctx) false
    #define display_is_info_nav_active(ctx) false
    #define display_set_info_nav_active(ctx, active) ((void)0)
    #define display_move_info_selection(ctx, diff) ((void)0)
    #define display_get_info_selection(ctx) 0
    #define display_set_logs_reset_dialog(ctx, visible, message) ((void)0)
    #define display_is_logs_reset_dialog_visible(ctx) false
    #define display_is_main_nav_active(ctx) false
    #define display_set_main_nav_active(ctx, active) ((void)0)
    #define display_move_main_selection(ctx, diff) ((void)0)
    #define display_get_main_selection(ctx) 0
    #define display_is_diag_nav_active(ctx) false
    #define display_set_diag_nav_active(ctx, active) ((void)0)
    #define display_move_diag_selection(ctx, diff) ((void)0)
    #define display_get_diag_selection(ctx) 0
    #define display_set_diag_compressor_avg_dialog(ctx, visible) ((void)0)
    #define display_is_diag_compressor_avg_dialog_visible(ctx) false
    #define display_set_main_minmax_reset_dialog(ctx, visible, message) ((void)0)
    #define display_is_main_minmax_reset_dialog_visible(ctx) false
    #define display_reset_main_minmax_from_current_temp(ctx) false
    #define display_increment_main_selected_value(ctx) ((void)0)
    #define display_decrement_main_selected_value(ctx) ((void)0)
    #define display_get_offset_principal(ctx) 0.0f
    #define display_get_offset_degelo(ctx) 0.0f
    #define display_start_offset_editor(ctx, target) ((void)0)
    #define display_is_offset_editor_active(ctx) false
    #define display_increment_offset_editor_digit(ctx) ((void)0)
    #define display_decrement_offset_editor_digit(ctx) ((void)0)
    #define display_confirm_offset_editor_digit(ctx) false
    #define display_update_current_screen(ctx, name, data, count, lamp, dial, comp, heat, door, s2t1v, s2t1, s2t2v, s2t2) \
        oled_update_current_screen(ctx, name, data, count, lamp, dial, comp, heat, door, s2t1v, s2t1, s2t2v, s2t2)
    #define display_set_usb_extraction_screen(ctx, visible, progress, message, is_error, is_done) ((void)0)
    #define display_process(ctx)        ((void)0)
    #define display_consume_nav_diff(ctx) 0
    #define display_consume_nav_click(ctx) false
    #define display_consume_exit_edit_click(ctx) false
    #define display_is_exit_button_pressed(ctx) false
    #define display_is_setpoint_edit_active(ctx) false
    #define display_set_setpoint_edit_active(ctx, active) ((void)0)
    #define display_get_current_screen(ctx) DISPLAY_SCREEN_MAIN_TEMP
    #define display_cancel_edit_modes(ctx) ((void)0)
    #define display_set_top_status(ctx, text) ((void)0)

#endif

#ifdef __cplusplus
}
#endif

#endif // DISPLAY_H
