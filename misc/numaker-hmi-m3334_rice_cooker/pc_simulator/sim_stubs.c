/**************************************************************************//**
 * @file     sim_stubs.c
 * @brief    Stub implementations for hardware-dependent functions
 *           so that ui_files/*.c can compile on PC without real HW.
 *****************************************************************************/
#include <stdio.h>
#include <stdint.h>
#include "lvgl.h"
#include "ui.h"
#include "ui_buzzer.h"
#include "ui_clock.h"

/*============================================================================
 * Buzzer stubs (just print to console)
 *============================================================================*/
void ui_buzzer_init(void)
{
    printf("[SIM] Buzzer init\n");
}

void ui_buzzer_beep(uint32_t freq_hz, uint32_t duration_ms)
{
    printf("[SIM] Beep %u Hz, %u ms\n", freq_hz, duration_ms);
}

void ui_buzzer_play_done(void)
{
    printf("[SIM] Buzzer: cooking done melody\n");
}

void ui_buzzer_stop(void)
{
    /* silent */
}

/*============================================================================
 * Clock stub – use PC system time
 *============================================================================*/
#include <time.h>

static lv_obj_t *s_clock_label = NULL;
static lv_timer_t *s_clock_timer = NULL;

static void clock_timer_cb(lv_timer_t *t)
{
    (void)t;
    if (!s_clock_label) return;

    time_t now = time(NULL);
    struct tm *lt = localtime(&now);
    char buf[16];
    snprintf(buf, sizeof(buf), "%02d:%02d", lt->tm_hour, lt->tm_min);
    lv_label_set_text(s_clock_label, buf);
}

void ui_clock_init(void)
{
    s_clock_timer = lv_timer_create(clock_timer_cb, 10000, NULL);
    clock_timer_cb(NULL);  /* immediate first update */
}

void ui_clock_register_label(lv_obj_t *lbl)
{
    s_clock_label = lbl;
    if (lbl) clock_timer_cb(NULL);
}

/*============================================================================
 * Keypad GPIO stub (no-op on PC, keys handled in main.c)
 *============================================================================*/
void ui_keypad_init(void)
{
    printf("[SIM] Keypad init (using PC keyboard)\n");
}
