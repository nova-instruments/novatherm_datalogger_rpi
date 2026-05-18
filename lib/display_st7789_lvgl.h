#ifndef DISPLAY_ST7789_LVGL_H
#define DISPLAY_ST7789_LVGL_H

#include <stdbool.h>
#include <stdint.h>

#include "modbus.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ST7789_SCREEN_MAIN_TEMP = 0,
    ST7789_SCREEN_INFO,
    ST7789_SCREEN_DIAGNOSTICS,
    ST7789_SCREEN_SLAVE2_TEMPS,
    ST7789_SCREEN_COUNT
} st7789_screen_t;

typedef struct st7789_context_s st7789_context_t;
typedef enum {
    ST7789_INFO_ITEM_AC = 0,
    ST7789_INFO_ITEM_DC,
    ST7789_INFO_ITEM_HYSTERESIS,
    ST7789_INFO_ITEM_LOGS
} st7789_info_item_t;
typedef enum {
    ST7789_DIAG_ITEM_COMPRESSOR = 0,
    ST7789_DIAG_ITEM_HEATER,
    ST7789_DIAG_ITEM_LAMP,
    ST7789_DIAG_ITEM_DIALER
} st7789_diag_item_t;
typedef enum {
    ST7789_MAIN_ITEM_OFFSET_PRINCIPAL = 0,
    ST7789_MAIN_ITEM_OFFSET_DEGELO,
    ST7789_MAIN_ITEM_SETPOINT,
    ST7789_MAIN_ITEM_MIN,
    ST7789_MAIN_ITEM_MAX
} st7789_main_item_t;
typedef enum {
    ST7789_OFFSET_TARGET_PRINCIPAL = 0,
    ST7789_OFFSET_TARGET_DEGELO
} st7789_offset_target_t;

st7789_context_t* st7789_init(void);
void st7789_cleanup(st7789_context_t* ctx);
void st7789_clear(st7789_context_t* ctx);
void st7789_display_splash(st7789_context_t* ctx, const char* device_name);
void st7789_next_screen(st7789_context_t* ctx);
void st7789_previous_screen(st7789_context_t* ctx);
void st7789_increment_setpoint(st7789_context_t* ctx);
void st7789_decrement_setpoint(st7789_context_t* ctx);
float st7789_get_setpoint(st7789_context_t* ctx);
void st7789_increment_hysteresis(st7789_context_t* ctx);
void st7789_decrement_hysteresis(st7789_context_t* ctx);
float st7789_get_hysteresis(st7789_context_t* ctx);
bool st7789_is_hysteresis_password_active(st7789_context_t* ctx);
bool st7789_is_hysteresis_password_result_visible(st7789_context_t* ctx);
bool st7789_consume_hysteresis_password_result(st7789_context_t* ctx);
void st7789_start_hysteresis_password(st7789_context_t* ctx);
void st7789_increment_hysteresis_password_digit(st7789_context_t* ctx);
void st7789_decrement_hysteresis_password_digit(st7789_context_t* ctx);
bool st7789_confirm_hysteresis_password_digit(st7789_context_t* ctx);
bool st7789_is_encoder_button_pressed(st7789_context_t* ctx);
bool st7789_is_info_nav_active(st7789_context_t* ctx);
void st7789_set_info_nav_active(st7789_context_t* ctx, bool active);
void st7789_move_info_selection(st7789_context_t* ctx, int32_t diff);
st7789_info_item_t st7789_get_info_selection(st7789_context_t* ctx);
void st7789_set_logs_reset_dialog(st7789_context_t* ctx, bool visible, const char* message);
bool st7789_is_logs_reset_dialog_visible(st7789_context_t* ctx);
bool st7789_is_main_nav_active(st7789_context_t* ctx);
void st7789_set_main_nav_active(st7789_context_t* ctx, bool active);
void st7789_move_main_selection(st7789_context_t* ctx, int32_t diff);
st7789_main_item_t st7789_get_main_selection(st7789_context_t* ctx);
bool st7789_is_diag_nav_active(st7789_context_t* ctx);
void st7789_set_diag_nav_active(st7789_context_t* ctx, bool active);
void st7789_move_diag_selection(st7789_context_t* ctx, int32_t diff);
st7789_diag_item_t st7789_get_diag_selection(st7789_context_t* ctx);
void st7789_set_diag_compressor_avg_dialog(st7789_context_t* ctx, bool visible);
bool st7789_is_diag_compressor_avg_dialog_visible(st7789_context_t* ctx);
void st7789_set_main_minmax_reset_dialog(st7789_context_t* ctx, bool visible, const char* message);
bool st7789_is_main_minmax_reset_dialog_visible(st7789_context_t* ctx);
bool st7789_reset_main_minmax_from_current_temp(st7789_context_t* ctx);
void st7789_increment_main_selected_value(st7789_context_t* ctx);
void st7789_decrement_main_selected_value(st7789_context_t* ctx);
float st7789_get_offset_principal(st7789_context_t* ctx);
float st7789_get_offset_degelo(st7789_context_t* ctx);
void st7789_start_offset_editor(st7789_context_t* ctx, st7789_offset_target_t target);
bool st7789_is_offset_editor_active(st7789_context_t* ctx);
void st7789_increment_offset_editor_digit(st7789_context_t* ctx);
void st7789_decrement_offset_editor_digit(st7789_context_t* ctx);
bool st7789_confirm_offset_editor_digit(st7789_context_t* ctx);
void st7789_update_current_screen(st7789_context_t* ctx, const char* device_name,
                                  const modbus_data_t* data, uint32_t record_count,
                                  bool lamp_on, bool dialer_on, bool compressor_on,
                                  bool heater_on, bool door_open,
                                  bool slave2_t1_valid, float slave2_t1,
                                  bool slave2_t2_valid, float slave2_t2);
void st7789_set_usb_extraction_screen(st7789_context_t* ctx, bool visible, int progress,
                                      const char* message, bool is_error, bool is_done);
void st7789_process(st7789_context_t* ctx);
int32_t st7789_consume_encoder_diff(st7789_context_t* ctx);
bool st7789_consume_encoder_click(st7789_context_t* ctx);
bool st7789_consume_exit_button_click(st7789_context_t* ctx);
bool st7789_is_exit_button_pressed(st7789_context_t* ctx);
bool st7789_is_setpoint_edit_active(st7789_context_t* ctx);
void st7789_set_setpoint_edit_active(st7789_context_t* ctx, bool active);
st7789_screen_t st7789_get_current_screen(st7789_context_t* ctx);
void st7789_cancel_edit_modes(st7789_context_t* ctx);
void st7789_set_top_status(st7789_context_t* ctx, const char* text);

#ifdef __cplusplus
}
#endif

#endif // DISPLAY_ST7789_LVGL_H
