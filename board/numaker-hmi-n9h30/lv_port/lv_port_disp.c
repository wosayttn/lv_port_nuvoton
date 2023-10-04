/**************************************************************************//**
 * @file     lv_port_disp.c
 * @brief    lvgl display port
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/

#include "lvgl.h"
#include "lv_glue.h"
#if (CONFIG_LV_GPU_USE_N9H30_2DGE==1)
    #include "lv_gpu_n9h30_2dge.h"
#endif

static void lv_port_disp_partial_update(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p)
{
    int32_t w = lv_area_get_width(area);

    int32_t h = lv_area_get_height(area);

    S_LCD_INFO *psLCDInfo = (S_LCD_INFO *)disp_drv->user_data;

    lv_color_t *pDisp = (lv_color_t *)((uint32_t)psLCDInfo->pvVramStartAddr | BIT31) + (LV_HOR_RES_MAX * area->y1 + area->x1);

    /* Update dirty region. */
    /* Only catch full-screen case due to GDMA constraint(No stride feature). */
    if ((lv_area_get_size(area) == (LV_HOR_RES_MAX * LV_VER_RES_MAX)))
    {
#define FULL_SCREEN_BYTE_COUNT (LV_HOR_RES_MAX * LV_VER_RES_MAX * sizeof(lv_color_t))

        sysCleanDcache((UINT32)color_p, FULL_SCREEN_BYTE_COUNT);

        /* Disable/Reset GDMA Channel 0 */
        outpw(REG_GDMA_CTL0,  0);

        /* GDMA Channel 0 Source Base Address Register */
        outpw(REG_GDMA_SRCB0, (uint32_t)color_p);

        /* GDMA Channel 0 Destination Base Address Register */
        outpw(REG_GDMA_DSTB0, (uint32_t)psLCDInfo->pvVramStartAddr);

        /* GDMA Channel 0 Transfer byte count */
        outpw(REG_GDMA_TCNT0, FULL_SCREEN_BYTE_COUNT >> 5);

        /* Configurae GDMAEN, BME, TWS=2, SoftReq field of GDMA Channel 0.*/
        outpw(REG_GDMA_CTL0, inpw(REG_GDMA_CTL0) | 0x12003);

        while (!(inpw(REG_GDMA_INTCS) & 0x100));

        outpw(REG_GDMA_INTCS, 0x100);
    }
    else
    {
        int32_t x, y;

        for (y = 0; y < h; y++)
        {
            for (x = 0; x < w; x++)
            {
                pDisp[x] = color_p[x];
            }
            pDisp += LV_HOR_RES_MAX;
            color_p += w;
        }
    }

    lv_disp_flush_ready(disp_drv);
}

static void *buf3_next = NULL;
static void lv_port_disp_full_refresh(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p)
{
    sysCleanDcache((UINT32)color_p, lv_area_get_size(area)* sizeof(lv_color_t));

    /* Use PANDISPLAY without H/W copying */
    LV_ASSERT(lcd_device_control(evLCD_CTRL_PAN_DISPLAY, (void *)color_p) == 0);

    if (buf3_next)
    {
        /* vsync-none: Use triple screen-sized buffers. */
        if (disp_drv->draw_buf->buf1 == color_p)
            disp_drv->draw_buf->buf1 = buf3_next;
        else
            disp_drv->draw_buf->buf2 = buf3_next;

        disp_drv->draw_buf->buf_act = buf3_next;
        buf3_next = color_p;
    }
    else
    {
        /* vsync-after: Use ping-pong screen-sized buffers only.*/
        LV_ASSERT(lcd_device_control(evLCD_CTRL_WAIT_VSYNC, (void *)NULL) == 0);
    }

    lv_disp_flush_ready(disp_drv);
}

void lv_port_disp_init(void)
{
    static lv_disp_draw_buf_t disp_buf;
    static lv_disp_drv_t disp_drv;  /*Descriptor of a display driver*/
    static S_LCD_INFO sLcdInfo = {0};
    void *buf1, *buf2;
    uint32_t u32FBSize;

    /* Initial display device */
    LV_ASSERT(lcd_device_initialize() == 0);
    LV_ASSERT(lcd_device_open() == 0);
    LV_ASSERT(lcd_device_control(evLCD_CTRL_GET_INFO, (void *)&sLcdInfo) == 0);
    u32FBSize = sLcdInfo.u32ResHeight * sLcdInfo.u32ResWidth * sLcdInfo.u32BytePerPixel;

    lv_disp_drv_init(&disp_drv); /*Basic initialization*/

    /*Set the resolution of the display*/
    disp_drv.hor_res = sLcdInfo.u32ResWidth;
    disp_drv.ver_res = sLcdInfo.u32ResHeight;
    disp_drv.user_data = &sLcdInfo;
    disp_drv.full_refresh = CONFIG_LV_DISP_FULL_REFRESH;
    if (disp_drv.full_refresh)
    {
        disp_drv.flush_cb = lv_port_disp_full_refresh;

        buf1 = (void *)((uint32_t)sLcdInfo.pvVramStartAddr);
        buf2 = (void *)((uint32_t)buf1 + u32FBSize);
        buf3_next = (void *)((uint32_t)buf2 + u32FBSize);

        /*Initialize `disp_buf` with the buffer(s).*/
        lv_disp_draw_buf_init(&disp_buf, buf1, buf2, sLcdInfo.u32ResHeight * sLcdInfo.u32ResWidth);

        LV_LOG_INFO("Use triple screen-size shadow buffer, buf1: 0x%08x, buf2: 0x%08x, buf3_next: 0x%08x", buf1, buf2, buf3_next);
    }
    else
    {
        disp_drv.flush_cb = lv_port_disp_partial_update;

        buf1 = (void *)(((uint32_t)sLcdInfo.pvVramStartAddr) + u32FBSize);

        /*Initialize `disp_buf` with the buffer(s).*/
        lv_disp_draw_buf_init(&disp_buf, buf1, NULL, sLcdInfo.u32ResHeight * sLcdInfo.u32ResWidth);

        LV_LOG_INFO("Use one screen-size shadow buffer, buf1: 0x%08x", buf1);
    }

    /*Set a display buffer*/
    disp_drv.draw_buf = &disp_buf;

#if (CONFIG_LV_GPU_USE_N9H30_2DGE==1)
    disp_drv.draw_ctx_init = lv_draw_n9h30_2dge_ctx_init;
    disp_drv.draw_ctx_deinit = lv_draw_n9h30_2dge_ctx_init;
    disp_drv.draw_ctx_size = sizeof(lv_draw_n9h30_2dge_ctx_t);
#endif

    /*Finally register the driver*/
    lv_disp_drv_register(&disp_drv);
}
