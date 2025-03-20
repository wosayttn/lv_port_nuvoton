#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "lv_glue.h"

#if (LV_USE_OS==LV_OS_FREERTOS)
    static SemaphoreHandle_t s_xBITBLTSem = NULL;
#endif

void bitbltWaitForCompletion(void)
{
#if (LV_USE_OS==LV_OS_FREERTOS)
    while (xSemaphoreTake(s_xBITBLTSem, portMAX_DELAY) != pdTRUE);
#endif
}

#if (LV_USE_OS==LV_OS_FREERTOS)
void bitbltISR(void)
{
    UINT32 bltIntStat = inp32(REG_BLTINTCR);

    /* BLT complete */
    if (bltIntStat & BLT_INTS)
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;

        /* clear interrupt (write 1 to clear) */
        outp32(REG_BLTINTCR, (bltIntStat & BLT_INTE) | BLT_INTS);

        xSemaphoreGiveFromISR(s_xBITBLTSem, &xHigherPriorityTaskWoken);

        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}
#endif

static int s_bitbltIsInit = 0;
void bitbltInterruptInit(void)
{
    if (s_bitbltIsInit) return;

    bltOpen();

#if (LV_USE_OS==LV_OS_FREERTOS)
    s_xBITBLTSem = xSemaphoreCreateBinary();
    LV_ASSERT(s_xBITBLTSem != NULL);

    outp32(REG_BLTINTCR, inp32(REG_BLTINTCR) | BLT_INTE);

    sysInstallISR(HIGH_LEVEL_SENSITIVE | IRQ_LEVEL_1, IRQ_BLT, (PVOID)bitbltISR);
    sysSetLocalInterrupt(ENABLE_IRQ);
    sysEnableInterrupt(IRQ_BLT);
#endif

    s_bitbltIsInit = 1;

}

void bitbltInterruptDeinit(void)
{
    if (!s_bitbltIsInit) return;

#if (LV_USE_OS==LV_OS_FREERTOS)
    vSemaphoreDelete(s_xBITBLTSem);
#endif

    bltClose();

    s_bitbltIsInit = 0 ;
}

#define PI_OVER_180 0.01745329252f
int bitbltRotate(const void *src, void *dest, int32_t src_width, int32_t src_height, int32_t src_stride,
                 int32_t src_x, int32_t src_y, int32_t dest_width, int32_t dest_height, int32_t dest_stride,
                 lv_display_rotation_t rot, lv_color_format_t color_format)
{
    float deg = 360.0f - (rot * 90.0f);
    float dst_x = 0;
    float dst_y = 0;
    uint32_t px_size;

    bitbltInterruptInit();

    //void bitbltRotate(float dst_x, float dst_y, float px, float py, uint32_t deg)
    //sysprintf("Src(W:%d H:%d, Stride: %d) -%d-> Dst(W:%d H:%d, X:%d, Y:%d, Stride: %d)\n",
    //          src_width, src_height, src_stride,
    //          rot,
    //          dest_width, dest_height, src_x, src_y, dest_stride);

    /* H/W limitation */
    /* 1. SRC/DEST START BUFFER ADDRESS MUST BE WORD-ALIGNED. */
    /* 2. SRC/DEST Stride MUST BE WORD-ALIGNED. */
    if (((uint32_t)src & 0x3u) || ((uint32_t)dest & 0x3u) || (src_stride & 0x3u) || (dest_stride & 0x3u))
        return -1;

    switch (color_format)
    {
    case LV_COLOR_FORMAT_RGB565:
        bltSetDisplayFormat(eDRVBLT_DEST_RGB565);
        bltSetSrcFormat(eDRVBLT_SRC_RGB565);
        px_size = 2;
        break;

    case LV_COLOR_FORMAT_XRGB8888:
        bltSetRevealAlpha(eDRVBLT_NO_EFFECTIVE);    // Source image format is non-premultiplied alpha.

    case LV_COLOR_FORMAT_ARGB8888:
        bltSetSrcFormat(eDRVBLT_SRC_ARGB8888);
        bltSetDisplayFormat(eDRVBLT_DEST_ARGB8888);
        px_size = 4;
        break;

    default:
        return -1;
    }

    switch (rot)
    {
    case LV_DISPLAY_ROTATION_0:
        dst_x = src_x;
        dst_y = src_y;
        break;
    case LV_DISPLAY_ROTATION_90:
        dst_x = src_y;
        dst_y = dest_height - src_x;
        break;
    case LV_DISPLAY_ROTATION_180:
        dst_x = dest_width - src_x;
        dst_y = dest_height - src_y;
        break;
    case LV_DISPLAY_ROTATION_270:
        dst_x = dest_width - src_y;
        dst_y = src_x;
        break;
    }

    /* Flush src image data in cache into system memory. */
    sysCleanDcache((UINT32)src, (UINT32) src_height * src_stride);

    bltSetFillOP((E_DRVBLT_FILLOP) FALSE);  // Blit operation.
    bltSetRevealAlpha(eDRVBLT_NO_EFFECTIVE);    // Source image format is non-premultiplied alpha.

    {
        // Set transform matrix a/b/c/d
        S_DRVBLT_MATRIX xform_mx = {0};

        xform_mx.a  =   cos(PI_OVER_180 * deg) * 0x10000;
        xform_mx.b  =  -sin(PI_OVER_180 * deg) * 0x10000;
        xform_mx.c  =   sin(PI_OVER_180 * deg) * 0x10000;
        xform_mx.d  =   cos(PI_OVER_180 * deg) * 0x10000;

        bltSetTransformMatrix(xform_mx);
    }

    {
        // Set color multiplier for color transform.
        S_DRVBLT_ARGB16 color_multiplier = {0};

        color_multiplier.i16Blue    =   0x100;
        color_multiplier.i16Green   =   0x100;
        color_multiplier.i16Red     =   0x100;
        color_multiplier.i16Alpha   =   0x100;

        bltSetColorMultiplier(color_multiplier);
    }

    {
        // Set color offset for color transform
        S_DRVBLT_ARGB16 color_offset = {0};

        color_offset.i16Blue    =   0;
        color_offset.i16Green   =   0;
        color_offset.i16Red     =   0;
        color_offset.i16Alpha   =   0;

        bltSetColorOffset(color_offset);
    }

    // Apply color transformation on all 4 channels.
    //if (SRCIMG_HASALPHA)
    //{
    //    bltSetTransformFlag(eDRVBLT_HASTRANSPARENCY | eDRVBLT_HASCOLORTRANSFORM);
    //}
    //else
    //{
    bltSetTransformFlag(eDRVBLT_HASCOLORTRANSFORM);
    //}

    bltSetFillStyle((E_DRVBLT_FILL_STYLE)(eDRVBLT_NONE_FILL | eDRVBLT_NOTSMOOTH));  // No smoothing.

    {
        // Set source image.
        S_DRVBLT_SRC_IMAGE src_img = {0};

        src_img.u32SrcImageAddr = (uint32_t)src;
        {
            S_DRVBLT_MATRIX xform_mx = {0};

            // Pivot
            src_img.i32XOffset = 0 * 0x10000;      // 16.16
            src_img.i32YOffset = 0 * 0x10000;      // 16.16

            // Translate after rotate
            src_img.i32XOffset += -(cos(PI_OVER_180 * deg) * dst_x + sin(PI_OVER_180 * deg) * dst_y) * 0x10000;    // 16.16
            src_img.i32YOffset += (sin(PI_OVER_180 * deg) * dst_x - cos(PI_OVER_180 * deg) * dst_y) * 0x10000;     // 16.16

            // Apply amendment to mapping point error.
            bltGetTransformMatrix(&xform_mx);
            src_img.i32XOffset += (xform_mx.a + xform_mx.c) / 2;
            src_img.i32YOffset += (xform_mx.b + xform_mx.d) / 2;
        }
        src_img.i16Width = src_width;
        src_img.i32Stride = src_stride;
        src_img.i16Height = src_height;

        bltSetSrcImage(src_img);
    }

    {
        // Set destination buffer.
        S_DRVBLT_DEST_FB dst_img = {0};

        dst_img.u32FrameBufAddr = (uint32_t)dest;
        dst_img.i32Stride = dest_stride;
        dst_img.i16Width = dest_width;
        dst_img.i16Height = dest_height;
        dst_img.i32XOffset = 0;
        dst_img.i32YOffset = 0;

        bltSetDestFrameBuf(dst_img);
    }

    bltTrigger();   // Trigger Blit operation.

    bitbltWaitForCompletion();

    return 0;
}
