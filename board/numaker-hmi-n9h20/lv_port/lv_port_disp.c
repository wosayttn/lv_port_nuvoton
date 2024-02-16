/**************************************************************************//**
 * @file     lv_port_disp.c
 * @brief    lvgl display port
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/

#include "lvgl.h"
#include "lv_glue.h"
#if (CONFIG_LV_GPU_USE_N9H20_BLT==1)
    #include "lv_gpu_n9h20_blt.h"
#endif

static void *buf3_next = NULL;

#if CONFIG_LV_DISP_FULL_REFRESH
static void lv_port_disp_full(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    sysCleanDcache((UINT32)px_map, (UINT32)lv_area_get_size(area) * (LV_COLOR_DEPTH / 8));

    /* Use PANDISPLAY without H/W copying */
    LV_ASSERT(lcd_device_control(evLCD_CTRL_PAN_DISPLAY, (void *)px_map) == 0);

#if 0
    // TODO
    if (buf3_next)
    {
        /* vsync-none: Use triple screen-sized buffers. */
        if (disp->buf_1->data == px_map)
            disp->buf_1->data = buf3_next;
        else
            disp->buf_2->data = buf3_next;

        disp->buf_act->data = buf3_next;
        buf3_next = px_map;
    }
    else
#endif
    {
        /* vsync-after: Use ping-pong screen-sized buffers only.*/
        LV_ASSERT(lcd_device_control(evLCD_CTRL_WAIT_VSYNC, (void *)NULL) == 0);
    }

    lv_display_flush_ready(disp);
}
#else
static void lv_port_disp_partial(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    int32_t w = lv_area_get_width(area);

    int32_t h = lv_area_get_height(area);

    S_LCD_INFO *psLCDInfo = (S_LCD_INFO *)lv_display_get_driver_data(disp);

#if (LV_COLOR_DEPTH==32)

    uint32_t *pDisp = (uint32_t *)((uint32_t)psLCDInfo->pvVramStartAddr | BIT31) + (LV_HOR_RES_MAX * area->y1 + area->x1);
    uint32_t *pSrc = (uint32_t *)px_map;

#elif (LV_COLOR_DEPTH==16)

    uint16_t *pDisp = (uint16_t *)((uint32_t)psLCDInfo->pvVramStartAddr | BIT31) + (LV_HOR_RES_MAX * area->y1 + area->x1);
    uint16_t *pSrc = (uint16_t *)px_map;

#endif

    /* Update dirty region. */
    if (lv_area_get_size(area) > (LV_HOR_RES_MAX * LV_VER_RES_MAX / 4))
    {
        E_DRVEDMA_CHANNEL_INDEX evVDMAIdx;

        LV_ASSERT((evVDMAIdx = (E_DRVEDMA_CHANNEL_INDEX)VDMA_FindandRequest()) == 0);

        sysCleanDcache((UINT32) px_map, (UINT32) lv_area_get_size(area) * (LV_COLOR_DEPTH / 8));

        EDMA_SetupSingle(evVDMAIdx, (uint32_t)px_map, (uint32_t)pDisp, w * h * (LV_COLOR_DEPTH / 8));

        DrvEDMA_SetSourceStride(evVDMAIdx, w * (LV_COLOR_DEPTH / 8), 0);

        DrvEDMA_SetDestinationStrideOffset(evVDMAIdx, (psLCDInfo->u32ResWidth - w) * (LV_COLOR_DEPTH / 8));

        outp32(REG_VDMA_CTCSR, inp32(REG_VDMA_CTCSR) | STRIDE_EN);

        EDMA_Trigger(evVDMAIdx);

        while (EDMA_IsBusy(evVDMAIdx));

        EDMA_Free(evVDMAIdx);
    }
    else
    {
        int32_t x, y;

        for (y = 0; y < h; y++)
        {
            for (x = 0; x < w; x++)
            {
                pDisp[x] = pSrc[x];
            }
            pDisp += LV_HOR_RES_MAX;
            pSrc += w;
        }
    }

    lv_display_flush_ready(disp);
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
    buf3_next = (void *)((uint32_t)buf2 + u32FBSize);

    disp = lv_display_create(sLcdInfo.u32ResWidth, sLcdInfo.u32ResHeight);
    LV_ASSERT(disp != NULL);

    lv_display_set_driver_data(disp, &sLcdInfo);

#if CONFIG_LV_DISP_FULL_REFRESH
    LV_LOG_INFO("Use triple screen-size shadow buffer, buf1: 0x%08x, buf2: 0x%08x, buf3_next: 0x%08x", buf1, buf2, buf3_next);

    lv_display_set_flush_cb(disp, lv_port_disp_full); /*Set a flush callback to draw to the display*/

    lv_display_set_buffers(disp, buf1, buf2, u32FBSize, LV_DISPLAY_RENDER_MODE_FULL); /*Set an initialized buffer*/

#else
    LV_LOG_INFO("Use two screen-size shadow buffer, 0x%08x, 0x%08x.", buf2, buf3_next);

    lv_display_set_flush_cb(disp, lv_port_disp_partial); /*Set a flush callback to draw to the display*/

    lv_display_set_buffers(disp, buf2, buf3_next, u32FBSize, LV_DISPLAY_RENDER_MODE_PARTIAL); /*Set an initialized buffer*/
#endif


#if (CONFIG_LV_GPU_USE_N9H20_BLT==1)
    disp_drv.draw_ctx_init = lv_draw_n9h20_blt_ctx_init;
    disp_drv.draw_ctx_deinit = lv_draw_n9h20_blt_ctx_init;
    disp_drv.draw_ctx_size = sizeof(lv_draw_n9h20_blt_ctx_t);
#endif

}
