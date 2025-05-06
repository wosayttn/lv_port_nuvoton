/**
 * @file lv_hwjpgd.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "../../draw/lv_image_decoder_private.h"
#include "../../../lvgl.h"

#if LV_USE_HWJPGD

#include "../../misc/lv_fs_private.h"
#include <string.h>
#include "drv_hwjpgd.h"
#include "nu_misc.h"

/*********************
 *      DEFINES
 *********************/
#define DECODER_NAME    "HWJPGD"

/**********************
 *      TYPEDEFS
 **********************/
typedef struct
{
    uint32_t m_u32Width;
    uint32_t m_u32Height;

    uint32_t m_u32SrcFormat;
    void  *m_pvSrcBufAddr;
    uint32_t m_u32SrcBufLen;

    uint32_t m_u32DstFormat;
    void *m_pvDstBufAddr;
    uint32_t m_u32DstBufLen;

    void *device;               /* Pointer to I/O device identifier for the session */
} S_JPEG_CTX;
/**********************
 *  STATIC PROTOTYPES
 **********************/
static lv_result_t decoder_info(lv_image_decoder_t *decoder, lv_image_decoder_dsc_t *dsc, lv_image_header_t *header);
static lv_result_t decoder_open(lv_image_decoder_t *decoder, lv_image_decoder_dsc_t *dsc);

static lv_result_t decoder_get_area(lv_image_decoder_t *decoder, lv_image_decoder_dsc_t *dsc,
                                    const lv_area_t *full_area, lv_area_t *decoded_area);
static void decoder_close(lv_image_decoder_t *decoder, lv_image_decoder_dsc_t *dsc);
static int is_jpg(const uint8_t *raw_data, size_t len);
static int32_t JPEG_Parse(uint8_t *pu8BitStream, uint32_t BitStreamLen, uint32_t *pu32Width, uint32_t *pu32Height, uint32_t *pu32SrcFormat, bool bPrimary);
static void *JPEG_AllocOutBuffer(uint32_t *pu32Width, uint32_t *pu32Height, uint32_t *pu32SrcFormat, uint32_t u32DstFormat);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_hwjpgd_init(void)
{
    /* Create decoder instance. */
    lv_image_decoder_t *dec = lv_image_decoder_create();

    if (dec)
    {
        /* Register decoder header info callback function. */
        lv_image_decoder_set_info_cb(dec, decoder_info);

        /* Register open decoder callback function. */
        lv_image_decoder_set_open_cb(dec, decoder_open);

        /* Register get decoded area callback function. */
        lv_image_decoder_set_get_area_cb(dec, decoder_get_area);

        /* Register close decoder callback function. */
        lv_image_decoder_set_close_cb(dec, decoder_close);

        dec->name = DECODER_NAME;

        jpegOpen();
    }
}

void lv_hwjpgd_deinit(void)
{
    lv_image_decoder_t *dec = NULL;

    /* Search decoder_info function address. */
    while ((dec = lv_image_decoder_get_next(dec)) != NULL)
    {
        /* Un-register instance. */
        if (dec->info_cb == decoder_info)
        {
            lv_image_decoder_delete(dec);

            jpegClose();
            break;
        }
    }
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
static void *JPEG_AllocOutBuffer(uint32_t *pu32Width, uint32_t *pu32Height, uint32_t *pu32SrcFormat, uint32_t u32DstFormat)
{
    uint32_t u32FrameBuffer, u32BufferSize;
    void *ret = NULL;

    /* For Normal Decode buffer allocation */
    switch (*pu32SrcFormat)
    {
    case JPEG_DEC_YUV422:
    {
        /* Alignment for YUV422 raw data */
        *pu32Width = NVT_ALIGN(*pu32Width, 16);
        *pu32Height = NVT_ALIGN(*pu32Height, 8);
    }
    break;

    case JPEG_DEC_YUV444:
    {
        /* Alignment for YUV444 raw data */
        *pu32Width = NVT_ALIGN(*pu32Width, 8);
        *pu32Height = NVT_ALIGN(*pu32Height, 8);
    }
    break;

    default:
    {
        *pu32Width = NVT_ALIGN(*pu32Width, 16);
        *pu32Height = NVT_ALIGN(*pu32Height, 16);
    }
    break;
    }

    /* Raw Data Buffer for Decode Operation */
    switch (u32DstFormat)
    {
    case JPEG_DEC_PRIMARY_PACKET_RGB888:
    {
        u32BufferSize = *pu32Width * *pu32Height * 4;
    }
    break;

    case JPEG_DEC_PRIMARY_PLANAR_YUV:
    {
        if (*pu32SrcFormat == JPEG_DEC_YUV444)
            u32BufferSize = *pu32Width * *pu32Height * 3;
        else if (*pu32SrcFormat == JPEG_DEC_YUV422)
            u32BufferSize = *pu32Width * *pu32Height * 2;
        else
            u32BufferSize = *pu32Width * *pu32Height * 1.5;

    }
    break;

    default:
    {
        u32BufferSize = *pu32Width * *pu32Height * 2;
    }
    break;
    }

    if (u32DstFormat == JPEG_DEC_PRIMARY_PLANAR_YUV)
    {
        uint32_t u32YBuffer, u32UBuffer, u32VBuffer;

        /* Allocate Raw Data Buffer for Decode Operation (Prepare 1MB for Planar output) */
        /* Or user needs to get image size to allocate buffer before Decode Trigger for Planar */
        ret = lv_malloc(sizeof(uint8_t) * NVT_ALIGN(u32BufferSize + 32, 32));
        if (ret == NULL)
        {
            goto _exit_buf_allocation;
        }

        u32FrameBuffer = u32YBuffer = NVT_ALIGN((uint32_t)ret, 32);

        /* For Normal Decode buffer allocation */
        if (*pu32SrcFormat == JPEG_DEC_YUV422)
        {
            u32UBuffer = u32YBuffer + *pu32Width * *pu32Height;
            u32VBuffer = u32UBuffer + *pu32Width * *pu32Height / 2;
        }
        else if (*pu32SrcFormat == JPEG_DEC_YUV444)
        {
            u32UBuffer = u32YBuffer + *pu32Width * *pu32Height;
            u32VBuffer = u32UBuffer + *pu32Width * *pu32Height;
        }
        else
        {
            u32UBuffer = u32YBuffer + *pu32Width * *pu32Height;
            u32VBuffer = u32UBuffer + *pu32Width * *pu32Height / 4;
        }

        /* Set Decoded Image Address (Only Can be set before Decode Trigger for Planar;The address can set any time before existing Header Decode Complete Callback function) */
        jpegIoctl(JPEG_IOCTL_SET_YADDR, u32YBuffer, 0);
        jpegIoctl(JPEG_IOCTL_SET_UADDR, u32UBuffer, 0);
        jpegIoctl(JPEG_IOCTL_SET_VADDR, u32VBuffer, 0);

        //sysprintf("\tThe Y/U/V Buffer prepared for Planar format starts from 0x%08X,0x%08X,0x%08X, size:%d\n", u32YBuffer, u32UBuffer, u32VBuffer, u32BufferSize);
    }
    else
    {
        /* Allocate Raw Data Buffer for Decode Operation (Final destination address for Decode Ouptut Wait) */
        ret = lv_malloc(sizeof(uint8_t) * NVT_ALIGN(u32BufferSize + 32, 32));
        if (ret == NULL)
        {
            sysprintf("Failed to allocate the buffer for decode (size 0x%X)\n", u32BufferSize);
            goto _exit_buf_allocation;
        }

        u32FrameBuffer = NVT_ALIGN((uint32_t)ret, 32);

        /* Set Decoded Image Address (Can be set before Decode Trigger for Packet/Planar format)*/
        jpegIoctl(JPEG_IOCTL_SET_YADDR, u32FrameBuffer, 0);

        //sysprintf("\tThe Packet Buffer prepared for packet format starts from 0x%08X, size:%d\n", u32FrameBuffer, u32BufferSize);
    }

_exit_buf_allocation:

    return ret;
}

static int32_t JPEG_Parse(uint8_t *pu8BitStream, uint32_t BitStreamLen, uint32_t *pu32Width, uint32_t *pu32Height, uint32_t *pu32SrcFormat, bool bPrimary)
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

                *pu32Width = u16Width;
                *pu32Height = u16Height;

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
                        *pu32SrcFormat = JPEG_DEC_YUV420;
                    }
                    else if (HSF[0] == 2 && VSF[0] == 1)
                    {
                        //sysprintf("YUV422\n");
                        *pu32SrcFormat = JPEG_DEC_YUV422;
                        if ((u16Height % 16) <= 8)
                            i32Result = 1;
                    }
                    else if (HSF[0] == VSF[0] == 1)
                    {
                        //sysprintf("YUV444\n");
                        *pu32SrcFormat = JPEG_DEC_YUV444;
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

static size_t stat_size(lv_fs_file_t *f)
{
    uint32_t size = 0;
    if (!f) return 0;

    if ((lv_fs_seek(f, 0,  LV_FS_SEEK_END) == LV_FS_RES_OK) &&
            (lv_fs_tell(f, &size) == LV_FS_RES_OK))
    {
        lv_fs_seek(f, 0,  LV_FS_SEEK_SET);
    }

    return size;
}

static lv_result_t decoder_info(lv_image_decoder_t *decoder, lv_image_decoder_dsc_t *dsc, lv_image_header_t *header)
{
    LV_UNUSED(decoder);

    const void *src = dsc->src;
    lv_image_src_t src_type = dsc->src_type;
    lv_result_t ret = LV_RESULT_INVALID;
    void *pvBuf = NULL;

    if (src_type == LV_IMAGE_SRC_VARIABLE)
    {
        /* Decode a bitstream buffer. */
        const lv_image_dsc_t *img_dsc = src;
        uint8_t *raw_data = (uint8_t *)img_dsc->data;
        const uint32_t raw_data_size = img_dsc->data_size;
        if (is_jpg(raw_data, raw_data_size) == true)
        {
#if LV_USE_FS_MEMFS
            header->cf = LV_COLOR_FORMAT_RGB565;
            header->w = img_dsc->header.w;
            header->h = img_dsc->header.h;
            header->stride = img_dsc->header.w * 2;

            return LV_RESULT_OK;
#else
            LV_LOG_WARN("LV_USE_FS_MEMFS needs to enabled to decode from data");
            return LV_RESULT_INVALID;
#endif
        }
    }
    else if (src_type == LV_IMAGE_SRC_FILE)
    {
        const char *fn = src;
        const char *ext = lv_fs_get_ext(fn);
        if ((lv_strcmp(ext, "jpg") == 0) || (lv_strcmp(ext, "jpeg") == 0))
        {
            lv_fs_file_t *f = &dsc->file;
            uint32_t size = stat_size(f);

            if (size > 0)
            {
                lv_fs_res_t res;
                uint32_t u32Width, u32Height, u32SrcFormat, u32RBn = 0;

                pvBuf = lv_malloc(sizeof(uint8_t) * NVT_ALIGN(size + 32, 32));
                if (!pvBuf)
                {
                    goto _exit_info;
                }

                res = lv_fs_read(f, pvBuf, (uint32_t)size, &u32RBn);
                if ((res != LV_FS_RES_OK) && (size != u32RBn))
                {
                    goto _exit_info;
                }

                if (JPEG_Parse((uint8_t *)pvBuf, (uint32_t)u32RBn, &u32Width, &u32Height, &u32SrcFormat, true) < 0)
                {
                    goto _exit_info;
                }

                header->cf = LV_COLOR_FORMAT_RGB565;
                header->w = u32Width;
                header->h = u32Height;
                header->stride = u32Width * 2;

                ret = LV_RESULT_OK;

            } // if (size > 0)
        }
    }

_exit_info:

    if (pvBuf)
        lv_free(pvBuf);

    return ret;
}

/**
 * Decode a JPG image and return the decoded data.
 * @param decoder pointer to the decoder
 * @param dsc     pointer to the decoder descriptor
 * @return LV_RESULT_OK: no error; LV_RESULT_INVALID: can't open the image
 */
static lv_result_t decoder_open(lv_image_decoder_t *decoder, lv_image_decoder_dsc_t *dsc)
{
    LV_UNUSED(decoder);

    lv_fs_file_t *f = lv_malloc(sizeof(lv_fs_file_t));
    if (dsc->src_type == LV_IMAGE_SRC_VARIABLE)
    {
#if LV_USE_FS_MEMFS
        const lv_image_dsc_t *img_dsc = dsc->src;

        if (is_jpg(img_dsc->data, img_dsc->data_size) == true)
        {
            lv_fs_path_ex_t path;
            lv_fs_make_path_from_buffer(&path, LV_FS_MEMFS_LETTER, img_dsc->data, img_dsc->data_size);
            lv_fs_res_t res;
            res = lv_fs_open(f, (const char *)&path, LV_FS_MODE_RD);
            if (res != LV_FS_RES_OK)
            {
                lv_free(f);
                return LV_RESULT_INVALID;
            }
        }
#else
        LV_LOG_WARN("LV_USE_FS_MEMFS needs to enabled to decode from data");
        return LV_RESULT_INVALID;
#endif
    }
    else if (dsc->src_type == LV_IMAGE_SRC_FILE)
    {
        const char *fn = dsc->src;

        if ((lv_strcmp(lv_fs_get_ext(fn), "jpg") == 0) || (lv_strcmp(lv_fs_get_ext(fn), "jpeg") == 0))
        {
            lv_fs_res_t res;
            res = lv_fs_open(f, fn, LV_FS_MODE_RD);
            if (res != LV_FS_RES_OK)
            {
                lv_free(f);
                return LV_RESULT_INVALID;
            }
        }
    }

    {
        uint32_t u32Width, u32Height, u32SrcFormat;
        uint32_t size;
        S_JPEG_CTX *ctx = lv_malloc(sizeof(S_JPEG_CTX));
        if (!ctx)
        {
            return LV_RESULT_INVALID;
        }

        lv_memset(ctx, 0, sizeof(S_JPEG_CTX));

        if (dsc->src_type == LV_IMAGE_SRC_VARIABLE)
        {
            const lv_image_dsc_t *img_dsc = dsc->src;

            ctx->m_pvSrcBufAddr = (void *)img_dsc->data;
            ctx->m_u32SrcBufLen = img_dsc->data_size;
        }
        else if ((dsc->src_type == LV_IMAGE_SRC_FILE) &&
                 ((size = stat_size(f)) > 0))
        {
            uint32_t u32RBn = 0;
            void *pvBuf = lv_malloc(sizeof(uint8_t) * NVT_ALIGN(size + 32, 32));
            if (!pvBuf)
            {
                lv_free(ctx);

                return LV_RESULT_INVALID;
            }

            lv_fs_read(f, pvBuf, (uint32_t)size, &u32RBn);
            if (size != u32RBn)
            {
                lv_free(ctx);
                lv_free(pvBuf);

                return LV_RESULT_INVALID;
            }

            ctx->m_pvSrcBufAddr = pvBuf;
            ctx->m_u32SrcBufLen = u32RBn;
        }
        else
        {
            lv_free(ctx);
            return LV_RESULT_INVALID;
        }

        if ((JPEG_Parse((uint8_t *)ctx->m_pvSrcBufAddr, (uint32_t)ctx->m_u32SrcBufLen, &u32Width, &u32Height, &u32SrcFormat, true) < 0) ||
                ((ctx->m_pvDstBufAddr = JPEG_AllocOutBuffer(&u32Width, &u32Height, &u32SrcFormat, JPEG_DEC_PRIMARY_PACKET_RGB565)) == NULL))
        {
            lv_free(ctx);
            return LV_RESULT_INVALID;
        }
        else
        {
            sysCleanDcache((UINT32)ctx->m_pvSrcBufAddr, ctx->m_u32SrcBufLen);

            ctx->m_u32Width = u32Width;
            ctx->m_u32Height = u32Height;
            ctx->m_u32SrcFormat = u32SrcFormat;
            ctx->m_u32DstBufLen = u32Width * u32Height * 2;
            ctx->m_u32DstFormat = JPEG_DEC_PRIMARY_PACKET_RGB565;
            ctx->device = f;

            dsc->header.cf = LV_COLOR_FORMAT_RGB565;
            dsc->header.w = u32Width;
            dsc->header.h = u32Height;
            dsc->header.stride = u32Width * 2;
            dsc->user_data = (void *)ctx;
        }
    }

    return LV_RESULT_OK;
}

static lv_result_t decoder_get_area(lv_image_decoder_t *decoder, lv_image_decoder_dsc_t *dsc,
                                    const lv_area_t *full_area, lv_area_t *decoded_area)
{
    LV_UNUSED(decoder);
    LV_UNUSED(full_area);

    if (decoded_area->y1 == LV_COORD_MIN)
    {
        lv_draw_buf_t *decoded = (void *)dsc->decoded;
        S_JPEG_CTX *ctx = (S_JPEG_CTX *)dsc->user_data;

        /* Initial JPEG engine. */
        jpegInit();

        /* Set Bitstream Address */
        jpegIoctl(JPEG_IOCTL_SET_BITSTREAM_ADDR, (UINT32)ctx->m_pvSrcBufAddr, 0);

        /* Decode mode */
        jpegIoctl(JPEG_IOCTL_SET_DECODE_MODE, (UINT32)ctx->m_u32DstFormat, 0);

        /* Trigger JPEG decoder */
        jpegIoctl(JPEG_IOCTL_DECODE_TRIGGER, 0, 0);

        /* Wait for complete */
        if (!jpegWait())
            goto _exit_get_area;

        sysInvalidateDcache(NVT_ALIGN((UINT32)ctx->m_pvDstBufAddr, 32), ctx->m_u32DstBufLen);

        decoded_area->y1 = 0;
        decoded_area->y2 = ctx->m_u32Height - 1;
        decoded_area->x1 = 0;
        decoded_area->x2 = ctx->m_u32Width - 1;
        if (decoded == NULL)
        {
            decoded = lv_malloc_zeroed(sizeof(lv_draw_buf_t));
            dsc->decoded = decoded;
        }
        decoded->header = dsc->header;
        decoded->header.w = lv_area_get_width(decoded_area);
        decoded->header.h = lv_area_get_height(decoded_area);
        decoded->header.stride = decoded->header.w * 2;
        decoded->data = ctx->m_pvDstBufAddr;
        decoded->data_size = decoded->header.stride * decoded->header.h;

        return LV_RESULT_OK;
    }

_exit_get_area:

    return LV_RESULT_INVALID;
}

/**
 * Free the allocated resources
 * @param decoder pointer to the decoder where this function belongs
 * @param dsc pointer to a descriptor which describes this decoding session
 */
static void decoder_close(lv_image_decoder_t *decoder, lv_image_decoder_dsc_t *dsc)
{
    LV_UNUSED(decoder);

    S_JPEG_CTX *ctx = (S_JPEG_CTX *)dsc->user_data;
    if (ctx)
    {
        if (dsc->src_type == LV_IMAGE_SRC_FILE)
        {
            if (ctx->m_pvSrcBufAddr)
                lv_free(ctx->m_pvSrcBufAddr);
        }

        if (ctx->device)
        {
            lv_fs_close(ctx->device);
            lv_free(ctx->device);
        }

        if (ctx->m_pvDstBufAddr)
            lv_free(ctx->m_pvDstBufAddr);

        lv_free(ctx);
    }

    if (dsc->decoded)
        lv_free((void *)dsc->decoded);

}

static int is_jpg(const uint8_t *raw_data, size_t len)
{
    const uint8_t jpg_signature[] = {0xFF, 0xD8, 0xFF,  0xE0,  0x00,  0x10, 0x4A,  0x46, 0x49, 0x46};

    if (len < sizeof(jpg_signature)) return false;

    return memcmp(jpg_signature, raw_data, sizeof(jpg_signature)) == 0;
}

#endif /*LV_USE_HWJPGD*/
