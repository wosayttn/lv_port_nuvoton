/**************************************************************************//**
 * @file     lv_port_disp.c
 * @brief    lvgl display port
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/

#include "lvgl.h"
#include "lv_glue.h"

#if defined(LV_USE_DRAW_GFX)
    #include "gfxlib.h"     /* gfxlib_init(), GFXLIB_Clear(), GFXLIB_Blit(), GFX_STATUS_* */
    #include "gfx_osal.h"   /* gfx_osal_printf(), gfx_va_to_pa() */
    #include "gfx_mem.h"    /* gfx_mem_init(), gfx_mem_alloc() */
    #include "gfx_disp.h"   /* gfx_disp_init(), gfx_disp_enable() */
    #include "gfx_cmd_2d_decode.h"    /* gfx_cmd_2d_decode_dump() */

    #define GFX_ALIGN_SURFACE_STRIDE(x)         (((x) + 15u) & ~15u)
#endif

#if CONFIG_DISP_FULL_REFRESH
static void lv_port_disp_full(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    S_LCD_INFO *psLCDInfo = (S_LCD_INFO *)lv_display_get_driver_data(disp);

    dcache_clean_by_mva(px_map, lv_area_get_size(area) * (psLCDInfo->u32BytePerPixel));

    /* Use PANDISPLAY without H/W copying */
    LV_ASSERT(lcd_device_control(evLCD_CTRL_PAN_DISPLAY, (void *)px_map) == 0);

    /* vsync-after: Use ping-pong screen-sized buffers only.*/
    LV_ASSERT(lcd_device_control(evLCD_CTRL_WAIT_VSYNC, (void *)NULL) == 0);

    lv_display_flush_ready(disp);
}

#else

static void *buf3_next = NULL;
static void lv_port_disp_partial(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    S_LCD_INFO *psLCDInfo = (S_LCD_INFO *)lv_display_get_driver_data(disp);

#if defined(LV_USE_DRAW_GFX)

    int ret;

    GFX_Surface src_surface;
    GFX_Surface dst_surface;
    GFX_Rect    dst_rect;

    uint32_t bpp = psLCDInfo->u32BytePerPixel;
    int32_t area_width  = lv_area_get_width(area);
    int32_t area_height = lv_area_get_height(area);

    src_surface.format = (bpp == 4) ? GFX_FORMAT_ARGB8888 : GFX_FORMAT_RGB565;
    src_surface.width  = area_width;
    src_surface.height = area_height;

    src_surface.stride = area_width * bpp; // MUST BE 16B aligned
    src_surface.pa     = gfx_va_to_pa((uint32_t)((uintptr_t)px_map & 0xFFFFFFFFU));

    dst_surface.format = (bpp == 4) ? GFX_FORMAT_ARGB8888 : GFX_FORMAT_RGB565;
    dst_surface.width  = psLCDInfo->u32ResWidth;
    dst_surface.height = psLCDInfo->u32ResHeight;
    dst_surface.stride = dst_surface.width * bpp;  // MUST BE 16B aligned
    dst_surface.pa     = gfx_va_to_pa((uint32_t)((uintptr_t)psLCDInfo->pvVramStartAddr & 0xFFFFFFFFU));

    dst_rect.left   = area->x1;
    dst_rect.top    = area->y1;
    dst_rect.right  = area->x2 + 1;
    dst_rect.bottom = area->y2 + 1;

    dcache_clean_by_mva((const void *)px_map, src_surface.height * src_surface.stride);
    gfx_va_t cursor0 = gfx_mem_mark();
    ret = GFXLIB_Blit(&src_surface, NULL, &dst_surface, &dst_rect);
    gfx_va_t cursor1 = gfx_mem_mark();

    //gfx_cmd_2d_decode_dump((const uint32_t *)cursor0, (uint32_t)((uintptr_t)(cursor1-cursor0))/4);

    gfx_mem_reset();
    if (ret != GFX_STATUS_OK)
    {
        LV_LOG_ERROR("GFXLIB_Blit(area -> dst_surface) returned %d", ret);
        goto fail;
    }

#else

    /* Update dirty region. */
    int32_t x, y;
    int32_t w = lv_area_get_width(area);
    int32_t h = lv_area_get_height(area);
    uint32_t *pDisp = (uint32_t *)nc_ptr(psLCDInfo->pvVramStartAddr + (psLCDInfo->u32ResWidth * area->y1 + area->x1) * sizeof(uint32_t));
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

#if defined(LV_USE_DRAW_GFX)
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

    u32FBSize = sLcdInfo.u32ResHeight * sLcdInfo.u32ResWidth * sLcdInfo.u32BytePerPixel;
    buf1 = (void *)sLcdInfo.pvVramStartAddr;
    buf2 = (void *)buf1 + u32FBSize;

    disp = lv_display_create(sLcdInfo.u32ResWidth, sLcdInfo.u32ResHeight);
    LV_ASSERT(disp != NULL);

    lv_display_set_driver_data(disp, &sLcdInfo);

#if CONFIG_DISP_FULL_REFRESH

    LV_LOG_INFO("Use two screen-size buffer, buf1: 0x%08x, buf2: 0x%08x: 0x%08x", buf1, buf2);
    lv_color_format_t cf = lv_display_get_color_format(disp);

    lv_display_set_flush_cb(disp, lv_port_disp_full); /*Set a flush callback to draw to the display*/
    lv_display_set_buffers(disp, buf1, buf2, u32FBSize, LV_DISPLAY_RENDER_MODE_FULL); /*Set an initialized buffer*/

#else
    buf3_next = (void *)(buf2 + u32FBSize);
    LV_LOG_INFO("Use two screen-size shadow buffer, 0x%08x, 0x%08x.", buf2, buf3_next);

#if defined(LV_USE_DRAW_GFX)
    lv_display_add_event_cb(disp, disp_invalidated_event_cb, LV_EVENT_INVALIDATE_AREA, NULL);
#endif

    lv_display_set_flush_cb(disp, lv_port_disp_partial);
    lv_display_set_buffers(disp, buf2, buf3_next, u32FBSize, LV_DISPLAY_RENDER_MODE_PARTIAL);
#endif
}
