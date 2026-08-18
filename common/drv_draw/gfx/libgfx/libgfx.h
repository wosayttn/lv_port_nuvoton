/*
 * Copyright (C) 2023 Nuvoton Technology Corp. All rights reserved.
 *
 * libgfx.h - 2D Graphics Hardware Acceleration API for Nuvoton MA35D1
 *
 */

#ifndef __LIBGFX_H__
#define __LIBGFX_H__

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * 1. Enumerations
 * ========================================================================= */

/**
 * @brief gfx_swizzle enumeration
 * Channel order programmed into SRC_CONFIG[21:20] / DEST_CONFIG[17:16].
 * Packed into gfx_format bits [17:16] (high 16-bit swizzle field).
 */
typedef enum gfx_swizzle {
  GFX_SWIZZLE_ARGB = 0, /**< DE_SWIZZLE_ARGB (hardware default) */
  GFX_SWIZZLE_RGBA = 1, /**< DE_SWIZZLE_RGBA */
  GFX_SWIZZLE_ABGR = 2, /**< DE_SWIZZLE_ABGR */
  GFX_SWIZZLE_BGRA = 3  /**< DE_SWIZZLE_BGRA */
} gfx_swizzle_t;

#define GFX_FMT_DE_MASK        0x0000FFFFu
#define GFX_FMT_SWIZZLE_SHIFT  16
#define GFX_FMT_SWIZZLE_MASK   0x00030000u
#define GFX_FMT_UV_VU_SHIFT    18
#define GFX_FMT_UV_VU          0x00040000u

#define GFX_MAKE_FORMAT(de, swz)                                               \
  ((enum gfx_format)((((uint32_t)(swz) << GFX_FMT_SWIZZLE_SHIFT) &             \
                      GFX_FMT_SWIZZLE_MASK) |                                  \
                     ((uint32_t)(de) & GFX_FMT_DE_MASK)))
#define GFX_FORMAT_DE(fmt)                                                     \
  ((enum gfx_format)((uint32_t)(fmt) & GFX_FMT_DE_MASK))
#define GFX_FORMAT_SWIZZLE(fmt)                                                \
  ((enum gfx_swizzle)(((uint32_t)(fmt) >> GFX_FMT_SWIZZLE_SHIFT) & 0x3u))
#define GFX_FORMAT_UV_VU(fmt)                                                  \
  ((((uint32_t)(fmt) & GFX_FMT_UV_VU) != 0u) ? 1 : 0)

/**
 * @brief gfx_format enumeration
 * Pixel format for source and destination surfaces.
 *
 * Encoding:
 * - bits [15:0]  DE_FORMAT (SRC_CONFIG / DEST_CONFIG format field)
 * - bits [17:16] DE_SWIZZLE (ARGB/RGBA/ABGR/BGRA)
 * - bit  [18]    UV_SWIZZLE VU (NV21 / NV61 / YVYU / VYUY)
 *
 * Notes:
 * - RGB and YUV formats can be set in source surface, but only RGB format
 *   can be set in destination surface.
 * - RGB pixel buffer uses planes[0].
 * - NV12: Y in planes[0], interleaved UV in planes[1].
 * - YV12: Y in planes[0], U in planes[1], V in planes[2].
 * - I420 uses the same DE_FORMAT as YV12; U/V plane order is planes[].
 */
typedef enum gfx_format {
  /* DE_FORMAT_* (state_2d.xml), swizzle = ARGB / UV */
  GFX_RGB565     = 0x04, /**< DE_FORMAT_R5G6B5 */
  GFX_XRGB8888   = 0x05, /**< DE_FORMAT_X8R8G8B8 */
  GFX_ARGB8888   = 0x06, /**< DE_FORMAT_A8R8G8B8 */
  GFX_YUYV       = 0x07, /**< DE_FORMAT_YUY2 */
  GFX_UYVY       = 0x08, /**< DE_FORMAT_UYVY */
  GFX_INDEX8     = 0x09, /**< DE_FORMAT_INDEX8 */
  GFX_MONOCHROME = 0x0A, /**< DE_FORMAT_MONOCHROME */
  GFX_YV12       = 0x0F, /**< DE_FORMAT_YV12 */
  GFX_A8         = 0x10, /**< DE_FORMAT_A8 */
  GFX_NV12       = 0x11, /**< DE_FORMAT_NV12 */
  GFX_NV16       = 0x12, /**< DE_FORMAT_NV16 */

  /* Same DE_FORMAT; high 16 bits select color / UV order */
  GFX_RGBA8888 = (GFX_SWIZZLE_RGBA << GFX_FMT_SWIZZLE_SHIFT) | GFX_ARGB8888,
  GFX_BGRA8888 = (GFX_SWIZZLE_BGRA << GFX_FMT_SWIZZLE_SHIFT) | GFX_ARGB8888,
  GFX_ABGR8888 = (GFX_SWIZZLE_ABGR << GFX_FMT_SWIZZLE_SHIFT) | GFX_ARGB8888,
  GFX_RGBX8888 = (GFX_SWIZZLE_RGBA << GFX_FMT_SWIZZLE_SHIFT) | GFX_XRGB8888,
  GFX_BGRX8888 = (GFX_SWIZZLE_BGRA << GFX_FMT_SWIZZLE_SHIFT) | GFX_XRGB8888,
  GFX_XBGR8888 = (GFX_SWIZZLE_ABGR << GFX_FMT_SWIZZLE_SHIFT) | GFX_XRGB8888,
  GFX_BGR565   = (GFX_SWIZZLE_ABGR << GFX_FMT_SWIZZLE_SHIFT) | GFX_RGB565,
  GFX_YVYU     = GFX_FMT_UV_VU | GFX_YUYV,
  GFX_VYUY     = GFX_FMT_UV_VU | GFX_UYVY,
  GFX_NV21     = GFX_FMT_UV_VU | GFX_NV12,
  GFX_I420     = GFX_YV12,
  GFX_NV61     = GFX_FMT_UV_VU | GFX_NV16
} gfx_format_t;

/**
 * @brief Get bytes per pixel for a gfx_format
 * Handles both plain DE formats and swizzled formats (e.g. GFX_BGR565).
 */
static inline int gfx_format_bpp(enum gfx_format fmt) {
  uint32_t de = (uint32_t)fmt & GFX_FMT_DE_MASK;
  switch (de) {
  case GFX_RGB565:
  case GFX_YUYV:
  case GFX_UYVY:
    return 2;
  case GFX_INDEX8:
  case GFX_MONOCHROME:
  case GFX_A8:
    return 1;
  default:
    return 4;
  }
}

/**
 * @brief gfx_blend_func enumeration
 * Describes the blend factor for source and destination.
 */
typedef enum gfx_blend_func {
  GFX_ZERO                 = 0,    /**< Blend factor with 0 */
  GFX_ONE                  = 1,    /**< Blend factor with 1 */
  GFX_SRC_ALPHA            = 2,    /**< Blend factor with source alpha */
  GFX_ONE_MINUS_SRC_ALPHA  = 3,    /**< Blend factor with 1 - source alpha */
  GFX_DST_ALPHA            = 4,    /**< Blend factor with destination alpha */
  GFX_ONE_MINUS_DST_ALPHA  = 5,    /**< Blend factor with 1 - destination alpha */
  GFX_PRE_MULTIPLIED_ALPHA = 0x10, /**< Extensive blend as pre-multiplied alpha */
  GFX_DEMULTIPLY_OUT_ALPHA = 0x20  /**< Extensive blend as demultiply out alpha */
} gfx_blend_func_t;

/**
 * @brief gfx_cap_mode enumeration
 * Describes the alternative capability in 2D BLT.
 * Note: GFX_GLOBAL_ALPHA is only valid when GFX_BLEND is enabled.
 */
typedef enum gfx_cap_mode {
  GFX_BLEND        = 0, /**< Enable alpha blend in 2D BLT */
  GFX_DITHER       = 1, /**< Enable dither in 2D BLT */
  GFX_GLOBAL_ALPHA = 2, /**< Enable global alpha in blend */
  GFX_SRC_COLORKEY = 3, /**< Enable source color key transparency */
  GFX_DST_COLORKEY = 4  /**< Enable destination color key transparency */
} gfx_cap_mode_t;

/**
 * @brief gfx_rotation enumeration
 * Describes the rotation mode in 2D BLT.
 *
 * Notes:
 * - GFX_ROTATION_0 / GFX_FLIP_H / GFX_FLIP_V shall be set in source surface.
 * - The clockwise rotation degree (90/180/270) shall be set in destination
 *   surface.
 */
typedef enum gfx_rotation {
  GFX_ROTATION_0   = 0, /**< No rotation */
  GFX_ROTATION_90  = 1, /**< Rotation with 90 degree */
  GFX_ROTATION_180 = 2, /**< Rotation with 180 degree */
  GFX_ROTATION_270 = 3, /**< Rotation with 270 degree */
  GFX_FLIP_H       = 4, /**< Horizontal flip */
  GFX_FLIP_V       = 5  /**< Vertical flip */
} gfx_rotation_t;

/**
 * @brief gfx_cache_mode enumeration
 * Describes the cache operation mode.
 */
typedef enum gfx_cache_mode {
  GFX_CACHE_CLEAN       = 0, /**< Clean the cacheable buffer */
  GFX_CACHE_FLUSH       = 1, /**< Clean and invalidate cacheable buffer */
  GFX_GLOBAL_INVALIDATE = 2  /**< Invalidate the cacheable buffer */
} gfx_cache_mode_t;

/**
 * @brief gfx_feature enumeration
 * Describes the features in GFX BLT.
 */
typedef enum gfx_feature {
  GFX_SCALING          = 0, /**< Scaling */
  GFX_ROTATION         = 1, /**< Rotation */
  GFX_SRC_YUV          = 2, /**< Source YUV format */
  GFX_DST_YUV          = 3, /**< Destination YUV format */
  GFX_MULTI_SOURCE_BLT = 4  /**< Multi-source blit */
} gfx_feature_t;

/* =========================================================================
 * 2. Structures
 * ========================================================================= */

/**
 * @brief gfx_point structure
 * Describes a 2D integer coordinate in pixel units.
 */
typedef struct gfx_point {
  int x; /**< X coordinate in pixels */
  int y; /**< Y coordinate in pixels */
} gfx_point_t;

/**
 * @brief gfx_rect structure
 * Describes a rectangular region with top-left and bottom-right points.
 */
typedef struct gfx_rect {
  gfx_point_t tl; /**< Top-left (x = left, y = top) */
  gfx_point_t br; /**< Bottom-right (x = right, y = bottom) */
} gfx_rect_t;

/**
 * @brief gfx_line structure
 * Describes a line segment with start and end points.
 */
typedef struct gfx_line {
  gfx_point_t p0; /**< Start point */
  gfx_point_t p1; /**< End point */
} gfx_line_t;

/**
 * @brief gfx_surface structure
 * Describes the surface with operation attributes.
 */
typedef struct gfx_surface {
  enum gfx_format format;        /**< Pixel format of surface buffer */
  int planes[3];                 /**< Physical addresses of surface buffer */
                                 /**< (planes[0]=Y/RGB, planes[1]=U/UV, planes[2]=V) */
  gfx_rect_t rect;               /**< Rectangular blit area */
  int stride;                    /**< RGB/Y stride of surface buffer (bytes) */
  int width;                     /**< Surface width in pixel unit */
  int height;                    /**< Surface height in pixel unit */
  enum gfx_blend_func blendfunc; /**< Alpha blend mode */
  int global_alpha;              /**< Global alpha value (0 ~ 255) */
  int clrcolor;                  /**< Clear color (32-bit RGBA) */
  enum gfx_rotation rot;         /**< Rotation mode */
  uint8_t colorkey_enable;       /**< 1: Enable hardware Color Key transparency */
  uint32_t colorkey_low;         /**< Color key lower bound (ARGB/RGB) */
  uint32_t colorkey_high;        /**< Color key upper bound (ARGB/RGB) */
} gfx_surface_t;

/**
 * @brief gfx_pattern_type enumeration
 */
typedef enum gfx_pattern_type {
  GFX_PAT_SOLID     = 0, /**< Solid color brush */
  GFX_PAT_COLOR_8X8 = 1, /**< 8x8 Color pattern brush */
  GFX_PAT_MONO_8X8  = 2  /**< 8x8 Monochrome mask pattern brush */
} gfx_pattern_type_t;

/**
 * @brief gfx_pattern structure
 * Describes an 8x8 repeating pattern brush for PatBlt and patterned lines.
 */
typedef struct gfx_pattern {
  enum gfx_pattern_type type; /**< Pattern type */
  enum gfx_format format;     /**< Color format for 8x8 color pattern */
  uint32_t paddr;             /**< Physical DDR address for 8x8 color pattern (64 pixels) */
  uint32_t mask_low;          /**< Low 32 bits of 8x8 mono mask (Rows 0..3) */
  uint32_t mask_high;         /**< High 32 bits of 8x8 mono mask (Rows 4..7) */
  uint32_t fg_color;          /**< Foreground color (for mono mask or solid) */
  uint32_t bg_color;          /**< Background color (for mono mask) */
  uint8_t origin_x;           /**< Pattern X origin phase offset (0..7) */
  uint8_t origin_y;           /**< Pattern Y origin phase offset (0..7) */
} gfx_pattern_t;

/**
 * @brief gfx_buf structure
 * Describes the buffer used as GFX interfaces.
 */
typedef struct gfx_buf {
  void *buf_handle; /**< The handle associated with buffer */
  void *buf_vaddr;  /**< Virtual address of the buffer */
  int buf_paddr;    /**< Physical address of the buffer */
  int buf_size;     /**< The actual size of the buffer */
} gfx_buf_t;

/**
 * @brief gfx_surface_pair structure
 * Binds one source gfx_surface and one destination gfx_surface as a pair.
 * When doing multi-source blit, they are one-to-one correspondent.
 */
typedef struct gfx_surface_pair {
  struct gfx_surface s; /**< Source gfx_surface */
  struct gfx_surface d; /**< Destination gfx_surface */
} gfx_surface_pair_t;

/* =========================================================================
 * 3. GFX Functions
 * ========================================================================= */

/**
 * @brief libgfx_init
 * Description: Initialize GFX hardware engine, clocks, command buffer,
 * and configure caller-provided memory pool for gfx_alloc/gfx_free.
 *
 * @param pool_buf  [in] Pointer / base address of memory pool buffer provided by application (must not be NULL).
 * @param pool_size [in] Total size in bytes of pool_buf (must be > 0).
 * @return Success with 0, fail with -1.
 */
int libgfx_init(void *pool_buf, unsigned int pool_size);

/**
 * @brief libgfx_deinit
 * Description: De-initialize GFX hardware and reset driver state.
 *
 * @return Success with 0, fail with -1.
 */
int libgfx_deinit(void);

/**
 * @brief gfx_open
 * Description: Open a GFX device and return a handle.
 *
 * @param handle [out] Pointer to receive GFX device handle.
 * @return Success with 0, fail with -1.
 */
int gfx_open(void **handle);

/**
 * @brief gfx_close
 * Description: Close GFX device with the handle.
 *
 * @param handle [in] GFX device handle.
 * @return Success with 0, fail with -1.
 */
int gfx_close(void *handle);

/**
 * @brief gfx_fill
 * Description: Fill a specific area with the color specified in surface
 * attributes (clrcolor).
 *
 * @param handle [in] GFX device handle.
 * @param area   [in] Pointer to gfx_surface specifying the area to be filled and clrcolor.
 * @return Success with 0, fail with -1.
 */
int gfx_fill(void *handle, struct gfx_surface *area);

/**
 * @brief gfx_blt
 * Description: GFX BLT from source to destination with alternative operation
 * (Blend, Dither, etc.).
 *
 * @param handle [in] GFX device handle.
 * @param src    [in] Pointer to source surface configuration.
 * @param dst    [in] Pointer to destination surface configuration.
 * @return Success with 0, fail with -1.
 */
int gfx_blt(void *handle, struct gfx_surface *src, struct gfx_surface *dst);

/**
 * @brief gfx_memcpy
 * Description: GFX memory copy with specified size.
 *
 * Limitations:
 * If the destination buffer is cacheable, it must be invalidated before
 * gfx_memcpy due to the alignment limitation of GFX driver.
 *
 * @param handle [in] GFX device handle.
 * @param d      [in] Destination buffer pointer (struct gfx_buf).
 * @param s      [in] Source buffer pointer (struct gfx_buf).
 * @param size   [in] Number of bytes to copy.
 * @return Success with 0, fail with -1.
 */
int gfx_memcpy(void *handle, struct gfx_buf *d, struct gfx_buf *s, int size);

/**
 * @brief gfx_enable
 * Description: Enable GFX capability with the specific mode.
 *
 * @param handle [in] GFX device handle.
 * @param cap    [in] GFX capability to enable (enum gfx_cap_mode).
 * @return Success with 0, fail with -1.
 */
int gfx_enable(void *handle, enum gfx_cap_mode cap);

/**
 * @brief gfx_disable
 * Description: Disable GFX capability with the specific mode.
 *
 * @param handle [in] GFX device handle.
 * @param cap    [in] GFX capability to disable (enum gfx_cap_mode).
 * @return Success with 0, fail with -1.
 */
int gfx_disable(void *handle, enum gfx_cap_mode cap);

/**
 * @brief gfx_cache_op
 * Description: Perform cache operations for the cacheable buffer allocated
 * through the GFX driver.
 *
 * @param buf [in] Pointer to gfx_buf to be handled with cache operations.
 * @param op  [in] Cache operation type (enum gfx_cache_mode).
 * @return Success with 0, fail with -1.
 */
int gfx_cache_op(struct gfx_buf *buf, enum gfx_cache_mode op);

/**
 * @brief gfx_alloc
 * Description: Allocate a contiguous/cacheable buffer through GFX device.
 *
 * @param size      [in] Allocated size in bytes.
 * @param cacheable [in] 0: non-cacheable, 1: cacheable attribute defined by system.
 * @return Success with valid GFX buffer pointer (struct gfx_buf *), fail with NULL (0).
 */
struct gfx_buf *gfx_alloc(int size, int cacheable);

/**
 * @brief gfx_free
 * Description: Free the buffer through GFX device.
 *
 * @param buf [in] GFX buffer pointer to free.
 * @return Success with 0, fail with -1.
 */
int gfx_free(struct gfx_buf *buf);

/**
 * @brief gfx_flush
 * Description: Flush GFX command and return without completing pipeline.
 *
 * @param handle [in] GFX device handle.
 * @return Success with 0, fail with -1.
 */
int gfx_flush(void *handle);

/**
 * @brief gfx_finish
 * Description: Flush GFX command and then return when pipeline is finished
 * (synchronous).
 *
 * @param handle [in] GFX device handle.
 * @return Success with 0, fail with -1.
 */
int gfx_finish(void *handle);

/**
 * @brief gfx_multi_blt
 * Description: Blit multiple sources to one destination.
 *
 * Restrictions/Notes:
 * - Hardware COMMAND_MULTI_SOURCE_BLT: one DRAW_2D composites up to 8 sources.
 * - Minor3 2D_MULTI_SOURCE_BLT_EX uses BLOCK8 (8 sources); else BLOCK4 (4).
 * - Although gfx_surface_pair binds one source and one destination as a pair,
 *   it only supports one destination surface (many-to-one).
 * - Destination surface rotation is set to 0 degree by default.
 * - Per-layer destination rectangles may be offset; the engine uses the union.
 *
 * @param handle [in] GFX device handle.
 * @param sp     [in] Array of pointers to struct gfx_surface_pair.
 * @param layers [in] Number of the source layers to blit.
 * @return Success with 0, fail with -1.
 */
int gfx_multi_blt(void *handle, struct gfx_surface_pair *sp[], int layers);

/**
 * @brief gfx_line
 * Description: Draw a hardware 2D vector line using COMMAND_LINE.
 *
 * @param handle [in] GFX device handle.
 * @param dst    [in] Destination surface.
 * @param line   [in] Line segment (p0 = start, p1 = end).
 * @param color  [in] Line color in 32-bit ARGB.
 * @return Success with 0, fail with -1.
 */
int gfx_line(void *handle, struct gfx_surface *dst, const struct gfx_line *line, uint32_t color);

/**
 * @brief gfx_draw_lines
 * Description: Draw multiple hardware 2D vector lines using COMMAND_LINE.
 *
 * @param handle [in] GFX device handle.
 * @param dst    [in] Destination surface.
 * @param lines  [in] Array of gfx_line segments (p0 = start, p1 = end).
 * @param count  [in] Number of lines to draw.
 * @param color  [in] Line color in 32-bit ARGB.
 * @return Success with 0, fail with -1.
 */
int gfx_draw_lines(void *handle, struct gfx_surface *dst, const struct gfx_line *lines, int count, uint32_t color);

/**
 * @brief gfx_set_colorkey
 * Description: Configure color key transparency for a surface.
 *
 * @param surf   [in,out] Target surface pointer.
 * @param key_lo [in] Color key lower match value (ARGB/RGB).
 * @param key_hi [in] Color key upper match value (ARGB/RGB, 0 for exact match with key_lo).
 * @return Success with 0, fail with -1.
 */
int gfx_set_colorkey(struct gfx_surface *surf, uint32_t key_lo, uint32_t key_hi);

/**
 * @brief gfx_set_swizzle
 * Description: Set RGB channel order (DE_SWIZZLE) on a surface format word.
 * High 16 bits of gfx_format carry the swizzle; low 16 bits stay DE_FORMAT.
 *
 * @param surf [in,out] Target surface pointer.
 * @param swz  [in] GFX_SWIZZLE_ARGB / RGBA / ABGR / BGRA.
 * @return Success with 0, fail with -1.
 */
int gfx_set_swizzle(struct gfx_surface *surf, enum gfx_swizzle swz);

/**
 * @brief gfx_blt_colorkey
 * Description: Perform hardware BitBLT with Source Color Key transparency.
 * Pixels matching colorkey in the source surface will be discarded by PE.
 *
 * @param handle   [in] GFX device handle.
 * @param src      [in] Source surface with colorkey background.
 * @param dst      [in] Destination surface.
 * @param colorkey [in] 32-bit RGB/ARGB transparent key color.
 * @return Success with 0, fail with -1.
 */
int gfx_blt_colorkey(void *handle, struct gfx_surface *src, struct gfx_surface *dst, uint32_t colorkey);

/**
 * @brief gfx_patblt
 * Description: Perform hardware Pattern Brush Blit (PatBlt) with ROP3/ROP4.
 *
 * @param handle   [in] GFX device handle.
 * @param dst      [in] Destination surface.
 * @param pat      [in] 8x8 Color/Mono Pattern Brush.
 * @param rop_code [in] ROP3/ROP4 code (e.g. 0x0030F0F0 PATCOPY, 0x00305A5A PATINVERT).
 * @return Success with 0, fail with -1.
 */
int gfx_patblt(void *handle, struct gfx_surface *dst, const struct gfx_pattern *pat, uint32_t rop_code);

/**
 * @brief gfx_draw_lines_pattern
 * Description: Draw hardware vector lines with 8x8 Pattern Brush texturing.
 *
 * @param handle [in] GFX device handle.
 * @param dst    [in] Destination surface.
 * @param lines  [in] Array of gfx_line segments (p0 = start, p1 = end).
 * @param count  [in] Line segment count.
 * @param pat    [in] 8x8 Color/Mono Pattern Brush.
 * @return Success with 0, fail with -1.
 */
int gfx_draw_lines_pattern(void *handle, struct gfx_surface *dst, const struct gfx_line *lines, int count, const struct gfx_pattern *pat);

/**
 * @brief gfx_diag_colorkey
 * Description: Diagnostic helper for GFX PE color-key transparency.
 * For bring-up and debug only: probes source/destination color-key and
 * PE_TRANSPARENCY programming for the given handle. Not a production blit API;
 * applications should use gfx_set_colorkey / gfx_blt_colorkey instead.
 *
 * @param handle [in] GFX device handle.
 * @return Success with 0, fail with -1.
 */
int gfx_diag_colorkey(void *handle);

/**
 * @brief gfx_query_feature
 * Description: Query if specific features are available in GFX BLT.
 *
 * @param handle    [in]  GFX device handle.
 * @param feature   [in]  GFX feature to query (enum gfx_feature).
 * @param available [out] Pointer to receive feature availability (1: available, 0: not).
 * @return Success with 0, fail with -1.
 */
int gfx_query_feature(void *handle, enum gfx_feature feature, int *available);

/**
 * @brief gfx_get_build_date
 * Description: Get the build date and time of the libgfx library.
 *
 * @return Constant string representing build date and time (e.g., "Aug 25 2026 15:50:00").
 */
const char *gfx_get_build_date(void);

/* =========================================================================
 * 4. Thread-Safety & OS Synchronization Hook APIs (RTOS / Multitasking)
 * ========================================================================= */

#define GFX_OSAL_WAIT_FOREVER 0xFFFFFFFFU

/**
 * @brief gfx_osal_ops structure
 * Function pointer table for OS synchronization primitives (mutex, semaphore,
 * etc.) in multitasking / RTOS environments.
 */
typedef struct gfx_osal_ops {
  /**
   * @brief Acquire GPU hardware / command buffer lock.
   * Recommendation: Use a recursive mutex so nested gfx_* calls do not deadlock.
   *
   * @param timeout_ms Timeout in milliseconds (GFX_OSAL_WAIT_FOREVER = wait forever).
   * @return 0 on success, negative on error/timeout.
   */
  int (*lock_gpu)(unsigned int timeout_ms);

  /**
   * @brief Release GPU hardware / command buffer lock.
   *
   * @return 0 on success, negative on error.
   */
  int (*unlock_gpu)(void);

  /**
   * @brief Acquire memory allocation (gfx_alloc/gfx_free) lock.
   *
   * @param timeout_ms Timeout in milliseconds.
   * @return 0 on success, negative on error/timeout.
   */
  int (*lock_mem)(unsigned int timeout_ms);

  /**
   * @brief Release memory allocation lock.
   *
   * @return 0 on success, negative on error.
   */
  int (*unlock_mem)(void);

  /**
   * @brief Acquire context management (gfx_open/gfx_close) lock.
   *
   * @param timeout_ms Timeout in milliseconds.
   * @return 0 on success, negative on error/timeout.
   */
  int (*lock_ctx)(unsigned int timeout_ms);

  /**
   * @brief Release context management lock.
   *
   * @return 0 on success, negative on error.
   */
  int (*unlock_ctx)(void);
} gfx_osal_ops_t;

/**
 * @brief gfx_osal_register_ops
 * Description: Register OS synchronization lock callbacks for RTOS
 *              multitasking environments.
 *
 * When registered, all standard gfx_* APIs (gfx_open, gfx_close, gfx_alloc,
 * gfx_free, gfx_fill, gfx_blt, gfx_memcpy, gfx_multi_blt, gfx_finish,
 * gfx_flush) automatically acquire and release the corresponding OS
 * mutex/semaphore to ensure thread safety.
 *
 * @param ops [in] Pointer to gfx_osal_ops_t table, or NULL to reset to baremetal/fallback mode.
 * @return 0 on success, -1 on error.
 */
int gfx_osal_register_ops(const struct gfx_osal_ops *ops);

/**
 * @brief gfx_osal_lock
 * Description: Explicitly acquire GPU lock for atomic multi-operation batch drawing.
 *
 * @param timeout_ms [in] Timeout in milliseconds (or GFX_OSAL_WAIT_FOREVER).
 * @return 0 on success, -1 on error.
 */
int gfx_osal_lock(unsigned int timeout_ms);

/**
 * @brief gfx_osal_unlock
 * Description: Explicitly release GPU lock.
 *
 * @return 0 on success, -1 on error.
 */
int gfx_osal_unlock(void);

#ifdef __cplusplus
}
#endif

#endif /* __LIBGFX_H__ */
