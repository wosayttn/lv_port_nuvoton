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
static lv_obj_t *s_time_label = NULL;
static lv_timer_t *s_clock_timer = NULL;

static lv_obj_t *clock_get_valid_label(lv_obj_t **lbl)
{
    if ((*lbl != NULL) && !lv_obj_is_valid(*lbl))
        *lbl = NULL;

    return *lbl;
}

static void clock_timer_cb(lv_timer_t *t)
{
    (void)t;
    lv_obj_t *clock_label = clock_get_valid_label(&s_clock_label);
    lv_obj_t *time_label = clock_get_valid_label(&s_time_label);
    if (!clock_label && !time_label) return;

    time_t now = time(NULL);
    struct tm *lt = localtime(&now);

    if (clock_label)
    {
        char buf[24];
        snprintf(buf, sizeof(buf), "%04d/%02d/%02d %02d:%02d:%02d",
                 lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday,
                 lt->tm_hour, lt->tm_min, lt->tm_sec);
        lv_label_set_text(clock_label, buf);
    }

    if (time_label)
    {
        char buf[16];
        snprintf(buf, sizeof(buf), "%02d:%02d:%02d", lt->tm_hour, lt->tm_min, lt->tm_sec);
        lv_label_set_text(time_label, buf);
    }
}

void ui_clock_init(void)
{
    if (s_clock_timer)
    {
        lv_timer_delete(s_clock_timer);
        s_clock_timer = NULL;
    }

    s_clock_timer = lv_timer_create(clock_timer_cb, 1000, NULL);
    clock_timer_cb(NULL);  /* immediate first update */
}

void ui_clock_register_label(lv_obj_t *lbl)
{
    s_clock_label = lbl;
    if (lbl) clock_timer_cb(NULL);
}

void ui_clock_register_time_label(lv_obj_t *lbl)
{
    s_time_label = lbl;
    if (lbl) clock_timer_cb(NULL);
}

/*============================================================================
 * Keypad GPIO stub (no-op on PC, keys handled in main.c)
 *============================================================================*/
void ui_keypad_init(void)
{
    printf("[SIM] Keypad init (using PC keyboard)\n");
}
