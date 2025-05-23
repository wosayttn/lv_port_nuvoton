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
#include "plat_jpeg.h"

/*********************
 *      DEFINES
 *********************/
#define DECODER_NAME    "HWJPGD"

/**********************
 *      TYPEDEFS
 **********************/
/**********************
 *  STATIC PROTOTYPES
 **********************/
static lv_result_t decoder_info(lv_image_decoder_t *decoder, lv_image_decoder_dsc_t *dsc, lv_image_header_t *header);
static lv_result_t decoder_open(lv_image_decoder_t *decoder, lv_image_decoder_dsc_t *dsc);

static lv_result_t decoder_get_area(lv_image_decoder_t *decoder, lv_image_decoder_dsc_t *dsc,
                                    const lv_area_t *full_area, lv_area_t *decoded_area);
static void decoder_close(lv_image_decoder_t *decoder, lv_image_decoder_dsc_t *dsc);
static int is_jpg(const uint8_t *raw_data, size_t len);

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

        plat_jpeg_init();
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

            plat_jpeg_deinit();
            break;
        }
    }
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

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
                uint32_t u32RBn = 0;
                S_JPEG_CTX ctx = {0};

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

                ctx.m_pvSrcBufAddr = pvBuf;
                ctx.m_u32SrcBufLen = u32RBn;
                if (plat_jpeg_parse(&ctx, true) < 0)
                {
                    goto _exit_info;
                }

                header->cf = LV_COLOR_FORMAT_RGB565;
                header->w = ctx.m_u32Width;
                header->h = ctx.m_u32Height;
                header->stride = ctx.m_u32Width * 2;

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

        ctx->m_u32DstFormat = JPEG_DEC_PRIMARY_PACKET_RGB565;
        if ((plat_jpeg_parse(ctx, true) < 0) ||
                (plat_jpeg_malloc(ctx) == NULL))
        {
            lv_free(ctx);
            return LV_RESULT_INVALID;
        }
        else
        {
            ctx->user_data = f;

            dsc->header.cf = LV_COLOR_FORMAT_RGB565;
            dsc->header.w = ctx->m_u32Width;
            dsc->header.h = ctx->m_u32Height;
            dsc->header.stride = ctx->m_u32Width * 2;
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

        /* Wait for complete */
        if (plat_jpeg_decode(ctx) < 0)
            goto _exit_get_area;

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

        if (ctx->user_data)
        {
            lv_fs_close(ctx->user_data);
            lv_free(ctx->user_data);
        }

        plat_jpeg_free(ctx);

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
