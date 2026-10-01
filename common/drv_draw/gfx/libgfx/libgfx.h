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
typedef enum gfx_swizzle
{
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
typedef enum gfx_format
{
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
 *
 * Alignment Limitations:
 * - Pure arithmetic utility; no hardware address or memory alignment constraints.
 */
static inline int gfx_format_bpp(enum gfx_format fmt)
{
    uint32_t de = (uint32_t)fmt & GFX_FMT_DE_MASK;
    switch (de)
    {
    case GFX_RGB565:
    case GFX_YUYV:
    case GFX_UYVY:
        return 2;
    case GFX_INDEX8:
    case GFX_MONOCHROME:
    case GFX_A8:
    case GFX_YV12:
    case GFX_NV12:
    case GFX_NV16:
        return 1;
    default:
        return 4;
    }
}

/**
 * @brief gfx_blend_func enumeration
 * Describes the blend factor for source and destination, and alpha blending modes.
 *
 * Blending Modes:
 * - Straight-Alpha mode (default when GFX_PRE_MULTIPLIED_ALPHA is not set):
 *   Unassociated alpha blending where source color is scaled by alpha in the ALU:
 *   Out = Src * Alpha + Dst * (1 - Alpha).
 * - Premultiplied Alpha mode (when GFX_PRE_MULTIPLIED_ALPHA is set):
 *   Source color is already premultiplied or premultiplied by hardware:
 *   Out = Src_premul + Dst * (1 - Alpha).
 */
typedef enum gfx_blend_func
{
    GFX_ZERO                 = 0,    /**< Blend factor with 0 */
    GFX_ONE                  = 1,    /**< Blend factor with 1 */
    GFX_SRC_ALPHA            = 2,    /**< Blend factor with source alpha */
    GFX_ONE_MINUS_SRC_ALPHA  = 3,    /**< Blend factor with 1 - source alpha */
    GFX_DST_ALPHA            = 4,    /**< Blend factor with destination alpha */
    GFX_ONE_MINUS_DST_ALPHA  = 5,    /**< Blend factor with 1 - destination alpha */
    GFX_PRE_MULTIPLIED_ALPHA = 0x10, /**< Extensive blend as pre-multiplied alpha.
                                    *   When set: Premultiplied Alpha mode (Out = Src_premul + Dst * (1 - Alpha)).
                                    *   When unset (default): Straight-Alpha mode (Out = Src * Alpha + Dst * (1 - Alpha)). */
    GFX_DEMULTIPLY_OUT_ALPHA = 0x20  /**< Extensive blend as demultiply out alpha */
} gfx_blend_func_t;

/**
 * @brief gfx_cap_mode enumeration
 * Describes the alternative capability in 2D BLT.
 *
 * Notes:
 * - GFX_GLOBAL_ALPHA is only valid when GFX_BLEND is enabled.
 * - Straight-Alpha mode (default when GFX_PRE_MULTIPLIED_ALPHA is not set):
 *   Unassociated alpha blending where source color is scaled by alpha in the ALU:
 *   Out = Src * Alpha + Dst * (1 - Alpha), where Alpha is modulated by global alpha.
 * - Premultiplied Alpha mode (when GFX_PRE_MULTIPLIED_ALPHA is set in blendfunc):
 *   Source color is already premultiplied or premultiplied by hardware:
 *   Out = Src_premul + Dst * (1 - Alpha).
 */
typedef enum gfx_cap_mode
{
    GFX_BLEND        = 0, /**< Enable alpha blend in 2D BLT */
    GFX_DITHER       = 1, /**< Enable dither in 2D BLT */
    GFX_GLOBAL_ALPHA = 2, /**< Enable global alpha in blend (modulates effective alpha 0..255).
                         *   Straight-Alpha: Out = Src * Alpha + Dst * (1 - Alpha).
                         *   Premultiplied:  Out = Src_premul + Dst * (1 - Alpha). */
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
typedef enum gfx_rotation
{
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
typedef enum gfx_cache_mode
{
    GFX_CACHE_CLEAN       = 0, /**< Clean the cacheable buffer */
    GFX_CACHE_FLUSH       = 1, /**< Clean and invalidate cacheable buffer */
    GFX_GLOBAL_INVALIDATE = 2  /**< Invalidate the cacheable buffer */
} gfx_cache_mode_t;

/**
 * @brief gfx_feature enumeration
 * Describes the features in GFX BLT.
 */
typedef enum gfx_feature
{
    GFX_SCALING          = 0, /**< Scaling */
    GFX_ROTATION         = 1, /**< Rotation */
    GFX_SRC_YUV          = 2, /**< Source YUV format */
    GFX_DST_YUV          = 3, /**< Destination YUV format */
    GFX_MULTI_SOURCE_BLT = 4  /**< Multi-source blit */
} gfx_feature_t;

/* =========================================================================
 * 2. Structures
 * ========================================================================= */

/* =========================================================================
 * GC520L PE 2.0 Register Macros & Format Constants
 * ========================================================================= */
#ifndef GFX_HW_FMT_A8
#define GFX_HW_FMT_A8                  0x10U /* DE_FORMAT_A8 */
#endif

#ifndef GFX_DEST_CONFIG_2D_ENABLE_BIT
#define GFX_DEST_CONFIG_2D_ENABLE_BIT  0x00002000UL /* VIVS_DE_DEST_CONFIG_COMMAND_BIT_BLT */
#endif

#ifndef GFX_ALPHA_CTRL_ENABLE
#define GFX_ALPHA_CTRL_ENABLE          0x00000001UL /* (1U << 0) */
#endif

#ifndef GFX_PACK_SRC_CONFIG
#define GFX_PACK_SRC_CONFIG(fmt)       (((uint32_t)(fmt) << 24) | ((uint32_t)(fmt) & 0x0FU))
#endif

/**
 * @brief Hardware cache-line and DMA alignment constraint for MA35 Family GFX Engine / Cortex-A35.
 */
#ifndef GFX_ALIGNMENT_BYTES
#define GFX_ALIGNMENT_BYTES 64
#endif

/**
 * @brief gfx_point structure
 * Describes a 2D integer coordinate in pixel units.
 */
typedef struct gfx_point
{
    int x; /**< X coordinate in pixels */
    int y; /**< Y coordinate in pixels */
} gfx_point_t;

/**
 * @brief gfx_rect structure
 * Describes a rectangular region with top-left and bottom-right points.
 */
typedef struct gfx_rect
{
    gfx_point_t tl; /**< Top-left (x = left, y = top) */
    gfx_point_t br; /**< Bottom-right (x = right, y = bottom) */
} gfx_rect_t;

/**
 * @brief gfx_line structure
 * Describes a line segment with start and end points.
 */
typedef struct gfx_line
{
    gfx_point_t p0; /**< Start point */
    gfx_point_t p1; /**< End point */
} gfx_line_t;

/**
 * @brief gfx_surface structure
 * Describes the surface with operation attributes.
 *
 * Alignment Limitations:
 * - planes[] Base Address:
 *   - 32-bpp RGB: Minimum 4-byte (DWORD) address alignment (addr & 3 == 0).
 *   - 16-bpp RGB: Minimum 2-byte (WORD) address alignment (addr & 1 == 0).
 *   - 8-bpp (A8, INDEX8): 1-byte alignment.
 *   - 24-bpp packed RGB: Minimum 16-byte address alignment (addr & 15 == 0).
 *   - Planar YUV (YV12, I420): All Y, U, and V plane addresses MUST be 64-byte aligned (addr & 63 == 0).
 *   - Semi-Planar YUV (NV12, NV21, NV16, NV61): Both Y and UV plane addresses MUST be 64-byte aligned (addr & 63 == 0).
 *   - Packed YUV (YUY2, UYVY): Source address MUST be 64-byte aligned for filter blit; destination address MUST be 4-byte aligned.
 *   - Full LCD Framebuffer: Minimum 128-byte alignment (__attribute__((aligned(128)))) to avoid DRAM bank conflicts.
 * - stride: Hardware DMA read engine requires 16-byte aligned scanline stride ((stride & 0x0F) == 0).
 *   Core blit functions (gfx_blt, gfx_multi_blt_ex) strictly enforce this hardware requirement and reject
 *   unaligned source strides immediately with -1 (zero implicit bounce buffers, zero heap allocation).
 *   For application surfaces with unaligned strides, use the explicit staging helpers
 *   (gfx_surface_alloc_staging, gfx_surface_copy_staging, gfx_surface_stage).
 *   Destination supports arbitrary stride up to 262,143 bytes (< 256KB), with 16-byte aligned stride recommended
 *   for optimal AXI bus throughput.
 * - rect: Coordinates must fall within [-32768, +32767].
 */
typedef struct gfx_surface
{
    enum gfx_format format;        /**< Pixel format of surface buffer */
    int planes[3];                 /**< Physical addresses of surface buffer */
    /**< (planes[0]=Y/RGB, planes[1]=U/UV, planes[2]=V) */
    /**< Alignment: 32bpp >= 4B, 16bpp >= 2B, 24bpp >= 16B, YUV >= 64B, FB >= 128B */
    gfx_rect_t rect;               /**< Rectangular blit area */
    int stride;                    /**< RGB/Y stride of surface buffer in bytes (max 262,143 bytes; 16B aligned recommended) */
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
typedef enum gfx_pattern_type
{
    GFX_PAT_SOLID     = 0, /**< Solid color brush */
    GFX_PAT_COLOR_8X8 = 1, /**< 8x8 Color pattern brush */
    GFX_PAT_MONO_8X8  = 2  /**< 8x8 Monochrome mask pattern brush */
} gfx_pattern_type_t;

/**
 * @brief gfx_pattern structure
 * Describes an 8x8 repeating pattern brush for PatBlt and patterned lines.
 *
 * Alignment Limitations:
 * - paddr: Physical DDR address for 8x8 color pattern MUST be 8-byte (64-bit) aligned (addr & 7 == 0).
 * - origin_x, origin_y: Brush origin phase offsets MUST be clamped to 3-bit values [0, 7] (origin & 7).
 */
typedef struct gfx_pattern
{
    enum gfx_pattern_type type; /**< Pattern type */
    enum gfx_format format;     /**< Color format for 8x8 color pattern */
    uint32_t paddr;             /**< Physical DDR address for 8x8 color pattern (64 pixels, MUST be 8-byte aligned) */
    uint32_t mask_low;          /**< Low 32 bits of 8x8 mono mask (Rows 0..3) */
    uint32_t mask_high;         /**< High 32 bits of 8x8 mono mask (Rows 4..7) */
    uint32_t fg_color;          /**< Foreground color (for mono mask or solid) */
    uint32_t bg_color;          /**< Background color (for mono mask) */
    uint8_t origin_x;           /**< Pattern X origin phase offset (0..7, 3-bit clamped) */
    uint8_t origin_y;           /**< Pattern Y origin phase offset (0..7, 3-bit clamped) */
} gfx_pattern_t;

/**
 * @brief gfx_glyph structure
 * Describes a font character glyph (1-bit monochrome or 8-bit anti-aliased A8 mask).
 *
 * Mask Formats:
 * - GFX_MONOCHROME (0 or 0x0A): 1-bit MSB-first packed monochrome mask data.
 *   stride: Stride in bytes per scanline (>= ceil(width / 8); if 0, auto-calculated as (width + 7) / 8).
 * - GFX_A8 (0x10): 8-bit multi-level alpha coverage mask (0..255).
 *   stride: Stride in bytes per scanline (>= width; if 0, auto-calculated as width).
 *
 * Alignment Limitations:
 * - stride: Staged into 16-byte aligned DMA bounce buffers.
 * - width, height: Arbitrary non-byte-aligned glyph widths are scissored by hardware window without buffer shearing.
 */
typedef struct gfx_glyph
{
    const uint8_t *mask;       /**< Mask data: 1-bit MSB-first (mono) or 8-bit coverage (A8) */
    int width;                 /**< Glyph width in pixels */
    int height;                /**< Glyph height in pixels */
    int stride;                /**< Stride in bytes per scanline (0 for auto: mono=(w+7)/8, A8=w) */
    enum gfx_format format;    /**< Mask format: 0 / GFX_MONOCHROME for 1-bit; GFX_A8 for 8-bit coverage mask */
    uint32_t fg_color;         /**< 32-bit ARGB8888 foreground text color */
    uint32_t bg_color;         /**< 32-bit ARGB8888 background color (used if transparent_bg == 0) */
    uint8_t transparent_bg;    /**< 1: Transparent background (alpha blend), 0: Opaque */
} gfx_glyph_t;

/**
 * @brief Glyph Rendering Session Structure
 *
 * Tracks hardware state, destination bindings, and stream rollback offset
 * across an active multi-glyph streaming transaction.
 */
typedef struct gfx_glyph_session
{
    void *handle;                /**< GFX context handle (Must be non-NULL and point to an active context from gfx_open()) */
    struct gfx_surface *dst;     /**< Bound destination surface (Target format must be ARGB8888 or RGB565) */
    uint32_t start_offset;       /**< Stream word offset at session begin */
    uint32_t glyph_count;        /**< Number of glyphs successfully appended */
    uint32_t last_src_paddr;     /**< Cached source physical address */
    uint32_t last_src_stride;    /**< Cached source scanline stride */
    int last_width;              /**< Width of previous glyph */
    int last_height;             /**< Height of previous glyph */
    enum gfx_format last_format; /**< Format of previous glyph */
    uint32_t last_fg_color;      /**< Foreground color of previous glyph */
    uint32_t last_bg_color;      /**< Background color of previous glyph */
    uint8_t last_transparent_bg; /**< Transparency mode of previous glyph */
    uint8_t is_active;           /**< 1: Session is active and holding GPU lock; 0: Idle/closed */
} gfx_glyph_session_t;

/**
 * @brief Horizontal block width configuration for multi-source blit
 */
typedef enum gfx_block_size_w
{
    GFX_BLOCK_W_AUTO = 0,   /**< Hardware auto / default (16 pixels) */
    GFX_BLOCK_W_16   = 16,  /**< 16 pixels horizontal block */
    GFX_BLOCK_W_32   = 32,  /**< 32 pixels horizontal block */
    GFX_BLOCK_W_64   = 64,  /**< 64 pixels horizontal block */
    GFX_BLOCK_W_128  = 128, /**< 128 pixels horizontal block */
    GFX_BLOCK_W_256  = 256, /**< 256 pixels horizontal block */
    GFX_BLOCK_W_512  = 512  /**< 512 pixels horizontal block */
} gfx_block_size_w_t;

/**
 * @brief Vertical block height configuration for multi-source blit
 */
typedef enum gfx_block_size_h
{
    GFX_BLOCK_H_AUTO = 0,   /**< Hardware auto / default (1 line) */
    GFX_BLOCK_H_1    = 1,   /**< 1 line vertical block */
    GFX_BLOCK_H_2    = 2,   /**< 2 lines vertical block */
    GFX_BLOCK_H_4    = 4,   /**< 4 lines vertical block */
    GFX_BLOCK_H_8    = 8,   /**< 8 lines vertical block */
    GFX_BLOCK_H_16   = 16,  /**< 16 lines vertical block */
    GFX_BLOCK_H_32   = 32,  /**< 32 lines vertical block */
    GFX_BLOCK_H_64   = 64,  /**< 64 lines vertical block */
    GFX_BLOCK_H_128  = 128  /**< 128 lines vertical block */
} gfx_block_size_h_t;

/**
 * @brief Memory block traversal walking direction
 */
typedef enum gfx_tile_walk
{
    GFX_WALK_RIGHT_BOTTOM = 0, /**< Right-to-Bottom scan (Horizontal-first) */
    GFX_WALK_BOTTOM_RIGHT = 1  /**< Bottom-to-Right scan (Vertical-first, optimal for 90/270 rot) */
} gfx_tile_walk_t;

/**
 * @brief Advanced configuration parameters for multi-source blit tuning
 */
typedef struct gfx_multi_blt_cfg
{
    gfx_block_size_w_t block_w;  /**< Horizontal block width (0 for AUTO/16px) */
    gfx_block_size_h_t block_h;  /**< Vertical block height (0 for AUTO/1line) */
    gfx_tile_walk_t    walk_dir; /**< Memory traversal walk direction */
} gfx_multi_blt_cfg_t;

/**
 * @brief gfx_buf structure
 * Describes the buffer used as GFX interfaces.
 *
 * Alignment Limitations:
 * - buf_vaddr, buf_paddr: Guaranteed 64-byte hardware cache-line aligned (GFX_ALIGNMENT_BYTES = 64).
 */
typedef struct gfx_buf
{
    void *buf_handle; /**< The handle associated with buffer */
    void *buf_vaddr;  /**< Virtual address of the buffer (64-byte cache-line aligned) */
    int buf_paddr;    /**< Physical address of the buffer (64-byte cache-line aligned) */
    int buf_size;     /**< The actual size of the buffer */
} gfx_buf_t;

/**
 * @brief gfx_surface_pair structure
 * Binds one source gfx_surface and one destination gfx_surface as a pair.
 * When doing multi-source blit, they are one-to-one correspondent.
 */
typedef struct gfx_surface_pair
{
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
 * Alignment Limitations:
 * - Pool Base Address: `pool_buf` MUST adhere to 64-byte hardware cache-line
 *   alignment (GFX_ALIGNMENT_BYTES = 64) matching Cortex-A35 cache architecture.
 * - Pool Size: `pool_size` should be a multiple of 64 bytes.
 *
 * @param pool_buf  [in] Pointer / base address of memory pool buffer provided by application (must not be NULL, 64-byte aligned).
 * @param pool_size [in] Total size in bytes of pool_buf (must be > 0, 64-byte aligned).
 * @return Success with 0, fail with -1.
 */
int libgfx_init(void *pool_buf, unsigned int pool_size);

/**
 * @brief libgfx_deinit
 * Description: De-initialize GFX hardware and reset driver state.
 *
 * Alignment Limitations:
 * - Resets driver state and clears the 64-byte aligned uncached memory pool.
 *
 * @return Success with 0, fail with -1.
 */
int libgfx_deinit(void);

/**
 * @brief gfx_open
 * Description: Open a GFX device and return a handle.
 *
 * Alignment Limitations:
 * - Allocates and initializes context control blocks and command buffer structures
 *   maintaining 64-bit (8-byte) alignment required by MA35 Family GFX Engine FE.
 *
 * @param handle [out] Pointer to receive GFX device handle.
 * @return Success with 0, fail with -1.
 */
int gfx_open(void **handle);

/**
 * @brief gfx_close
 * Description: Close GFX device with the handle.
 *
 * Alignment Limitations:
 * - Releases context resources and internal 64-bit aligned command buffer structures.
 *
 * @param handle [in] GFX device handle.
 * @return Success with 0, fail with -1.
 */
int gfx_close(void *handle);

/**
 * @brief gfx_fill
 * Description: Fill a specific area with the color specified in surface
 * attributes (clrcolor). Supports both high-speed opaque hardware clear and
 * translucent alpha-blended rectangular fill.
 *
 * Operation Modes:
 * - Opaque Clear (default): When blending capabilities (GFX_BLEND, GFX_GLOBAL_ALPHA)
 *   and surface blending attributes (global_alpha, blendfunc) are unset, executes
 *   maximum-throughput hardware memory clear (COMMAND_CLEAR) with defensive alpha
 *   register sanitization to prevent state leakage from preceding BLTs.
 * - Blended Fill: When GFX_BLEND or GFX_GLOBAL_ALPHA is enabled on the handle, or
 *   area->global_alpha > 0 or area->blendfunc != 0, renders a translucent filled
 *   rectangle blended over existing destination content using PE 2.0 alpha blending.
 *
 * Alignment Limitations:
 * - Surface Base Address: Destination buffer physical address
 *   (area->planes[0]) must satisfy format alignment:
 *   - 32-bpp RGB (ARGB8888, XRGB8888, RGBA8888): Minimum 4-byte (DWORD) address alignment (`addr & 3 == 0`).
 *   - 16-bpp RGB (RGB565, BGR565): Minimum 2-byte (WORD) address alignment (`addr & 1 == 0`).
 *   - 8-bpp (A8, INDEX8): 1-byte alignment.
 *   - Full LCD Framebuffer: Minimum 128-byte hardware cache-line alignment
 *     (`__attribute__((aligned(128)))`) to avoid AXI DRAM bank-conflict penalties.
 * - Scanline Stride: Maximum stride is 262,143 bytes (< 256KB); 16-byte aligned
 *   stride is recommended for optimal AXI burst throughput.
 * - Geometric Dimensions & Tiling: Coordinates must fall within
 *   [-32768, +32767]. High-speed clear requires 64x64 Super Tile alignment.
 *
 * @param handle [in] GFX device handle.
 * @param area   [in] Pointer to gfx_surface specifying the area to be filled, clrcolor,
 *                    and optional global_alpha / blendfunc for translucent blending.
 * @return Success with 0, fail with -1.
 */
int gfx_fill(void *handle, struct gfx_surface *area);

/**
 * @brief gfx_blt
 * Description: GFX BLT from source to destination with alternative operation
 * (Blend, Dither, etc.).
 *
 * Alignment Limitations:
 * - Surface Base Address Alignment (Per-Format):
 *   - 32-bpp RGB (ARGB8888, XRGB8888, RGBA8888, ABGR8888, BGRA8888, RGBX8888, BGRX8888, XBGR8888):
 *     Minimum 4-byte (DWORD) address alignment (`addr & 3 == 0`).
 *   - 16-bpp RGB (RGB565, BGR565): Minimum 2-byte (WORD) address alignment (`addr & 1 == 0`).
 *   - 8-bpp (A8, INDEX8): 1-byte alignment (no address alignment restriction).
 *   - 24-bpp packed RGB: Minimum 16-byte address alignment (`addr & 15 == 0`).
 *   - Planar YUV (YV12, I420): All Y (planes[0]), U (planes[1]), and V (planes[2])
 *     plane addresses MUST be 64-byte aligned (`addr & 63 == 0`).
 *   - Semi-Planar YUV (NV12, NV21, NV16, NV61): Both Y (planes[0]) and UV (planes[1])
 *     plane addresses MUST be 64-byte aligned (`addr & 63 == 0`).
 *   - Packed YUV (YUY2, UYVY, YVYU, VYUY): Source address MUST be 64-byte aligned
 *     for filter blit; destination address MUST be 4-byte aligned (`addr & 3 == 0`).
 *   - Full LCD Framebuffer: Minimum 128-byte hardware cache-line alignment
 *     (`__attribute__((aligned(128)))`) to avoid AXI DRAM bank-conflict penalties.
 * - Scanline Stride & Hardware DMA Constraints:
 *   - Destination Stride: Maximum stride is 262,143 bytes (< 256KB, 18-bit register field);
 *     16-byte aligned stride is recommended for optimal AXI burst throughput.
 *   - Source Stride: MA35 Family GFX Engine AXI DMA read engine strictly requires 16-byte aligned stride.
 *     If an input source surface has an unaligned stride (`(stride & 0x0F) != 0`), the driver immediately
 *     rejects the blit and returns -1 without allocating memory or submitting commands.
 *     Callers must use the explicit staging helpers (`gfx_surface_alloc_staging`, `gfx_surface_copy_staging`,
 *     `gfx_surface_stage`) to stage unaligned buffers into hardware-compliant memory.
 *   - Maximum Scanline Stride: 262,143 bytes (< 256KB, 18-bit register field).
 *   - YUV Linear Output: Y plane stride MUST be 64-byte aligned; U/V plane strides
 *     MUST be 32-byte (planar) or 64-byte (semi-planar) aligned.
 * - Geometric Dimensions & Coordinates:
 *   - Coordinate range: [-32768, +32767] (32K x 32K); surface width/height up to 65,535.
 * - Cache Coherency:
 *   - CPU-modified source buffers must be cleaned before GPU blit.
 *   - GPU-modified destination buffers must be invalidated before CPU reads.
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
 * Alignment Limitations:
 * - Buffer Addresses: Both source (`s->buf_paddr`) and destination (`d->buf_paddr`)
 *   physical addresses should be 64-byte cache-line aligned (GFX_ALIGNMENT_BYTES = 64)
 *   for optimal AXI burst and D-cache line coherency; minimum 4-byte (DWORD) word-aligned
 *   for hardware 2D DMA transfer.
 * - Chunking & Stride: Scanline chunk width is 256 words (1024 bytes); residual tail words
 *   are padded to at least 16-byte stride (`(stride & 0x0F) == 0`). Byte remainders under
 *   4 bytes are copied via CPU.
 * - Cache Maintenance: If the destination buffer is cacheable, it must be
 *   invalidated before/after gfx_memcpy due to hardware DMA cache coherency.
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
 * Alignment Limitations:
 * - None directly on capability flags. Bound surfaces during subsequent BLTs
 *   must comply with hardware address and stride alignment rules.
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
 * Alignment Limitations:
 * - None directly on capability flags.
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
 * Alignment Limitations:
 * - Cortex-A35 Cache-Line Granularity: Operates on 64-byte hardware cache-line
 *   boundaries (GFX_ALIGNMENT_BYTES = 64). Buffer base address and size should
 *   align with 64-byte cache lines to avoid unintentional false-sharing invalidation.
 *
 * @param buf [in] Pointer to gfx_buf to be handled with cache operations (64-byte aligned).
 * @param op  [in] Cache operation type (enum gfx_cache_mode).
 * @return Success with 0, fail with -1.
 */
int gfx_cache_op(struct gfx_buf *buf, enum gfx_cache_mode op);

/**
 * @brief gfx_alloc
 * Description: Allocate a contiguous/cacheable buffer through GFX device.
 *
 * Alignment Limitations:
 * - Cache-Line Alignment: All buffers returned by gfx_alloc are guaranteed
 *   strictly 64-byte cache-line aligned (GFX_ALIGNMENT_BYTES = 64) for both
 *   virtual address (`buf_vaddr`) and physical address (`buf_paddr`), matching
 *   Cortex-A35 L1/L2 cache-line architecture.
 * - LCD Framebuffer Note: Dedicated full-screen LCD framebuffers should be
 *   allocated with 128-byte alignment (`__attribute__((aligned(128)))`) to avoid
 *   AXI DRAM bank-conflict penalties.
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
 * Alignment Limitations:
 * - Buffer pointer must have been allocated via gfx_alloc from the 64-byte aligned
 *   memory pool.
 *
 * @param buf [in] GFX buffer pointer to free.
 * @return Success with 0, fail with -1.
 */
int gfx_free(struct gfx_buf *buf);

/**
 * @brief gfx_flush
 * Description: Flush GFX command and return without completing pipeline.
 *
 * Alignment Limitations:
 * - Command Stream Buffer (`AQCmdBufferAddr`, MMIO 0x00654): Physical base address
 *   MUST be 64-bit (8-byte) aligned (`paddr % 8 == 0`).
 * - Command Stream Control (`AQCmdBufferCtrl`, MMIO 0x00658): Command stream is
 *   automatically padded with `FE_OPCODE_NOP` (0x03 << 27) to an even number of
 *   32-bit words (64-bit boundary) before execution trigger.
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
 * Alignment Limitations:
 * - Command Stream Buffer (`AQCmdBufferAddr`, MMIO 0x00654): Physical base address
 *   MUST be 64-bit (8-byte) aligned (`paddr % 8 == 0`).
 * - Double-Word Padding: Command stream is automatically padded with `FE_OPCODE_NOP`
 *   to an even number of 32-bit words (64-bit boundary) before submission.
 * - Cache Invalidation: Destination cacheable surfaces must be
 *   invalidated after gfx_finish returns to ensure CPU reads updated pixel data.
 *
 * @param handle [in] GFX device handle.
 * @return Success with 0, fail with -1.
 */
int gfx_finish(void *handle);

/**
 * @brief gfx_multi_blt_ex
 * Description: Advanced multi-source blit with variable block size tuning and rotation composition.
 *
 * Composites up to 8 distinct source layers into one destination surface in a single
 * hardware rendering pass. Supports:
 * - Per-layer independent rotation and flipping via sp[i]->s.rot
 * - Destination surface rotation via sp[0]->d.rot
 * - Custom hardware block dimensions and memory traversal walking direction via cfg
 *
 * Alignment Limitations:
 * - Surface Base Address: Each source layer (`sp[i]->s.planes[]`) and
 *   destination (`sp[0]->d.planes[0]`) must satisfy format alignment:
 *   - 32-bpp: Minimum 4-byte (DWORD) address alignment (`addr & 3 == 0`).
 *   - 16-bpp: Minimum 2-byte (WORD) address alignment (`addr & 1 == 0`).
 *   - 24-bpp packed: Minimum 16-byte address alignment (`addr & 15 == 0`).
 *   - Planar/Semi-Planar YUV: All plane addresses MUST be 64-byte aligned (`addr & 63 == 0`).
 *   - Full LCD Framebuffer: Minimum 128-byte alignment (`__attribute__((aligned(128)))`).
 * - Scanline Stride: Each source layer stride MUST be 16-byte aligned (`(stride & 0x0F) == 0`).
 *   If any source layer has an unaligned stride, the blit is immediately rejected with -1 (zero implicit bounce buffers);
 *   destination supports arbitrary stride up to 262,143 bytes (< 256KB),
 *   with 16-byte aligned stride recommended for optimal AXI burst throughput.
 * - Block Dimension Tuning: Horizontal block widths (`cfg->block_w`:
 *   16, 32, 64, 128, 256, 512 pixels) align with AXI burst lengths to maximize DRAM
 *   page-hit efficiency.
 * - Geometric Coordinates: All coordinates must fall within [-32768, +32767].
 *
 * @param handle [in] GFX device handle.
 * @param sp     [in] Array of pointers to struct gfx_surface_pair (up to 8 pairs).
 * @param layers [in] Number of the source layers to composite (1 .. 8).
 * @param cfg    [in] Pointer to advanced configuration (NULL for default auto block behavior).
 * @return Success with 0, fail with -1.
 */
int gfx_multi_blt_ex(void *handle, struct gfx_surface_pair *sp[], int layers, const struct gfx_multi_blt_cfg *cfg);

/**
 * @brief gfx_multi_blt
 * Description: Standard multi-source blit (delegates to gfx_multi_blt_ex with default cfg=NULL).
 *
 * Reads per-layer rotation/flipping from sp[i]->s.rot and destination rotation
 * from sp[0]->d.rot, utilizing default automatic block sizing.
 *
 * Restrictions/Notes:
 * - Hardware COMMAND_MULTI_SOURCE_BLT: one DRAW_2D composites up to 8 sources.
 * - Minor3 2D_MULTI_SOURCE_BLT_EX uses BLOCK8 (8 sources); else BLOCK4 (4).
 * - Although gfx_surface_pair binds one source and one destination as a pair,
 *   it only supports one destination surface (many-to-one).
 * - Per-layer destination rectangles may be offset; the engine uses the union.
 *
 * Alignment Limitations:
 * - Surface Base Address: All source and destination surface addresses
 *   must adhere to format alignment (32bpp >= 4B, 16bpp >= 2B, YUV planes >= 64B;
 *   LCD FB >= 128B).
 * - Scanline Stride: Unaligned source strides are automatically staged into 16-byte
 *   aligned bounce buffers; destination stride up to 262,143 bytes (< 256KB).
 * - Coordinates: Range [-32768, +32767].
 *
 * @param handle [in] GFX device handle.
 * @param sp     [in] Array of pointers to struct gfx_surface_pair.
 * @param layers [in] Number of the source layers to blit.
 * @return Success with 0, fail with -1.
 */
int gfx_multi_blt(void *handle, struct gfx_surface_pair *sp[], int layers);

/**
 * @brief gfx_draw_lines
 * Description: Draw multiple hardware 2D vector lines using COMMAND_LINE.
 *
 * Alignment Limitations:
 * - Destination Base Address: Destination buffer (`dst->planes[0]`)
 *   must satisfy format alignment (32-bpp >= 4B, 16-bpp >= 2B; LCD FB >= 128B).
 * - Destination Stride: Maximum stride is 262,143 bytes (< 256KB);
 *   16-byte aligned stride is recommended for optimal AXI burst throughput.
 * - Geometric Coordinates: All line endpoints must fall within [-32768, +32767].
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
 * Alignment Limitations:
 * - Helper function configuring surface attributes; the target surface buffer
 *   must satisfy format base address and 16-byte stride alignment when submitted to BLT.
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
 * Alignment Limitations:
 * - Helper function configuring channel order; surface buffer must satisfy format
 *   address alignment and 16-byte stride alignment when submitted to BLT.
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
 * Alignment Limitations:
 * - Surface Base Address: Source and destination surface base
 *   addresses must satisfy format alignment (32-bpp >= 4B, 16-bpp >= 2B; LCD FB >= 128B).
 * - Scanline Stride: If source stride is unaligned, driver stages rows into
 *   a 16-byte aligned bounce buffer automatically; destination stride supports up
 *   to 262,143 bytes (< 256KB), with 16-byte aligned stride recommended.
 * - Coordinates: Range [-32768, +32767].
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
 * Alignment Limitations:
 * - Pattern Physical Address: For 8x8 color patterns, `pat->paddr`
 *   MUST be 8-byte (64-bit) aligned (`paddr & 7 == 0`).
 * - Pattern Origin Offsets: `pat->origin_x` and `pat->origin_y`
 *   MUST be clamped to 3-bit values [0, 7] (`origin & 7`).
 * - Destination Surface: Base address must satisfy format
 *   alignment (32-bpp >= 4B, 16-bpp >= 2B; LCD FB >= 128B). Destination stride
 *   supports up to 262,143 bytes (< 256KB), with 16-byte aligned stride recommended.
 * - Coordinates: Destination blit rectangle within [-32768, +32767].
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
 * Alignment Limitations:
 * - Pattern Physical Address: 8x8 color pattern `pat->paddr`
 *   MUST be 8-byte (64-bit) aligned (`paddr & 7 == 0`).
 * - Pattern Origin Offsets: `pat->origin_x` and `pat->origin_y`
 *   clamped to 3-bit values [0, 7].
 * - Destination Surface: Base address format alignment
 *   (32-bpp >= 4B, 16-bpp >= 2B; LCD FB >= 128B); stride supports up to
 *   262,143 bytes (< 256KB), with 16-byte aligned stride recommended.
 * - Coordinates: Line endpoints within [-32768, +32767].
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
 * @brief Begin a zero-copy glyph rendering session.
 *
 * Acquires the recursive GPU hardware lock, validates destination surface parameters,
 * binds the destination canvas, sets hardware scissoring/clipping window, and prepares
 * the internal command stream for incremental character appending.
 *
 * @param handle  [in]  GFX device handle returned by gfx_open().
 * @param dst     [in]  Destination surface (must have valid plane[0] and valid stride).
 * @param session [out] Pointer to caller-allocated gfx_glyph_session_t instance.
 *
 * @return 0 on success, negative value on error:
 *         -1: Invalid parameters (NULL handle, NULL dst, NULL session).
 *         -2: Destination surface validation failed (unaligned address/stride).
 *         -3: Failed to acquire GPU lock (timeout).
 *         -4: Another session is already active on this context.
 */
int gfx_glyph_session_begin(void *handle, struct gfx_surface *dst, gfx_glyph_session_t *session);

/**
 * @brief Append an individual glyph directly to the active session command stream.
 *
 * Emits hardware register states and DRAW_2D commands directly referencing the source
 * mask physical address. Strictly prohibits dynamic memory allocations (gfx_alloc/malloc)
 * and CPU bounce buffering. If mask buffer is unaligned to hardware DMA requirements,
 * the call is immediately rejected.
 *
 * @param session [in,out] Active session pointer initialized by gfx_glyph_session_begin().
 * @param glyph   [in]     Pointer to glyph descriptor containing mask, dimensions, and styling.
 * @param x       [in]     Destination X coordinate for glyph top-left corner.
 * @param y       [in]     Destination Y coordinate for glyph top-left corner.
 *
 * @return 0 on success, negative value on error:
 *         -1: Invalid session (NULL, inactive, or invalid internal state).
 *         -2: Invalid glyph parameters (NULL glyph, NULL mask, non-positive dimensions).
 *         -3: Hardware alignment violation: base address not 64-byte aligned.
 *         -4: Hardware alignment violation: scanline stride not a multiple of 16 bytes.
 *         -5: Command stream buffer overflow (could not allocate or flush stream).
 */
int gfx_glyph_session_append(gfx_glyph_session_t *session, const struct gfx_glyph *glyph, int x, int y);

/**
 * @brief Commit and execute all queued glyph commands in the session.
 *
 * Emits completion event tokens, flushes the 2D pipeline, dispatches the accumulated
 * command buffer to the GPU front-end DMA engine, waits synchronously for hardware idle,
 * and releases the GPU lock.
 *
 * @param session [in,out] Active session pointer.
 *
 * @return 0 on success, negative value on error:
 *         -1: Invalid or inactive session.
 *         -2: Hardware execution timeout (GPU hang or AXI bus error).
 */
int gfx_glyph_session_finish(gfx_glyph_session_t *session);

/**
 * @brief Abort and roll back the active glyph session.
 *
 * Reverts the command stream buffer offset to the position recorded at session begin,
 * discarding all uncommitted draw packets. Releases the GPU lock without submitting
 * any commands to the hardware front-end.
 *
 * @param session [in,out] Active session pointer to abort.
 *
 * @return 0 on success, negative value on error:
 *         -1: Invalid or inactive session.
 */
int gfx_glyph_session_abort(gfx_glyph_session_t *session);

/**
 * @brief gfx_diag_colorkey
 * Description: Diagnostic helper for GFX PE color-key transparency.
 * For bring-up and debug only: probes source/destination color-key and
 * PE_TRANSPARENCY programming for the given handle. Not a production blit API;
 * applications should use gfx_set_colorkey / gfx_blt_colorkey instead.
 *
 * Alignment Limitations:
 * - Internal test surface buffer allocations adhere to 64-byte alignment
 *   (GFX_ALIGNMENT_BYTES = 64).
 *
 * @param handle [in] GFX device handle.
 * @return Success with 0, fail with -1.
 */
int gfx_diag_colorkey(void *handle);

/**
 * @brief gfx_query_feature
 * Description: Query if specific features are available in GFX BLT.
 *
 * Alignment Limitations:
 * - None.
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
 * Alignment Limitations:
 * - None.
 *
 * @return Constant string representing build date and time (e.g., "Aug 25 2026 15:50:00").
 */
const char *gfx_get_build_date(void);

/* =========================================================================
 * 3.1 Explicit Surface Staging Helper APIs (Unaligned Stride Management)
 * ========================================================================= */

/**
 * @brief Allocate an aligned DDR staging surface buffer for an unaligned surface.
 *
 * Calculates a 16-byte aligned scanline stride for the specified source dimensions
 * and format, allocates a contiguous 64-byte cache-line aligned physical DDR buffer
 * from the GFX pool (GFX_ALIGNMENT_BYTES = 64), and initializes the output staging
 * surface descriptor with the aligned stride and physical base address.
 *
 * @param src         [in]  Pointer to source surface configuration (must specify valid width, height, format).
 * @param staged_surf [out] Pointer to destination surface descriptor to be populated with aligned attributes.
 *
 * @return Pointer to allocated struct gfx_buf on success, NULL on failure or invalid parameters.
 */
struct gfx_buf *gfx_surface_alloc_staging(const struct gfx_surface *src,
        struct gfx_surface *staged_surf);

/**
 * @brief Copy pixel rows from an unaligned source into an aligned staging buffer and clean CPU D-cache.
 *
 * Copies pixel rows scanline-by-scanline from CPU memory into the staging buffer,
 * zeroes out any trailing padding bytes up to the 16-byte aligned stride boundary,
 * and performs D-cache clean (dcache_clean_by_mva) to guarantee GPU DMA visibility.
 *
 * @param src         [in] Pointer to original unaligned surface descriptor.
 * @param src_pixels  [in] Optional pointer to source virtual memory pixels. If NULL, resolved from src->planes[0].
 * @param staged_surf [in] Pointer to aligned staging surface descriptor (from gfx_surface_alloc_staging).
 * @param staged_buf  [in] Pointer to staging buffer descriptor (returned by gfx_surface_alloc_staging).
 *
 * @return 0 on success, -1 on invalid parameter.
 */
int gfx_surface_copy_staging(const struct gfx_surface *src,
                             const void *src_pixels,
                             struct gfx_surface *staged_surf,
                             struct gfx_buf *staged_buf);

/**
 * @brief Free an allocated staging surface buffer.
 *
 * Releases the contiguous memory buffer allocated by gfx_surface_alloc_staging
 * back to the GFX pool.
 *
 * @param staged_buf [in] Pointer to staging buffer descriptor to release.
 *
 * @return 0 on success, -1 on invalid parameter.
 */
int gfx_surface_free_staging(struct gfx_buf *staged_buf);

/**
 * @brief Convenience one-shot helper to allocate, copy, and clean an unaligned surface into a staging buffer.
 *
 * Allocates an aligned staging buffer and immediately copies/cleans the source pixel data.
 * If copy fails, automatically frees the buffer before returning NULL.
 *
 * @param src         [in]  Pointer to original unaligned surface descriptor.
 * @param src_pixels  [in]  Optional pointer to source virtual memory pixels (NULL uses src->planes[0]).
 * @param staged_surf [out] Pointer to staging surface descriptor populated with aligned attributes.
 *
 * @return Pointer to allocated struct gfx_buf on success, NULL on failure.
 */
struct gfx_buf *gfx_surface_stage(const struct gfx_surface *src,
                                  const void *src_pixels,
                                  struct gfx_surface *staged_surf);

/* =========================================================================
 * 4. Thread-Safety & OS Synchronization Hook APIs (RTOS / Multitasking)
 * ========================================================================= */

#define GFX_OSAL_WAIT_FOREVER 0xFFFFFFFFU

/**
 * @brief gfx_osal_ops structure
 * Function pointer table for OS synchronization primitives (mutex, semaphore,
 * etc.) in multitasking / RTOS environments.
 */
typedef struct gfx_osal_ops
{
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

    /**
     * @brief Wait for GPU hardware idle / rendering completion via OS semaphore/event.
     *
     * When provided, synchronous completion waits (e.g. gfx_finish, gfx_submit_and_wait)
     * block on this callback instead of busy-polling FE_STATUS register, yielding
     * CPU execution to other tasks in RTOS environments.
     *
     * @param timeout_ms Timeout in milliseconds (GFX_OSAL_WAIT_FOREVER = wait forever).
     * @return 0 on success (GPU idle signaled), negative on timeout or error.
     */
    int (*wait_gpu_idle)(unsigned int timeout_ms);
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
 * Alignment Limitations:
 * - Synchronizes multi-task access to 64-bit aligned GPU command streams
 *   and 64-byte cache-line aligned memory allocation pools.
 *
 * @param ops [in] Pointer to gfx_osal_ops_t table, or NULL to reset to baremetal/fallback mode.
 * @return 0 on success, -1 on error.
 */
int gfx_osal_register_ops(const struct gfx_osal_ops *ops);

/**
 * @brief gfx_osal_lock
 * Description: Explicitly acquire GPU lock for atomic multi-operation batch drawing.
 *
 * Alignment Limitations:
 * - Serializes multi-step rendering batches to prevent interleaved command stream corruption.
 *
 * @param timeout_ms [in] Timeout in milliseconds (or GFX_OSAL_WAIT_FOREVER).
 * @return 0 on success, -1 on error.
 */
int gfx_osal_lock(unsigned int timeout_ms);

/**
 * @brief gfx_osal_unlock
 * Description: Explicitly release GPU lock.
 *
 * Alignment Limitations:
 * - Releases GPU lock after atomic batch submission.
 *
 * @return 0 on success, -1 on error.
 */
int gfx_osal_unlock(void);

#ifdef __cplusplus
}
#endif

#endif /* __LIBGFX_H__ */
