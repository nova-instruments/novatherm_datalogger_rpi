#include "display_st7789_lvgl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "display_st7789_ec11.h"
#include "lvgl.h"

// Paleta azul para IHM moderna (Tela 1)
#define UI_BLUE_BG_TOP         0x040B17
#define UI_BLUE_BG_BOTTOM      0x0B2340
#define UI_BLUE_TITLE          0xA9DBFF
#define UI_BLUE_TITLE_SUBTLE   0x8DC9F2
#define UI_BLUE_PRIMARY_BORDER 0x39B7FF
#define UI_BLUE_PRIMARY_BG_A   0x0A2742
#define UI_BLUE_PRIMARY_BG_B   0x104A73
#define UI_BLUE_PRIMARY_TEXT   0xBFE5FF
#define UI_BLUE_CARD_BORDER    0x275C86
#define UI_BLUE_CARD_BG        0x0C233A
#define UI_BLUE_CARD_HEADER    0x8DC8F0
#define UI_BLUE_MIN_TEXT       0x9CD9FF
#define UI_BLUE_MAX_TEXT       0x9ED4FF
#define UI_BLUE_SECONDARY      0x84B4D9
#define UI_BLUE_HINT           0x679FCB
#define UI_BLUE_SHADOW         0x020A14
#define UI_SELECTION_YELLOW    0xFFD54A
#define UI_EDIT_RED            0xFF3B30

struct st7789_context_s {
    bool initialized;
    st7789_screen_t current_screen;
    bool setpoint_edit_active;
    float setpoint;
    float hysteresis;
    float offset_principal;
    float offset_degelo;
    bool offset_editor_active;
    st7789_offset_target_t offset_editor_target;
    uint8_t offset_editor_pos;
    bool offset_editor_negative;
    uint8_t offset_editor_digits[4]; // 00.00
    bool hysteresis_password_active;
    bool hysteresis_unlocked;
    bool hysteresis_password_error;
    bool hysteresis_password_result_visible;
    bool hysteresis_password_last_ok;
    uint32_t hysteresis_password_result_tick;
    uint8_t hysteresis_password_digits[4];
    uint8_t hysteresis_password_index;
    uint8_t hysteresis_password_current;
    float pr1_min;
    float pr1_max;
    bool pr1_minmax_valid;
    float last_pr1_value;
    bool last_pr1_valid;

    lv_obj_t* title_label;
    lv_obj_t* edit_header_label;
    char top_status_text[64];
    lv_obj_t* line_labels[8];
    lv_obj_t* base_bg;

    // Layout moderno (Tela 1)
    lv_obj_t* modern_root;
    lv_obj_t* modern_bg;
    lv_obj_t* modern_logo;
    lv_obj_t* modern_header_title;
    lv_obj_t* modern_datetime;
    lv_obj_t* modern_pr1_value;
    lv_obj_t* modern_min_value;
    lv_obj_t* modern_max_value;
    lv_obj_t* modern_sp_value;
    lv_obj_t* modern_pr2_status;
    lv_obj_t* modern_door_value;
    bool main_nav_active;
    st7789_main_item_t main_selected_item;
    lv_obj_t* main_reset_overlay;
    lv_obj_t* main_reset_card;
    lv_obj_t* main_reset_title;
    lv_obj_t* main_reset_status;
    bool main_reset_dialog_visible;

    // Layout em quadros para Tela 2
    lv_obj_t* info_root;
    lv_obj_t* info_bg;
    lv_obj_t* info_ac_card;
    lv_obj_t* info_dc_card;
    lv_obj_t* info_hyst_card;
    lv_obj_t* info_logs_card;
    lv_obj_t* info_ac_value;
    lv_obj_t* info_dc_value;
    lv_obj_t* info_hyst_value;
    lv_obj_t* info_logs_value;
    bool info_nav_active;
    st7789_info_item_t info_selected_item;
    lv_obj_t* diag_root;
    lv_obj_t* diag_bg;
    lv_obj_t* diag_comp_card;
    lv_obj_t* diag_heat_card;
    lv_obj_t* diag_lamp_card;
    lv_obj_t* diag_dialer_card;
    lv_obj_t* diag_comp_value;
    lv_obj_t* diag_heat_value;
    lv_obj_t* diag_lamp_value;
    lv_obj_t* diag_dialer_value;
    lv_obj_t* diag_comp_on_time_value;
    lv_obj_t* diag_comp_off_time_value;
    lv_obj_t* diag_avg_overlay;
    lv_obj_t* diag_avg_card;
    lv_obj_t* diag_avg_title;
    lv_obj_t* diag_avg_on;
    lv_obj_t* diag_avg_off;
    bool diag_avg_dialog_visible;
    bool diag_nav_active;
    st7789_diag_item_t diag_selected_item;
    bool compressor_state_initialized;
    bool compressor_last_state;
    time_t compressor_last_tick;
    uint32_t compressor_state_elapsed_seconds;
    uint32_t compressor_on_seconds;
    uint32_t compressor_off_seconds;
    uint32_t compressor_on_completed_seconds;
    uint32_t compressor_off_completed_seconds;
    uint32_t compressor_on_periods_count;
    uint32_t compressor_off_periods_count;

    lv_obj_t* logs_overlay;
    lv_obj_t* logs_card;
    lv_obj_t* logs_title;
    lv_obj_t* logs_status;
    bool logs_dialog_visible;

    // Janela de senha para liberar edição da histerese
    lv_obj_t* pass_overlay;
    lv_obj_t* pass_card;
    lv_obj_t* pass_title;
    lv_obj_t* pass_value;
    lv_obj_t* pass_status;

    lv_obj_t* offset_overlay;
    lv_obj_t* offset_card;
    lv_obj_t* offset_title;
    lv_obj_t* offset_value;
    lv_obj_t* offset_status;

    // Overlay de extração USB (sobreposto à tela atual)
    lv_obj_t* usb_overlay;
    lv_obj_t* usb_card;
    lv_obj_t* usb_title;
    lv_obj_t* usb_status;
    lv_obj_t* usb_bar;
    lv_obj_t* usb_percent;
};

#define HYSTERESIS_PASSWORD_CODE "1760"

extern const lv_image_dsc_t bg_ihm;
extern const lv_image_dsc_t bg_ihm_2;
extern const lv_image_dsc_t logo_320x240;

static void format_sensor_line(char* out, size_t out_sz, const char* label,
                               const modbus_data_t* data, int index, const char* unit) {
    if (data && data->ch_valid[index] && !data->ch_error[index]) {
        snprintf(out, out_sz, "%s: %5.1f %s", label, data->ch_temp[index], unit);
    } else {
        snprintf(out, out_sz, "%s:   --.- %s", label, unit);
    }
}

static void format_sensor_value(char* out, size_t out_sz, const modbus_data_t* data,
                                int index, const char* unit) {
    bool has_unit = (unit && unit[0] != '\0');
    if (data && data->ch_valid[index] && !data->ch_error[index]) {
        if (has_unit) {
            snprintf(out, out_sz, "%5.1f %s", data->ch_temp[index], unit);
        } else {
            snprintf(out, out_sz, "%5.1f", data->ch_temp[index]);
        }
    } else {
        if (has_unit) {
            snprintf(out, out_sz, "--.- %s", unit);
        } else {
            snprintf(out, out_sz, "--.-");
        }
    }
}

static lv_obj_t* create_modern_card(lv_obj_t* parent, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h,
                                    const char* icon, const char* title, lv_obj_t** value_label) {
    lv_obj_t* card = lv_obj_create(parent);
    lv_obj_set_size(card, w, h);
    lv_obj_set_pos(card, x, y);
    lv_obj_set_style_radius(card, 14, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(UI_BLUE_CARD_BORDER), 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(UI_BLUE_CARD_BG), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_shadow_width(card, 10, 0);
    lv_obj_set_style_shadow_opa(card, LV_OPA_20, 0);
    lv_obj_set_style_shadow_color(card, lv_color_hex(UI_BLUE_SHADOW), 0);
    lv_obj_set_style_outline_width(card, 0, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(card, 0, LV_STATE_PRESSED);
    lv_obj_set_style_pad_all(card, 10, 0);

    lv_obj_t* header = lv_label_create(card);
    lv_obj_set_style_text_font(header, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(header, lv_color_hex(UI_BLUE_CARD_HEADER), 0);
    lv_obj_align(header, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_label_set_text_fmt(header, "%s %s", icon, title);

    *value_label = lv_label_create(card);
    lv_obj_set_style_text_font(*value_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(*value_label, lv_color_hex(UI_BLUE_PRIMARY_TEXT), 0);
    lv_obj_align(*value_label, LV_ALIGN_BOTTOM_LEFT, 0, -2);
    lv_label_set_text(*value_label, "--.-");

    return card;
}

static void format_average_minutes(char* out, size_t out_sz, uint32_t total_seconds, uint32_t periods) {
    if (periods == 0U) {
        snprintf(out, out_sz, "-- min");
        return;
    }
    uint32_t avg_minutes = (total_seconds / periods) / 60U;
    snprintf(out, out_sz, "%lu min", (unsigned long)avg_minutes);
}

static void st7789_set_main_temp_layout(st7789_context_t* ctx, bool enabled) {
    if (!ctx) {
        return;
    }

    if (enabled) {
        if (ctx->modern_root) {
            lv_obj_clear_flag(ctx->modern_root, LV_OBJ_FLAG_HIDDEN);
        }

        if (ctx->title_label) {
            lv_obj_add_flag(ctx->title_label, LV_OBJ_FLAG_HIDDEN);
        }
        for (int i = 0; i < 8; i++) {
            if (ctx->line_labels[i]) {
                lv_obj_add_flag(ctx->line_labels[i], LV_OBJ_FLAG_HIDDEN);
            }
        }
    } else {
        if (ctx->modern_root) {
            lv_obj_add_flag(ctx->modern_root, LV_OBJ_FLAG_HIDDEN);
        }

        if (ctx->title_label) {
            lv_obj_clear_flag(ctx->title_label, LV_OBJ_FLAG_HIDDEN);
        }
        for (int i = 0; i < 8; i++) {
            if (ctx->line_labels[i]) {
                lv_obj_clear_flag(ctx->line_labels[i], LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
}

static void st7789_set_info_layout(st7789_context_t* ctx, bool enabled) {
    if (!ctx) {
        return;
    }

    if (enabled) {
        if (ctx->info_root) {
            lv_obj_clear_flag(ctx->info_root, LV_OBJ_FLAG_HIDDEN);
        }

        if (ctx->title_label) {
            lv_obj_add_flag(ctx->title_label, LV_OBJ_FLAG_HIDDEN);
        }
        for (int i = 0; i < 8; i++) {
            if (ctx->line_labels[i]) {
                lv_obj_add_flag(ctx->line_labels[i], LV_OBJ_FLAG_HIDDEN);
            }
        }
    } else {
        if (ctx->info_root) {
            lv_obj_add_flag(ctx->info_root, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void st7789_set_diag_layout(st7789_context_t* ctx, bool enabled) {
    if (!ctx) {
        return;
    }

    if (enabled) {
        if (ctx->diag_root) {
            lv_obj_clear_flag(ctx->diag_root, LV_OBJ_FLAG_HIDDEN);
        }

        if (ctx->title_label) {
            lv_obj_add_flag(ctx->title_label, LV_OBJ_FLAG_HIDDEN);
        }
        for (int i = 0; i < 8; i++) {
            if (ctx->line_labels[i]) {
                lv_obj_add_flag(ctx->line_labels[i], LV_OBJ_FLAG_HIDDEN);
            }
        }
    } else {
        if (ctx->diag_root) {
            lv_obj_add_flag(ctx->diag_root, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void st7789_update_diag_card_selection(st7789_context_t* ctx) {
    if (!ctx) return;

    lv_obj_t* cards[4] = {
        ctx->diag_comp_card, ctx->diag_heat_card, ctx->diag_lamp_card, ctx->diag_dialer_card
    };

    for (int i = 0; i < 4; i++) {
        if (!cards[i]) continue;
        bool selected = ctx->diag_nav_active && (ctx->diag_selected_item == (st7789_diag_item_t)i);
        lv_obj_set_style_border_color(cards[i],
                                      lv_color_hex(selected ? UI_SELECTION_YELLOW : UI_BLUE_CARD_BORDER), 0);
        lv_obj_set_style_border_width(cards[i], selected ? 2 : 1, 0);
    }
}

static void st7789_update_info_card_selection(st7789_context_t* ctx) {
    if (!ctx) return;

    lv_obj_t* cards[4] = {
        ctx->info_ac_card, ctx->info_dc_card, ctx->info_hyst_card, ctx->info_logs_card
    };

    for (int i = 0; i < 4; i++) {
        if (!cards[i]) continue;
        bool selected = ctx->info_nav_active && (ctx->info_selected_item == (st7789_info_item_t)i);
        lv_obj_set_style_border_color(cards[i],
                                      lv_color_hex(selected ? UI_SELECTION_YELLOW : UI_BLUE_CARD_BORDER), 0);
        lv_obj_set_style_border_width(cards[i], selected ? 2 : 1, 0);
    }
}

static void st7789_update_main_value_selection(st7789_context_t* ctx) {
    if (!ctx) return;
    bool active = ctx->main_nav_active && !ctx->setpoint_edit_active;
    bool sel_min = active && ctx->main_selected_item == ST7789_MAIN_ITEM_MIN;
    bool sel_max = active && ctx->main_selected_item == ST7789_MAIN_ITEM_MAX;
    bool sel_sp = ((active && ctx->main_selected_item == ST7789_MAIN_ITEM_SETPOINT) ||
                   (ctx->setpoint_edit_active && ctx->main_selected_item == ST7789_MAIN_ITEM_SETPOINT));
    bool sel_pr2 = ((active && ctx->main_selected_item == ST7789_MAIN_ITEM_OFFSET_DEGELO) ||
                    (ctx->setpoint_edit_active && ctx->main_selected_item == ST7789_MAIN_ITEM_OFFSET_DEGELO));

    lv_color_t c_min = lv_color_hex(sel_min ? UI_SELECTION_YELLOW : 0xFFFFFF);
    lv_color_t c_max = lv_color_hex(sel_max ? UI_SELECTION_YELLOW : 0xFFFFFF);
    lv_color_t c_sp  = lv_color_hex(((active && ctx->main_selected_item == ST7789_MAIN_ITEM_SETPOINT) ||
                                     (ctx->setpoint_edit_active && ctx->main_selected_item == ST7789_MAIN_ITEM_SETPOINT))
                                    ? (ctx->setpoint_edit_active ? UI_EDIT_RED : UI_SELECTION_YELLOW) : 0xFFFFFF);
    lv_color_t c_pr1 = lv_color_hex(((active && ctx->main_selected_item == ST7789_MAIN_ITEM_OFFSET_PRINCIPAL) ||
                                      (ctx->setpoint_edit_active && ctx->main_selected_item == ST7789_MAIN_ITEM_OFFSET_PRINCIPAL))
                                     ? (ctx->setpoint_edit_active ? UI_EDIT_RED : UI_SELECTION_YELLOW) : 0xFFFFFF);
    lv_color_t c_pr2 = lv_color_hex(((active && ctx->main_selected_item == ST7789_MAIN_ITEM_OFFSET_DEGELO) ||
                                      (ctx->setpoint_edit_active && ctx->main_selected_item == ST7789_MAIN_ITEM_OFFSET_DEGELO))
                                     ? (ctx->setpoint_edit_active ? UI_EDIT_RED : UI_SELECTION_YELLOW) : 0xFFFFFF);
    if (ctx->modern_min_value) lv_obj_set_style_text_color(ctx->modern_min_value, c_min, 0);
    if (ctx->modern_max_value) lv_obj_set_style_text_color(ctx->modern_max_value, c_max, 0);
    if (ctx->modern_sp_value) lv_obj_set_style_text_color(ctx->modern_sp_value, c_sp, 0);
    if (ctx->modern_pr1_value) lv_obj_set_style_text_color(ctx->modern_pr1_value, c_pr1, 0);
    if (ctx->modern_pr2_status) lv_obj_set_style_text_color(ctx->modern_pr2_status, c_pr2, 0);

    // Destaque adicional para selecionados (fallback quando nao ha fonte bold configurada).
    if (ctx->modern_min_value) lv_obj_set_style_text_font(ctx->modern_min_value, sel_min ? &lv_font_montserrat_24 : &lv_font_montserrat_22, 0);
    if (ctx->modern_max_value) lv_obj_set_style_text_font(ctx->modern_max_value, sel_max ? &lv_font_montserrat_24 : &lv_font_montserrat_22, 0);
    if (ctx->modern_sp_value) lv_obj_set_style_text_font(ctx->modern_sp_value, sel_sp ? &lv_font_montserrat_24 : &lv_font_montserrat_22, 0);
    if (ctx->modern_pr2_status) lv_obj_set_style_text_font(ctx->modern_pr2_status, sel_pr2 ? &lv_font_montserrat_24 : &lv_font_montserrat_22, 0);
}

static float st7789_offset_editor_to_value(st7789_context_t* ctx) {
    if (!ctx) return 0.0f;
    float v = (float)(ctx->offset_editor_digits[0] * 10 + ctx->offset_editor_digits[1]) +
              ((float)(ctx->offset_editor_digits[2] * 10 + ctx->offset_editor_digits[3]) / 100.0f);
    return ctx->offset_editor_negative ? -v : v;
}

static void st7789_offset_editor_from_value(st7789_context_t* ctx, float v) {
    if (!ctx) return;
    ctx->offset_editor_negative = (v < 0.0f);
    if (v < 0.0f) v = -v;
    if (v > 99.99f) v = 99.99f;
    int scaled = (int)(v * 100.0f + 0.5f);
    int int_part = scaled / 100;
    int frac_part = scaled % 100;
    ctx->offset_editor_digits[0] = (uint8_t)((int_part / 10) % 10);
    ctx->offset_editor_digits[1] = (uint8_t)(int_part % 10);
    ctx->offset_editor_digits[2] = (uint8_t)((frac_part / 10) % 10);
    ctx->offset_editor_digits[3] = (uint8_t)(frac_part % 10);
    ctx->offset_editor_pos = 0;
}

static void st7789_build_ui(st7789_context_t* ctx) {
    lv_obj_t* scr = lv_screen_active();
    lv_obj_clean(scr);

    lv_obj_set_style_bg_opa(scr, LV_OPA_TRANSP, 0);

    ctx->base_bg = lv_image_create(scr);
    lv_image_set_src(ctx->base_bg, &bg_ihm_2);
    lv_obj_set_pos(ctx->base_bg, 0, 0);

    ctx->title_label = lv_label_create(scr);
    lv_obj_set_style_text_color(ctx->title_label, lv_color_hex(0x7EC2F1), 0);
    lv_obj_set_style_text_font(ctx->title_label, &lv_font_montserrat_16, 0);
    lv_obj_align(ctx->title_label, LV_ALIGN_TOP_LEFT, 8, 6);

    ctx->edit_header_label = lv_label_create(scr);
    lv_obj_set_style_text_color(ctx->edit_header_label, lv_color_hex(UI_SELECTION_YELLOW), 0);
    lv_obj_set_style_text_font(ctx->edit_header_label, &lv_font_montserrat_16, 0);
    lv_obj_align(ctx->edit_header_label, LV_ALIGN_TOP_MID, 0, 6);
    lv_label_set_text(ctx->edit_header_label, "");
    lv_obj_add_flag(ctx->edit_header_label, LV_OBJ_FLAG_HIDDEN);

    for (int i = 0; i < 8; i++) {
        ctx->line_labels[i] = lv_label_create(scr);
        lv_obj_set_style_text_color(ctx->line_labels[i], lv_color_hex(0xB7D8EF), 0);
        lv_obj_set_style_text_font(ctx->line_labels[i], &lv_font_montserrat_16, 0);
        lv_obj_align(ctx->line_labels[i], LV_ALIGN_TOP_LEFT, 8, 36 + (i * 24));
        lv_label_set_text(ctx->line_labels[i], "");
    }

    // Estrutura moderna para Tela 1 (baseada em ui.yaml)
    ctx->modern_root = lv_obj_create(scr);
    lv_obj_set_size(ctx->modern_root, 320, 240);
    lv_obj_set_pos(ctx->modern_root, 0, 0);
    lv_obj_set_style_bg_opa(ctx->modern_root, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ctx->modern_root, 0, 0);
    lv_obj_set_style_pad_all(ctx->modern_root, 0, 0);
    lv_obj_set_style_radius(ctx->modern_root, 0, 0);
    lv_obj_clear_flag(ctx->modern_root, LV_OBJ_FLAG_SCROLLABLE);

    // Fundo estático desenhado no LVGL Image Converter
    ctx->modern_bg = lv_image_create(ctx->modern_root);
    lv_image_set_src(ctx->modern_bg, &bg_ihm);
    lv_obj_set_pos(ctx->modern_bg, 0, 0);

    // Modo overlay: arte da tela vem de imagem convertida; aqui criamos somente valores dinâmicos.
    ctx->modern_logo = NULL;
    ctx->modern_header_title = NULL;

    ctx->modern_datetime = lv_label_create(ctx->modern_root);
    lv_obj_set_style_text_color(ctx->modern_datetime, lv_color_hex(0xA8D8FF), 0);
    lv_obj_set_style_text_font(ctx->modern_datetime, &lv_font_montserrat_18, 0);
    lv_obj_set_pos(ctx->modern_datetime, 218, 10);
    lv_label_set_text(ctx->modern_datetime, "--/-- --:--");

    ctx->modern_pr1_value = lv_label_create(ctx->modern_root);
    lv_obj_set_style_text_color(ctx->modern_pr1_value, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(ctx->modern_pr1_value, &lv_font_montserrat_48, 0);
    lv_obj_set_pos(ctx->modern_pr1_value, 110, 125);
    lv_label_set_text(ctx->modern_pr1_value, "--.-");

    ctx->modern_min_value = lv_label_create(ctx->modern_root);
    lv_obj_set_style_text_color(ctx->modern_min_value, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(ctx->modern_min_value, &lv_font_montserrat_22, 0);
    lv_obj_set_pos(ctx->modern_min_value, 55, 65);
    lv_label_set_text(ctx->modern_min_value, "--.-");

    ctx->modern_max_value = lv_label_create(ctx->modern_root);
    lv_obj_set_style_text_color(ctx->modern_max_value, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(ctx->modern_max_value, &lv_font_montserrat_22, 0);
    lv_obj_set_pos(ctx->modern_max_value, 260, 65);
    lv_label_set_text(ctx->modern_max_value, "--.-");

    ctx->modern_sp_value = lv_label_create(ctx->modern_root);
    lv_obj_set_style_text_color(ctx->modern_sp_value, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(ctx->modern_sp_value, &lv_font_montserrat_22, 0);
    lv_obj_set_pos(ctx->modern_sp_value, 150, 210);
    lv_label_set_text(ctx->modern_sp_value, "--.-");

    ctx->modern_pr2_status = lv_label_create(ctx->modern_root);
    lv_obj_set_style_text_color(ctx->modern_pr2_status, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(ctx->modern_pr2_status, &lv_font_montserrat_22, 0);
    lv_obj_set_pos(ctx->modern_pr2_status, 30, 210);
    lv_label_set_text(ctx->modern_pr2_status, "--.-");

    ctx->modern_door_value = lv_label_create(ctx->modern_root);
    lv_obj_set_style_text_color(ctx->modern_door_value, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(ctx->modern_door_value, &lv_font_montserrat_22, 0);
    lv_obj_set_pos(ctx->modern_door_value, 270, 210);
    lv_label_set_text(ctx->modern_door_value, "0");
    st7789_update_main_value_selection(ctx);

    // Estrutura em quadros para Tela 2
    ctx->info_root = lv_obj_create(scr);
    lv_obj_set_size(ctx->info_root, 320, 240);
    lv_obj_set_pos(ctx->info_root, 0, 0);
    lv_obj_set_style_bg_opa(ctx->info_root, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ctx->info_root, 0, 0);
    lv_obj_set_style_pad_all(ctx->info_root, 0, 0);
    lv_obj_set_style_radius(ctx->info_root, 0, 0);
    lv_obj_clear_flag(ctx->info_root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(ctx->info_root, LV_OBJ_FLAG_HIDDEN);

    ctx->info_bg = lv_image_create(ctx->info_root);
    lv_image_set_src(ctx->info_bg, &bg_ihm_2);
    lv_obj_set_pos(ctx->info_bg, 0, 0);

    ctx->info_ac_card = create_modern_card(ctx->info_root, 8, 42, 148, 84, "AC", "Rede", &ctx->info_ac_value);
    ctx->info_dc_card = create_modern_card(ctx->info_root, 164, 42, 148, 84, "DC", "Fonte", &ctx->info_dc_value);
    ctx->info_hyst_card = create_modern_card(ctx->info_root, 8, 138, 148, 84, "", "Histerese", &ctx->info_hyst_value);
    ctx->info_logs_card = create_modern_card(ctx->info_root, 164, 138, 148, 84, "", "Registros", &ctx->info_logs_value);
    st7789_update_info_card_selection(ctx);

    // Estrutura em quadros para Tela 3 (Diagnosticos)
    ctx->diag_root = lv_obj_create(scr);
    lv_obj_set_size(ctx->diag_root, 320, 240);
    lv_obj_set_pos(ctx->diag_root, 0, 0);
    lv_obj_set_style_bg_opa(ctx->diag_root, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ctx->diag_root, 0, 0);
    lv_obj_set_style_pad_all(ctx->diag_root, 0, 0);
    lv_obj_set_style_radius(ctx->diag_root, 0, 0);
    lv_obj_clear_flag(ctx->diag_root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(ctx->diag_root, LV_OBJ_FLAG_HIDDEN);

    ctx->diag_bg = lv_image_create(ctx->diag_root);
    lv_image_set_src(ctx->diag_bg, &bg_ihm_2);
    lv_obj_set_pos(ctx->diag_bg, 0, 0);

    ctx->diag_comp_card = create_modern_card(ctx->diag_root, 8, 42, 148, 84, "", "Compressor", &ctx->diag_comp_value);
    ctx->diag_heat_card = create_modern_card(ctx->diag_root, 164, 42, 148, 84, "", "Resistencia", &ctx->diag_heat_value);
    ctx->diag_lamp_card = create_modern_card(ctx->diag_root, 8, 138, 148, 84, "", "Lampada", &ctx->diag_lamp_value);
    ctx->diag_dialer_card = create_modern_card(ctx->diag_root, 164, 138, 148, 84, "", "Discadora", &ctx->diag_dialer_value);

    ctx->diag_comp_on_time_value = lv_label_create(ctx->diag_comp_card);
    lv_obj_set_style_text_font(ctx->diag_comp_on_time_value, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(ctx->diag_comp_on_time_value, lv_color_hex(UI_BLUE_HINT), 0);
    lv_obj_align(ctx->diag_comp_on_time_value, LV_ALIGN_BOTTOM_LEFT, 0, -20);
    lv_label_set_text(ctx->diag_comp_on_time_value, "ON m  -- min");

    ctx->diag_comp_off_time_value = lv_label_create(ctx->diag_comp_card);
    lv_obj_set_style_text_font(ctx->diag_comp_off_time_value, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(ctx->diag_comp_off_time_value, lv_color_hex(UI_BLUE_HINT), 0);
    lv_obj_align(ctx->diag_comp_off_time_value, LV_ALIGN_BOTTOM_LEFT, 0, -4);
    lv_label_set_text(ctx->diag_comp_off_time_value, "OFFm  -- min");

    // Evita sobreposição com os textos de média (mantém ON/OFF no topo do card).
    lv_obj_align(ctx->diag_comp_value, LV_ALIGN_TOP_LEFT, 0, 22);
    st7789_update_diag_card_selection(ctx);

    ctx->diag_avg_overlay = lv_obj_create(scr);
    lv_obj_set_size(ctx->diag_avg_overlay, 320, 240);
    lv_obj_set_pos(ctx->diag_avg_overlay, 0, 0);
    lv_obj_set_style_bg_color(ctx->diag_avg_overlay, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(ctx->diag_avg_overlay, LV_OPA_50, 0);
    lv_obj_set_style_border_width(ctx->diag_avg_overlay, 0, 0);
    lv_obj_set_style_radius(ctx->diag_avg_overlay, 0, 0);
    lv_obj_set_style_pad_all(ctx->diag_avg_overlay, 0, 0);
    lv_obj_clear_flag(ctx->diag_avg_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(ctx->diag_avg_overlay, LV_OBJ_FLAG_HIDDEN);

    ctx->diag_avg_card = lv_obj_create(ctx->diag_avg_overlay);
    lv_obj_set_size(ctx->diag_avg_card, 280, 148);
    lv_obj_align(ctx->diag_avg_card, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(ctx->diag_avg_card, lv_color_hex(0x0C233A), 0);
    lv_obj_set_style_bg_opa(ctx->diag_avg_card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(ctx->diag_avg_card, 2, 0);
    lv_obj_set_style_border_color(ctx->diag_avg_card, lv_color_hex(0x39B7FF), 0);
    lv_obj_set_style_radius(ctx->diag_avg_card, 14, 0);
    lv_obj_set_style_pad_all(ctx->diag_avg_card, 12, 0);
    lv_obj_clear_flag(ctx->diag_avg_card, LV_OBJ_FLAG_SCROLLABLE);

    ctx->diag_avg_title = lv_label_create(ctx->diag_avg_card);
    lv_obj_set_style_text_color(ctx->diag_avg_title, lv_color_hex(0xBFE5FF), 0);
    lv_obj_set_style_text_font(ctx->diag_avg_title, &lv_font_montserrat_18, 0);
    lv_obj_align(ctx->diag_avg_title, LV_ALIGN_TOP_MID, 0, 2);
    lv_label_set_text(ctx->diag_avg_title, "Media Compressor");

    ctx->diag_avg_on = lv_label_create(ctx->diag_avg_card);
    lv_obj_set_style_text_color(ctx->diag_avg_on, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(ctx->diag_avg_on, &lv_font_montserrat_18, 0);
    lv_obj_align(ctx->diag_avg_on, LV_ALIGN_CENTER, 0, -14);
    lv_label_set_text(ctx->diag_avg_on, "ON m: -- min");

    ctx->diag_avg_off = lv_label_create(ctx->diag_avg_card);
    lv_obj_set_style_text_color(ctx->diag_avg_off, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(ctx->diag_avg_off, &lv_font_montserrat_18, 0);
    lv_obj_align(ctx->diag_avg_off, LV_ALIGN_CENTER, 0, 16);
    lv_label_set_text(ctx->diag_avg_off, "OFFm: -- min");

    // Janela modal de senha (sem dica de codigo)
    ctx->pass_overlay = lv_obj_create(scr);
    lv_obj_set_size(ctx->pass_overlay, 320, 240);
    lv_obj_set_pos(ctx->pass_overlay, 0, 0);
    lv_obj_set_style_bg_color(ctx->pass_overlay, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(ctx->pass_overlay, LV_OPA_50, 0);
    lv_obj_set_style_border_width(ctx->pass_overlay, 0, 0);
    lv_obj_set_style_radius(ctx->pass_overlay, 0, 0);
    lv_obj_set_style_pad_all(ctx->pass_overlay, 0, 0);
    lv_obj_clear_flag(ctx->pass_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(ctx->pass_overlay, LV_OBJ_FLAG_HIDDEN);

    ctx->pass_card = lv_obj_create(ctx->pass_overlay);
    lv_obj_set_size(ctx->pass_card, 260, 142);
    lv_obj_align(ctx->pass_card, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(ctx->pass_card, lv_color_hex(0x0C233A), 0);
    lv_obj_set_style_bg_opa(ctx->pass_card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(ctx->pass_card, 2, 0);
    lv_obj_set_style_border_color(ctx->pass_card, lv_color_hex(0x39B7FF), 0);
    lv_obj_set_style_radius(ctx->pass_card, 14, 0);
    lv_obj_set_style_pad_all(ctx->pass_card, 12, 0);
    lv_obj_clear_flag(ctx->pass_card, LV_OBJ_FLAG_SCROLLABLE);

    ctx->pass_title = lv_label_create(ctx->pass_card);
    lv_obj_set_style_text_color(ctx->pass_title, lv_color_hex(0xBFE5FF), 0);
    lv_obj_set_style_text_font(ctx->pass_title, &lv_font_montserrat_18, 0);
    lv_obj_align(ctx->pass_title, LV_ALIGN_TOP_MID, 0, 2);
    lv_label_set_text(ctx->pass_title, "Senha Adm");

    ctx->pass_value = lv_label_create(ctx->pass_card);
    lv_obj_set_style_text_color(ctx->pass_value, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(ctx->pass_value, &lv_font_montserrat_24, 0);
    lv_obj_align(ctx->pass_value, LV_ALIGN_CENTER, 0, -8);
    lv_label_set_text(ctx->pass_value, "0 * * *");

    ctx->pass_status = lv_label_create(ctx->pass_card);
    lv_obj_set_style_text_color(ctx->pass_status, lv_color_hex(0xD0E9FF), 0);
    lv_obj_set_style_text_font(ctx->pass_status, &lv_font_montserrat_14, 0);
    lv_obj_align(ctx->pass_status, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_label_set_text(ctx->pass_status, "Digito 1/4");

    ctx->offset_overlay = lv_obj_create(scr);
    lv_obj_set_size(ctx->offset_overlay, 320, 240);
    lv_obj_set_pos(ctx->offset_overlay, 0, 0);
    lv_obj_set_style_bg_color(ctx->offset_overlay, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(ctx->offset_overlay, LV_OPA_50, 0);
    lv_obj_set_style_border_width(ctx->offset_overlay, 0, 0);
    lv_obj_set_style_radius(ctx->offset_overlay, 0, 0);
    lv_obj_set_style_pad_all(ctx->offset_overlay, 0, 0);
    lv_obj_clear_flag(ctx->offset_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(ctx->offset_overlay, LV_OBJ_FLAG_HIDDEN);

    ctx->offset_card = lv_obj_create(ctx->offset_overlay);
    lv_obj_set_size(ctx->offset_card, 280, 148);
    lv_obj_align(ctx->offset_card, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(ctx->offset_card, lv_color_hex(0x0C233A), 0);
    lv_obj_set_style_bg_opa(ctx->offset_card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(ctx->offset_card, 2, 0);
    lv_obj_set_style_border_color(ctx->offset_card, lv_color_hex(0x39B7FF), 0);
    lv_obj_set_style_radius(ctx->offset_card, 14, 0);
    lv_obj_set_style_pad_all(ctx->offset_card, 12, 0);
    lv_obj_clear_flag(ctx->offset_card, LV_OBJ_FLAG_SCROLLABLE);

    ctx->offset_title = lv_label_create(ctx->offset_card);
    lv_obj_set_style_text_color(ctx->offset_title, lv_color_hex(0xBFE5FF), 0);
    lv_obj_set_style_text_font(ctx->offset_title, &lv_font_montserrat_18, 0);
    lv_obj_align(ctx->offset_title, LV_ALIGN_TOP_MID, 0, 2);
    lv_label_set_text(ctx->offset_title, "Offset");

    ctx->offset_value = lv_label_create(ctx->offset_card);
    lv_obj_set_style_text_color(ctx->offset_value, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(ctx->offset_value, &lv_font_montserrat_24, 0);
    lv_obj_align(ctx->offset_value, LV_ALIGN_CENTER, 0, -8);
    lv_label_set_text(ctx->offset_value, "[+] 0 0 . 0 0");

    ctx->offset_status = lv_label_create(ctx->offset_card);
    lv_obj_set_style_text_color(ctx->offset_status, lv_color_hex(0xD0E9FF), 0);
    lv_obj_set_style_text_font(ctx->offset_status, &lv_font_montserrat_14, 0);
    lv_obj_align(ctx->offset_status, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_label_set_text(ctx->offset_status, "Digite offset");

    ctx->logs_overlay = lv_obj_create(scr);
    lv_obj_set_size(ctx->logs_overlay, 320, 240);
    lv_obj_set_pos(ctx->logs_overlay, 0, 0);
    lv_obj_set_style_bg_color(ctx->logs_overlay, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(ctx->logs_overlay, LV_OPA_50, 0);
    lv_obj_set_style_border_width(ctx->logs_overlay, 0, 0);
    lv_obj_set_style_radius(ctx->logs_overlay, 0, 0);
    lv_obj_set_style_pad_all(ctx->logs_overlay, 0, 0);
    lv_obj_clear_flag(ctx->logs_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(ctx->logs_overlay, LV_OBJ_FLAG_HIDDEN);

    ctx->logs_card = lv_obj_create(ctx->logs_overlay);
    lv_obj_set_size(ctx->logs_card, 280, 148);
    lv_obj_align(ctx->logs_card, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(ctx->logs_card, lv_color_hex(0x0C233A), 0);
    lv_obj_set_style_bg_opa(ctx->logs_card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(ctx->logs_card, 2, 0);
    lv_obj_set_style_border_color(ctx->logs_card, lv_color_hex(0x39B7FF), 0);
    lv_obj_set_style_radius(ctx->logs_card, 14, 0);
    lv_obj_set_style_pad_all(ctx->logs_card, 12, 0);
    lv_obj_clear_flag(ctx->logs_card, LV_OBJ_FLAG_SCROLLABLE);

    ctx->logs_title = lv_label_create(ctx->logs_card);
    lv_obj_set_style_text_color(ctx->logs_title, lv_color_hex(0xBFE5FF), 0);
    lv_obj_set_style_text_font(ctx->logs_title, &lv_font_montserrat_18, 0);
    lv_obj_align(ctx->logs_title, LV_ALIGN_TOP_MID, 0, 2);
    lv_label_set_text(ctx->logs_title, "Reset de Registros");

    ctx->logs_status = lv_label_create(ctx->logs_card);
    lv_obj_set_width(ctx->logs_status, 250);
    lv_obj_set_style_text_align(ctx->logs_status, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(ctx->logs_status, lv_color_hex(0xD0E9FF), 0);
    lv_obj_set_style_text_font(ctx->logs_status, &lv_font_montserrat_14, 0);
    lv_obj_align(ctx->logs_status, LV_ALIGN_CENTER, 0, 10);
    lv_label_set_text(ctx->logs_status, "Pressione o botao por 3s");

    ctx->main_reset_overlay = lv_obj_create(scr);
    lv_obj_set_size(ctx->main_reset_overlay, 320, 240);
    lv_obj_set_pos(ctx->main_reset_overlay, 0, 0);
    lv_obj_set_style_bg_color(ctx->main_reset_overlay, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(ctx->main_reset_overlay, LV_OPA_50, 0);
    lv_obj_set_style_border_width(ctx->main_reset_overlay, 0, 0);
    lv_obj_set_style_radius(ctx->main_reset_overlay, 0, 0);
    lv_obj_set_style_pad_all(ctx->main_reset_overlay, 0, 0);
    lv_obj_clear_flag(ctx->main_reset_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(ctx->main_reset_overlay, LV_OBJ_FLAG_HIDDEN);

    ctx->main_reset_card = lv_obj_create(ctx->main_reset_overlay);
    lv_obj_set_size(ctx->main_reset_card, 280, 148);
    lv_obj_align(ctx->main_reset_card, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(ctx->main_reset_card, lv_color_hex(0x0C233A), 0);
    lv_obj_set_style_bg_opa(ctx->main_reset_card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(ctx->main_reset_card, 2, 0);
    lv_obj_set_style_border_color(ctx->main_reset_card, lv_color_hex(0x39B7FF), 0);
    lv_obj_set_style_radius(ctx->main_reset_card, 14, 0);
    lv_obj_set_style_pad_all(ctx->main_reset_card, 12, 0);
    lv_obj_clear_flag(ctx->main_reset_card, LV_OBJ_FLAG_SCROLLABLE);

    ctx->main_reset_title = lv_label_create(ctx->main_reset_card);
    lv_obj_set_style_text_color(ctx->main_reset_title, lv_color_hex(0xBFE5FF), 0);
    lv_obj_set_style_text_font(ctx->main_reset_title, &lv_font_montserrat_18, 0);
    lv_obj_align(ctx->main_reset_title, LV_ALIGN_TOP_MID, 0, 2);
    lv_label_set_text(ctx->main_reset_title, "Reset Min/Max");

    ctx->main_reset_status = lv_label_create(ctx->main_reset_card);
    lv_obj_set_width(ctx->main_reset_status, 250);
    lv_obj_set_style_text_align(ctx->main_reset_status, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(ctx->main_reset_status, lv_color_hex(0xD0E9FF), 0);
    lv_obj_set_style_text_font(ctx->main_reset_status, &lv_font_montserrat_14, 0);
    lv_obj_align(ctx->main_reset_status, LV_ALIGN_CENTER, 0, 10);
    lv_label_set_text(ctx->main_reset_status, "Pressione o botao por 3s");

    // Overlay de extração USB
    ctx->usb_overlay = lv_obj_create(scr);
    lv_obj_set_size(ctx->usb_overlay, 320, 240);
    lv_obj_set_pos(ctx->usb_overlay, 0, 0);
    lv_obj_set_style_bg_color(ctx->usb_overlay, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(ctx->usb_overlay, LV_OPA_50, 0);
    lv_obj_set_style_border_width(ctx->usb_overlay, 0, 0);
    lv_obj_set_style_radius(ctx->usb_overlay, 0, 0);
    lv_obj_set_style_pad_all(ctx->usb_overlay, 0, 0);
    lv_obj_clear_flag(ctx->usb_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(ctx->usb_overlay, LV_OBJ_FLAG_HIDDEN);

    ctx->usb_card = lv_obj_create(ctx->usb_overlay);
    lv_obj_set_size(ctx->usb_card, 286, 160);
    lv_obj_align(ctx->usb_card, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(ctx->usb_card, lv_color_hex(0x0C233A), 0);
    lv_obj_set_style_bg_opa(ctx->usb_card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(ctx->usb_card, 2, 0);
    lv_obj_set_style_border_color(ctx->usb_card, lv_color_hex(0x39B7FF), 0);
    lv_obj_set_style_radius(ctx->usb_card, 14, 0);
    lv_obj_set_style_pad_all(ctx->usb_card, 12, 0);
    lv_obj_clear_flag(ctx->usb_card, LV_OBJ_FLAG_SCROLLABLE);

    ctx->usb_title = lv_label_create(ctx->usb_card);
    lv_obj_set_style_text_color(ctx->usb_title, lv_color_hex(0xBFE5FF), 0);
    lv_obj_set_style_text_font(ctx->usb_title, &lv_font_montserrat_20, 0);
    lv_obj_align(ctx->usb_title, LV_ALIGN_TOP_MID, 0, 2);
    lv_label_set_text(ctx->usb_title, "Extracao USB");

    ctx->usb_status = lv_label_create(ctx->usb_card);
    lv_obj_set_width(ctx->usb_status, 250);
    lv_obj_set_style_text_align(ctx->usb_status, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(ctx->usb_status, lv_color_hex(0xD0E9FF), 0);
    lv_obj_set_style_text_font(ctx->usb_status, &lv_font_montserrat_14, 0);
    lv_obj_align(ctx->usb_status, LV_ALIGN_TOP_MID, 0, 40);
    lv_label_set_text(ctx->usb_status, "Aguardando...");

    ctx->usb_bar = lv_bar_create(ctx->usb_card);
    lv_obj_set_size(ctx->usb_bar, 238, 18);
    lv_obj_align(ctx->usb_bar, LV_ALIGN_CENTER, 0, 18);
    lv_bar_set_range(ctx->usb_bar, 0, 100);
    lv_bar_set_value(ctx->usb_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(ctx->usb_bar, lv_color_hex(0x17344F), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(ctx->usb_bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(ctx->usb_bar, lv_color_hex(0x39B7FF), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(ctx->usb_bar, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_radius(ctx->usb_bar, 8, LV_PART_MAIN);
    lv_obj_set_style_radius(ctx->usb_bar, 8, LV_PART_INDICATOR);

    ctx->usb_percent = lv_label_create(ctx->usb_card);
    lv_obj_set_style_text_color(ctx->usb_percent, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(ctx->usb_percent, &lv_font_montserrat_16, 0);
    lv_obj_align(ctx->usb_percent, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_label_set_text(ctx->usb_percent, "0%");

    // Inicia no layout legado; Telas 1 e 2 ativam layouts dedicados no render
    st7789_set_main_temp_layout(ctx, false);
    st7789_set_info_layout(ctx, false);
    st7789_set_diag_layout(ctx, false);
}

void st7789_set_usb_extraction_screen(st7789_context_t* ctx, bool visible, int progress,
                                      const char* message, bool is_error, bool is_done) {
    if (!ctx || !ctx->initialized || !ctx->usb_overlay) {
        return;
    }

    if (progress < 0) progress = 0;
    if (progress > 100) progress = 100;

    if (visible) {
        lv_obj_clear_flag(ctx->usb_overlay, LV_OBJ_FLAG_HIDDEN);

        if (ctx->usb_title) {
            lv_label_set_text(ctx->usb_title, is_done ? (is_error ? "Falha na Extracao" : "Extracao Concluida")
                                                       : "Extracao USB");
            lv_obj_set_style_text_color(ctx->usb_title,
                                        lv_color_hex(is_error ? 0xFF9E9E : 0xBFE5FF), 0);
        }

        if (ctx->usb_status) {
            lv_label_set_text(ctx->usb_status, message && message[0] ? message : "Processando...");
        }

        if (ctx->usb_bar) {
            lv_bar_set_value(ctx->usb_bar, progress, LV_ANIM_OFF);
            lv_obj_set_style_bg_color(ctx->usb_bar,
                                      lv_color_hex(is_error ? 0x7A1F1F : 0x17344F), LV_PART_MAIN);
            lv_obj_set_style_bg_color(ctx->usb_bar,
                                      lv_color_hex(is_error ? 0xFF5A5A : 0x39B7FF), LV_PART_INDICATOR);
        }

        if (ctx->usb_percent) {
            lv_label_set_text_fmt(ctx->usb_percent, "%d%%", progress);
        }
    } else {
        lv_obj_add_flag(ctx->usb_overlay, LV_OBJ_FLAG_HIDDEN);
    }

    display_st7789_ec11_poll(0);
}

static void st7789_render_screen(st7789_context_t* ctx, const char* device_name,
                                 const modbus_data_t* data, uint32_t record_count,
                                 bool lamp_on, bool dialer_on, bool compressor_on,
                                 bool heater_on, bool door_open,
                                 bool slave2_t1_valid, float slave2_t1,
                                 bool slave2_t2_valid, float slave2_t2) {
    if (!ctx || !ctx->initialized) {
        return;
    }

    char line[96];
    (void)record_count;
    (void)device_name;

    switch (ctx->current_screen) {
        case ST7789_SCREEN_MAIN_TEMP:
            st7789_set_main_temp_layout(ctx, true);
            st7789_set_info_layout(ctx, false);
            st7789_set_diag_layout(ctx, false);
            lv_image_set_src(ctx->modern_bg, &bg_ihm);

            {
                time_t now = time(NULL);
                struct tm* tm_now = localtime(&now);
                if (tm_now) {
                    strftime(line, sizeof(line), "%d/%m %H:%M", tm_now);
                    lv_label_set_text(ctx->modern_datetime, line);
                }
            }

            format_sensor_value(line, sizeof(line), data, 0, "");
            lv_label_set_text(ctx->modern_pr1_value, line);

            if (data && data->ch_valid[0] && !data->ch_error[0]) {
                float pr1 = data->ch_temp[0];
                ctx->last_pr1_value = pr1;
                ctx->last_pr1_valid = true;
                if (!ctx->pr1_minmax_valid) {
                    ctx->pr1_min = pr1;
                    ctx->pr1_max = pr1;
                    ctx->pr1_minmax_valid = true;
                } else {
                    if (pr1 < ctx->pr1_min) ctx->pr1_min = pr1;
                    if (pr1 > ctx->pr1_max) ctx->pr1_max = pr1;
                }
            }

            if (ctx->pr1_minmax_valid) {
                lv_label_set_text_fmt(ctx->modern_min_value, "%.1f", ctx->pr1_min);
                lv_label_set_text_fmt(ctx->modern_max_value, "%.1f", ctx->pr1_max);
            } else {
                lv_label_set_text(ctx->modern_min_value, "--.-");
                lv_label_set_text(ctx->modern_max_value, "--.-");
            }

            snprintf(line, sizeof(line), "%4.1f", ctx->setpoint);
            lv_label_set_text(ctx->modern_sp_value, line);
            st7789_update_main_value_selection(ctx);

            format_sensor_value(line, sizeof(line), data, 1, "");
            lv_label_set_text(ctx->modern_pr2_status, line);
            lv_label_set_text_fmt(ctx->modern_door_value, "%d", door_open ? 1 : 0);
            break;

        case ST7789_SCREEN_INFO:
            st7789_set_main_temp_layout(ctx, false);
            st7789_set_info_layout(ctx, true);
            st7789_set_diag_layout(ctx, false);
            st7789_update_info_card_selection(ctx);
            if (ctx->info_hyst_value) {
                lv_obj_set_style_text_color(ctx->info_hyst_value,
                                            lv_color_hex(ctx->setpoint_edit_active ? 0xFF3B30 : UI_BLUE_PRIMARY_TEXT), 0);
            }

            format_sensor_value(line, sizeof(line), data, 2, "V");
            lv_label_set_text(ctx->info_ac_value, line);
            format_sensor_value(line, sizeof(line), data, 3, "V");
            lv_label_set_text(ctx->info_dc_value, line);

            lv_label_set_text_fmt(ctx->info_hyst_value, "%.1f C", ctx->hysteresis);
            lv_label_set_text_fmt(ctx->info_logs_value, "%lu", (unsigned long)record_count);
            break;

        case ST7789_SCREEN_DIAGNOSTICS:
            ctx->setpoint_edit_active = false;
            st7789_set_main_temp_layout(ctx, false);
            st7789_set_info_layout(ctx, false);
            st7789_set_diag_layout(ctx, true);
            st7789_update_diag_card_selection(ctx);

            time_t now = time(NULL);
            if (!ctx->compressor_state_initialized) {
                ctx->compressor_state_initialized = true;
                ctx->compressor_last_state = compressor_on;
                ctx->compressor_last_tick = now;
                ctx->compressor_state_elapsed_seconds = 0U;
            } else if (now > ctx->compressor_last_tick) {
                uint32_t delta = (uint32_t)(now - ctx->compressor_last_tick);
                if (ctx->compressor_last_state) {
                    ctx->compressor_on_seconds += delta;
                } else {
                    ctx->compressor_off_seconds += delta;
                }
                ctx->compressor_state_elapsed_seconds += delta;
                ctx->compressor_last_tick = now;

                if (compressor_on != ctx->compressor_last_state) {
                    if (ctx->compressor_last_state) {
                        ctx->compressor_on_completed_seconds += ctx->compressor_state_elapsed_seconds;
                        ctx->compressor_on_periods_count++;
                    } else {
                        ctx->compressor_off_completed_seconds += ctx->compressor_state_elapsed_seconds;
                        ctx->compressor_off_periods_count++;
                    }
                    ctx->compressor_state_elapsed_seconds = 0U;
                    ctx->compressor_last_state = compressor_on;
                }
            } else {
                ctx->compressor_last_state = compressor_on;
            }

            lv_label_set_text(ctx->diag_comp_value, compressor_on ? "ON" : "OFF");
            lv_label_set_text(ctx->diag_heat_value, heater_on ? "ON" : "OFF");
            lv_label_set_text(ctx->diag_lamp_value, lamp_on ? "ON" : "OFF");
            lv_label_set_text(ctx->diag_dialer_value, dialer_on ? "ON" : "OFF");

            lv_label_set_text(ctx->diag_comp_on_time_value, "");
            lv_label_set_text(ctx->diag_comp_off_time_value, "");
            break;

        case ST7789_SCREEN_SLAVE2_TEMPS:
            st7789_set_main_temp_layout(ctx, false);
            st7789_set_info_layout(ctx, false);
            st7789_set_diag_layout(ctx, false);
            if (ctx->title_label) {
                lv_label_set_text(ctx->title_label, "MODBUS SLAVE 2");
            }
            if (ctx->line_labels[0]) lv_label_set_text(ctx->line_labels[0], "T1/T2 + NTC1R/NTC2R");
            if (ctx->line_labels[1]) lv_label_set_text(ctx->line_labels[1], "S2:0x0200/0x0201");
            if (ctx->line_labels[2]) {
                if (slave2_t1_valid) {
                    lv_label_set_text_fmt(ctx->line_labels[2], "T1: %.1f C", slave2_t1);
                } else {
                    lv_label_set_text(ctx->line_labels[2], "T1: ERRO");
                }
            }
            if (ctx->line_labels[3]) {
                if (slave2_t2_valid) {
                    lv_label_set_text_fmt(ctx->line_labels[3], "T2: %.1f C", slave2_t2);
                } else {
                    lv_label_set_text(ctx->line_labels[3], "T2: ERRO");
                }
            }
            if (ctx->line_labels[4]) {
                if (data && data->ch_valid[4]) {
                    lv_label_set_text_fmt(ctx->line_labels[4], "N1R(S1): %.0f ohm", data->ch_temp[4]);
                } else {
                    lv_label_set_text(ctx->line_labels[4], "N1R(S1): ERRO");
                }
            }
            if (ctx->line_labels[5]) {
                if (data && data->ch_valid[5]) {
                    lv_label_set_text_fmt(ctx->line_labels[5], "N2R(S1): %.0f ohm", data->ch_temp[5]);
                } else {
                    lv_label_set_text(ctx->line_labels[5], "N2R(S1): ERRO");
                }
            }
            if (ctx->line_labels[6]) lv_label_set_text(ctx->line_labels[6], "S1 regs: 0x0004/0x0005");
            if (ctx->line_labels[7]) lv_label_set_text(ctx->line_labels[7], "");
            break;

        default:
            st7789_set_main_temp_layout(ctx, false);
            st7789_set_info_layout(ctx, false);
            st7789_set_diag_layout(ctx, false);
            break;
    }

    if (ctx->edit_header_label) {
        if (ctx->top_status_text[0] != '\0') {
            lv_label_set_text(ctx->edit_header_label, ctx->top_status_text);
            lv_obj_clear_flag(ctx->edit_header_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_move_foreground(ctx->edit_header_label);
        } else if (ctx->setpoint_edit_active || ctx->info_nav_active || ctx->main_nav_active ||
                   ctx->diag_nav_active ||
                   ctx->offset_editor_active) {
            lv_label_set_text(ctx->edit_header_label, "Edicao");
            lv_obj_clear_flag(ctx->edit_header_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_move_foreground(ctx->edit_header_label);
        } else {
            lv_obj_add_flag(ctx->edit_header_label, LV_OBJ_FLAG_HIDDEN);
        }
    }

    if (ctx->pass_overlay && ctx->pass_value && ctx->pass_status) {
        if (ctx->hysteresis_password_active || ctx->hysteresis_password_result_visible) {
            lv_obj_clear_flag(ctx->pass_overlay, LV_OBJ_FLAG_HIDDEN);
            if (ctx->hysteresis_password_active) {
                char d0 = (ctx->hysteresis_password_index > 0) ? '*' : '_';
                char d1 = (ctx->hysteresis_password_index > 1) ? '*' : '_';
                char d2 = (ctx->hysteresis_password_index > 2) ? '*' : '_';
                char d3 = (char)('0' + ctx->hysteresis_password_current);
                if (ctx->hysteresis_password_index == 0) {
                    snprintf(line, sizeof(line), "[%c]  %c  %c  %c", d3, d1, d2, '_');
                } else if (ctx->hysteresis_password_index == 1) {
                    snprintf(line, sizeof(line), "%c [%c]  %c  %c", d0, d3, d2, '_');
                } else if (ctx->hysteresis_password_index == 2) {
                    snprintf(line, sizeof(line), "%c  %c [%c]  %c", d0, d1, d3, '_');
                } else {
                    snprintf(line, sizeof(line), "%c  %c  %c [%c]", d0, d1, d2, d3);
                }
                lv_label_set_text(ctx->pass_value, line);
                lv_label_set_text_fmt(ctx->pass_status, "Digite a senha (%u/4)",
                                      (unsigned)(ctx->hysteresis_password_index + 1));
            } else {
                lv_label_set_text(ctx->pass_value, "*  *  *  *");
                lv_label_set_text(ctx->pass_status,
                                  ctx->hysteresis_password_last_ok ? "Senha correta" : "Senha invalida");
            }
        } else {
            lv_obj_add_flag(ctx->pass_overlay, LV_OBJ_FLAG_HIDDEN);
        }
    }

    if (ctx->offset_overlay && ctx->offset_value && ctx->offset_status && ctx->offset_title) {
        if (ctx->offset_editor_active) {
            lv_obj_clear_flag(ctx->offset_overlay, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(ctx->offset_title,
                              ctx->offset_editor_target == ST7789_OFFSET_TARGET_PRINCIPAL
                                  ? "Offset Principal"
                                  : "Offset Degelo");
            char sign_ch = ctx->offset_editor_negative ? '-' : '+';
            char text[48];
            if (ctx->offset_editor_pos == 0) {
                snprintf(text, sizeof(text), "[%c] %u %u . %u %u", sign_ch,
                         (unsigned)ctx->offset_editor_digits[0], (unsigned)ctx->offset_editor_digits[1],
                         (unsigned)ctx->offset_editor_digits[2], (unsigned)ctx->offset_editor_digits[3]);
            } else if (ctx->offset_editor_pos == 1) {
                snprintf(text, sizeof(text), " %c [%u] %u . %u %u", sign_ch,
                         (unsigned)ctx->offset_editor_digits[0], (unsigned)ctx->offset_editor_digits[1],
                         (unsigned)ctx->offset_editor_digits[2], (unsigned)ctx->offset_editor_digits[3]);
            } else if (ctx->offset_editor_pos == 2) {
                snprintf(text, sizeof(text), " %c %u [%u] . %u %u", sign_ch,
                         (unsigned)ctx->offset_editor_digits[0], (unsigned)ctx->offset_editor_digits[1],
                         (unsigned)ctx->offset_editor_digits[2], (unsigned)ctx->offset_editor_digits[3]);
            } else if (ctx->offset_editor_pos == 3) {
                snprintf(text, sizeof(text), " %c %u %u . [%u] %u", sign_ch,
                         (unsigned)ctx->offset_editor_digits[0], (unsigned)ctx->offset_editor_digits[1],
                         (unsigned)ctx->offset_editor_digits[2], (unsigned)ctx->offset_editor_digits[3]);
            } else {
                snprintf(text, sizeof(text), " %c %u %u . %u [%u]", sign_ch,
                         (unsigned)ctx->offset_editor_digits[0], (unsigned)ctx->offset_editor_digits[1],
                         (unsigned)ctx->offset_editor_digits[2], (unsigned)ctx->offset_editor_digits[3]);
            }
            lv_label_set_text(ctx->offset_value, text);
            lv_label_set_text_fmt(ctx->offset_status, "Posicao %u/5", (unsigned)(ctx->offset_editor_pos + 1));
        } else {
            lv_obj_add_flag(ctx->offset_overlay, LV_OBJ_FLAG_HIDDEN);
        }
    }

    if (ctx->logs_overlay) {
        if (ctx->logs_dialog_visible) {
            lv_obj_clear_flag(ctx->logs_overlay, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(ctx->logs_overlay, LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (ctx->main_reset_overlay) {
        if (ctx->main_reset_dialog_visible) {
            lv_obj_clear_flag(ctx->main_reset_overlay, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(ctx->main_reset_overlay, LV_OBJ_FLAG_HIDDEN);
        }
    }

    if (ctx->diag_avg_overlay) {
        if (ctx->diag_avg_dialog_visible) {
            char on_text[16];
            char off_text[16];
            format_average_minutes(on_text, sizeof(on_text),
                                   ctx->compressor_on_completed_seconds,
                                   ctx->compressor_on_periods_count);
            format_average_minutes(off_text, sizeof(off_text),
                                   ctx->compressor_off_completed_seconds,
                                   ctx->compressor_off_periods_count);
            if (ctx->diag_avg_on) lv_label_set_text_fmt(ctx->diag_avg_on, "ON m: %s", on_text);
            if (ctx->diag_avg_off) lv_label_set_text_fmt(ctx->diag_avg_off, "OFFm: %s", off_text);
            lv_obj_clear_flag(ctx->diag_avg_overlay, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(ctx->diag_avg_overlay, LV_OBJ_FLAG_HIDDEN);
        }
    }

    display_st7789_ec11_poll(0);
}

st7789_context_t* st7789_init(void) {
    st7789_context_t* ctx = (st7789_context_t*)malloc(sizeof(st7789_context_t));
    if (!ctx) {
        fprintf(stderr, "Erro ao alocar contexto ST7789\n");
        return NULL;
    }

    memset(ctx, 0, sizeof(*ctx));

    display_st7789_ec11_config_t cfg = {
        .spi_dev = "/dev/spidev0.0",
        .spi_hz = 40000000U,
        .dc_gpio = 22,
        .rst_gpio = 27,
        .width = 320,
        .height = 240,
        .x_offset = 0,
        .y_offset = 0,
        .encoder_evdev = "/dev/input/event1",
        .button_evdev = "/dev/input/event0",
        .create_default_ui = false
    };

    if (display_st7789_ec11_init(&cfg) != 0) {
        fprintf(stderr, "Falha ao inicializar driver ST7789 EC11\n");
        free(ctx);
        return NULL;
    }

    ctx->current_screen = ST7789_SCREEN_MAIN_TEMP;
    ctx->setpoint = 4.5f;
    ctx->hysteresis = 0.6f;
    ctx->offset_principal = 0.0f;
    ctx->offset_degelo = 0.0f;
    ctx->hysteresis_password_active = false;
    ctx->hysteresis_unlocked = false;
    ctx->hysteresis_password_error = false;
    ctx->hysteresis_password_result_visible = false;
    ctx->hysteresis_password_last_ok = false;
    ctx->hysteresis_password_result_tick = 0;
    ctx->info_nav_active = false;
    ctx->info_selected_item = ST7789_INFO_ITEM_AC;
    ctx->diag_nav_active = false;
    ctx->diag_selected_item = ST7789_DIAG_ITEM_COMPRESSOR;
    ctx->diag_avg_dialog_visible = false;
    ctx->logs_dialog_visible = false;
    ctx->main_nav_active = false;
    ctx->main_selected_item = ST7789_MAIN_ITEM_SETPOINT;
    ctx->main_reset_dialog_visible = false;
    ctx->offset_editor_active = false;
    ctx->offset_editor_target = ST7789_OFFSET_TARGET_PRINCIPAL;
    ctx->offset_editor_pos = 0;
    ctx->offset_editor_negative = false;
    memset(ctx->offset_editor_digits, 0, sizeof(ctx->offset_editor_digits));
    ctx->hysteresis_password_index = 0;
    ctx->hysteresis_password_current = 0;
    ctx->setpoint_edit_active = false;
    ctx->pr1_min = 0.0f;
    ctx->pr1_max = 0.0f;
    ctx->pr1_minmax_valid = false;
    ctx->last_pr1_value = 0.0f;
    ctx->last_pr1_valid = false;
    ctx->compressor_state_initialized = false;
    ctx->compressor_last_state = false;
    ctx->compressor_last_tick = 0;
    ctx->compressor_state_elapsed_seconds = 0U;
    ctx->compressor_on_seconds = 0;
    ctx->compressor_off_seconds = 0;
    ctx->compressor_on_completed_seconds = 0U;
    ctx->compressor_off_completed_seconds = 0U;
    ctx->compressor_on_periods_count = 0U;
    ctx->compressor_off_periods_count = 0U;
    ctx->initialized = true;

    st7789_build_ui(ctx);
    st7789_render_screen(ctx, "NovaTherm", NULL, 0, false, false, false, false, false,
                         false, 0.0f, false, 0.0f);

    printf("Display ST7789 + LVGL inicializado\n");
    return ctx;
}

void st7789_cleanup(st7789_context_t* ctx) {
    if (!ctx) {
        return;
    }

    if (ctx->initialized) {
        display_st7789_ec11_deinit();
    }

    free(ctx);
}

void st7789_clear(st7789_context_t* ctx) {
    if (!ctx || !ctx->initialized) {
        return;
    }

    for (int i = 0; i < 8; i++) {
        lv_label_set_text(ctx->line_labels[i], "");
    }
    if (ctx->modern_logo) lv_label_set_text(ctx->modern_logo, "");
    if (ctx->modern_header_title) lv_label_set_text(ctx->modern_header_title, "");
    if (ctx->modern_datetime) lv_label_set_text(ctx->modern_datetime, "");
    if (ctx->modern_min_value) lv_label_set_text(ctx->modern_min_value, "");
    if (ctx->modern_max_value) lv_label_set_text(ctx->modern_max_value, "");
    if (ctx->modern_pr1_value) lv_label_set_text(ctx->modern_pr1_value, "");
    if (ctx->modern_sp_value) lv_label_set_text(ctx->modern_sp_value, "");
    if (ctx->modern_pr2_status) lv_label_set_text(ctx->modern_pr2_status, "");
    if (ctx->modern_door_value) lv_label_set_text(ctx->modern_door_value, "");
    ctx->pr1_minmax_valid = false;
    display_st7789_ec11_poll(0);
}

void st7789_display_splash(st7789_context_t* ctx, const char* device_name) {
    if (!ctx || !ctx->initialized) {
        return;
    }

    (void)device_name;
    st7789_set_main_temp_layout(ctx, true);
    lv_image_set_src(ctx->modern_bg, &logo_320x240);

    if (ctx->modern_datetime) lv_label_set_text(ctx->modern_datetime, "");
    if (ctx->modern_pr1_value) lv_label_set_text(ctx->modern_pr1_value, "");
    if (ctx->modern_min_value) lv_label_set_text(ctx->modern_min_value, "");
    if (ctx->modern_max_value) lv_label_set_text(ctx->modern_max_value, "");
    if (ctx->modern_sp_value) lv_label_set_text(ctx->modern_sp_value, "");
    if (ctx->modern_pr2_status) lv_label_set_text(ctx->modern_pr2_status, "");
    if (ctx->modern_door_value) lv_label_set_text(ctx->modern_door_value, "");

    display_st7789_ec11_poll(0);
}

void st7789_next_screen(st7789_context_t* ctx) {
    if (!ctx) {
        return;
    }

    ctx->current_screen = (st7789_screen_t)((ctx->current_screen + 1) % ST7789_SCREEN_COUNT);
    if (ctx->current_screen != ST7789_SCREEN_MAIN_TEMP) {
        ctx->setpoint_edit_active = false;
        ctx->main_nav_active = false;
        ctx->main_reset_dialog_visible = false;
    }
    if (ctx->current_screen != ST7789_SCREEN_INFO) {
        ctx->hysteresis_password_active = false;
        ctx->hysteresis_unlocked = false;
        ctx->hysteresis_password_index = 0;
        ctx->hysteresis_password_current = 0;
        ctx->hysteresis_password_result_visible = false;
        ctx->hysteresis_password_last_ok = false;
        ctx->hysteresis_password_result_tick = 0;
        ctx->info_nav_active = false;
        ctx->logs_dialog_visible = false;
    }
    if (ctx->current_screen != ST7789_SCREEN_DIAGNOSTICS) {
        ctx->diag_nav_active = false;
        ctx->diag_avg_dialog_visible = false;
    }
}

void st7789_previous_screen(st7789_context_t* ctx) {
    if (!ctx) {
        return;
    }

    ctx->current_screen = (st7789_screen_t)((ctx->current_screen + ST7789_SCREEN_COUNT - 1) % ST7789_SCREEN_COUNT);
    if (ctx->current_screen != ST7789_SCREEN_MAIN_TEMP) {
        ctx->setpoint_edit_active = false;
        ctx->main_nav_active = false;
        ctx->main_reset_dialog_visible = false;
    }
    if (ctx->current_screen != ST7789_SCREEN_INFO) {
        ctx->hysteresis_password_active = false;
        ctx->hysteresis_unlocked = false;
        ctx->hysteresis_password_index = 0;
        ctx->hysteresis_password_current = 0;
        ctx->hysteresis_password_result_visible = false;
        ctx->hysteresis_password_last_ok = false;
        ctx->hysteresis_password_result_tick = 0;
        ctx->info_nav_active = false;
        ctx->logs_dialog_visible = false;
    }
    if (ctx->current_screen != ST7789_SCREEN_DIAGNOSTICS) {
        ctx->diag_nav_active = false;
        ctx->diag_avg_dialog_visible = false;
    }
}

void st7789_increment_setpoint(st7789_context_t* ctx) {
    if (!ctx) {
        return;
    }

    ctx->setpoint += 0.5f;
    if (ctx->setpoint > 10.0f) {
        ctx->setpoint = 10.0f;
    }
}

void st7789_increment_hysteresis(st7789_context_t* ctx) {
    if (!ctx) {
        return;
    }

    ctx->hysteresis += 0.1f;
    if (ctx->hysteresis > 5.0f) {
        ctx->hysteresis = 5.0f;
    }
}

void st7789_decrement_setpoint(st7789_context_t* ctx) {
    if (!ctx) {
        return;
    }

    ctx->setpoint -= 0.5f;
    if (ctx->setpoint < -10.0f) {
        ctx->setpoint = -10.0f;
    }
}

void st7789_decrement_hysteresis(st7789_context_t* ctx) {
    if (!ctx) {
        return;
    }

    ctx->hysteresis -= 0.1f;
    if (ctx->hysteresis < 0.1f) {
        ctx->hysteresis = 0.1f;
    }
}

float st7789_get_setpoint(st7789_context_t* ctx) {
    return ctx ? ctx->setpoint : 0.0f;
}

float st7789_get_hysteresis(st7789_context_t* ctx) {
    return ctx ? ctx->hysteresis : 0.6f;
}

void st7789_update_current_screen(st7789_context_t* ctx, const char* device_name,
                                  const modbus_data_t* data, uint32_t record_count,
                                  bool lamp_on, bool dialer_on, bool compressor_on,
                                  bool heater_on, bool door_open,
                                  bool slave2_t1_valid, float slave2_t1,
                                  bool slave2_t2_valid, float slave2_t2) {
    st7789_render_screen(ctx, device_name, data, record_count, lamp_on, dialer_on,
                         compressor_on, heater_on, door_open,
                         slave2_t1_valid, slave2_t1, slave2_t2_valid, slave2_t2);
}

void st7789_process(st7789_context_t* ctx) {
    if (!ctx || !ctx->initialized) {
        return;
    }

    display_st7789_ec11_poll(0);
}

int32_t st7789_consume_encoder_diff(st7789_context_t* ctx) {
    if (!ctx || !ctx->initialized) {
        return 0;
    }

    return display_st7789_ec11_consume_encoder_diff();
}

bool st7789_consume_encoder_click(st7789_context_t* ctx) {
    if (!ctx || !ctx->initialized) {
        return false;
    }

    return display_st7789_ec11_consume_button_click();
}

bool st7789_consume_exit_button_click(st7789_context_t* ctx) {
    if (!ctx || !ctx->initialized) {
        return false;
    }
    return display_st7789_ec11_consume_exit_button_click();
}

bool st7789_is_exit_button_pressed(st7789_context_t* ctx) {
    if (!ctx || !ctx->initialized) {
        return false;
    }
    return display_st7789_ec11_is_exit_button_pressed();
}

bool st7789_is_setpoint_edit_active(st7789_context_t* ctx) {
    if (!ctx || !ctx->initialized) {
        return false;
    }
    return ctx->setpoint_edit_active;
}

void st7789_set_setpoint_edit_active(st7789_context_t* ctx, bool active) {
    if (!ctx || !ctx->initialized) {
        return;
    }
    if (ctx->current_screen != ST7789_SCREEN_MAIN_TEMP &&
        ctx->current_screen != ST7789_SCREEN_INFO) {
        ctx->setpoint_edit_active = false;
        return;
    }
    if (ctx->current_screen == ST7789_SCREEN_INFO && active && !ctx->hysteresis_unlocked) {
        ctx->setpoint_edit_active = false;
        return;
    }
    if (ctx->current_screen == ST7789_SCREEN_INFO && !active) {
        ctx->hysteresis_unlocked = false;
    }
    if (ctx->current_screen == ST7789_SCREEN_MAIN_TEMP && active) {
        ctx->main_nav_active = false;
    }
    ctx->setpoint_edit_active = active;
    st7789_update_main_value_selection(ctx);
}

bool st7789_is_hysteresis_password_active(st7789_context_t* ctx) {
    if (!ctx || !ctx->initialized) {
        return false;
    }
    return ctx->hysteresis_password_active;
}

bool st7789_is_hysteresis_password_result_visible(st7789_context_t* ctx) {
    if (!ctx || !ctx->initialized) {
        return false;
    }
    return ctx->hysteresis_password_result_visible;
}

bool st7789_consume_hysteresis_password_result(st7789_context_t* ctx) {
    if (!ctx || !ctx->initialized || !ctx->hysteresis_password_result_visible) {
        return false;
    }
    if ((lv_tick_get() - ctx->hysteresis_password_result_tick) < 1000U) {
        return false;
    }

    bool ok = ctx->hysteresis_password_last_ok;
    ctx->hysteresis_password_result_visible = false;
    ctx->hysteresis_password_last_ok = false;
    ctx->hysteresis_password_result_tick = 0;
    return ok;
}

bool st7789_is_encoder_button_pressed(st7789_context_t* ctx) {
    if (!ctx || !ctx->initialized) {
        return false;
    }
    return display_st7789_ec11_is_button_pressed();
}

bool st7789_is_info_nav_active(st7789_context_t* ctx) {
    if (!ctx || !ctx->initialized) return false;
    return ctx->info_nav_active;
}

void st7789_set_info_nav_active(st7789_context_t* ctx, bool active) {
    if (!ctx || !ctx->initialized) return;
    ctx->info_nav_active = active;
    st7789_update_info_card_selection(ctx);
}

void st7789_move_info_selection(st7789_context_t* ctx, int32_t diff) {
    if (!ctx || !ctx->initialized || !ctx->info_nav_active) return;
    while (diff > 0) {
        ctx->info_selected_item = (st7789_info_item_t)((ctx->info_selected_item + 1) % 4);
        diff--;
    }
    while (diff < 0) {
        if (ctx->info_selected_item == ST7789_INFO_ITEM_AC) {
            ctx->info_selected_item = ST7789_INFO_ITEM_LOGS;
        } else {
            ctx->info_selected_item = (st7789_info_item_t)(ctx->info_selected_item - 1);
        }
        diff++;
    }
    st7789_update_info_card_selection(ctx);
}

st7789_info_item_t st7789_get_info_selection(st7789_context_t* ctx) {
    if (!ctx || !ctx->initialized) return ST7789_INFO_ITEM_AC;
    return ctx->info_selected_item;
}

void st7789_set_logs_reset_dialog(st7789_context_t* ctx, bool visible, const char* message) {
    if (!ctx || !ctx->initialized) return;
    ctx->logs_dialog_visible = visible;
    if (ctx->logs_status && message && message[0]) {
        lv_label_set_text(ctx->logs_status, message);
    }
}

bool st7789_is_logs_reset_dialog_visible(st7789_context_t* ctx) {
    if (!ctx || !ctx->initialized) return false;
    return ctx->logs_dialog_visible;
}

bool st7789_is_main_nav_active(st7789_context_t* ctx) {
    if (!ctx || !ctx->initialized) return false;
    return ctx->main_nav_active;
}

void st7789_set_main_nav_active(st7789_context_t* ctx, bool active) {
    if (!ctx || !ctx->initialized) return;
    ctx->main_nav_active = active;
    st7789_update_main_value_selection(ctx);
}

void st7789_move_main_selection(st7789_context_t* ctx, int32_t diff) {
    if (!ctx || !ctx->initialized || !ctx->main_nav_active) return;
    while (diff > 0) {
        ctx->main_selected_item = (st7789_main_item_t)((ctx->main_selected_item + 1) % 5);
        diff--;
    }
    while (diff < 0) {
        if (ctx->main_selected_item == ST7789_MAIN_ITEM_OFFSET_PRINCIPAL) {
            ctx->main_selected_item = ST7789_MAIN_ITEM_MAX;
        } else {
            ctx->main_selected_item = (st7789_main_item_t)(ctx->main_selected_item - 1);
        }
        diff++;
    }
    st7789_update_main_value_selection(ctx);
}

st7789_main_item_t st7789_get_main_selection(st7789_context_t* ctx) {
    if (!ctx || !ctx->initialized) return ST7789_MAIN_ITEM_SETPOINT;
    return ctx->main_selected_item;
}

bool st7789_is_diag_nav_active(st7789_context_t* ctx) {
    if (!ctx || !ctx->initialized) return false;
    return ctx->diag_nav_active;
}

void st7789_set_diag_nav_active(st7789_context_t* ctx, bool active) {
    if (!ctx || !ctx->initialized) return;
    ctx->diag_nav_active = active;
    st7789_update_diag_card_selection(ctx);
}

void st7789_move_diag_selection(st7789_context_t* ctx, int32_t diff) {
    if (!ctx || !ctx->initialized || !ctx->diag_nav_active) return;
    while (diff > 0) {
        ctx->diag_selected_item = (st7789_diag_item_t)((ctx->diag_selected_item + 1) % 4);
        diff--;
    }
    while (diff < 0) {
        if (ctx->diag_selected_item == ST7789_DIAG_ITEM_COMPRESSOR) {
            ctx->diag_selected_item = ST7789_DIAG_ITEM_DIALER;
        } else {
            ctx->diag_selected_item = (st7789_diag_item_t)(ctx->diag_selected_item - 1);
        }
        diff++;
    }
    st7789_update_diag_card_selection(ctx);
}

st7789_diag_item_t st7789_get_diag_selection(st7789_context_t* ctx) {
    if (!ctx || !ctx->initialized) return ST7789_DIAG_ITEM_COMPRESSOR;
    return ctx->diag_selected_item;
}

void st7789_set_diag_compressor_avg_dialog(st7789_context_t* ctx, bool visible) {
    if (!ctx || !ctx->initialized) return;
    ctx->diag_avg_dialog_visible = visible;
}

bool st7789_is_diag_compressor_avg_dialog_visible(st7789_context_t* ctx) {
    if (!ctx || !ctx->initialized) return false;
    return ctx->diag_avg_dialog_visible;
}

void st7789_set_main_minmax_reset_dialog(st7789_context_t* ctx, bool visible, const char* message) {
    if (!ctx || !ctx->initialized) return;
    ctx->main_reset_dialog_visible = visible;
    if (ctx->main_reset_status && message && message[0]) {
        lv_label_set_text(ctx->main_reset_status, message);
    }
}

bool st7789_is_main_minmax_reset_dialog_visible(st7789_context_t* ctx) {
    if (!ctx || !ctx->initialized) return false;
    return ctx->main_reset_dialog_visible;
}

bool st7789_reset_main_minmax_from_current_temp(st7789_context_t* ctx) {
    if (!ctx || !ctx->initialized || !ctx->last_pr1_valid) return false;
    ctx->pr1_min = ctx->last_pr1_value;
    ctx->pr1_max = ctx->last_pr1_value;
    ctx->pr1_minmax_valid = true;
    return true;
}

void st7789_increment_main_selected_value(st7789_context_t* ctx) {
    if (!ctx || !ctx->initialized) return;
    switch (ctx->main_selected_item) {
        case ST7789_MAIN_ITEM_OFFSET_PRINCIPAL:
            ctx->offset_principal += 0.1f;
            if (ctx->offset_principal > 20.0f) ctx->offset_principal = 20.0f;
            break;
        case ST7789_MAIN_ITEM_OFFSET_DEGELO:
            ctx->offset_degelo += 0.1f;
            if (ctx->offset_degelo > 20.0f) ctx->offset_degelo = 20.0f;
            break;
        case ST7789_MAIN_ITEM_SETPOINT:
            st7789_increment_setpoint(ctx);
            break;
        default:
            break;
    }
}

void st7789_decrement_main_selected_value(st7789_context_t* ctx) {
    if (!ctx || !ctx->initialized) return;
    switch (ctx->main_selected_item) {
        case ST7789_MAIN_ITEM_OFFSET_PRINCIPAL:
            ctx->offset_principal -= 0.1f;
            if (ctx->offset_principal < -20.0f) ctx->offset_principal = -20.0f;
            break;
        case ST7789_MAIN_ITEM_OFFSET_DEGELO:
            ctx->offset_degelo -= 0.1f;
            if (ctx->offset_degelo < -20.0f) ctx->offset_degelo = -20.0f;
            break;
        case ST7789_MAIN_ITEM_SETPOINT:
            st7789_decrement_setpoint(ctx);
            break;
        default:
            break;
    }
}

float st7789_get_offset_principal(st7789_context_t* ctx) {
    return ctx ? ctx->offset_principal : 0.0f;
}

float st7789_get_offset_degelo(st7789_context_t* ctx) {
    return ctx ? ctx->offset_degelo : 0.0f;
}

void st7789_start_offset_editor(st7789_context_t* ctx, st7789_offset_target_t target) {
    if (!ctx || !ctx->initialized) return;
    ctx->offset_editor_target = target;
    ctx->offset_editor_active = true;
    if (target == ST7789_OFFSET_TARGET_PRINCIPAL) {
        st7789_offset_editor_from_value(ctx, ctx->offset_principal);
    } else {
        st7789_offset_editor_from_value(ctx, ctx->offset_degelo);
    }
}

bool st7789_is_offset_editor_active(st7789_context_t* ctx) {
    if (!ctx || !ctx->initialized) return false;
    return ctx->offset_editor_active;
}

void st7789_increment_offset_editor_digit(st7789_context_t* ctx) {
    if (!ctx || !ctx->initialized || !ctx->offset_editor_active) return;
    if (ctx->offset_editor_pos == 0) {
        ctx->offset_editor_negative = !ctx->offset_editor_negative;
    } else {
        uint8_t idx = (uint8_t)(ctx->offset_editor_pos - 1);
        ctx->offset_editor_digits[idx] = (uint8_t)((ctx->offset_editor_digits[idx] + 1) % 10);
    }
}

void st7789_decrement_offset_editor_digit(st7789_context_t* ctx) {
    if (!ctx || !ctx->initialized || !ctx->offset_editor_active) return;
    if (ctx->offset_editor_pos == 0) {
        ctx->offset_editor_negative = !ctx->offset_editor_negative;
    } else {
        uint8_t idx = (uint8_t)(ctx->offset_editor_pos - 1);
        ctx->offset_editor_digits[idx] = (uint8_t)((ctx->offset_editor_digits[idx] + 9) % 10);
    }
}

bool st7789_confirm_offset_editor_digit(st7789_context_t* ctx) {
    if (!ctx || !ctx->initialized || !ctx->offset_editor_active) return false;
    if (ctx->offset_editor_pos < 4) {
        ctx->offset_editor_pos++;
        return false;
    }
    float val = st7789_offset_editor_to_value(ctx);
    if (ctx->offset_editor_target == ST7789_OFFSET_TARGET_PRINCIPAL) {
        ctx->offset_principal = val;
    } else {
        ctx->offset_degelo = val;
    }
    ctx->offset_editor_active = false;
    return true;
}

void st7789_start_hysteresis_password(st7789_context_t* ctx) {
    if (!ctx || !ctx->initialized ||
        (ctx->current_screen != ST7789_SCREEN_INFO &&
         ctx->current_screen != ST7789_SCREEN_MAIN_TEMP &&
         ctx->current_screen != ST7789_SCREEN_DIAGNOSTICS)) {
        return;
    }
    ctx->hysteresis_password_active = true;
    ctx->hysteresis_unlocked = false;
    ctx->hysteresis_password_error = false;
    ctx->hysteresis_password_result_visible = false;
    ctx->hysteresis_password_last_ok = false;
    ctx->hysteresis_password_result_tick = 0;
    ctx->hysteresis_password_index = 0;
    ctx->hysteresis_password_current = 0;
    memset(ctx->hysteresis_password_digits, 0, sizeof(ctx->hysteresis_password_digits));
}

void st7789_increment_hysteresis_password_digit(st7789_context_t* ctx) {
    if (!ctx || !ctx->initialized || !ctx->hysteresis_password_active) {
        return;
    }
    ctx->hysteresis_password_current = (uint8_t)((ctx->hysteresis_password_current + 1) % 10);
}

void st7789_decrement_hysteresis_password_digit(st7789_context_t* ctx) {
    if (!ctx || !ctx->initialized || !ctx->hysteresis_password_active) {
        return;
    }
    ctx->hysteresis_password_current =
        (uint8_t)((ctx->hysteresis_password_current + 9) % 10);
}

bool st7789_confirm_hysteresis_password_digit(st7789_context_t* ctx) {
    if (!ctx || !ctx->initialized || !ctx->hysteresis_password_active) {
        return false;
    }

    if (ctx->hysteresis_password_index < 4) {
        ctx->hysteresis_password_digits[ctx->hysteresis_password_index++] =
            ctx->hysteresis_password_current;
        ctx->hysteresis_password_current = 0;
    }

    if (ctx->hysteresis_password_index < 4) {
        return false;
    }

    bool valid = (ctx->hysteresis_password_digits[0] == (HYSTERESIS_PASSWORD_CODE[0] - '0')) &&
                 (ctx->hysteresis_password_digits[1] == (HYSTERESIS_PASSWORD_CODE[1] - '0')) &&
                 (ctx->hysteresis_password_digits[2] == (HYSTERESIS_PASSWORD_CODE[2] - '0')) &&
                 (ctx->hysteresis_password_digits[3] == (HYSTERESIS_PASSWORD_CODE[3] - '0'));

    ctx->hysteresis_password_active = false;
    ctx->hysteresis_password_index = 0;
    ctx->hysteresis_password_current = 0;
    memset(ctx->hysteresis_password_digits, 0, sizeof(ctx->hysteresis_password_digits));
    ctx->hysteresis_unlocked = valid;
    ctx->hysteresis_password_error = !valid;
    ctx->hysteresis_password_result_visible = true;
    ctx->hysteresis_password_last_ok = valid;
    ctx->hysteresis_password_result_tick = lv_tick_get();

    return valid;
}

st7789_screen_t st7789_get_current_screen(st7789_context_t* ctx) {
    if (!ctx || !ctx->initialized) {
        return ST7789_SCREEN_MAIN_TEMP;
    }
    return ctx->current_screen;
}

void st7789_cancel_edit_modes(st7789_context_t* ctx) {
    if (!ctx || !ctx->initialized) {
        return;
    }
    ctx->setpoint_edit_active = false;
    ctx->info_nav_active = false;
    ctx->hysteresis_password_active = false;
    ctx->hysteresis_password_result_visible = false;
    ctx->hysteresis_unlocked = false;
    ctx->hysteresis_password_error = false;
    ctx->logs_dialog_visible = false;
    ctx->main_nav_active = false;
    ctx->diag_nav_active = false;
    ctx->diag_avg_dialog_visible = false;
    ctx->main_reset_dialog_visible = false;
    ctx->offset_editor_active = false;
    ctx->hysteresis_password_index = 0;
    ctx->hysteresis_password_current = 0;
    memset(ctx->hysteresis_password_digits, 0, sizeof(ctx->hysteresis_password_digits));
    st7789_update_info_card_selection(ctx);
    st7789_update_main_value_selection(ctx);
    st7789_update_diag_card_selection(ctx);
}

void st7789_set_top_status(st7789_context_t* ctx, const char* text) {
    if (!ctx || !ctx->initialized) {
        return;
    }
    if (text && text[0]) {
        snprintf(ctx->top_status_text, sizeof(ctx->top_status_text), "%s", text);
    } else {
        ctx->top_status_text[0] = '\0';
    }
}
