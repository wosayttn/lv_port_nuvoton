/**
 * @file plat_jpeg.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include <lvgl.h>
#include "plat_jpeg.h"
#include "vc8000_lib.h"

/*********************
 *      DEFINES
 *********************/

#define JPEG_INIT()     lv_mutex_init(&jpeg_lock)

#define JPEG_LOCK()     lv_mutex_lock(&jpeg_lock)

#define JPEG_UNLOCK()   lv_mutex_unlock(&jpeg_lock)

#define JPEG_DEINIT()   lv_mutex_delete(&jpeg_lock)

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *   Static Variables
 **********************/

static lv_mutex_t       jpeg_lock;
static volatile uint8_t _VDEBuf[0x1000000] __attribute__((aligned(64))); /* 16 MB */

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void plat_jpeg_init(void)
{
    int ret;

    JPEG_INIT();

    ret = VC8000_Init(ptr_to_u32(_VDEBuf), sizeof(_VDEBuf));
    if (ret < 0)
    {
        LV_LOG_WARN("VC8000_Init failed! (%d)\n", ret);
    }
}

void plat_jpeg_deinit(void)
{
    JPEG_DEINIT();
}

int32_t plat_jpeg_parse(S_JPEG_CTX *ctx, bool bPrimary)
{
    uint32_t index = 0;
    int32_t result = 0;

    uint8_t *pu8BitStream = ctx->m_pvSrcBufAddr;
    uint32_t BitStreamLen = ctx->m_u32SrcBufLen;

    while (index + 1 < BitStreamLen)
    {
        uint8_t marker_prefix = pu8BitStream[index++];
        if (marker_prefix != 0xFF)
            continue;

        uint8_t marker = pu8BitStream[index++];

        switch (marker)
        {
        case 0xD8: // SOI - Start of Image
            break;

        case 0xDB: // DQT - Define Quantization Tables
        case 0xC4: // DHT - Define Huffman Tables
        case 0xE0 ... 0xEF: // APP0~APP15 - Application-specific
        case 0xFE: // COM - Comment
        {
            if (index + 2 > BitStreamLen) return -2;
            uint16_t segment_length = (pu8BitStream[index] << 8) + pu8BitStream[index + 1];
            index += 2;
            if (index + segment_length - 2 > BitStreamLen) return -2;
            if (bPrimary) index += segment_length - 2;
        }
        break;

        case 0xC0: // SOF0 - Baseline DCT
        {
            if (index + 2 > BitStreamLen) return -2;

            uint16_t segment_length = (pu8BitStream[index] << 8) + pu8BitStream[index + 1];
            index += 2;
            if (index + segment_length - 2 > BitStreamLen) return -2;

            index++; // Sample precision (usually 8)
            uint16_t height = (pu8BitStream[index] << 8) + pu8BitStream[index + 1];
            index += 2;
            uint16_t width  = (pu8BitStream[index] << 8) + pu8BitStream[index + 1];
            index += 2;

            ctx->m_u32Width = width;
            ctx->m_u32Height = height;

            uint8_t num_components = pu8BitStream[index++]; // Should be 3 for YCbCr
            if (num_components != 3) return -1;

            uint8_t hsf[3], vsf[3];
            for (int i = 0; i < 3; i++)
            {
                index++; // Component ID
                uint8_t hv = pu8BitStream[index++];
                hsf[i] = hv >> 4;
                vsf[i] = hv & 0x0F;
                index++; // Quantization table selector
            }

            if (hsf[1] == 1 && vsf[1] == 1 &&
                    hsf[2] == 1 && vsf[2] == 1)
            {
                if (hsf[0] == 2 && vsf[0] == 2)
                    ctx->m_u32SrcFormat = JPEG_DEC_YUV420;
                else if (hsf[0] == 2 && vsf[0] == 1)
                {
                    ctx->m_u32SrcFormat = JPEG_DEC_YUV422;
                    if ((height % 16) <= 8)
                        result = 1;
                }
                else if (hsf[0] == 1 && vsf[0] == 1)
                {
                    ctx->m_u32SrcFormat  = JPEG_DEC_YUV444;
                    if ((height % 16) <= 8)
                        result = 1;
                }
                else return -1;
            }
            else return -1;

            index = index - (segment_length - 2) + segment_length - 2;
        }
        break;

        case 0xDA: // SOS - Start of Scan
        {
            if (index + 2 > BitStreamLen) return -2;
            uint16_t segment_length = (pu8BitStream[index] << 8) + pu8BitStream[index + 1];
            index += 2;
            if (index + segment_length - 2 > BitStreamLen) return -2;

            uint8_t num_components = pu8BitStream[index++];
            for (int i = 0; i < num_components; i++)
            {
                index += 2; // Component selector + Huffman table info
            }

            index += 3; // Ss, Se, Ah/Al
            if (index > BitStreamLen) return -2;

            return result;
        }

        case 0xC1:
        case 0xC2:
        case 0xC3:
        case 0xC5:
        case 0xC6:
        case 0xC7:
        case 0xC9:
        case 0xCA:
        case 0xCB:
        case 0xCD:
        case 0xCE:
        case 0xCF:
        case 0xD9: // EOI or unsupported SOF (progressive JPEG, etc.)
            LV_LOG_WARN("Unsupported JPEG format (marker 0x%02X)", marker);
            return -1;

        default:
            // Unknown marker, try to skip
            if (index + 2 > BitStreamLen) return -2;
            uint16_t segment_length = (pu8BitStream[index] << 8) + pu8BitStream[index + 1];
            index += 2;
            if (index + segment_length - 2 > BitStreamLen) return -2;
            index += segment_length - 2;
            break;
        }
    }

    return -2; // Didn't find SOS marker
}

void *plat_jpeg_malloc(S_JPEG_CTX *ctx)
{
    uint32_t u32BufferSize;

    if (!ctx)
        return NULL;

    u32BufferSize = ctx->m_u32Width * ctx->m_u32Height * (LV_COLOR_DEPTH / 8);

    ctx->m_pvDstBufAddr = nvt_malloc_align(sizeof(uint8_t) * u32BufferSize, 64);
    if (ctx->m_pvDstBufAddr == NULL)
    {
        goto _exit_buf_allocation;
    }

    ctx->m_u32DstBufLen = u32BufferSize;

_exit_buf_allocation:

    return ctx->m_pvDstBufAddr;
}

void plat_jpeg_free(S_JPEG_CTX *ctx)
{
    if (ctx && ctx->m_pvDstBufAddr)
        nvt_free_align(ctx->m_pvDstBufAddr);
}


int32_t plat_jpeg_decode(S_JPEG_CTX *ctx)
{
    static struct pp_params _pp;
    int32_t ret = -1;

    if (!ctx)
        goto _exit_decode;

    _pp.img_out_x       = 0;
    _pp.img_out_y       = 0;
    _pp.contrast        = 8;
    _pp.brightness      = 0;
    _pp.saturation      = 32;
    _pp.alpha           = 255;
    _pp.transparency    = 0;

#if (LV_COLOR_DEPTH==16)
    _pp.img_out_fmt     = VC8000_PP_F_RGB565;
#elif (LV_COLOR_DEPTH==32)
    _pp.img_out_fmt     = VC8000_PP_F_RGB888;
#endif
    _pp.rotation        = VC8000_PP_ROTATION_NONE;
    _pp.pp_out_dst      = VC8000_PP_OUT_DST_USER;
    _pp.frame_buf_w     = ctx->m_u32Width;
    _pp.frame_buf_h     = ctx->m_u32Height;
    _pp.img_out_w       = ctx->m_u32Width;
    _pp.img_out_h       = ctx->m_u32Height;
    _pp.pp_out_paddr    = (uint32_t)(uintptr_t)ctx->m_pvDstBufAddr;

    JPEG_LOCK();

    int handle = VC8000_JPEG_Open_Instance();
    if (handle < 0)
    {
        LV_LOG_WARN("VC8000_JPEG_Open_Instance failed (%d)", handle);
        goto _exit_decode;
    }

    ret = VC8000_JPEG_Enable_PP(handle, &_pp);
    if (ret < 0)
    {
        LV_LOG_WARN("VC8000_JPEG_Enable_PP failed (%d)", ret);
        VC8000_JPEG_Close_Instance(handle);
        goto _exit_decode;
    }

    /* clean bitstream buffer. */
    dcache_clean_by_mva((void const *)ctx->m_pvSrcBufAddr, ctx->m_u32SrcBufLen);

    /* Invalidated decoded buffer. */
    dcache_invalidate_by_mva((void const *)ptr_s(_pp.pp_out_paddr), (size_t)ctx->m_u32DstBufLen);

    ret = VC8000_JPEG_Decode_Run(handle, (uint8_t *)ctx->m_pvSrcBufAddr, (uint32_t)ctx->m_u32SrcBufLen, NULL);
    if (ret != 0)
    {
        LV_LOG_WARN("VC8000_JPEG_Decode_Run error: %d", ret);
    }

    VC8000_JPEG_Close_Instance(handle);

_exit_decode:

    JPEG_UNLOCK();

    return ret;
}
