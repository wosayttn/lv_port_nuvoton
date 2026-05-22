/**************************************************************************//**
 * @file     ui_screen_cooking.c
 * @brief    Rice Cooker UI - Cooking screen (countdown + temperature)
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#include "ui_common.h"
#include "ui_img_mascot.h"

static lv_obj_t   *s_scr_cook    = NULL;
static lv_obj_t   *s_lbl_time    = NULL;
static lv_obj_t   *s_lbl_temp    = NULL;
static lv_obj_t   *s_lbl_mode    = NULL;
static lv_obj_t   *s_img_mascot  = NULL;
static lv_timer_t *s_timer       = NULL;
static lv_timer_t *s_anim_timer  = NULL;
static uint8_t     s_mascot_frame = 0;

/* Mascot animation frames: front -> side -> back -> side -> (repeat) */
static const lv_image_dsc_t *s_mascot_frames[] = {
    &img_mascot_front,
    &img_mascot_side,
    &img_mascot_back,
    &img_mascot_side,
};

/*============================================================================
 * Mascot animation timer – runs every 500 ms
 *============================================================================*/
static void mascot_anim_cb(lv_timer_t *t)
{
    (void)t;
    if (s_img_mascot == NULL) return;
    if (g_cook_ctx.is_paused) return;

    s_mascot_frame = (s_mascot_frame + 1) % 4;
    lv_image_set_src(s_img_mascot, s_mascot_frames[s_mascot_frame]);
}

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
        /* Cooking done – stop timers first, then switch to finish screen */
        lv_timer_delete(s_timer);
        s_timer = NULL;
        if (s_anim_timer) { lv_timer_delete(s_anim_timer); s_anim_timer = NULL; }
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
}

/*============================================================================
 * Touch callbacks
 *============================================================================*/
static void cooking_pause_btn_cb(lv_event_t *e)
{
    (void)e;
    g_cook_ctx.is_paused = !g_cook_ctx.is_paused;
    if (g_cook_ctx.is_paused)
        lv_label_set_text(s_lbl_mode, ui_str(STR_PAUSED));
    else
        lv_label_set_text(s_lbl_mode, ui_str((ui_str_id_t)(STR_MODE_WHITE_RICE + g_cook_ctx.mode)));
}

static void cooking_stop_btn_cb(lv_event_t *e)
{
    (void)e;
    if (s_timer) { lv_timer_delete(s_timer); s_timer = NULL; }
    if (s_anim_timer) { lv_timer_delete(s_anim_timer); s_anim_timer = NULL; }
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
                lv_label_set_text(s_lbl_mode, ui_str(STR_PAUSED));
            else
                lv_label_set_text(s_lbl_mode, ui_str((ui_str_id_t)(STR_MODE_WHITE_RICE + g_cook_ctx.mode)));
            break;

        case UI_KEY_MENU:
            /* Stop cooking, back to home */
            if (s_timer) { lv_timer_delete(s_timer); s_timer = NULL; }
            if (s_anim_timer) { lv_timer_delete(s_anim_timer); s_anim_timer = NULL; }
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
    ui_create_status_bar(s_scr_cook, ui_str(STR_COOKING));

    /* Content area */
    lv_obj_t *cont = lv_obj_create(s_scr_cook);
    lv_obj_set_size(cont, UI_HOR_RES - 20, UI_VER_RES - UI_STATUS_BAR_H - UI_SOFTKEY_BAR_H - 10);
    lv_obj_align(cont, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

    /* Mode name – top left */
    s_lbl_mode = lv_label_create(cont);
    lv_label_set_text(s_lbl_mode, ui_str((ui_str_id_t)(STR_MODE_WHITE_RICE + g_cook_ctx.mode)));
    lv_obj_set_style_text_color(s_lbl_mode, UI_COLOR_ACCENT, 0);
    lv_obj_set_style_text_font(s_lbl_mode, ui_lang_font_normal(), 0);
    lv_obj_align(s_lbl_mode, LV_ALIGN_TOP_LEFT, 5, 2);

    /* Large countdown – left side */
    s_lbl_time = lv_label_create(cont);
    uint32_t min = g_cook_ctx.remaining_seconds / 60;
    uint32_t sec = g_cook_ctx.remaining_seconds % 60;
    lv_label_set_text_fmt(s_lbl_time, "%02lu:%02lu", (unsigned long)min, (unsigned long)sec);
    lv_obj_set_style_text_color(s_lbl_time, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(s_lbl_time, &lv_font_montserrat_24, 0);
    lv_obj_align(s_lbl_time, LV_ALIGN_TOP_LEFT, 10, 22);

    /* Temperature – left side */
    s_lbl_temp = lv_label_create(cont);
    lv_label_set_text_fmt(s_lbl_temp, "%d.%d " LV_SYMBOL_CHARGE "C",
                          g_cook_ctx.temperature / 10,
                          g_cook_ctx.temperature % 10);
    lv_obj_set_style_text_color(s_lbl_temp, UI_COLOR_DANGER, 0);
    lv_obj_set_style_text_font(s_lbl_temp, ui_lang_font_normal(), 0);
    lv_obj_align(s_lbl_temp, LV_ALIGN_TOP_LEFT, 10, 50);

    /* Nuvoton mascot animation – right side */
    s_img_mascot = lv_image_create(cont);
    lv_image_set_src(s_img_mascot, s_mascot_frames[0]);
    lv_obj_align(s_img_mascot, LV_ALIGN_TOP_RIGHT, 0, 0);
    s_mascot_frame = 0;

    /* Start mascot animation timer (100ms per frame) */
    s_anim_timer = lv_timer_create(mascot_anim_cb, 100, NULL);

    /* Touch: Pause button – placed inside content area */
    lv_obj_t *btn_pause = lv_button_create(cont);
    lv_obj_set_size(btn_pause, 120, 34);
    lv_obj_align(btn_pause, LV_ALIGN_BOTTOM_LEFT, 10, 0);
    lv_obj_set_style_bg_color(btn_pause, UI_COLOR_ACCENT, 0);
    lv_obj_set_style_radius(btn_pause, 6, 0);
    lv_obj_add_event_cb(btn_pause, cooking_pause_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *pl = lv_label_create(btn_pause);
    lv_label_set_text(pl, ui_str(STR_BTN_PAUSE));
    lv_obj_set_style_text_color(pl, lv_color_hex(0x000000), 0);
    lv_obj_set_style_text_font(pl, ui_lang_font_normal(), 0);
    lv_obj_center(pl);

    /* Touch: Stop button – placed inside content area */
    lv_obj_t *btn_stop = lv_button_create(cont);
    lv_obj_set_size(btn_stop, 120, 34);
    lv_obj_align(btn_stop, LV_ALIGN_BOTTOM_RIGHT, -10, 0);
    lv_obj_set_style_bg_color(btn_stop, UI_COLOR_DANGER, 0);
    lv_obj_set_style_radius(btn_stop, 6, 0);
    lv_obj_add_event_cb(btn_stop, cooking_stop_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *sl = lv_label_create(btn_stop);
    lv_label_set_text(sl, ui_str(STR_BTN_STOP));
    lv_obj_set_style_text_color(sl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(sl, ui_lang_font_normal(), 0);
    lv_obj_center(sl);

    /* Softkey bar */
    ui_create_softkey_bar(s_scr_cook, ui_str(STR_HINT_COOKING));

    /* Key handler */
    lv_obj_add_event_cb(s_scr_cook, cooking_key_cb, LV_EVENT_KEY, NULL);

    /* Start 1-second timer */
    s_timer = lv_timer_create(cooking_timer_cb, 1000, NULL);

    /* Load screen */
    lv_screen_load_anim(s_scr_cook, LV_SCREEN_LOAD_ANIM_MOVE_LEFT, 200, 0, true);
}
