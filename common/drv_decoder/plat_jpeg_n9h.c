/**
 * @file plat_jpeg.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include <lvgl.h>
#include "../osal/lv_os_private.h"
#include "plat_jpeg.h"

#if defined(PLAT_NUVOTON_N9H3X) && (PLAT_NUVOTON_N9H3X==1)
    #include "jpegcodec.h"
    #include "jpeg.h"
#endif

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

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void plat_jpeg_init(void)
{
    JPEG_INIT();
    jpegOpen();
}

void plat_jpeg_deinit(void)
{
    jpegClose();
    JPEG_DEINIT();
}

void *plat_jpeg_malloc(S_JPEG_CTX *ctx)
{
    uint32_t u32BufferSize = 0;

    /* For Normal Decode buffer allocation */
    switch (ctx->m_u32SrcFormat)
    {
    case JPEG_DEC_YUV422:
    {
        /* Alignment for YUV422 raw data */
        ctx->m_u32Width = NVT_ALIGN(ctx->m_u32Width, 16);
        ctx->m_u32Height = NVT_ALIGN(ctx->m_u32Height, 8);
    }
    break;

    case JPEG_DEC_YUV444:
    {
        /* Alignment for YUV444 raw data */
        ctx->m_u32Width = NVT_ALIGN(ctx->m_u32Width, 8);
        ctx->m_u32Height = NVT_ALIGN(ctx->m_u32Height, 8);
    }
    break;

    default:
    {
        ctx->m_u32Width = NVT_ALIGN(ctx->m_u32Width, 16);
        ctx->m_u32Height = NVT_ALIGN(ctx->m_u32Height, 16);
    }
    break;
    }

    /* Raw Data Buffer for Decode Operation */
    switch (ctx->m_u32DstFormat)
    {
    case LV_COLOR_FORMAT_XRGB8888:
    {
        u32BufferSize = ctx->m_u32Width * ctx->m_u32Height * 4;
    }
    break;

    case LV_COLOR_FORMAT_RGB565:
    {
        u32BufferSize = ctx->m_u32Width * ctx->m_u32Height * 2;
    }
    break;

    default:
    {
        goto _exit_buf_allocation;
    }
    break;
    }

    ctx->m_pvDstBufAddr = nvt_malloc_align(sizeof(uint8_t) * u32BufferSize, 32);
    if (ctx->m_pvDstBufAddr == NULL)
    {
        //sysprintf("Failed to allocate the buffer for decode (size 0x%X)\n", u32BufferSize);
        goto _exit_buf_allocation;
    }

    ctx->m_u32DstBufLen = u32BufferSize;

    //sysprintf("Allocated the buffer %08x for decode (size 0x%X)\n", ctx->m_pvDstBufAddr, ctx->m_u32DstBufLen);

_exit_buf_allocation:

    return ctx->m_pvDstBufAddr;
}

void plat_jpeg_free(S_JPEG_CTX *ctx)
{
    if (ctx && ctx->m_pvDstBufAddr)
        nvt_free_align(ctx->m_pvDstBufAddr);
}

int32_t plat_jpeg_parse(S_JPEG_CTX *ctx, bool bPrimary)
{
#define CONFIG_WORKAROUND            0

    //  HByte,LByte : For JPEG Marker decode
    //  MLength : Length of Marker (all data in the marker)
    //  index : The address index of the JPEG bit-stream
    //  QT_Count : Quantization Table Counter

    uint8_t HByte, LByte;
    uint8_t *pu8Addr;
    uint16_t MLength;
    int32_t index;
    int32_t HuffTable = 0xF;
    int32_t HuffCount = 0;
    int32_t HuffIndex = 0;
    int32_t HuffmanIndex[4] = {0};
    int32_t HuffmanSize[5] = {0};
    int32_t i32Result = 0;
    index = 0;

    uint8_t *pu8BitStream = ctx->m_pvSrcBufAddr;
    uint32_t BitStreamLen = ctx->m_u32SrcBufLen;

    while (index < BitStreamLen)
    {
        HByte = pu8BitStream[index++];
        if (HByte == 0xFF)
        {
            LByte = pu8BitStream[index++];
            switch (LByte)    //May be a Marker
            {
            case 0xD8: /* SOI Marker (Start Of Image) */
                break;

            case 0xDB: /* DQT - Define Quantizer Tables */
            {
                if (index + 1 > BitStreamLen)
                    return -2;        /* Wrong file format */

                HByte = pu8BitStream[index++];
                LByte = pu8BitStream[index++];
                MLength = (HByte << 8) + LByte;
                index += MLength - 2;         /* Skip DQT Data */

                if (index > BitStreamLen)
                    return -2;        /* Wrong file format */
            }
            break;

            case 0xC0:  /* SOF0 (baseline JPEG) */
            {
                uint16_t u16Height, u16Width, end_index, Nf, HSF[3], VSF[3], i;

                if (index + 1 > BitStreamLen)
                    return -2;        /* Wrong file format */

                HByte = pu8BitStream[index++];
                LByte = pu8BitStream[index++];
                MLength = (HByte << 8) + LByte;
                end_index = index + MLength - 2;
                index++;
                HByte = pu8BitStream[index++];
                LByte = pu8BitStream[index++];
                u16Height = (HByte << 8) + LByte;    /* Get Image Height */
                HByte = pu8BitStream[index++];
                LByte = pu8BitStream[index++];
                u16Width = (HByte << 8) + LByte;     /* Get Width Height */

                ctx->m_u32Width = u16Width;
                ctx->m_u32Height = u16Height;

                Nf = pu8BitStream[index++];

                //sysprintf("\tWidth:%d, Height:%d, Component: %d\n", u16Width, u16Height, Nf);

                if (Nf != 3)
                    return -1;

                for (i = 0; i < Nf; i++) /* Get Sampling factors */
                {
                    index++;
                    HByte = pu8BitStream[index++];
                    HSF[i] = HByte / 16;
                    VSF[i] = HByte % 16;
                    index++;
                }

                if (HSF[1] == VSF[1] == HSF[2] == VSF[2] == 1)
                {
                    if (HSF[0] == 2 && VSF[0] == 2)
                    {
                        //sysprintf("YUV420\n");
                        ctx->m_u32SrcFormat = JPEG_DEC_YUV420;
                    }
                    else if (HSF[0] == 2 && VSF[0] == 1)
                    {
                        //sysprintf("YUV422\n");
                        ctx->m_u32SrcFormat = JPEG_DEC_YUV422;
                        if ((u16Height % 16) <= 8)
                            i32Result = 1;
                    }
                    else if (HSF[0] == VSF[0] == 1)
                    {
                        //sysprintf("YUV444\n");
                        ctx->m_u32SrcFormat = JPEG_DEC_YUV444;
                        if ((u16Height % 16) <= 8)
                            i32Result = 1;
                    }
                    else
                        return -1;
                }
                else
                    return -1;

                if ((end_index != index) || (index > BitStreamLen))
                    return -2;    /* Wrong file format */

            }
            break;

            case 0xDA:  /* SOS - Start of Scan */
            {
                int32_t i, Ns, Td[3], Ta[3], end_index, Tda[3];

                if (index + 1 > BitStreamLen)
                    return -2;    /* Wrong file format */

                HByte = pu8BitStream[index++];
                LByte = pu8BitStream[index++];
                MLength = (HByte << 8) + LByte;
                end_index = index + MLength - 2;
                Ns = pu8BitStream[index++];

                for (i = 0; i < Ns; i++)
                {
                    index++;
                    Tda[i] = index;
                    HByte = pu8BitStream[index++];
                    Td[i] = HByte / 16;
                    Ta[i] = HByte % 16;
                }

#if CONFIG_WORKAROUND
                {
                    if (Td[0] == 1 && Td[1] == 0 && Td[2] == 0)
                    {
                        pu8BitStream[Tda[0]] =  pu8BitStream[Tda[0]] & ~0xF0;
                        pu8BitStream[Tda[1]] =  pu8BitStream[Tda[1]] | 0x10;
                        pu8BitStream[Tda[2]] =  pu8BitStream[Tda[2]] | 0x10;
                        pu8BitStream[HuffmanIndex[0] - 1] = 0x01;
                        pu8BitStream[HuffmanIndex[1] - 1] = 0x00;
                    }
                    if (Ta[0] == 1 && Ta[1] == 0 && Ta[2] == 0)
                    {
                        pu8BitStream[Tda[0]] =  pu8BitStream[Tda[0]] & ~0x0F;
                        pu8BitStream[Tda[1]] =  pu8BitStream[Tda[1]] | 0x01;
                        pu8BitStream[Tda[2]] =  pu8BitStream[Tda[2]] | 0x01;
                        pu8BitStream[HuffmanIndex[2] - 1] = 0x11;
                        pu8BitStream[HuffmanIndex[3] - 1] = 0x10;
                    }
                }
#endif

                index += 3;

                if ((end_index != index) || (index > BitStreamLen))
                    return -2;    /* Wrong file format */

#if CONFIG_WORKAROUND
                {
                    index = 0;
                    pu8Addr = (uint8_t *)pu8BitStream;
                    if (HuffCount == 2 || HuffCount == 3)
                    {
                        uint32_t u32Length = 0;
                        if (HuffTable != 0xA)
                        {
                            pu8Addr[256] = 0x00;    /* Clear Original 0xFF */
                            pu8Addr[257] = 0x00;    /* Clear Original 0xD8 */
                            pu8Addr[index++] = 0xFF;
                            pu8Addr[index++] = 0xD8;
                            pu8Addr[index++] = 0xFF;
                            pu8Addr[index++] = 0xC4;
                            index += 2;
                            for (i = 0; i < 4; i++)
                            {
                                if (HuffTable & (1 << i))
                                {
                                    switch (i)
                                    {
                                    case 0:
                                        pu8Addr[index++] = 0x00;
                                        u32Length += HuffmanSize[1] + 1;
                                        memcpy(pu8Addr + index, pu8Addr + HuffmanIndex[1], HuffmanSize[1]);
                                        index += HuffmanSize[1];
                                        break;
                                    case 1:
                                        pu8Addr[index++] = 0x01;
                                        u32Length += HuffmanSize[0] + 1;
                                        memcpy(pu8Addr + index, pu8Addr + HuffmanIndex[0], HuffmanSize[0]);
                                        index += HuffmanSize[0];
                                        break;
                                    case 2:
                                        pu8Addr[index++] = 0x10;
                                        u32Length += HuffmanSize[3] + 1;
                                        memcpy(pu8Addr + index, pu8Addr + HuffmanIndex[3], HuffmanSize[3]);
                                        index += HuffmanSize[3];
                                        break;
                                    case 3:
                                        pu8Addr[index++] = 0x11;
                                        u32Length += HuffmanSize[2] + 1;
                                        memcpy(pu8Addr + index, pu8Addr + HuffmanIndex[2], HuffmanSize[2]);
                                        index += HuffmanSize[2];
                                        break;
                                    }
                                }
                            }
                            u32Length += 2;
                            pu8Addr[4] = (u32Length >> 8) & 0xFF;
                            pu8Addr[5] = u32Length & 0xFF;
                        }
                    }
                }
#endif

                return i32Result;
            }

            case 0xC4: /* DHT - Define Huffman Tables */
            {
                int32_t Li, i;
                if (index + 1 > BitStreamLen)
                    return -2;    /* Wrong file format */
                HByte = pu8BitStream[index++];
                LByte = pu8BitStream[index++];
                MLength = (HByte << 8) + LByte - 2;
                if ((index + MLength) > BitStreamLen)
                    return -2;    /* Wrong file format */

                do
                {
                    switch (pu8BitStream[index++]) /* Tc & Th */
                    {
                    case 0x00:
                        HuffmanIndex[0] = index;
                        HuffTable ^= 0x1;
                        HuffIndex = 0;
                        break;
                    case 0x01:
                        HuffmanIndex[1] = index;
                        HuffTable ^= 0x2;
                        HuffIndex = 1;
                        break;
                    case 0x10:
                        HuffmanIndex[2] = index;
                        HuffTable ^= 0x4;
                        HuffIndex = 2;
                        break;
                    case 0x11:
                        HuffmanIndex[3] = index;
                        HuffTable ^= 0x8;
                        HuffIndex = 3;
                        break;
                    }
                    HuffCount++;

                    Li = 0;
                    for (i = 1; i <= 16; i++)
                    {
                        Li += pu8BitStream[index++];
                    }
                    HuffmanSize[HuffIndex] = Li + 16;
                    MLength = MLength - 17 - Li;
                    index = index + Li;
                }
                while (MLength != 0);
            }
            break;

            case 0xE0:
            case 0xE1:
            case 0xE2:
            case 0xE3:
            case 0xE4:
            case 0xE5:
            case 0xE6:
            case 0xE7:
            case 0xE8:
            case 0xE9:
            case 0xEA:
            case 0xEB:
            case 0xEC:
            case 0xED:
            case 0xEE:
            case 0xEF:
            case 0xFE:
            {
                /* Application Marker && Comment */
                if (index + 1 > BitStreamLen)
                    return -2;    /* Wrong file format */
                HByte = pu8BitStream[index++];
                LByte = pu8BitStream[index++];
                MLength = (HByte << 8) + LByte;
                if (bPrimary)
                    index += MLength - 2;    /* Skip Application or Comment Data */
            }
            break;

            case 0xC1:  /* SOF1 */
            case 0xC2:  /* SOF2 */
            case 0xC3:  /* SOF3 */
            case 0xC5:  /* SOF5 */
            case 0xC6:  /* SOF6 */
            case 0xC7:  /* SOF7 */
            case 0xC9:  /* SOF9 */
            case 0xCA:  /* SOF10 */
            case 0xCB:  /* SOF11 */
            case 0xCD:  /* SOF13 */
            case 0xCE:  /* SOF14 */
            case 0xCF:  /* SOF15 */
            case 0xD9:  /* EOI */
                return -1;    /* Unsupported JPEG standard (may be progressive JPEG) */
            } //Switch

        }

    }

    return -2;    /* Wrong file format */
}

int32_t plat_jpeg_decode(S_JPEG_CTX *ctx)
{
    if (!ctx)
        goto _exit_decode;

    if (JPEG_LOCK() != LV_RESULT_OK)
        goto _exit_decode;


    /* Set Decoded Image Address (Can be set before Decode Trigger for Packet/Planar format)*/
    jpegIoctl(JPEG_IOCTL_SET_YADDR, (uint32_t)ctx->m_pvDstBufAddr, 0);
    //sysprintf("\tThe Packet Buffer prepared for packet format starts from 0x%08X, size:%d\n", ctx->m_pvDstBufAddr, ctx->m_u32DstBufLen);

    sysCleanDcache((UINT32)ctx->m_pvSrcBufAddr, ctx->m_u32SrcBufLen);

    /* Initial JPEG engine. */
    jpegInit();

    /* Set Bitstream Address */
    jpegIoctl(JPEG_IOCTL_SET_BITSTREAM_ADDR, (UINT32)ctx->m_pvSrcBufAddr, 0);

    /* Decode mode */
    switch (ctx->m_u32DstFormat)
    {
    case LV_COLOR_FORMAT_XRGB8888:
    {
        jpegIoctl(JPEG_IOCTL_SET_DECODE_MODE, JPEG_DEC_PRIMARY_PACKET_RGB888, 0);
    }
    break;
    case LV_COLOR_FORMAT_RGB565:
    {
        jpegIoctl(JPEG_IOCTL_SET_DECODE_MODE, JPEG_DEC_PRIMARY_PACKET_RGB565, 0);
    }
    break;
    default:
        goto _exit_decode;
    }

    /* Trigger JPEG decoder */
    jpegIoctl(JPEG_IOCTL_DECODE_TRIGGER, 0, 0);

    /* Wait for complete */
    if (!jpegWait())
    {
        JPEG_UNLOCK();
        goto _exit_decode;
    }

    JPEG_UNLOCK();

    sysInvalidateDcache((UINT32)ctx->m_pvDstBufAddr, ctx->m_u32DstBufLen);

    return 0;

_exit_decode:

    return -1;
}
