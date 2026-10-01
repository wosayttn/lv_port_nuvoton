/**
 * @file lv_draw_gfx_label.c
 *
 */

/**
 * Copyright 2026 Nuvoton
 *
 * SPDX-License-Identifier: MIT
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_draw_gfx.h"

#if LV_USE_DRAW_GFX

#include "../../draw/lv_draw_label_private.h"
#include "../../draw/sw/lv_draw_sw.h"
#include "../../misc/lv_area_private.h"
#include "../../misc/lv_text_private.h"
#include "../../misc/lv_bidi.h"
#include "cache.h"
#include <string.h>

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
typedef struct {
  gfx_surface_t surface;
  gfx_glyph_session_t session;
  uint8_t *dest_buf;
  int32_t dest_stride;
  uint32_t bpp;
  int32_t buf_x1;
  int32_t buf_y1;
  lv_area_t cache_area;
  bool failed;
} gfx_label_context_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void _gfx_end_session(void);

/**********************
 *  STATIC VARIABLES
 **********************/
/* LVGL's glyph callback has no user-data parameter; the GFX draw unit runs
 * one label/letter task at a time. */
static gfx_label_context_t s_ctx;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

static void _gfx_cache_dest(bool invalidate) {
  if (s_ctx.dest_buf == NULL || s_ctx.dest_stride <= 0 || s_ctx.bpp == 0)
    return;

  const lv_area_t *a = &s_ctx.cache_area;
  int32_t w = lv_area_get_width(a);
  int32_t h = lv_area_get_height(a);
  if (w <= 0 || h <= 0)
    return;

  uint32_t bytes = (uint32_t)w * s_ctx.bpp;
  uint8_t *row = s_ctx.dest_buf + a->y1 * s_ctx.dest_stride + a->x1 * s_ctx.bpp;
  for (int32_t y = a->y1; y <= a->y2; y++, row += s_ctx.dest_stride) {
    if (invalidate)
      dcache_clean_invalidate_by_mva(row, bytes);
    else
      dcache_clean_by_mva(row, bytes);
  }
}

static void _gfx_end_session(void) {
  if (!s_ctx.session.is_active)
    return;

  int ret = gfx_glyph_session_finish(&s_ctx.session);
  if (ret != 0) {
    sysprintf("[GFX_LABEL] session finish failed: %d\n", ret);
    s_ctx.failed = true;
  }

  /* Invalidate CPU cache so CPU reads newly rendered GPU pixels directly from DDR */
  _gfx_cache_dest(true);
}

static bool _gfx_begin_session(void) {
  if (s_ctx.failed)
    return false;
  if (s_ctx.session.is_active)
    return true;

  /* libgfx acquires the hardware GPU lock internally in gfx_glyph_session_begin
   */
  int ret = gfx_glyph_session_begin(g_gfx_handle, &s_ctx.surface, &s_ctx.session);
  if (ret != 0) {
    sysprintf("[GFX_LABEL] session begin failed: %d\n", ret);
    s_ctx.failed = true;
    return false;
  }

  /* Clean destination cache lines to DDR while GPU lock is held */
  _gfx_cache_dest(false);
  return true;
}

static bool _gfx_init_context(lv_draw_task_t *t) {
  if (t == NULL || t->target_layer == NULL ||
      t->target_layer->draw_buf == NULL || g_gfx_handle == NULL)
    return false;

  lv_layer_t *layer = t->target_layer;
  lv_draw_buf_t *draw_buf = layer->draw_buf;
  if (draw_buf->data == NULL || draw_buf->header.stride == 0 ||
      (draw_buf->header.stride & 15u) != 0 ||
      ((uintptr_t)draw_buf->data & 3u) != 0)
    return false;
  if (draw_buf->header.cf != LV_COLOR_FORMAT_RGB565 &&
      draw_buf->header.cf != LV_COLOR_FORMAT_ARGB8888 &&
      draw_buf->header.cf != LV_COLOR_FORMAT_XRGB8888)
    return false;

  lv_area_t bounds = {layer->buf_area.x1, layer->buf_area.y1,
                      layer->buf_area.x1 + draw_buf->header.w - 1,
                      layer->buf_area.y1 + draw_buf->header.h - 1};
  lv_area_t clipped;
  if (!lv_area_intersect(&clipped, &t->clip_area, &bounds))
    return false;

  if (s_ctx.session.is_active)
    _gfx_end_session();

  lv_memzero(&s_ctx, sizeof(s_ctx));
  s_ctx.dest_buf = draw_buf->data;
  s_ctx.dest_stride = draw_buf->header.stride;
  s_ctx.bpp = lv_color_format_get_size(draw_buf->header.cf);
  s_ctx.buf_x1 = layer->buf_area.x1;
  s_ctx.buf_y1 = layer->buf_area.y1;
  s_ctx.cache_area.x1 = clipped.x1 - s_ctx.buf_x1;
  s_ctx.cache_area.y1 = clipped.y1 - s_ctx.buf_y1;
  s_ctx.cache_area.x2 = clipped.x2 - s_ctx.buf_x1;
  s_ctx.cache_area.y2 = clipped.y2 - s_ctx.buf_y1;

  gfx_surface_t *dst = &s_ctx.surface;
  dst->format = draw_buf->header.cf == LV_COLOR_FORMAT_RGB565 ? GFX_RGB565 : GFX_ARGB8888;
  dst->width = draw_buf->header.w;
  dst->height = draw_buf->header.h;
  dst->stride = draw_buf->header.stride;
  dst->planes[0] = (int)((uintptr_t)draw_buf->data & 0xFFFFFFFFU);
  dst->rect.tl.x = s_ctx.cache_area.x1;
  dst->rect.tl.y = s_ctx.cache_area.y1;
  dst->rect.br.x = s_ctx.cache_area.x2 + 1;
  dst->rect.br.y = s_ctx.cache_area.y2 + 1;
  return true;
}

static bool _gfx_append_glyph(lv_draw_task_t *t, lv_font_glyph_dsc_t *g,
                             const lv_area_t *letter_coords, uint32_t fg_color) {
  if (lv_area_is_out(letter_coords, &t->clip_area, 0))
    return true;

  const lv_font_t *resolved_font = g->resolved_font;
  if (resolved_font != NULL && !lv_font_has_static_bitmap(resolved_font))
    return false;

  g->req_raw_bitmap = 1;
  const void *mask = lv_font_get_glyph_static_bitmap(g);
  if (mask == NULL)
    return false;

  int stride = g->stride;
  if (stride < g->box_w)
    return false;

  /* Check zero-copy hardware DMA constraints: 64-byte base address and 16-byte stride multiple */
  if (((uintptr_t)mask & 63u) != 0 || (stride & 15u) != 0) {
    sysprintf("[GFX_GLYPH] unaligned glyph mask=%08X stride=%d. skip the op.\n",
              (uintptr_t)mask & 0xffffffffu, stride);
    return false;
  }

  int pos_x = letter_coords->x1 - s_ctx.buf_x1;
  int pos_y = letter_coords->y1 - s_ctx.buf_y1;
  if (pos_x > s_ctx.cache_area.x2 || pos_y > s_ctx.cache_area.y2 ||
      pos_x + g->box_w <= s_ctx.cache_area.x1 ||
      pos_y + g->box_h <= s_ctx.cache_area.y1)
    return true;

  gfx_glyph_t glyph = {
      .mask = (const uint8_t *)mask,
      .width = g->box_w,
      .height = g->box_h,
      .stride = stride,
      .format = GFX_A8,
      .fg_color = fg_color,
      .bg_color = 0,
      .transparent_bg = 1
  };

  if (!_gfx_begin_session())
    return false;

  int ret = gfx_glyph_session_append(&s_ctx.session, &glyph, pos_x, pos_y);
  if (ret != 0) {
    sysprintf("[GFX_GLYPH] session append failed: %d\n", ret);
    gfx_glyph_session_abort(&s_ctx.session);
    _gfx_cache_dest(true);
    s_ctx.failed = true;
    return false;
  }

  return true;
}

static void _gfx_draw_label_fast(lv_draw_task_t *t, const lv_draw_label_dsc_t *dsc) {
  const lv_font_t *font = dsc->font;
  if (font == NULL || !lv_font_has_static_bitmap(font))
    return;

  const lv_area_t *coords = &t->area;
  int32_t w = (dsc->flag & LV_TEXT_FLAG_EXPAND) == 0 ? lv_area_get_width(coords) : LV_COORD_MAX;

  int32_t line_height_font = lv_font_get_line_height(font);
  int32_t line_height = line_height_font + dsc->line_space;

  lv_text_align_t align = dsc->align;
  lv_base_dir_t base_dir = dsc->bidi_dir;
  lv_bidi_calculate_align(&align, &base_dir, dsc->text);

  lv_text_attributes_t attributes = {0};
  attributes.letter_space = dsc->letter_space;
  attributes.line_space = dsc->line_space;
  attributes.text_flags = dsc->flag;
  attributes.max_width = w;

  uint32_t remaining_len = dsc->text_length;
  if (remaining_len == 0)
    remaining_len = (uint32_t)strlen(dsc->text);

  uint32_t line_start = 0;
  uint32_t line_end = line_start + lv_text_get_next_line(&dsc->text[line_start], remaining_len, font, NULL, &attributes);

  lv_point_t pos;
  pos.x = coords->x1;
  pos.y = coords->y1;

  /* Skip lines positioned above the clip area */
  while (pos.y + line_height_font < t->clip_area.y1) {
    remaining_len -= line_end - line_start;
    line_start = line_end;
    line_end += lv_text_get_next_line(&dsc->text[line_start], remaining_len, font, NULL, &attributes);
    pos.y += line_height;
    if (dsc->text[line_start] == '\0')
      return;
  }

  uint32_t color_u32 = lv_color_to_u32(dsc->color);
  uint32_t fg_color = (color_u32 & 0x00FFFFFFu) | ((uint32_t)dsc->opa << 24);

  while (line_start < line_end) {
    pos.x = coords->x1;
    if (align == LV_TEXT_ALIGN_CENTER) {
      int32_t line_width = lv_text_get_width(&dsc->text[line_start], line_end - line_start, font, &attributes);
      pos.x += (lv_area_get_width(coords) - line_width) / 2;
    } else if (align == LV_TEXT_ALIGN_RIGHT) {
      int32_t line_width = lv_text_get_width(&dsc->text[line_start], line_end - line_start, font, &attributes);
      pos.x += lv_area_get_width(coords) - line_width;
    }

    uint32_t ofs = line_start;
    while (ofs < line_end) {
      uint32_t letter = lv_text_encoded_next(dsc->text, &ofs);
      uint32_t letter_next = 0;
      if (ofs < line_end) {
        uint32_t ofs_peek = ofs;
        letter_next = lv_text_encoded_next(dsc->text, &ofs_peek);
      }

      if (lv_text_is_marker(letter))
        continue;

      lv_font_glyph_dsc_t g;
      if (!lv_font_get_glyph_dsc(font, &g, letter, letter_next))
        continue;

      int32_t letter_w = g.adv_w;
      if (g.box_w == 0 || g.box_h == 0) {
        pos.x += letter_w + dsc->letter_space;
        continue;
      }

      lv_area_t letter_coords;
      letter_coords.x1 = pos.x + g.ofs_x;
      letter_coords.x2 = letter_coords.x1 + g.box_w - 1;
      letter_coords.y1 = pos.y + (font->line_height - font->base_line) - g.box_h - g.ofs_y;
      letter_coords.y2 = letter_coords.y1 + g.box_h - 1;

      if (!_gfx_append_glyph(t, &g, &letter_coords, fg_color)) {
        if (s_ctx.failed)
          return;
      }

      pos.x += letter_w + dsc->letter_space;
    }

    remaining_len -= line_end - line_start;
    line_start = line_end;
    if (remaining_len) {
      line_end += lv_text_get_next_line(&dsc->text[line_start], remaining_len, font, NULL, &attributes);
    }
    pos.y += line_height;
    if (pos.y > t->clip_area.y2)
      break;
  }
}

void lv_draw_gfx_label(lv_draw_task_t *t) {
  if (t == NULL || t->draw_dsc == NULL)
    return;

  const lv_draw_label_dsc_t *dsc = (const lv_draw_label_dsc_t *)t->draw_dsc;
  if (dsc->opa <= LV_OPA_MIN || dsc->font == NULL || dsc->text == NULL ||
      dsc->text[0] == '\0' || !_gfx_init_context(t))
    return;

  _gfx_draw_label_fast(t, dsc);
  _gfx_end_session();
}

void lv_draw_gfx_letter(lv_draw_task_t *t) {
  if (t == NULL || t->draw_dsc == NULL)
    return;
  const lv_draw_letter_dsc_t *dsc = (const lv_draw_letter_dsc_t *)t->draw_dsc;
  if (dsc->opa <= LV_OPA_MIN || dsc->font == NULL || !_gfx_init_context(t))
    return;

  if (lv_text_is_marker(dsc->unicode))
    return;

  const lv_font_t *font = dsc->font;
  if (!lv_font_has_static_bitmap(font))
    return;

  lv_font_glyph_dsc_t g;
  if (!lv_font_get_glyph_dsc(font, &g, dsc->unicode, 0))
    return;

  if (g.box_w == 0 || g.box_h == 0)
    return;

  lv_area_t letter_coords;
  letter_coords.x1 = t->area.x1 + g.ofs_x - dsc->pivot.x;
  letter_coords.x2 = letter_coords.x1 + g.box_w - 1;
  letter_coords.y1 = t->area.y1 + (font->line_height - font->base_line) - g.box_h - g.ofs_y - dsc->pivot.y;
  letter_coords.y2 = letter_coords.y1 + g.box_h - 1;

  uint32_t color_u32 = lv_color_to_u32(dsc->color);
  uint32_t fg_color = (color_u32 & 0x00FFFFFFu) | ((uint32_t)dsc->opa << 24);

  _gfx_append_glyph(t, &g, &letter_coords, fg_color);
  _gfx_end_session();
}

void lv_draw_gfx_label_deinit(void) {
  _gfx_end_session();
  lv_memzero(&s_ctx, sizeof(s_ctx));
}

#endif /*LV_USE_DRAW_GFX*/
