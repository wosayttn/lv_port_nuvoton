/**************************************************************************//**
 * @file     ui.c
 * @brief    UI initialization for GIF animation playback from SPI NOR Flash
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include "lvgl.h"
#include "ui.h"
#include "fatfs_spinor.h"

#define MAX_GIF_FILES    16

static char s_szGifFiles[MAX_GIF_FILES][32];
static int  s_iGifCount = 0;
static int  s_iCurrentGifIdx = 0;

static lv_obj_t   *s_pGifObj = NULL;
static lv_timer_t *s_pSwitchTimer = NULL;
static uint32_t    s_u32GifStartTime = 0;
static uint32_t    s_u32LoopCount = 0;
static bool        s_bSwitchPending = false;

static void play_current_gif(void);

static void trigger_switch(uint32_t delay_ms)
{
    if (s_iGifCount <= 1 || s_bSwitchPending)
        return;

    s_bSwitchPending = true;
    if (s_pSwitchTimer)
    {
        lv_timer_set_period(s_pSwitchTimer, delay_ms);
        lv_timer_reset(s_pSwitchTimer);
        lv_timer_resume(s_pSwitchTimer);
    }
}

static void switch_timer_cb(lv_timer_t *timer)
{
    lv_timer_pause(timer);
    s_bSwitchPending = false;

    if (s_iGifCount <= 1)
        return;

    s_iCurrentGifIdx = (s_iCurrentGifIdx + 1) % s_iGifCount;
    play_current_gif();
}

static void play_current_gif(void)
{
    if (s_iGifCount <= 0 || s_pGifObj == NULL)
        return;

    if (s_pSwitchTimer)
    {
        lv_timer_pause(s_pSwitchTimer);
    }

    printf("[UI] Carousel: Playing [%d/%d] %s\n",
           s_iCurrentGifIdx + 1, s_iGifCount, s_szGifFiles[s_iCurrentGifIdx]);

    s_u32GifStartTime = lv_tick_get();
    s_u32LoopCount = 0;
    s_bSwitchPending = false;

    /* Invalidate entire screen so area outside smaller GIFs is cleared to black */
    lv_obj_invalidate(lv_screen_active());

    lv_gif_set_src(s_pGifObj, s_szGifFiles[s_iCurrentGifIdx]);
    lv_obj_center(s_pGifObj);

    if (!lv_gif_is_loaded(s_pGifObj))
    {
        printf("[UI] Warning: Failed to load %s, skipping to next\n", s_szGifFiles[s_iCurrentGifIdx]);
        trigger_switch(500);
    }
}

static void gif_event_cb(lv_event_t *e)
{
    if (s_iGifCount <= 1)
        return;

    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_READY)
    {
        s_u32LoopCount++;
        uint32_t elapsed = lv_tick_elaps(s_u32GifStartTime);
        printf("[UI] GIF Loop #%u completed (elapsed: %u ms)\n", s_u32LoopCount, elapsed);

        /* Only switch when the animation has played to completion */
        /* For short clips (< 2.5s), allow 2 full loops so the viewer can see the animation */
        if ((elapsed >= 2500 || s_u32LoopCount >= 2) && !s_bSwitchPending)
        {
            printf("[UI] Animation complete! Transitioning to next GIF in 600ms...\n");
            /* Keep the last frame on screen for 600ms before switching cleanly */
            trigger_switch(600);
        }
    }
}

static void gif_watchdog_cb(lv_timer_t *timer)
{
    LV_UNUSED(timer);
    if (s_iGifCount <= 1 || s_bSwitchPending)
        return;

    uint32_t elapsed = lv_tick_elaps(s_u32GifStartTime);
    /* Safe watchdog (60 seconds) only in case a file is single-frame or stuck without sending READY */
    if (elapsed >= 60000)
    {
        printf("[UI] Watchdog timeout (60s) reached for %s, switching to next\n", s_szGifFiles[s_iCurrentGifIdx]);
        trigger_switch(500);
    }
}

void ui_init(void)
{
    /* 1. Initialize SPI NOR Flash and mount FatFs drive A: */
    int ret = fatfs_spinor_init();

    /* 2. Configure screen background */
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0);

    if (ret != 0)
    {
        /* Header title */
        lv_obj_t *title = lv_label_create(scr);
        lv_label_set_text(title, "LVGL GIF Animation");
        lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
        lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 16);

        /* Subtitle indicating flash source and 2-buf concurrent pipeline */
        lv_obj_t *sub = lv_label_create(scr);
#if defined(NUFUN) && (NUFUN==1)
        lv_label_set_text(sub, "QSPI0 Flash (PC0~5) | 2 Buf // Flush(PDMA)");
#else
        lv_label_set_text(sub, "QSPI0 Flash (PA0~5) | 2 Buf // Flush(PDMA)");
#endif
        lv_obj_set_style_text_color(sub, lv_color_hex(0x20D890), 0);
        lv_obj_set_style_text_font(sub, &lv_font_montserrat_12, 0);
        lv_obj_align(sub, LV_ALIGN_TOP_MID, 0, 38);

        /* Mount or file open failed - show Mount Fail! message */
        lv_obj_t *err_card = lv_obj_create(scr);
        lv_obj_set_size(err_card, 200, 80);
        lv_obj_align(err_card, LV_ALIGN_CENTER, 0, 10);
        lv_obj_set_style_bg_color(err_card, lv_color_hex(0x3B2020), 0);
        lv_obj_set_style_border_color(err_card, lv_color_hex(0xE04040), 0);
        lv_obj_set_style_border_width(err_card, 2, 0);
        lv_obj_set_style_radius(err_card, 10, 0);

        lv_obj_t *msg = lv_label_create(err_card);
        lv_label_set_text(msg, "Mount Fail!");
        lv_obj_set_style_text_color(msg, lv_color_hex(0xFF5555), 0);
        lv_obj_set_style_text_font(msg, &lv_font_montserrat_20, 0);
        lv_obj_center(msg);

        lv_obj_t *info = lv_label_create(scr);
        lv_label_set_text(info, "Check QSPI0 NOR Flash or FAT filesystem");
        lv_obj_set_style_text_color(info, lv_color_hex(0xA0A0A0), 0);
        lv_obj_set_style_text_font(info, &lv_font_montserrat_12, 0);
        lv_obj_align(info, LV_ALIGN_BOTTOM_MID, 0, -10);
        return;
    }

    /* 3. Scan all GIF files on drive A: */
    s_iGifCount = fatfs_spinor_scan_gifs(s_szGifFiles, MAX_GIF_FILES);
    if (s_iGifCount <= 0)
    {
        printf("[UI] Error: No GIF files found on drive A:\n");
        return;
    }

    printf("[UI] Carousel initialized with %d GIF file(s)\n", s_iGifCount);

    /* 4. Full-screen GIF animation loaded from SPI NOR Flash FAT */
    s_pGifObj = lv_gif_create(scr);
    lv_gif_set_color_format(s_pGifObj, LV_COLOR_FORMAT_RGB565);
    lv_obj_add_event_cb(s_pGifObj, gif_event_cb, LV_EVENT_READY, NULL);

    /* 5. Create persistent switch timer (paused initially) and safety watchdog */
    s_pSwitchTimer = lv_timer_create(switch_timer_cb, 600, NULL);
    lv_timer_pause(s_pSwitchTimer);
    lv_timer_create(gif_watchdog_cb, 2000, NULL);

    /* 6. Start playing first GIF */
    s_iCurrentGifIdx = 0;
    play_current_gif();
}
