/**************************************************************************//**
 * @file     ui_common.h
 * @brief    Rice Cooker UI - Internal shared definitions
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#ifndef __UI_COMMON_H__
#define __UI_COMMON_H__

#include "ui.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 * Display geometry (320 x 240)
 *============================================================================*/
#define UI_HOR_RES      320
#define UI_VER_RES      240

#define UI_STATUS_BAR_H  30
#define UI_SOFTKEY_BAR_H 28

/*============================================================================
 * Color palette
 *============================================================================*/
#define UI_COLOR_BG          lv_color_hex(0x1E1E2E)
#define UI_COLOR_STATUS_BG   lv_color_hex(0x313244)
#define UI_COLOR_PRIMARY     lv_color_hex(0x89B4FA)
#define UI_COLOR_ACCENT      lv_color_hex(0xF9E2AF)
#define UI_COLOR_SUCCESS     lv_color_hex(0xA6E3A1)
#define UI_COLOR_DANGER      lv_color_hex(0xF38BA8)
#define UI_COLOR_TEXT        lv_color_hex(0xCDD6F4)
#define UI_COLOR_TEXT_DIM    lv_color_hex(0x6C7086)

/*============================================================================
 * Cooking context (shared across screens)
 *============================================================================*/
typedef struct
{
    cook_mode_t   mode;
    uint32_t      total_seconds;
    uint32_t      remaining_seconds;
    int16_t       temperature;     /* current temp in °C x10 */
    bool          is_paused;
} ui_cook_ctx_t;

extern ui_cook_ctx_t g_cook_ctx;

/* Mode name strings */
extern const char *cook_mode_names[COOK_MODE_MAX];

/* Default cooking durations per mode (seconds) */
extern const uint32_t cook_mode_durations[COOK_MODE_MAX];

/*============================================================================
 * Helpers
 *============================================================================*/
void ui_create_status_bar(lv_obj_t *parent, const char *title);
void ui_create_softkey_bar(lv_obj_t *parent, const char *hint);

#ifdef __cplusplus
}
#endif

#endif /* __UI_COMMON_H__ */
