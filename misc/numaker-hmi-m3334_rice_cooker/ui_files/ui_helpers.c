/**************************************************************************//**
 * @file     ui_helpers.c
 * @brief    Rice Cooker UI - Shared helper widgets (status bar, softkey bar)
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#include "ui_common.h"
#include "ui_clock.h"

/*============================================================================
 * Status bar (top 30px) – title + simulated time
 *============================================================================*/
void ui_create_status_bar(lv_obj_t *parent, const char *title)
{
    lv_obj_t *bar = lv_obj_create(parent);
    lv_obj_set_size(bar, UI_HOR_RES, UI_STATUS_BAR_H);
    lv_obj_align(bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(bar, UI_COLOR_STATUS_BG, 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(bar, 0, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_set_style_pad_hor(bar, 8, 0);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

    /* Title label (left) */
    lv_obj_t *lbl_title = lv_label_create(bar);
    lv_label_set_text(lbl_title, title);
    lv_obj_set_style_text_color(lbl_title, UI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_16, 0);
    lv_obj_align(lbl_title, LV_ALIGN_LEFT_MID, 0, 0);

    /* Clock label (right) – updated by RTC */
    lv_obj_t *lbl_clock = lv_label_create(bar);
    lv_label_set_text(lbl_clock, "--:--:--");
    lv_obj_set_style_text_color(lbl_clock, UI_COLOR_TEXT_DIM, 0);
    lv_obj_set_style_text_font(lbl_clock, &lv_font_montserrat_16, 0);
    lv_obj_align(lbl_clock, LV_ALIGN_RIGHT_MID, 0, 0);

    /* Register this label for RTC clock updates */
    ui_clock_register_label(lbl_clock);
}

/*============================================================================
 * Soft-key hint bar (bottom 28px)
 *============================================================================*/
void ui_create_softkey_bar(lv_obj_t *parent, const char *hint)
{
    lv_obj_t *bar = lv_obj_create(parent);
    lv_obj_set_size(bar, UI_HOR_RES, UI_SOFTKEY_BAR_H);
    lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(bar, UI_COLOR_STATUS_BG, 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(bar, 0, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl = lv_label_create(bar);
    lv_label_set_text(lbl, hint);
    lv_obj_set_style_text_color(lbl, UI_COLOR_TEXT_DIM, 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, 0);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(lbl, UI_HOR_RES - 16);
    lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 8, 0);
}
