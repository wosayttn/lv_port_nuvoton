/**************************************************************************//**
 * @file     ui_clock.h
 * @brief    Rice Cooker UI - RTC clock display API
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#ifndef __UI_CLOCK_H__
#define __UI_CLOCK_H__

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialise RTC hardware and start the clock update timer.
 *        Call once after ui_init().
 */
void ui_clock_init(void);

/**
 * @brief Register a label to receive clock updates.
 *        Each status bar calls this so the active label gets refreshed.
 * @param lbl  Pointer to an lv_label object, or NULL to unregister.
 */
void ui_clock_register_label(lv_obj_t *lbl);

#ifdef __cplusplus
}
#endif

#endif /* __UI_CLOCK_H__ */
