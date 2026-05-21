/**************************************************************************//**
 * @file     ui_keypad.c
 * @brief    Physical keypad driver – scans 4 GPIO buttons and injects
 *           key events into the Rice Cooker UI via ui_key_pressed().
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#include "ui_common.h"
#include "NuMicro.h"
#include "nu_misc.h"

/*============================================================================
 * TODO: Adjust these GPIO pin definitions to match your board schematic.
 *       Each button connects between the GPIO pin and GND (active-low).
 *============================================================================*/
#define KEY_MENU_PORT       PH
#define KEY_MENU_PIN        4

#define KEY_UP_PORT         PH
#define KEY_UP_PIN          5

#define KEY_DOWN_PORT       PH
#define KEY_DOWN_PIN        6

#define KEY_START_PORT      PH
#define KEY_START_PIN       7

/*============================================================================
 * Configuration
 *============================================================================*/
#define KEY_DEBOUNCE_MS     30      /* debounce threshold */
#define KEY_LONGPRESS_MS    800     /* long-press threshold (reserved) */
#define KEY_SCAN_PERIOD_MS  20      /* LVGL timer scan period */

/*============================================================================
 * Per-key state
 *============================================================================*/
typedef struct
{
    GPIO_T            *port;        /* GPIO port data register base */
    uint32_t           pin;         /* pin bit mask */
    ui_key_t           key;         /* mapped UI key */
    uint8_t            debounce_cnt;/* debounce counter (in scan ticks) */
    bool               last_state;  /* true = pressed */
    bool               pressed;     /* debounced pressed flag */
} key_info_t;

#define DEBOUNCE_TICKS  (KEY_DEBOUNCE_MS / KEY_SCAN_PERIOD_MS)

static key_info_t s_keys[] =
{
    { KEY_MENU_PORT,  KEY_MENU_PIN,  UI_KEY_MENU,  0, false, false },
    { KEY_UP_PORT,    KEY_UP_PIN,    UI_KEY_UP,    0, false, false },
    { KEY_DOWN_PORT,  KEY_DOWN_PIN,  UI_KEY_DOWN,  0, false, false },
    { KEY_START_PORT, KEY_START_PIN, UI_KEY_START, 0, false, false },
};

#define KEY_COUNT   (sizeof(s_keys) / sizeof(s_keys[0]))

/*============================================================================
 * Read raw pin state (active-low: pressed = pin reads 0)
 *============================================================================*/
static inline bool key_is_raw_pressed(key_info_t *k)
{
    uint32_t port_idx = (((uint32_t)k->port) - GPIOA_BASE) / 0x40;

    /* Read the whole port and check the pin bit */
    return (GPIO_PIN_DATA_S(port_idx, k->pin) == 0);
}

/*============================================================================
 * Scan callback – called by LVGL timer every KEY_SCAN_PERIOD_MS
 *============================================================================*/
static void keypad_scan_cb(lv_timer_t *t)
{
    (void)t;

    for (uint32_t i = 0; i < KEY_COUNT; i++)
    {
        key_info_t *k = &s_keys[i];
        bool raw = key_is_raw_pressed(k);

        if (raw == k->last_state)
        {
            if (k->debounce_cnt < DEBOUNCE_TICKS)
                k->debounce_cnt++;
        }
        else
        {
            k->debounce_cnt = 0;
            k->last_state = raw;
        }

        /* State confirmed after debounce */
        if (k->debounce_cnt == DEBOUNCE_TICKS)
        {
            if (raw && !k->pressed)
            {
                /* Rising edge (just pressed) → inject key event */
                k->pressed = true;
                ui_key_pressed(k->key);
            }
            else if (!raw && k->pressed)
            {
                /* Falling edge (released) */
                k->pressed = false;
            }
        }
    }
}

/*============================================================================
 * Initialise GPIO pins and start LVGL scan timer
 *
 * Call this ONCE after ui_init(), e.g. in lv_nuvoton_task().
 *============================================================================*/
void ui_keypad_init(void)
{
    /*
     * Configure each pin as quasi-bidirectional (internal pull-up)
     * so that an active-low button reads 0 when pressed.
     *
     * TODO: Replace with your actual GPIO mode macros if different.
     */
    GPIO_SetMode(KEY_MENU_PORT,  KEY_MENU_PIN,  GPIO_MODE_QUASI);
    GPIO_SetMode(KEY_UP_PORT,    KEY_UP_PIN,    GPIO_MODE_QUASI);
    GPIO_SetMode(KEY_DOWN_PORT,  KEY_DOWN_PIN,  GPIO_MODE_QUASI);
    GPIO_SetMode(KEY_START_PORT, KEY_START_PIN, GPIO_MODE_QUASI);

    /* Create periodic LVGL timer for scanning */
    lv_timer_create(keypad_scan_cb, KEY_SCAN_PERIOD_MS, NULL);
}
