/**************************************************************************//**
 * @file     ui_screen_home.c
 * @brief    Rice Cooker UI - Home / Ready screen
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#include "ui_common.h"

static lv_obj_t *s_scr_home = NULL;

/*============================================================================
 * Touch callback
 *============================================================================*/
static void home_menu_btn_cb(lv_event_t *e)
{
    (void)e;
    ui_switch_state(UI_STATE_MENU);
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
    ui_create_status_bar(s_scr_home, "Rice Cooker");

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
    lv_label_set_text(lbl_ready, LV_SYMBOL_OK "  Ready");
    lv_obj_set_style_text_color(lbl_ready, UI_COLOR_SUCCESS, 0);
    lv_obj_set_style_text_font(lbl_ready, &lv_font_montserrat_24, 0);

    /* Current time (placeholder) */
    lv_obj_t *lbl_time = lv_label_create(cont);
    lv_label_set_text(lbl_time, "12:00");
    lv_obj_set_style_text_color(lbl_time, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(lbl_time, &lv_font_montserrat_20, 0);

    /* Touch: Menu button */
    lv_obj_t *btn_menu = lv_button_create(cont);
    lv_obj_set_size(btn_menu, 120, 36);
    lv_obj_set_style_bg_color(btn_menu, UI_COLOR_PRIMARY, 0);
    lv_obj_set_style_radius(btn_menu, 6, 0);
    lv_obj_add_event_cb(btn_menu, home_menu_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *btn_lbl = lv_label_create(btn_menu);
    lv_label_set_text(btn_lbl, LV_SYMBOL_LIST "  Menu");
    lv_obj_set_style_text_color(btn_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(btn_lbl);

    /* Softkey bar */
    ui_create_softkey_bar(s_scr_home, "[MENU] Enter Menu");

    /* Register key handler */
    lv_obj_add_event_cb(s_scr_home, home_key_cb, LV_EVENT_KEY, NULL);

    /* Load screen */
    lv_screen_load_anim(s_scr_home, LV_SCREEN_LOAD_ANIM_FADE_IN, 200, 0, true);
}
