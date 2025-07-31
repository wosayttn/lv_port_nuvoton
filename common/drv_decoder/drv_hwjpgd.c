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
#include <stdio.h>
#include "drv_hwjpgd.h"
#include "../../core/lv_global.h"
#include "nu_misc.h"
#include "plat_jpeg.h"

/*********************
 *      DEFINES
 *********************/
#define DECODER_NAME                    "HWJPGD"

#define image_cache_draw_buf_handlers   &(LV_GLOBAL_DEFAULT()->image_cache_draw_buf_handlers)
#define JPEG_SIGNATURE                  0xFFD8FF
#define IS_JPEG_SIGNATURE(x)            (((x) & 0x00FFFFFF) == JPEG_SIGNATURE)

#ifndef nc_ptr
    #define nc_ptr(x)                   (volatile uint32_t*)((uint32_t)x|0x80000000)
#endif

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static lv_result_t decoder_info(lv_image_decoder_t *decoder, lv_image_decoder_dsc_t *dsc, lv_image_header_t *header);
static lv_result_t decoder_open(lv_image_decoder_t *decoder, lv_image_decoder_dsc_t *dsc);
static void decoder_close(lv_image_decoder_t *decoder, lv_image_decoder_dsc_t *dsc);
static bool get_jpeg_head_info(const char *filename, uint32_t *width, uint32_t *height, uint32_t *orientation);
static lv_draw_buf_t *decode_jpeg_file(const char *filename);
static lv_draw_buf_t *decode_jpeg_data(const lv_image_dsc_t *img_dsc);
static uint8_t *read_file(const char *filename, uint32_t *size);
static bool get_jpeg_resolution(uint8_t *data, uint32_t data_size, uint32_t *width, uint32_t *height);
static bool get_jpeg_direction(uint8_t *data, uint32_t data_size, uint32_t *orientation);

/**********************
 *  STATIC VARIABLES
 **********************/
static volatile uint8_t _BitStreamBuf[0x800000] __attribute__((aligned(64))); /* 8 MB */
static uint32_t jpg_width, jpg_height, jpg_srcFormat;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
void lv_hwjpgd_init(void)
{
    lv_image_decoder_t *dec = lv_image_decoder_create();

    if (dec)
    {
        /* Register decoder header info callback function. */
        lv_image_decoder_set_info_cb(dec, decoder_info);
        /* Register open decoder callback function. */
        lv_image_decoder_set_open_cb(dec, decoder_open);
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
static lv_result_t decoder_info(lv_image_decoder_t *decoder, lv_image_decoder_dsc_t *dsc, lv_image_header_t *header)
{
    LV_UNUSED(decoder); /*Unused*/
    lv_image_src_t src_type = dsc->src_type; /*Get the source type*/
    uint32_t width, height;
    uint32_t orientation = 0;

    /*If it's a JPEG file...*/
    if (src_type == LV_IMAGE_SRC_FILE)
    {
        const char *src = dsc->src;
        uint32_t jpg_signature = 0;
        uint32_t rn;
        lv_fs_read(&dsc->file, &jpg_signature, sizeof(jpg_signature), &rn);

        if (rn != sizeof(jpg_signature))
        {
            LV_LOG_WARN("file: %s signature len = %" LV_PRIu32 " error", src, rn);
            return LV_RESULT_INVALID;
        }

        const char *ext = lv_fs_get_ext(src);
        bool is_jpeg_ext = (lv_strcmp(ext, "jpg") == 0) || (lv_strcmp(ext, "jpeg") == 0);

        if (!IS_JPEG_SIGNATURE(jpg_signature))
        {
            if (is_jpeg_ext)
            {
                LV_LOG_WARN("file: %s signature = 0X%" LV_PRIX32 " error", src, jpg_signature);
            }
            return LV_RESULT_INVALID;
        }

        if (!get_jpeg_head_info(src, &width, &height, &orientation))
        {
            return LV_RESULT_INVALID;
        }

#if (LV_COLOR_DEPTH==16)
        header->cf = LV_COLOR_FORMAT_RGB565;
#elif (LV_COLOR_DEPTH==32)
        header->cf = LV_COLOR_FORMAT_XRGB8888;
#endif

        /*Save the data in the header*/
        header->w = jpg_width = (orientation % 180) ? height : width;
        header->h = jpg_height = (orientation % 180) ? width : height;

        return LV_RESULT_OK;
    }
    else if (src_type == LV_IMAGE_SRC_VARIABLE)
    {
        const lv_image_dsc_t *img_dsc = dsc->src;
        const uint32_t data_size = img_dsc->data_size;
        uint32_t jpg_signature = 0;

        jpg_signature = img_dsc->data[0] << 16 | img_dsc->data[1] << 8 | img_dsc->data[2];
        if (jpg_signature != JPEG_SIGNATURE)
        {
            LV_LOG_WARN("It is not jpeg data");
            return LV_RESULT_INVALID;
        }

        /*Save the data in the header*/
#if (LV_COLOR_DEPTH==16)
        header->cf = LV_COLOR_FORMAT_RGB565;
#elif (LV_COLOR_DEPTH==32)
        header->cf = LV_COLOR_FORMAT_XRGB8888;
#endif
        header->w = jpg_width = img_dsc->header.w;
        header->h = jpg_height = img_dsc->header.h;

        return LV_RESULT_OK;
    }

    return LV_RESULT_INVALID; /*If didn't succeeded earlier then it's an error*/
}
/**
 * Open a JPEG image and return the decoded image.
 * @param decoder Pointer to the decoder.
 * @param dsc     Pointer to the decoder descriptor.
 * @return LV_RESULT_OK on success; LV_RESULT_INVALID on failure.
 */
static lv_result_t decoder_open(lv_image_decoder_t *decoder, lv_image_decoder_dsc_t *dsc)
{
    LV_UNUSED(decoder);

    lv_draw_buf_t *decoded = NULL;

    if (dsc->src_type == LV_IMAGE_SRC_FILE)
    {
        decoded = decode_jpeg_file(dsc->src);
        if (!decoded)
        {
            LV_LOG_WARN("Failed to decode JPEG file: %s", (const char *)dsc->src);
            return LV_RESULT_INVALID;
        }
    }
    else if (dsc->src_type == LV_IMAGE_SRC_VARIABLE)
    {
        const lv_image_dsc_t *img_dsc = dsc->src;

        // Optionally preprocess or copy data
        lv_memcpy(nc_ptr(_BitStreamBuf), img_dsc->data, img_dsc->data_size);

        decoded = decode_jpeg_data(img_dsc);
        if (!decoded)
        {
            LV_LOG_WARN("Failed to decode JPEG data");
            return LV_RESULT_INVALID;
        }
    }
    else
    {
        return LV_RESULT_INVALID;
    }

    dsc->decoded = decoded;

    // Skip cache if explicitly disabled or caching is globally off
    if (dsc->args.no_cache || !lv_image_cache_is_enabled())
    {
        return LV_RESULT_OK;
    }

    // Try to cache the decoded image
    lv_image_cache_data_t search_key =
    {
        .src_type = dsc->src_type,
        .src = dsc->src,
        .slot.size = decoded->data_size
    };

    lv_cache_entry_t *entry = lv_image_decoder_add_to_cache(decoder, &search_key, decoded, NULL);
    if (!entry)
    {
        lv_draw_buf_destroy(decoded);
        return LV_RESULT_INVALID;
    }

    dsc->cache_entry = entry;

    return LV_RESULT_OK;
}


// Shared JPEG decoding backend
static lv_draw_buf_t *decode_jpeg_common(const void *jpeg_data, uint32_t data_size)
{
    S_JPEG_CTX ctx = {0};

    if ((jpg_width % 8) != 0 || (jpg_height  % 8) != 0)
    {
        LV_LOG_WARN("jpg_width (%d) or jpg_height (%d) is not a multiple of 32", jpg_width, jpg_height);
        return NULL;
    }

    lv_draw_buf_t *decoded = lv_draw_buf_create_ex(image_cache_draw_buf_handlers,
                             jpg_width, jpg_height,
                             ctx.m_u32DstFormat,
                             LV_STRIDE_AUTO);
    if (!decoded)
    {
        LV_LOG_WARN("Failed to create draw buffer");
        goto fail_jpgd_common;
    }

    ctx.m_u32Width       = jpg_width;
    ctx.m_u32Height      = jpg_height;
    ctx.m_pvSrcBufAddr   = nc_ptr(jpeg_data);
    ctx.m_u32SrcBufLen   = data_size;
    ctx.m_pvDstBufAddr   = (void *)decoded->data;
    ctx.m_u32DstBufLen   = decoded->data_size;

    int ret = plat_jpeg_decode(&ctx);
    if (ret < 0)
    {
        LV_LOG_WARN("plat_jpeg_decode failed (%d)", ret);
        goto fail_jpgd_common;
    }

    return decoded;

fail_jpgd_common:

    if (decoded)
        lv_draw_buf_destroy(decoded);

    return NULL;

}

// Decode JPEG from file
static lv_draw_buf_t *decode_jpeg_file(const char *filename)
{
    lv_fs_file_t f;
    lv_fs_res_t res;
    uint32_t file_size = 0, read_size = 0;


    res = lv_fs_open(&f, filename, LV_FS_MODE_RD);
    if (res != LV_FS_RES_OK)
    {
        LV_LOG_WARN("Failed to open file: %s", filename);
        return NULL;
    }

    if (lv_fs_seek(&f, 0, LV_FS_SEEK_END) != LV_FS_RES_OK ||
            lv_fs_tell(&f, &file_size) != LV_FS_RES_OK ||
            lv_fs_seek(&f, 0, LV_FS_SEEK_SET) != LV_FS_RES_OK)
    {
        LV_LOG_WARN("Failed to determine file size: %s", filename);
        lv_fs_close(&f);
        return NULL;
    }

    if (lv_fs_read(&f, nc_ptr(_BitStreamBuf), file_size, &read_size) != LV_FS_RES_OK || read_size != file_size)
    {
        LV_LOG_WARN("Failed to read JPEG file: %s", filename);
        lv_fs_close(&f);
        return NULL;
    }

    lv_fs_close(&f);

    return decode_jpeg_common(nc_ptr(_BitStreamBuf), file_size);
}

// Decode JPEG from in-memory descriptor
static lv_draw_buf_t *decode_jpeg_data(const lv_image_dsc_t *img_dsc)
{
    return decode_jpeg_common(nc_ptr(_BitStreamBuf), img_dsc->data_size);
}

/**
 * Read an entire file into a dynamically allocated buffer.
 * @param filename The file path.
 * @param size     Output pointer to receive the data size.
 * @return Pointer to the allocated buffer, or NULL on failure.
 */
static uint8_t *read_file(const char *filename, uint32_t *size)
{
    if (!filename || !size) return NULL;

    *size = 0;
    lv_fs_file_t f;
    uint32_t file_size = 0, read_size = 0;

    if (lv_fs_open(&f, filename, LV_FS_MODE_RD) != LV_FS_RES_OK)
    {
        LV_LOG_WARN("Failed to open file: %s", filename);
        return NULL;
    }

    if (lv_fs_seek(&f, 0, LV_FS_SEEK_END) != LV_FS_RES_OK ||
            lv_fs_tell(&f, &file_size) != LV_FS_RES_OK ||
            lv_fs_seek(&f, 0, LV_FS_SEEK_SET) != LV_FS_RES_OK)
    {
        LV_LOG_WARN("Failed to get file size: %s", filename);
        lv_fs_close(&f);
        return NULL;
    }

    uint8_t *data = lv_malloc(file_size);
    if (!data)
    {
        LV_LOG_WARN("Memory allocation failed (%d bytes) for file: %s", file_size, filename);
        lv_fs_close(&f);
        return NULL;
    }

    if (lv_fs_read(&f, data, file_size, &read_size) != LV_FS_RES_OK || read_size != file_size)
    {
        LV_LOG_WARN("Failed to read full file: %s", filename);
        lv_free(data);
        data = NULL;
    }
    else
    {
        *size = read_size;
    }

    lv_fs_close(&f);
    return data;
}

/**
 * Extract JPEG metadata (dimensions + orientation) from file.
 * @return true on success, false otherwise.
 */
static bool get_jpeg_head_info(const char *filename, uint32_t *width, uint32_t *height, uint32_t *orientation)
{
    if (!filename || !width || !height || !orientation)
        return false;

    uint32_t data_size = 0;
    uint8_t *data = read_file(filename, &data_size);
    if (!data)
        return false;

    bool ok = true;

    if (!get_jpeg_resolution(data, data_size, width, height))
    {
        LV_LOG_WARN("Failed to get JPEG size from: %s", filename);
        ok = false;
    }

    if (!get_jpeg_direction(data, data_size, orientation))
    {
        LV_LOG_WARN("Failed to get JPEG orientation from: %s", filename);
        // Orientation failure is non-fatal; we don't override `ok`
    }

    lv_free(data);

    return ok;
}

/**
 * Free resources when decoding is done.
 */
static void decoder_close(lv_image_decoder_t *decoder, lv_image_decoder_dsc_t *dsc)
{
    LV_UNUSED(decoder);
    if (dsc->args.no_cache || !lv_image_cache_is_enabled())
    {
        lv_draw_buf_destroy((lv_draw_buf_t *)dsc->decoded);
    }
}

/**
 * Parse JPEG resolution (width & height) from data.
 */
static bool get_jpeg_resolution(uint8_t *data, uint32_t data_size, uint32_t *width, uint32_t *height)
{
    if (!data || !width || !height)
        return false;

    S_JPEG_CTX ctx = {0};
    ctx.m_pvSrcBufAddr = data;
    ctx.m_u32SrcBufLen = data_size;

    int result = plat_jpeg_parse(&ctx, true);
    if (result < 0)
    {
        LV_LOG_WARN("JPEG_Parse() failed.");
        return false;
    }

    *width = ctx.m_u32Width;
    *height = ctx.m_u32Height;

    return true;
}

/**
 * Parse JPEG EXIF orientation. Placeholder implementation.
 */
static bool get_jpeg_direction(uint8_t *data, uint32_t data_size, uint32_t *orientation)
{
    LV_UNUSED(data);
    LV_UNUSED(data_size);

    if (!orientation) return false;

    *orientation = 0; // Stub: Always return 0 for now
    return true;
}

#endif /*LV_USE_HWJPGD*/
