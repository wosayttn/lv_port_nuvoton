/**************************************************************************//**
 * @file     ui_screen_finish.c
 * @brief    Rice Cooker UI - Finish / Keep Warm screen
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#include "ui_common.h"
#include "ui_buzzer.h"

static lv_obj_t   *s_scr_finish = NULL;
static lv_obj_t   *s_lbl_temp   = NULL;
static lv_timer_t *s_timer      = NULL;

/*============================================================================
 * Timer callback – keep warm display refresh
 *============================================================================*/
static void finish_timer_cb(lv_timer_t *t)
{
    (void)t;

    if (s_lbl_temp == NULL) return;

    /* Simulate hold at keep-warm temperature (~70°C) */
    if (g_cook_ctx.temperature > 700)
        g_cook_ctx.temperature -= 2;
    else if (g_cook_ctx.temperature < 700)
        g_cook_ctx.temperature += 2;

    lv_label_set_text_fmt(s_lbl_temp, "Keep Warm: %d.%d " LV_SYMBOL_CHARGE "C",
                          g_cook_ctx.temperature / 10,
                          g_cook_ctx.temperature % 10);
}

/*============================================================================
 * Key handler
 *============================================================================*/
static void finish_home_btn_cb(lv_event_t *e)
{
    (void)e;
    if (s_timer) { lv_timer_delete(s_timer); s_timer = NULL; }
    s_lbl_temp = NULL;
    s_scr_finish = NULL;
    ui_switch_state(UI_STATE_HOME);
}

static void finish_key_cb(lv_event_t *e)
{
    ui_key_t key = (ui_key_t)(uintptr_t)lv_event_get_param(e);

    if (key == UI_KEY_MENU || key == UI_KEY_START)
    {
        if (s_timer) { lv_timer_delete(s_timer); s_timer = NULL; }
        s_lbl_temp = NULL;
        s_scr_finish = NULL;
        ui_switch_state(UI_STATE_HOME);
    }
}

/*============================================================================
 * Create
 *============================================================================*/
void ui_screen_finish_create(void)
{
    /* Buzzer: play "cooking done" melody */
    ui_buzzer_play_done();

    s_scr_finish = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(s_scr_finish, UI_COLOR_BG, 0);
    lv_obj_set_style_bg_opa(s_scr_finish, LV_OPA_COVER, 0);
    lv_obj_clear_flag(s_scr_finish, LV_OBJ_FLAG_SCROLLABLE);

    /* Status bar */
    ui_create_status_bar(s_scr_finish, "Complete!");

    /* Content */
    lv_obj_t *cont = lv_obj_create(s_scr_finish);
    lv_obj_set_size(cont, UI_HOR_RES - 20, UI_VER_RES - UI_STATUS_BAR_H - UI_SOFTKEY_BAR_H - 10);
    lv_obj_align(cont, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* Done icon */
    lv_obj_t *lbl_done = lv_label_create(cont);
    lv_label_set_text(lbl_done, LV_SYMBOL_OK "  Cooking Done!");
    lv_obj_set_style_text_color(lbl_done, UI_COLOR_SUCCESS, 0);
    lv_obj_set_style_text_font(lbl_done, &lv_font_montserrat_24, 0);

    /* Temperature / Keep Warm */
    s_lbl_temp = lv_label_create(cont);
    lv_label_set_text_fmt(s_lbl_temp, "Keep Warm: %d.%d " LV_SYMBOL_CHARGE "C",
                          g_cook_ctx.temperature / 10,
                          g_cook_ctx.temperature % 10);
    lv_obj_set_style_text_color(s_lbl_temp, UI_COLOR_ACCENT, 0);
    lv_obj_set_style_text_font(s_lbl_temp, &lv_font_montserrat_16, 0);

    /* Touch: Home button */
    lv_obj_t *btn_home = lv_button_create(cont);
    lv_obj_set_size(btn_home, 120, 36);
    lv_obj_set_style_bg_color(btn_home, UI_COLOR_PRIMARY, 0);
    lv_obj_set_style_radius(btn_home, 6, 0);
    lv_obj_add_event_cb(btn_home, finish_home_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *hl = lv_label_create(btn_home);
    lv_label_set_text(hl, LV_SYMBOL_HOME "  Home");
    lv_obj_set_style_text_color(hl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(hl);

    /* Softkey bar */
    ui_create_softkey_bar(s_scr_finish, "[MENU] or [START] Return Home");

    /* Key handler */
    lv_obj_add_event_cb(s_scr_finish, finish_key_cb, LV_EVENT_KEY, NULL);

    /* Keep warm timer (refresh every 2s) */
    s_timer = lv_timer_create(finish_timer_cb, 2000, NULL);

    /* Load screen */
    lv_screen_load_anim(s_scr_finish, LV_SCREEN_LOAD_ANIM_FADE_IN, 300, 0, true);
}
