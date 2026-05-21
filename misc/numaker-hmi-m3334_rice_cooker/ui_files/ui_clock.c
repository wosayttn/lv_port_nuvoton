/**************************************************************************//**
 * @file     ui_clock.c
 * @brief    Rice Cooker UI - RTC clock display driver
 *
 * Reads the M3334 on-chip RTC every second and updates the registered
 * status-bar label with HH:MM:SS.
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#include "ui_common.h"
#include "ui_clock.h"
#include "NuMicro.h"

static lv_obj_t   *s_lbl_clock = NULL;
static lv_timer_t *s_timer     = NULL;

/*============================================================================
 * Timer callback – read RTC and update label
 *============================================================================*/
static void clock_update_cb(lv_timer_t *t)
{
    (void)t;

    if (s_lbl_clock == NULL)
        return;

    S_RTC_TIME_DATA_T sTime;
    RTC_GetDateAndTime(&sTime);

    lv_label_set_text_fmt(s_lbl_clock, "%02lu:%02lu:%02lu",
                          (unsigned long)sTime.u32Hour,
                          (unsigned long)sTime.u32Minute,
                          (unsigned long)sTime.u32Second);
}

/*============================================================================
 * Public API
 *============================================================================*/
void ui_clock_register_label(lv_obj_t *lbl)
{
    s_lbl_clock = lbl;

    /* Immediately update so it doesn't show stale "12:00" */
    if (lbl)
        clock_update_cb(NULL);
}

void ui_clock_init(void)
{
    /* Enable RTC clock source */
    CLK_EnableModuleClock(RTC_MODULE);

    /* Open RTC with a default time if not already running */
    S_RTC_TIME_DATA_T sInitTime;
    sInitTime.u32Year    = 2026;
    sInitTime.u32Month   = 5;
    sInitTime.u32Day     = 21;
    sInitTime.u32Hour    = 12;
    sInitTime.u32Minute  = 0;
    sInitTime.u32Second  = 0;
    sInitTime.u32DayOfWeek = RTC_WEDNESDAY;
    sInitTime.u32TimeScale = RTC_CLOCK_24;

    if (RTC_Open(&sInitTime) != 0)
    {
        /* RTC already running – just read it */
    }

    /* Create 1-second LVGL timer to refresh the clock */
    s_timer = lv_timer_create(clock_update_cb, 1000, NULL);
}
