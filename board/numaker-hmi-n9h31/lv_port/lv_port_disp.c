/**************************************************************************//**
 * @file     lv_port_disp.c
 * @brief    lvgl display port
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/

#include "lvgl.h"
#include "lv_glue.h"
#include "2d.h"

#if CONFIG_LV_DISP_FULL_REFRESH

static void lv_port_disp_full(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    sysCleanDcache((UINT32)px_map, (UINT32)lv_area_get_size(area) * (LV_COLOR_DEPTH / 8));

    /* Use PANDISPLAY without H/W copying */
    LV_ASSERT(lcd_device_control(evLCD_CTRL_PAN_DISPLAY, (void *)px_map) == 0);

    /* vsync-after: Use ping-pong screen-sized buffers only.*/
    LV_ASSERT(lcd_device_control(evLCD_CTRL_WAIT_VSYNC, (void *)NULL) == 0);

    lv_display_flush_ready(disp);
}

#else

static volatile int32_t s_i32GDMAIdx = -1;

static bool _isConstrained(uint32_t px_size, const lv_area_t *area)
{
    /* 2DGE limitation? */
    if (px_size == 2)
    {
        uint32_t dst_stride = px_size * lv_area_get_width(area);

        /* Check Hardware constraint: The stride must be a word-alignment. */
        bool bAlignedWord = ((dst_stride & 0x3) == 0) &&
                            (((area->x1 * px_size) & 0x3) == 0) ? true : false;

        if (!bAlignedWord)
            goto exit_isConstrained;
    }

    return false;

exit_isConstrained:

    return true;
}

static void lv_port_disp_partial(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    S_LCD_INFO *psLCDInfo = (S_LCD_INFO *)lv_display_get_driver_data(disp);
    LV_ASSERT(psLCDInfo != NULL);
    uint32_t px_size = psLCDInfo->u32BytePerPixel;

    /* Update dirty region. */
    if ((lv_area_get_size(area) == (psLCDInfo->u32ResWidth * psLCDInfo->u32ResHeight)))
    {
        /* Only catch full-screen case due to GDMA constraint(No stride feature). */
#define FULL_SCREEN_BYTE_COUNT (psLCDInfo->u32ResWidth * psLCDInfo->u32ResHeight * psLCDInfo->u32BytePerPixel)

        void GDMA_M2M_Transfer(uint32_t u32DestAddr, uint32_t u32SrcAddr, uint32_t u32TransferSize);
        GDMA_M2M_Transfer((uint32_t)psLCDInfo->pvVramStartAddr, (uint32_t)px_map, FULL_SCREEN_BYTE_COUNT);

        s_i32GDMAIdx = 0;
    }
#if LV_USE_DRAW_2DGE
    else if (!_isConstrained(px_size, area))
    {
        int32_t dest_w = lv_area_get_width(area);
        int32_t dest_h = lv_area_get_height(area);
        int32_t dest_stride = psLCDInfo->u32ResWidth * px_size;

        ge2dInit(px_size * 8, psLCDInfo->u32ResWidth, psLCDInfo->u32ResHeight, (void *)psLCDInfo->pvVramStartAddr);

        ge2dBitblt_SetAlphaMode(0, 0, 0);
        ge2dBitblt_SetDrawMode(0, 0, 0);

        const uint8_t *dest_start_buf = psLCDInfo->pvVramStartAddr + (area->y1 * dest_stride);

        /* Clean. then invalidate cache line of dest buffer to memory */
        sysCleanInvalidatedDcache((UINT32)dest_start_buf, dest_stride * dest_h);

        /* Flush cache line of src buffer to memory */
        sysCleanDcache((uint32_t)px_map, dest_w * dest_h * px_size);

        ge2dSpriteBlt_Screen(area->x1, area->y1, dest_w, dest_h, (void *)px_map);
    }
#endif
    else
    {
        int32_t x, y;
        int32_t w = lv_area_get_width(area);
        int32_t h = lv_area_get_height(area);

#if (LV_COLOR_DEPTH==32)
        uint32_t *pDisp = (uint32_t *)((uint32_t)psLCDInfo->pvVramStartAddr) + (psLCDInfo->u32ResWidth * area->y1 + area->x1);
        uint32_t *pSrc = (uint32_t *)px_map;
#elif (LV_COLOR_DEPTH==16)
        uint16_t *pDisp = (uint16_t *)(uint32_t)psLCDInfo->pvVramStartAddr + (psLCDInfo->u32ResWidth * area->y1 + area->x1);
        uint16_t *pSrc = (uint16_t *)px_map;
#endif

        for (y = 0; y < h; y++)
        {
            for (x = 0; x < w; x++)
                pDisp[x] = pSrc[x];

            sysCleanInvalidatedDcache((UINT32)pDisp, w * (LV_COLOR_DEPTH / 8));

            pDisp += psLCDInfo->u32ResWidth;
            pSrc += w;
        }
    }
}

static void lv_port_disp_flush_wait(lv_display_t *disp)
{
    if (s_i32GDMAIdx == 0) // Is GDMA channel?
    {
        void GDMA_WaitForCompletion(void);
        GDMA_WaitForCompletion();

        s_i32GDMAIdx = -1;
    }
}
#endif

void lv_port_disp_init(void)
{
    lv_display_t *disp;
    static S_LCD_INFO sLcdInfo = {0};
    void *buf1, *buf2;
    uint32_t u32FBSize;

    /* Initial display device */
    LV_ASSERT(lcd_device_initialize() == 0);
    LV_ASSERT(lcd_device_open() == 0);
    LV_ASSERT(lcd_device_control(evLCD_CTRL_GET_INFO, (void *)&sLcdInfo) == 0);

    u32FBSize = sLcdInfo.u32ResHeight * sLcdInfo.u32ResWidth * sLcdInfo.u32BytePerPixel;
    buf1 = (void *)((uint32_t)sLcdInfo.pvVramStartAddr);
    buf2 = (void *)((uint32_t)buf1 + u32FBSize);

    disp = lv_display_create(sLcdInfo.u32ResWidth, sLcdInfo.u32ResHeight);
    LV_ASSERT(disp != NULL);

    lv_display_set_driver_data(disp, &sLcdInfo);

#if CONFIG_LV_DISP_FULL_REFRESH
    LV_LOG_INFO("Use two screen-size buffer, buf1: 0x%08x, buf2: 0x%08x: 0x%08x", buf1, buf2);
    lv_color_format_t cf = lv_display_get_color_format(disp);

    lv_display_set_flush_cb(disp, lv_port_disp_full); /*Set a flush callback to draw to the display*/
    lv_display_set_buffers(disp, buf1, buf2, u32FBSize, LV_DISPLAY_RENDER_MODE_FULL); /*Set an initialized buffer*/

#else
    void *buf3 = (void *)((uint32_t)buf2 + u32FBSize);

    LV_LOG_INFO("Use two screen-size shadow buffer, 0x%08x, 0x%08x.", buf2, buf3);

    lv_display_set_flush_cb(disp, lv_port_disp_partial);               /*Set a flush callback to draw to the display*/
    lv_display_set_flush_wait_cb(disp, lv_port_disp_flush_wait);       /*Set a flush wait callback*/
    lv_display_set_buffers(disp, buf2, buf3, u32FBSize, LV_DISPLAY_RENDER_MODE_PARTIAL); /*Set an initialized buffer*/
#endif
}
