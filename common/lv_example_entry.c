/**************************************************************************//**
 * @file     lv_examle_app.c
 * @brief    LVGL run an example.
 *
 * @note
 * Copyright (C) 2025 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/

#include "lvgl.h"

static lv_obj_t *needle_line;
static lv_obj_t *led;
static lv_obj_t *scr;

static void set_needle_line_value(void *obj, int32_t v)
{
    lv_scale_set_line_needle_value((lv_obj_t *)obj, needle_line, 60, v);
}

/* touch event callback */
static void screen_touch_event_cb(lv_event_t * e)
{
    lv_indev_t * indev = lv_indev_get_act();
    if (!indev) return;

    lv_point_t p;
    lv_indev_get_point(indev, &p);
	
    lv_area_t cont_coords;
    lv_obj_get_coords(scr, &cont_coords);

    int32_t x = p.x - cont_coords.x1;
    int32_t y = p.y - cont_coords.y1;
    x -= lv_obj_get_width(led) / 2;
    y -= lv_obj_get_height(led) / 2;
	
    lv_obj_set_pos(led, x, y);
}

void ui_init(void)
{
    lv_obj_t *scale_line = lv_scale_create(lv_screen_active());

    lv_obj_set_size(scale_line, 150, 150);
    lv_scale_set_mode(scale_line, LV_SCALE_MODE_ROUND_INNER);
    lv_obj_set_style_bg_opa(scale_line, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(scale_line, lv_palette_lighten(LV_PALETTE_RED, 5), 0);
    lv_obj_set_style_radius(scale_line, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_clip_corner(scale_line, true, 0);
    lv_obj_align(scale_line, LV_ALIGN_CENTER, 0, 0);

    lv_scale_set_label_show(scale_line, true);

    lv_scale_set_total_tick_count(scale_line, 31);
    lv_scale_set_major_tick_every(scale_line, 5);

    lv_obj_set_style_length(scale_line, 5, LV_PART_ITEMS);
    lv_obj_set_style_length(scale_line, 10, LV_PART_INDICATOR);
    lv_scale_set_range(scale_line, 10, 40);

    lv_scale_set_angle_range(scale_line, 270);
    lv_scale_set_rotation(scale_line, 135);

    needle_line = lv_line_create(scale_line);

    lv_obj_set_style_line_width(needle_line, 6, LV_PART_MAIN);
    lv_obj_set_style_line_rounded(needle_line, true, LV_PART_MAIN);

    lv_anim_t anim_scale_line;
    lv_anim_init(&anim_scale_line);
    lv_anim_set_var(&anim_scale_line, scale_line);
    lv_anim_set_exec_cb(&anim_scale_line, set_needle_line_value);
    lv_anim_set_duration(&anim_scale_line, 1000);
    lv_anim_set_repeat_count(&anim_scale_line, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_reverse_duration(&anim_scale_line, 1000);
    lv_anim_set_values(&anim_scale_line, 10, 40);
    lv_anim_start(&anim_scale_line);

    // -----------------------------------------------------------------
		
    scr = lv_screen_active();

    led = lv_led_create(scr);
    lv_obj_set_size(led, 30, 30);
    lv_led_set_brightness(led, 150);
    lv_led_set_color(led, lv_palette_main(LV_PALETTE_BLUE));

    lv_obj_add_flag(scr, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(scr, screen_touch_event_cb, LV_EVENT_PRESSED, NULL);
}
