/**************************************************************************//**
 * @file     ui_screen_menu.c
 * @brief    Rice Cooker UI - Menu / Mode Selection screen
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#include "ui_common.h"

static lv_obj_t *s_scr_menu = NULL;
static int8_t    s_cursor = 0;          /* currently highlighted index */
static lv_obj_t *s_items[COOK_MODE_MAX];

/*============================================================================
 * Touch callbacks
 *============================================================================*/
static void menu_item_click_cb(lv_event_t *e)
{
    int idx = (int)(uintptr_t)lv_event_get_user_data(e);
    g_cook_ctx.mode = (cook_mode_t)idx;
    g_cook_ctx.total_seconds = cook_mode_durations[idx];
    g_cook_ctx.remaining_seconds = g_cook_ctx.total_seconds;
    g_cook_ctx.temperature = 250;
    g_cook_ctx.is_paused = false;
    ui_switch_state(UI_STATE_COOKING);
}

static void menu_back_btn_cb(lv_event_t *e)
{
    (void)e;
    ui_switch_state(UI_STATE_HOME);
}

/*============================================================================
 * Helpers
 *============================================================================*/
static void menu_highlight_item(int8_t idx)
{
    for (int i = 0; i < COOK_MODE_MAX; i++)
    {
        if (i == idx)
        {
            lv_obj_set_style_bg_color(s_items[i], UI_COLOR_PRIMARY, 0);
            lv_obj_set_style_bg_opa(s_items[i], LV_OPA_30, 0);
            lv_obj_set_style_text_color(lv_obj_get_child(s_items[i], 0), UI_COLOR_PRIMARY, 0);
        }
        else
        {
            lv_obj_set_style_bg_opa(s_items[i], LV_OPA_TRANSP, 0);
            lv_obj_set_style_text_color(lv_obj_get_child(s_items[i], 0), UI_COLOR_TEXT, 0);
        }
    }

    /* Auto-scroll the list container so the focused item is visible */
    lv_obj_scroll_to_view(s_items[idx], LV_ANIM_ON);
}

/*============================================================================
 * Key handler
 *============================================================================*/
static void menu_key_cb(lv_event_t *e)
{
    ui_key_t key = (ui_key_t)(uintptr_t)lv_event_get_param(e);

    switch (key)
    {
        case UI_KEY_UP:
            s_cursor = (s_cursor > 0) ? s_cursor - 1 : COOK_MODE_MAX - 1;
            menu_highlight_item(s_cursor);
            break;

        case UI_KEY_DOWN:
            s_cursor = (s_cursor < COOK_MODE_MAX - 1) ? s_cursor + 1 : 0;
            menu_highlight_item(s_cursor);
            break;

        case UI_KEY_START:
            /* Confirm selection and start cooking */
            g_cook_ctx.mode = (cook_mode_t)s_cursor;
            g_cook_ctx.total_seconds = cook_mode_durations[s_cursor];
            g_cook_ctx.remaining_seconds = g_cook_ctx.total_seconds;
            g_cook_ctx.temperature = 250;  /* 25.0 °C initial */
            g_cook_ctx.is_paused = false;
            ui_switch_state(UI_STATE_COOKING);
            break;

        case UI_KEY_MENU:
            /* Back to home */
            ui_switch_state(UI_STATE_HOME);
            break;

        default:
            break;
    }
}

/*============================================================================
 * Create
 *============================================================================*/
void ui_screen_menu_create(void)
{
    s_cursor = 0;
    s_scr_menu = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(s_scr_menu, UI_COLOR_BG, 0);
    lv_obj_set_style_bg_opa(s_scr_menu, LV_OPA_COVER, 0);
    lv_obj_clear_flag(s_scr_menu, LV_OBJ_FLAG_SCROLLABLE);

    /* Status bar */
    ui_create_status_bar(s_scr_menu, "Select Mode");

    /* List area */
    lv_obj_t *list_cont = lv_obj_create(s_scr_menu);
    lv_obj_set_size(list_cont, UI_HOR_RES - 20,
                    UI_VER_RES - UI_STATUS_BAR_H - UI_SOFTKEY_BAR_H - 10);
    lv_obj_align(list_cont, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_opa(list_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(list_cont, 0, 0);
    lv_obj_set_style_pad_row(list_cont, 4, 0);
    lv_obj_set_flex_flow(list_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(list_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_scrollbar_mode(list_cont, LV_SCROLLBAR_MODE_OFF);

    for (int i = 0; i < COOK_MODE_MAX; i++)
    {
        s_items[i] = lv_obj_create(list_cont);
        lv_obj_set_size(s_items[i], UI_HOR_RES - 40, 34);
        lv_obj_set_style_radius(s_items[i], 4, 0);
        lv_obj_set_style_border_width(s_items[i], 0, 0);
        lv_obj_set_style_pad_left(s_items[i], 12, 0);
        lv_obj_clear_flag(s_items[i], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(s_items[i], LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(s_items[i], menu_item_click_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)i);

        lv_obj_t *lbl = lv_label_create(s_items[i]);
        lv_label_set_text(lbl, cook_mode_names[i]);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
        lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 0, 0);
    }

    menu_highlight_item(s_cursor);

    /* Touch: Back button */
    lv_obj_t *btn_back = lv_button_create(s_scr_menu);
    lv_obj_set_size(btn_back, 70, 28);
    lv_obj_align(btn_back, LV_ALIGN_BOTTOM_LEFT, 10, -2);
    lv_obj_set_style_bg_color(btn_back, UI_COLOR_STATUS_BG, 0);
    lv_obj_add_event_cb(btn_back, menu_back_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *bl = lv_label_create(btn_back);
    lv_label_set_text(bl, LV_SYMBOL_LEFT " Back");
    lv_obj_set_style_text_color(bl, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(bl, &lv_font_montserrat_12, 0);
    lv_obj_center(bl);

    /* Softkey bar */
    ui_create_softkey_bar(s_scr_menu, "[UP/DN] Select  [START] Confirm  [MENU] Back");

    /* Key handler */
    lv_obj_add_event_cb(s_scr_menu, menu_key_cb, LV_EVENT_KEY, NULL);

    /* Load */
    lv_screen_load_anim(s_scr_menu, LV_SCREEN_LOAD_ANIM_MOVE_LEFT, 200, 0, true);
}
