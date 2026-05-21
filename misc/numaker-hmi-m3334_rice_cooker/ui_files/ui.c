/**************************************************************************//**
 * @file     ui.c
 * @brief    Rice Cooker UI - State machine & init
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#include "ui_common.h"
#include "ui_buzzer.h"
#include "ui_clock.h"
#include <string.h>

/*============================================================================
 * Globals
 *============================================================================*/
ui_cook_ctx_t g_cook_ctx;

const char *cook_mode_names[COOK_MODE_MAX] =
{
    "White Rice",
    "Quick Cook",
    "Porridge",
    "Steam",
    "Keep Warm"
};

const uint32_t cook_mode_durations[COOK_MODE_MAX] =
{
    45 * 60,   /* White Rice: 45 min */
    20 * 60,   /* Quick Cook: 20 min */
    60 * 60,   /* Porridge: 60 min */
    25 * 60,   /* Steam: 25 min */
    0          /* Keep Warm: indefinite */
};

static ui_state_t s_current_state = UI_STATE_HOME;

/*============================================================================
 * State machine
 *============================================================================*/
ui_state_t ui_get_state(void)
{
    return s_current_state;
}

void ui_switch_state(ui_state_t new_state)
{
    if (new_state >= UI_STATE_MAX)
        return;

    s_current_state = new_state;

    switch (new_state)
    {
        case UI_STATE_HOME:
            ui_screen_home_create();
            break;
        case UI_STATE_MENU:
            ui_screen_menu_create();
            break;
        case UI_STATE_COOKING:
            ui_screen_cooking_create();
            break;
        case UI_STATE_FINISH:
            ui_screen_finish_create();
            break;
        default:
            break;
    }
}

/*============================================================================
 * Key dispatch
 *============================================================================*/
void ui_key_pressed(ui_key_t key)
{
    /* Forward key to active screen via LVGL event or direct handling.
     * Each screen registers its own key handler via lv_obj_add_event_cb.
     * Here we send a custom event with the key code to the active screen. */
    lv_obj_t *scr = lv_screen_active();
    if (scr)
    {
        lv_obj_send_event(scr, LV_EVENT_KEY, (void *)(uintptr_t)key);
    }
}

/*============================================================================
 * Init
 *============================================================================*/
void ui_init(void)
{
    /* Reset cooking context */
    memset(&g_cook_ctx, 0, sizeof(g_cook_ctx));

    /* Initialise buzzer hardware */
    ui_buzzer_init();

    /* Initialise RTC clock display */
    ui_clock_init();

    /* Start with Home screen */
    ui_switch_state(UI_STATE_HOME);

    /* Start physical keypad scanning */
    ui_keypad_init();
}
