/**************************************************************************//**
 * @file     lv_port_disp.c
 * @brief    lvgl display port
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/

#include "lvgl.h"
#include "lv_glue.h"

#include<math.h>
#define PI 3.14159265

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

static volatile E_DRVEDMA_CHANNEL_INDEX evVDMAIdx = -1;
static uint8_t s_au8RotateBuf[LV_HOR_RES_MAX * LV_VER_RES_MAX * (LV_COLOR_DEPTH / 8) + DEF_CACHE_LINE_SIZE] __attribute__((aligned(DEF_CACHE_LINE_SIZE)));
static void lv_port_disp_partial(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    lv_color_format_t cf = lv_display_get_color_format(disp);
    uint8_t px_sz = lv_color_format_get_size(cf);
    S_LCD_INFO *psLCDInfo = (S_LCD_INFO *)lv_display_get_driver_data(disp);

    uint32_t w = lv_area_get_width(area);
    uint32_t h = lv_area_get_height(area);
    uint32_t w_stride = lv_draw_buf_width_to_stride(w, cf);
    uint32_t h_stride = lv_draw_buf_width_to_stride(h, cf);
    lv_area_t rotated_area = *area;

    lv_display_rotation_t rot = lv_display_get_rotation(disp);

    LV_ASSERT(psLCDInfo != NULL);
    if (rot != LV_DISPLAY_ROTATION_0)
    {
        int bitbltRotate(const void *src, void *dest, int32_t src_width, int32_t src_height, int32_t src_stride,
                         int32_t src_x, int32_t src_y, int32_t dest_width, int32_t dest_height, int32_t dest_stride,
                         lv_display_rotation_t rot, lv_color_format_t color_format);

        if (bitbltRotate(px_map, psLCDInfo->pvVramStartAddr, w, h, w_stride, area->x1, area->y1, psLCDInfo->u32ResWidth, psLCDInfo->u32ResHeight, psLCDInfo->u32ResWidth * px_sz, rot, cf) == 0)  //OK
        {
            return;
        }
        else
        {
            /* Failed to use H/W rotation, switch to S/W. */
            switch (rot)
            {
            case LV_DISPLAY_ROTATION_0:
                break;
            case LV_DISPLAY_ROTATION_90:
                lv_draw_sw_rotate(px_map, &s_au8RotateBuf[0], w, h, w_stride, h_stride, rot, cf);
                break;
            case LV_DISPLAY_ROTATION_180:
                lv_draw_sw_rotate(px_map, &s_au8RotateBuf[0], w, h, w_stride, w_stride, rot, cf);
                break;
            case LV_DISPLAY_ROTATION_270:
                lv_draw_sw_rotate(px_map, &s_au8RotateBuf[0], w, h, w_stride, h_stride, rot, cf);
                break;
            }
            lv_display_rotate_area(disp, &rotated_area);
            area = &rotated_area;
            px_map = &s_au8RotateBuf[0];
        }
    }

    /* Ensure that we're within the framebuffer's bounds */
    if (area->x2 < 0 || area->y2 < 0 || area->x1 > (int32_t)psLCDInfo->u32ResWidth - 1 || area->y1 > (int32_t)psLCDInfo->u32ResHeight - 1)
    {
        return;
    }

    {
        w = lv_area_get_width(area);
        h = lv_area_get_height(area);

        uint32_t u32Disp = (uint32_t)psLCDInfo->pvVramStartAddr + (psLCDInfo->u32ResWidth * area->y1 + area->x1) * px_sz;
        /* Update dirty region. */
        evVDMAIdx = (E_DRVEDMA_CHANNEL_INDEX)VDMA_FindandRequest();
        if (evVDMAIdx == 0) //VDMA channel
        {
            sysCleanDcache((UINT32) px_map, (UINT32) lv_area_get_size(area) * px_sz);

            EDMA_SetupSingle(evVDMAIdx, (uint32_t)px_map, u32Disp, w * h * px_sz);

            DrvEDMA_SetSourceStride(evVDMAIdx, w * px_sz, 0);

            DrvEDMA_SetDestinationStrideOffset(evVDMAIdx, (psLCDInfo->u32ResWidth - w) * px_sz);

            void vdmaISR(unsigned int arg);
            EDMA_SetupHandlers(evVDMAIdx, eDRVEDMA_BLKD_FLAG, vdmaISR, 0);

            outp32(REG_VDMA_CTCSR, inp32(REG_VDMA_CTCSR) | STRIDE_EN);

            EDMA_Trigger(evVDMAIdx);
        }
    }
}

static void lv_port_disp_flush_wait(lv_display_t *disp)
{
    if (evVDMAIdx == 0) //Initialized
    {
        void EDMA_WaitForCompletion();
        EDMA_WaitForCompletion();

        EDMA_Free(evVDMAIdx);
        evVDMAIdx = -1;
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

#if defined(CONFIG_LV_DISP_ROTATION)
    lv_display_set_rotation(disp, CONFIG_LV_DISP_ROTATION);
#endif
    lv_display_set_flush_cb(disp, lv_port_disp_partial);               /*Set a flush callback to draw to the display*/
    lv_display_set_flush_wait_cb(disp, lv_port_disp_flush_wait);       /*Set a flush wait callback*/
    lv_display_set_buffers(disp, buf2, buf3, u32FBSize, LV_DISPLAY_RENDER_MODE_PARTIAL); /*Set an initialized buffer*/
#endif
}
