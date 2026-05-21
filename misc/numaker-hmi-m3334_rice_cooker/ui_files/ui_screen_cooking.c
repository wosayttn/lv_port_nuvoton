/**************************************************************************//**
 * @file     ui_screen_cooking.c
 * @brief    Rice Cooker UI - Cooking screen (countdown + temperature)
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#include "ui_common.h"

static lv_obj_t   *s_scr_cook    = NULL;
static lv_obj_t   *s_lbl_time    = NULL;
static lv_obj_t   *s_lbl_temp    = NULL;
static lv_obj_t   *s_lbl_mode    = NULL;
static lv_obj_t   *s_bar_progress = NULL;
static lv_timer_t *s_timer       = NULL;

/*============================================================================
 * Timer callback – runs every 1 s
 *============================================================================*/
static void cooking_timer_cb(lv_timer_t *t)
{
    (void)t;

    /* Guard: if screen already left, do nothing */
    if (s_lbl_time == NULL) return;

    if (g_cook_ctx.is_paused)
        return;

    if (g_cook_ctx.remaining_seconds > 0)
    {
        g_cook_ctx.remaining_seconds--;

        /* Simulate temperature ramp-up (simple linear) */
        if (g_cook_ctx.temperature < 1000) /* max 100.0 °C */
            g_cook_ctx.temperature += 5;
    }
    else
    {
        /* Cooking done – stop timer first, then switch to finish screen */
        lv_timer_delete(s_timer);
        s_timer = NULL;
        ui_switch_state(UI_STATE_FINISH);
        return;
    }

    /* Update labels */
    uint32_t min = g_cook_ctx.remaining_seconds / 60;
    uint32_t sec = g_cook_ctx.remaining_seconds % 60;
    lv_label_set_text_fmt(s_lbl_time, "%02lu:%02lu", (unsigned long)min, (unsigned long)sec);
    lv_label_set_text_fmt(s_lbl_temp, "%d.%d " LV_SYMBOL_CHARGE "C",
                          g_cook_ctx.temperature / 10,
                          g_cook_ctx.temperature % 10);

    /* Progress bar */
    if (g_cook_ctx.total_seconds > 0)
    {
        int32_t pct = 100 - (int32_t)(g_cook_ctx.remaining_seconds * 100 / g_cook_ctx.total_seconds);
        lv_bar_set_value(s_bar_progress, pct, LV_ANIM_ON);
    }
}

/*============================================================================
 * Touch callbacks
 *============================================================================*/
static void cooking_pause_btn_cb(lv_event_t *e)
{
    (void)e;
    g_cook_ctx.is_paused = !g_cook_ctx.is_paused;
    if (g_cook_ctx.is_paused)
        lv_label_set_text(s_lbl_mode, "PAUSED");
    else
        lv_label_set_text_fmt(s_lbl_mode, "%s", cook_mode_names[g_cook_ctx.mode]);
}

static void cooking_stop_btn_cb(lv_event_t *e)
{
    (void)e;
    if (s_timer) { lv_timer_delete(s_timer); s_timer = NULL; }
    ui_switch_state(UI_STATE_HOME);
}

/*============================================================================
 * Key handler
 *============================================================================*/
static void cooking_key_cb(lv_event_t *e)
{
    ui_key_t key = (ui_key_t)(uintptr_t)lv_event_get_param(e);

    switch (key)
    {
        case UI_KEY_START:
            /* Toggle pause / resume */
            g_cook_ctx.is_paused = !g_cook_ctx.is_paused;
            if (g_cook_ctx.is_paused)
                lv_label_set_text(s_lbl_mode, "PAUSED");
            else
                lv_label_set_text_fmt(s_lbl_mode, "%s", cook_mode_names[g_cook_ctx.mode]);
            break;

        case UI_KEY_MENU:
            /* Stop cooking, back to home */
            if (s_timer) { lv_timer_delete(s_timer); s_timer = NULL; }
            ui_switch_state(UI_STATE_HOME);
            break;

        default:
            break;
    }
}

/*============================================================================
 * Create
 *============================================================================*/
void ui_screen_cooking_create(void)
{
    s_scr_cook = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(s_scr_cook, UI_COLOR_BG, 0);
    lv_obj_set_style_bg_opa(s_scr_cook, LV_OPA_COVER, 0);
    lv_obj_clear_flag(s_scr_cook, LV_OBJ_FLAG_SCROLLABLE);

    /* Status bar */
    ui_create_status_bar(s_scr_cook, "Cooking");

    /* Content area */
    lv_obj_t *cont = lv_obj_create(s_scr_cook);
    lv_obj_set_size(cont, UI_HOR_RES - 20, UI_VER_RES - UI_STATUS_BAR_H - UI_SOFTKEY_BAR_H - 10);
    lv_obj_align(cont, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

    /* Mode name */
    s_lbl_mode = lv_label_create(cont);
    lv_label_set_text(s_lbl_mode, cook_mode_names[g_cook_ctx.mode]);
    lv_obj_set_style_text_color(s_lbl_mode, UI_COLOR_ACCENT, 0);
    lv_obj_set_style_text_font(s_lbl_mode, &lv_font_montserrat_16, 0);
    lv_obj_align(s_lbl_mode, LV_ALIGN_TOP_MID, 0, 5);

    /* Large countdown */
    s_lbl_time = lv_label_create(cont);
    uint32_t min = g_cook_ctx.remaining_seconds / 60;
    uint32_t sec = g_cook_ctx.remaining_seconds % 60;
    lv_label_set_text_fmt(s_lbl_time, "%02lu:%02lu", (unsigned long)min, (unsigned long)sec);
    lv_obj_set_style_text_color(s_lbl_time, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(s_lbl_time, &lv_font_montserrat_24, 0);
    lv_obj_align(s_lbl_time, LV_ALIGN_CENTER, 0, -15);

    /* Temperature */
    s_lbl_temp = lv_label_create(cont);
    lv_label_set_text_fmt(s_lbl_temp, "%d.%d " LV_SYMBOL_CHARGE "C",
                          g_cook_ctx.temperature / 10,
                          g_cook_ctx.temperature % 10);
    lv_obj_set_style_text_color(s_lbl_temp, UI_COLOR_DANGER, 0);
    lv_obj_set_style_text_font(s_lbl_temp, &lv_font_montserrat_16, 0);
    lv_obj_align(s_lbl_temp, LV_ALIGN_CENTER, 0, 10);

    /* Progress bar */
    s_bar_progress = lv_bar_create(cont);
    lv_obj_set_size(s_bar_progress, UI_HOR_RES - 80, 12);
    lv_obj_align(s_bar_progress, LV_ALIGN_BOTTOM_MID, 0, -50);
    lv_bar_set_range(s_bar_progress, 0, 100);
    lv_bar_set_value(s_bar_progress, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(s_bar_progress, UI_COLOR_STATUS_BG, 0);
    lv_obj_set_style_bg_color(s_bar_progress, UI_COLOR_PRIMARY, LV_PART_INDICATOR);

    /* Touch: Pause button – placed inside content area */
    lv_obj_t *btn_pause = lv_button_create(cont);
    lv_obj_set_size(btn_pause, 120, 38);
    lv_obj_align(btn_pause, LV_ALIGN_BOTTOM_LEFT, 10, -5);
    lv_obj_set_style_bg_color(btn_pause, UI_COLOR_ACCENT, 0);
    lv_obj_set_style_radius(btn_pause, 6, 0);
    lv_obj_add_event_cb(btn_pause, cooking_pause_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *pl = lv_label_create(btn_pause);
    lv_label_set_text(pl, LV_SYMBOL_PAUSE " Pause");
    lv_obj_set_style_text_color(pl, lv_color_hex(0x000000), 0);
    lv_obj_set_style_text_font(pl, &lv_font_montserrat_16, 0);
    lv_obj_center(pl);

    /* Touch: Stop button – placed inside content area */
    lv_obj_t *btn_stop = lv_button_create(cont);
    lv_obj_set_size(btn_stop, 120, 38);
    lv_obj_align(btn_stop, LV_ALIGN_BOTTOM_RIGHT, -10, -5);
    lv_obj_set_style_bg_color(btn_stop, UI_COLOR_DANGER, 0);
    lv_obj_set_style_radius(btn_stop, 6, 0);
    lv_obj_add_event_cb(btn_stop, cooking_stop_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *sl = lv_label_create(btn_stop);
    lv_label_set_text(sl, LV_SYMBOL_CLOSE " Stop");
    lv_obj_set_style_text_color(sl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(sl, &lv_font_montserrat_16, 0);
    lv_obj_center(sl);

    /* Softkey bar */
    ui_create_softkey_bar(s_scr_cook, "[START] Pause/Resume  [MENU] Stop");

    /* Key handler */
    lv_obj_add_event_cb(s_scr_cook, cooking_key_cb, LV_EVENT_KEY, NULL);

    /* Start 1-second timer */
    s_timer = lv_timer_create(cooking_timer_cb, 1000, NULL);

    /* Load screen */
    lv_screen_load_anim(s_scr_cook, LV_SCREEN_LOAD_ANIM_MOVE_LEFT, 200, 0, true);
}
