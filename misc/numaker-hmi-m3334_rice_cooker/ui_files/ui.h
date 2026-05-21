/**************************************************************************//**
 * @file     ui.h
 * @brief    Rice Cooker UI - Public API
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#ifndef __UI_H__
#define __UI_H__

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 * Screen State Enum
 *============================================================================*/
typedef enum
{
    UI_STATE_HOME = 0,
    UI_STATE_MENU,
    UI_STATE_COOKING,
    UI_STATE_FINISH,
    UI_STATE_MAX
} ui_state_t;

/*============================================================================
 * Cooking Mode Enum
 *============================================================================*/
typedef enum
{
    COOK_MODE_WHITE_RICE = 0,
    COOK_MODE_QUICK_COOK,
    COOK_MODE_PORRIDGE,
    COOK_MODE_STEAM,
    COOK_MODE_KEEP_WARM,
    COOK_MODE_MAX
} cook_mode_t;

/*============================================================================
 * Public API
 *============================================================================*/
void ui_init(void);
void ui_switch_state(ui_state_t new_state);
ui_state_t ui_get_state(void);

/* Screen create functions */
void ui_screen_home_create(void);
void ui_screen_menu_create(void);
void ui_screen_cooking_create(void);
void ui_screen_finish_create(void);

/* Button event injection (call from physical key driver) */
typedef enum
{
    UI_KEY_MENU = 0,
    UI_KEY_UP,
    UI_KEY_DOWN,
    UI_KEY_START
} ui_key_t;

void ui_key_pressed(ui_key_t key);

/* Physical keypad GPIO driver */
void ui_keypad_init(void);

#ifdef __cplusplus
}
#endif

#endif /* __UI_H__ */
