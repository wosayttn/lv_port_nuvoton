#include "lvgl.h"
#include "avilib.h"

#if LV_USE_LODEPNG && LV_USE_IMAGE
/**
 * Show a PNG image from a file
 */
static void demo_show_png_image(void)
{
    lv_obj_t *img = lv_image_create(lv_screen_active());
    lv_image_set_src(img, "0:wink.png");
    lv_obj_align(img, LV_ALIGN_LEFT_MID, 20, 0);
}
#endif

#if LV_USE_HWJPGD && LV_USE_IMAGE
/**
 * Show a JPEG image from a file
 */
static void demo_show_jpg_image(void)
{
    lv_obj_t *img = lv_image_create(lv_screen_active());
    lv_image_set_src(img, "0:logo.jpg");
    lv_obj_center(img);
}
#endif

#if LV_USE_GIF && LV_USE_IMAGE
/**
 * Open a GIF image from a file
 */
static void demo_show_gif_image(void)
{
    /* Important: To Fix(gif): fix bounds check in v9.2.2 --> PR: https://github.com/lvgl/lvgl/pull/7675 */
    lv_obj_t *img = lv_gif_create(lv_screen_active());
    lv_gif_set_src(img, "0:bulb.gif");
    lv_obj_center(img);
}
#endif

#if LV_USE_BMP && LV_USE_IMAGE
/**
 * Open a BMP file from a file
 */
static void demo_show_bmp_image(void)
{
    lv_obj_t *img = lv_image_create(lv_screen_active());
    lv_image_set_src(img, "0:fly.bmp");
    lv_obj_align(img, LV_ALIGN_TOP_MID, 0, 10);
}
#endif

void ui_init(void)
{
    int fatfs_ramdisk_init(void);
    fatfs_ramdisk_init();

#if LV_USE_LODEPNG && LV_USE_IMAGE
    demo_show_png_image();
#endif

#if LV_USE_GIF && LV_USE_IMAGE
    demo_show_gif_image();
#endif

#if LV_USE_BMP && LV_BUILD_EXAMPLES
    demo_show_bmp_image();
#endif

#if LV_USE_HWJPGD && LV_USE_IMAGE
    demo_show_jpg_image();
#endif

}
