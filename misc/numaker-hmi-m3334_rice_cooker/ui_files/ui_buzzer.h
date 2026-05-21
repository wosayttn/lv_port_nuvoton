/**************************************************************************//**
 * @file     ui_buzzer.h
 * @brief    Rice Cooker UI - Buzzer driver API
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#ifndef __UI_BUZZER_H__
#define __UI_BUZZER_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialise buzzer hardware (GPIO/PWM).
 *        Call once at startup.
 */
void ui_buzzer_init(void);

/**
 * @brief Play a single beep.
 * @param freq_hz   Tone frequency in Hz (e.g. 2700)
 * @param duration_ms Beep duration in milliseconds
 */
void ui_buzzer_beep(uint32_t freq_hz, uint32_t duration_ms);

/**
 * @brief Play the "cooking done" melody (3 short beeps).
 *        Non-blocking – uses an LVGL timer internally.
 */
void ui_buzzer_play_done(void);

/**
 * @brief Immediately silence the buzzer.
 */
void ui_buzzer_stop(void);

#ifdef __cplusplus
}
#endif

#endif /* __UI_BUZZER_H__ */
