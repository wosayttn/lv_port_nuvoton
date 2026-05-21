/**************************************************************************//**
 * @file     ui_buzzer.c
 * @brief    Rice Cooker UI - Buzzer driver (PWM-based)
 *
 * Hardware assumption:
 *   - Buzzer is connected to a PWM-capable pin.
 *   - Default: TIMER2 + PB5 (adjust to your board).
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#include "ui_common.h"
#include "ui_buzzer.h"
#include "NuMicro.h"

/*============================================================================
 * TODO: Adjust these to match your board schematic
 *============================================================================*/
#define BUZZER_PORT         PG
#define BUZZER_PIN          BIT4
#define BUZZER_MFP_REG      SYS->GPG_MFP1
#define BUZZER_MFP_MSK      SYS_GPG_MFP1_PG4MFP_Msk
#define BUZZER_MFP_VAL      SYS_GPG_MFP1_PG4MFP_GPIO  /* Use GPIO toggle; or set PWM MFP */

/* Simple GPIO-toggle buzzer (works without dedicated PWM channel) */
#define BUZZER_USE_GPIO_TOGGLE   1

/*============================================================================
 * Internal state for non-blocking melody
 *============================================================================*/
static lv_timer_t *s_melody_timer = NULL;
static uint8_t     s_melody_step  = 0;

/* Melody pattern: {duration_on_ms, pause_ms} repeated */
#define MELODY_BEEPS      3
#define MELODY_ON_MS      150
#define MELODY_OFF_MS     100

/*============================================================================
 * Low-level GPIO buzzer
 *============================================================================*/
static volatile bool s_buzzer_on = false;
static lv_timer_t   *s_tone_timer = NULL;

/* Toggle pin at desired half-period to produce square wave */
static void tone_toggle_cb(lv_timer_t *t)
{
    (void)t;
    if (!s_buzzer_on) return;

    /* Toggle GPIO */
    BUZZER_PORT->DOUT ^= BUZZER_PIN;
}

/*============================================================================
 * Public API
 *============================================================================*/
void ui_buzzer_init(void)
{
    /* Configure buzzer pin as push-pull output, default LOW */
    GPIO_SetMode(BUZZER_PORT, BUZZER_PIN, GPIO_MODE_OUTPUT);
    BUZZER_PORT->DOUT &= ~BUZZER_PIN;
}

void ui_buzzer_beep(uint32_t freq_hz, uint32_t duration_ms)
{
#if BUZZER_USE_GPIO_TOGGLE
    if (freq_hz == 0) freq_hz = 2700;

    /* half-period in ms (integer approximation) */
    uint32_t half_period_ms = 500 / freq_hz;
    if (half_period_ms < 1) half_period_ms = 1;

    s_buzzer_on = true;

    /* Create a fast timer to toggle GPIO */
    if (s_tone_timer)
        lv_timer_delete(s_tone_timer);
    s_tone_timer = lv_timer_create(tone_toggle_cb, half_period_ms, NULL);

    /* Schedule stop after duration_ms */
    /* We'll use a one-shot approach: just record start and stop in melody cb */
#else
    /* TODO: If using hardware PWM, configure timer/PWM channel here */
    (void)freq_hz;
    (void)duration_ms;
#endif
    (void)duration_ms; /* duration handled by melody timer */
}

void ui_buzzer_stop(void)
{
    s_buzzer_on = false;
    BUZZER_PORT->DOUT &= ~BUZZER_PIN;  /* ensure pin LOW */

    if (s_tone_timer)
    {
        lv_timer_delete(s_tone_timer);
        s_tone_timer = NULL;
    }
}

/*============================================================================
 * Melody state machine (non-blocking, driven by LVGL timer)
 *============================================================================*/
static void melody_step_cb(lv_timer_t *t)
{
    (void)t;

    /* Even steps = beep ON, odd steps = silence */
    if (s_melody_step >= MELODY_BEEPS * 2)
    {
        /* Done */
        ui_buzzer_stop();
        if (s_melody_timer) { lv_timer_delete(s_melody_timer); s_melody_timer = NULL; }
        return;
    }

    if (s_melody_step % 2 == 0)
    {
        /* Beep ON */
        ui_buzzer_beep(2700, MELODY_ON_MS);
        lv_timer_set_period(s_melody_timer, MELODY_ON_MS);
    }
    else
    {
        /* Silence */
        ui_buzzer_stop();
        lv_timer_set_period(s_melody_timer, MELODY_OFF_MS);
    }

    s_melody_step++;
}

void ui_buzzer_play_done(void)
{
    /* Cancel any previous melody */
    if (s_melody_timer)
    {
        lv_timer_delete(s_melody_timer);
        s_melody_timer = NULL;
    }
    ui_buzzer_stop();

    s_melody_step = 0;
    s_melody_timer = lv_timer_create(melody_step_cb, 1, NULL); /* fire immediately */
}
