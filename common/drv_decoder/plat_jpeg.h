/**
 * @file plat_jpeg.h
 *
 */

#ifndef PLAT_JPEG_H
#define PLAT_JPEG_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "stdbool.h"
#include "nu_misc.h"

/*********************
 *      DEFINES
 *********************/
#ifndef JPEG_DEC_YUV420
#define JPEG_DEC_YUV420                 0
#endif

#ifndef JPEG_DEC_YUV422
#define JPEG_DEC_YUV422                 1
#endif

#ifndef JPEG_DEC_YUV444
#define JPEG_DEC_YUV444                 2
#endif
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

    void *user_data;

    void (*done)(void *p);

} S_JPEG_CTX;

/**********************
 * GLOBAL PROTOTYPES
 **********************/
void plat_jpeg_init(void);
void plat_jpeg_deinit(void);
void *plat_jpeg_malloc(S_JPEG_CTX *ctx);
void plat_jpeg_free(S_JPEG_CTX *ctx);
int32_t plat_jpeg_parse(S_JPEG_CTX *ctx, bool bPrimary);
int32_t plat_jpeg_decode(S_JPEG_CTX *ctx);

/**********************
 *      MACROS
 **********************/


#ifdef __cplusplus
}
#endif

#endif /* PLAT_JPEG_H */
