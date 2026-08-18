/****************************************************************************
 * @file     lv_port_disp.c
 * @brief    lvgl display port
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/

#include "FreeRTOS.h"
#include "lv_glue.h"
#include "lvgl.h"
#include "task.h"

#if defined(LV_USE_DRAW_GFX) && (LV_USE_DRAW_GFX == 1)
    #include "libgfx.h" /* gfx_blt(), gfx_surface_t, gfx_rect_t, GFX_* formats */
#endif

#if CONFIG_DISP_DIRECT_REFRESH

static void lv_port_disp_direct(lv_display_t *disp, const lv_area_t *area,
                                uint8_t *px_map)
{
    S_LCD_INFO *psLCDInfo = (S_LCD_INFO *)lv_display_get_driver_data(disp);
    uint32_t bpp = psLCDInfo->u32BytePerPixel;
    uint32_t stride = psLCDInfo->u32ResWidth * bpp;

    LV_UNUSED(area);

    if (lv_display_flush_is_last(disp))
    {
        dcache_clean_by_mva((const void *)px_map, psLCDInfo->u32ResHeight * stride);

        /*Pan display to the newly rendered active framebuffer*/
        LV_ASSERT(lcd_device_control(evLCD_CTRL_PAN_DISPLAY, (void *)px_map) == 0);
    }

    lv_display_flush_ready(disp);
}

#else

static void *buf3_next = NULL;
static void lv_port_disp_partial(lv_display_t *disp, const lv_area_t *area,
                                 uint8_t *px_map)
{
    S_LCD_INFO *psLCDInfo = (S_LCD_INFO *)lv_display_get_driver_data(disp);

#if defined(LV_USE_DRAW_GFX) && (LV_USE_DRAW_GFX == 1)

    int ret;

    gfx_surface_t src_surface;
    gfx_surface_t dst_surface;

    uint32_t bpp = psLCDInfo->u32BytePerPixel;
    int32_t area_width = lv_area_get_width(area);
    int32_t area_height = lv_area_get_height(area);

    memset(&src_surface, 0, sizeof(src_surface));
    src_surface.format = (bpp == 4) ? GFX_ARGB8888 : GFX_RGB565;
    src_surface.width = area_width;
    src_surface.height = area_height;
    src_surface.stride = area_width * bpp; // MUST BE 16B aligned
    src_surface.planes[0] = (int)((uintptr_t)px_map & 0xFFFFFFFFU);
    src_surface.rect.tl.x = 0;
    src_surface.rect.tl.y = 0;
    src_surface.rect.br.x = area_width;
    src_surface.rect.br.y = area_height;

    memset(&dst_surface, 0, sizeof(dst_surface));
    dst_surface.format = (bpp == 4) ? GFX_ARGB8888 : GFX_RGB565;
    dst_surface.width = psLCDInfo->u32ResWidth;
    dst_surface.height = psLCDInfo->u32ResHeight;
    dst_surface.stride = dst_surface.width * bpp; // MUST BE 16B aligned
    dst_surface.planes[0] =
        (int)((uintptr_t)psLCDInfo->pvVramStartAddr & 0xFFFFFFFFU);
    dst_surface.rect.tl.x = area->x1;
    dst_surface.rect.tl.y = area->y1;
    dst_surface.rect.br.x = area->x2 + 1;
    dst_surface.rect.br.y = area->y2 + 1;

    dcache_clean_by_mva((const void *)px_map,
                        src_surface.height * src_surface.stride);

    gfx_osal_lock(GFX_OSAL_WAIT_FOREVER);
    ret = gfx_blt(g_gfx_disp_handle, &src_surface, &dst_surface);
    if (ret != 0)
    {
        LV_LOG_ERROR("gfx_blt(area -> dst_surface) returned %d", ret);
        gfx_osal_unlock();
        goto fail;
    }

    gfx_finish(g_gfx_disp_handle);
    gfx_osal_unlock();

#else

    /* Update dirty region. */
    int32_t x, y;
    int32_t w = lv_area_get_width(area);
    int32_t h = lv_area_get_height(area);
    uint32_t *pDisp = (uint32_t *)nc_ptr(
                          psLCDInfo->pvVramStartAddr +
                          (psLCDInfo->u32ResWidth * area->y1 + area->x1) * sizeof(uint32_t));
    uint32_t *pSrc = (uint32_t *)px_map;

    for (y = 0; y < h; y++)
    {
        for (x = 0; x < w; x++)
        {
            pDisp[x] = pSrc[x];
        }
        pDisp += psLCDInfo->u32ResWidth;
        pSrc += w;
    }
#endif

fail:

    lv_display_flush_ready(disp);
}

#endif

#if defined(LV_USE_DRAW_GFX) && (LV_USE_DRAW_GFX == 1)
/**
 * @brief  Display invalidated event callback (Rounder Callback).
 * @note   Aligns the invalidated area coordinates (x1, x2) so that the
 *         resulting width in bytes is always a multiple of 16, satisfying
 *         the 2D GFX hardware acceleration requirements.
 * @param  e Pointer to the LVGL event structure.
 */
static void disp_invalidated_event_cb(lv_event_t *e)
{
    lv_area_t *area = lv_event_get_param(e);
    lv_display_t *disp = lv_event_get_target(e);
    S_LCD_INFO *psLCDInfo = (S_LCD_INFO *)lv_display_get_driver_data(disp);

    uint32_t bpp = psLCDInfo->u32BytePerPixel;
    uint32_t align_pixels = 16 / bpp;

    area->x1 = (area->x1 / align_pixels) * align_pixels;

    area->x2 = ((area->x2 + align_pixels) / align_pixels) * align_pixels - 1;

    if (area->x2 >= (int32_t)psLCDInfo->u32ResWidth)
    {
        area->x2 = psLCDInfo->u32ResWidth - 1;
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

    u32FBSize =
        sLcdInfo.u32ResHeight * sLcdInfo.u32ResWidth * sLcdInfo.u32BytePerPixel;
    buf1 = (void *)sLcdInfo.pvVramStartAddr;
    buf2 = (void *)buf1 + u32FBSize;

    disp = lv_display_create(sLcdInfo.u32ResWidth, sLcdInfo.u32ResHeight);
    LV_ASSERT(disp != NULL);

    lv_display_set_driver_data(disp, &sLcdInfo);

#if CONFIG_DISP_DIRECT_REFRESH

    static lv_draw_buf_t s_static_buf3;
    void *buf3 = (void *)buf2 + u32FBSize;

#if defined(LV_USE_DRAW_GFX) && (LV_USE_DRAW_GFX == 1)
    lv_display_add_event_cb(disp, disp_invalidated_event_cb,
                            LV_EVENT_INVALIDATE_AREA, NULL);
#endif

    lv_display_set_flush_cb(disp, lv_port_disp_direct);
    lv_display_set_buffers(disp, buf1, buf2, u32FBSize, LV_DISPLAY_RENDER_MODE_DIRECT);

    lv_color_format_t cf = lv_display_get_color_format(disp);
    uint32_t stride = lv_draw_buf_width_to_stride(sLcdInfo.u32ResWidth, cf);
    lv_draw_buf_init(&s_static_buf3, sLcdInfo.u32ResWidth, sLcdInfo.u32ResHeight,
                     cf, stride, buf3, u32FBSize);
    lv_display_set_3rd_draw_buffer(disp, &s_static_buf3);

#else
    buf3_next = (void *)(buf2 + u32FBSize);
    LV_LOG_INFO(
        "Use two screen-size shadow buffer (PARTIAL refresh), 0x%08x, 0x%08x.",
        buf2, buf3_next);

#if defined(LV_USE_DRAW_GFX) && (LV_USE_DRAW_GFX == 1)
    lv_display_add_event_cb(disp, disp_invalidated_event_cb,
                            LV_EVENT_INVALIDATE_AREA, NULL);
#endif

    lv_display_set_flush_cb(disp, lv_port_disp_partial);
    lv_display_set_buffers(disp, buf2, buf3_next, u32FBSize,
                           LV_DISPLAY_RENDER_MODE_PARTIAL);
#endif
}
