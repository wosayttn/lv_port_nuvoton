/**************************************************************************//**
 * @file     ui_screen_home.c
 * @brief    Rice Cooker UI - Home / Ready screen
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#include "ui_common.h"
#include "ui_clock.h"

static lv_obj_t *s_scr_home = NULL;

/*============================================================================
 * Touch callback
 *============================================================================*/
static void home_menu_btn_cb(lv_event_t *e)
{
    (void)e;
    ui_switch_state(UI_STATE_MENU);
}

static void home_lang_btn_cb(lv_event_t *e)
{
    (void)e;
    ui_lang_next();
    /* Reload home screen with new language */
    ui_switch_state(UI_STATE_HOME);
}

static void home_perf_btn_cb(lv_event_t *e)
{
    (void)e;
    ui_perf_monitor_toggle();
}

/*============================================================================
 * Key handler
 *============================================================================*/
static void home_key_cb(lv_event_t *e)
{
    ui_key_t key = (ui_key_t)(uintptr_t)lv_event_get_param(e);

    if (key == UI_KEY_MENU)
    {
        ui_switch_state(UI_STATE_MENU);
    }
}

/*============================================================================
 * Create
 *============================================================================*/
void ui_screen_home_create(void)
{
    s_scr_home = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(s_scr_home, UI_COLOR_BG, 0);
    lv_obj_set_style_bg_opa(s_scr_home, LV_OPA_COVER, 0);
    lv_obj_clear_flag(s_scr_home, LV_OBJ_FLAG_SCROLLABLE);

    /* Status bar */
    ui_create_status_bar(s_scr_home, ui_str(STR_APP_TITLE));

    /* Center content area */
    lv_obj_t *cont = lv_obj_create(s_scr_home);
    lv_obj_set_size(cont, UI_HOR_RES - 20, UI_VER_RES - UI_STATUS_BAR_H - UI_SOFTKEY_BAR_H - 10);
    lv_obj_align(cont, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* Ready icon / text */
    lv_obj_t *lbl_ready = lv_label_create(cont);
    lv_label_set_text(lbl_ready, ui_str(STR_READY));
    lv_obj_set_style_text_color(lbl_ready, UI_COLOR_SUCCESS, 0);
    lv_obj_set_style_text_font(lbl_ready,
                               (ui_lang_get() == UI_LANG_EN) ? ui_lang_font_normal() : ui_lang_font_large(),
                               0);

    /* Current time (live RTC) */
    lv_obj_t *lbl_time = lv_label_create(cont);
    lv_label_set_text(lbl_time, "--:--:--");
    lv_obj_set_style_text_color(lbl_time, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(lbl_time, &lv_font_montserrat_20, 0);
    ui_clock_register_time_label(lbl_time);

    /* Touch: Menu button */
    lv_obj_t *btn_menu = lv_button_create(cont);
    lv_obj_set_size(btn_menu, 120, 36);
    lv_obj_set_style_bg_color(btn_menu, UI_COLOR_PRIMARY, 0);
    lv_obj_set_style_radius(btn_menu, 6, 0);
    lv_obj_add_event_cb(btn_menu, home_menu_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *btn_lbl = lv_label_create(btn_menu);
    lv_label_set_text(btn_lbl, ui_str(STR_BTN_MENU));
    lv_obj_set_style_text_color(btn_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(btn_lbl, ui_lang_font_normal(), 0);
    lv_obj_center(btn_lbl);

    /* Softkey bar */
    ui_create_softkey_bar(s_scr_home, ui_str(STR_HINT_HOME));

    /* Touch: Language switch button (bottom-right, above softkey bar) */
    lv_obj_t *btn_lang = lv_button_create(s_scr_home);
    lv_obj_set_size(btn_lang, 50, 24);
    lv_obj_align(btn_lang, LV_ALIGN_BOTTOM_RIGHT, -10, -UI_SOFTKEY_BAR_H - 4);
    lv_obj_set_style_bg_color(btn_lang, UI_COLOR_STATUS_BG, 0);
    lv_obj_set_style_radius(btn_lang, 4, 0);
    lv_obj_add_event_cb(btn_lang, home_lang_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lang_lbl = lv_label_create(btn_lang);
    const char *lang_names[] = {"EN", "JA", "TW", "CN", "KO", "DE"};
    lv_label_set_text(lang_lbl, lang_names[ui_lang_get()]);
    lv_obj_set_style_text_color(lang_lbl, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(lang_lbl, &lv_font_montserrat_12, 0);
    lv_obj_center(lang_lbl);

    /* Touch: PERF toggle button (bottom-left, above softkey bar) */
    lv_obj_t *btn_perf = lv_button_create(s_scr_home);
    lv_obj_set_size(btn_perf, 50, 24);
    lv_obj_align(btn_perf, LV_ALIGN_BOTTOM_LEFT, 10, -UI_SOFTKEY_BAR_H - 4);
    lv_obj_set_style_bg_color(btn_perf, lv_color_hex(0x444444), 0);
    lv_obj_set_style_radius(btn_perf, 4, 0);
    lv_obj_add_event_cb(btn_perf, home_perf_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *perf_lbl = lv_label_create(btn_perf);
    lv_label_set_text(perf_lbl, "PERF");
    lv_obj_set_style_text_color(perf_lbl, lv_color_hex(0x00FF00), 0);
    lv_obj_set_style_text_font(perf_lbl, &lv_font_montserrat_12, 0);
    lv_obj_center(perf_lbl);

    /* Register key handler */
    lv_obj_add_event_cb(s_scr_home, home_key_cb, LV_EVENT_KEY, NULL);

    /* Load screen */
    lv_screen_load_anim(s_scr_home, LV_SCREEN_LOAD_ANIM_FADE_IN, 200, 0, true);
}
