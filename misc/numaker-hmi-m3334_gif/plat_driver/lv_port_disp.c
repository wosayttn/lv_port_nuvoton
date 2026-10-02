/**************************************************************************//**
 * @file     lv_port_disp.c
 * @brief    LVGL display port with dual-buffer asynchronous SPI PDMA flush
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "lvgl.h"
#include "lv_glue.h"
#include "numaker_disp.h"

#if defined(__FREERTOS__)
    #include "FreeRTOS.h"
    #include "task.h"
    #include "semphr.h"
#endif

#ifndef CONFIG_DISP_LINE_BUFFER_NUMBER
    #define CONFIG_DISP_LINE_BUFFER_NUMBER    (DISP_VER_RES_MAX / 8) /* 30 lines */
#endif

#define BUFFER_SIZE     (DISP_HOR_RES_MAX * CONFIG_DISP_LINE_BUFFER_NUMBER * (DISP_COLOR_DEPTH / 8))

/* Two display buffers for concurrent rendering and flushing */
static uint8_t s_au8FrameBuf1[BUFFER_SIZE] __attribute__((aligned(4)));
static uint8_t s_au8FrameBuf2[BUFFER_SIZE] __attribute__((aligned(4)));

#if defined(__FREERTOS__)
    static SemaphoreHandle_t s_xSemFlushDone = NULL;
#endif

static volatile bool s_bFlushing = false;

extern void disp_fillrect_async(uint16_t *pixels, const disp_area_t *area, void (*cb)(void *), void *pvUserData);

static void lv_port_disp_flush_done_cb(void *pvUserData)
{
    lv_display_t *disp = (lv_display_t *)pvUserData;
    s_bFlushing = false;

#if defined(__FREERTOS__)
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(s_xSemFlushDone, &xHigherPriorityTaskWoken);
#endif

    lv_display_flush_ready(disp);

#if defined(__FREERTOS__)
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
#endif
}

static void lv_port_disp_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    disp_area_t disp_area = {
        .x1 = area->x1,
        .y1 = area->y1,
        .x2 = area->x2,
        .y2 = area->y2
    };

    s_bFlushing = true;

#if defined(__FREERTOS__)
    xSemaphoreTake(s_xSemFlushDone, 0);
#endif

    /* Start asynchronous SPI PDMA transmission. Returns immediately to caller */
    disp_fillrect_async((uint16_t *)px_map, &disp_area, lv_port_disp_flush_done_cb, (void *)disp);
}

static void lv_port_disp_flush_wait_cb(lv_display_t *disp)
{
    if (s_bFlushing)
    {
#if defined(__FREERTOS__)
        xSemaphoreTake(s_xSemFlushDone, portMAX_DELAY);
#else
        while (s_bFlushing);
#endif
    }
}

void lv_port_disp_init(void)
{
    lv_display_t *disp;

    /* Initialize display controller and hardware */
    LV_ASSERT(lcd_device_initialize() == 0);
    LV_ASSERT(lcd_device_open() == 0);

#if defined(__FREERTOS__)
    if (s_xSemFlushDone == NULL)
    {
        s_xSemFlushDone = xSemaphoreCreateBinary();
        LV_ASSERT(s_xSemFlushDone != NULL);
    }
#endif

    disp = lv_display_create(DISP_HOR_RES_MAX, DISP_VER_RES_MAX);
    LV_ASSERT(disp != NULL);

    /* Set flush callbacks */
    lv_display_set_flush_cb(disp, lv_port_disp_flush_cb);
    lv_display_set_flush_wait_cb(disp, lv_port_disp_flush_wait_cb);

    /* Set two buffers for concurrent rendering and flushing */
    lv_display_set_buffers(disp, s_au8FrameBuf1, s_au8FrameBuf2, BUFFER_SIZE, LV_DISPLAY_RENDER_MODE_PARTIAL);

    LV_LOG_INFO("Dual display buffers initialized: Buf1=0x%08X, Buf2=0x%08X, Size=%u",
                (uint32_t)s_au8FrameBuf1, (uint32_t)s_au8FrameBuf2, BUFFER_SIZE);
}
