#include "lvgl.h"
#include "avilib.h"

#define CONFIG_MJPEGD_TASK_PRIORITY      (configMAX_PRIORITIES-2)
static void img_timer(lv_timer_t *t)
{
    static avi_t *avi = NULL;
    static lv_img_dsc_t JpgImg = {0};
    static uint32_t idx = 0;
    lv_obj_t *img = (lv_obj_t *)lv_timer_get_user_data(t);
    static char *framebuf = (char *)0x82000000;

    if (!avi)
    {
        // AVI-MJPEG Player
        if ((avi = AVI_open_input_file("A:movie.avi", 1)) == NULL)
        {
            LV_LOG_INFO("[%s]Failed to open the avi file\n", __func__);
            AVI_close(avi);
            avi = NULL;
            return;
        }
        else
        {
            LV_LOG_INFO("AVI chunks: %ld, Width: %d, Height: %d, fps:%d\n",
                        AVI_video_frames(avi),
                        AVI_video_width(avi),
                        AVI_video_height(avi),
                        (uint32_t)AVI_frame_rate(avi));

            // Update period.
            lv_timer_set_period(t, 1000 / (AVI_frame_rate(avi) + 1));

        }
    }

    if (idx < AVI_video_frames(avi))
    {
        int key = 0;
        int framesize = AVI_read_frame(avi, &framebuf[0], &key);
        if (key && (framesize > 0))
        {
            /* Jpeg image is variable. */
            JpgImg.header.magic = LV_IMAGE_HEADER_MAGIC;
            JpgImg.header.cf = LV_COLOR_FORMAT_RAW;
            JpgImg.header.flags = 0;
            JpgImg.header.w = avi->width;
            JpgImg.header.h = avi->height;
            JpgImg.header.stride = 0;
            JpgImg.data = (const uint8_t *)&framebuf[0];
            JpgImg.data_size = framesize;

            /* Jpeg image decoding and rendering. */
            lv_image_set_src(img, (const void *)&JpgImg);
            lv_obj_center(img);

            idx++;
        }
    }
    else
    {
        idx = 0;
        AVI_set_video_position(avi, idx);
    }
}

void ui_init(void)
{
    lv_obj_t *img = lv_image_create(lv_screen_active());
    lv_obj_center(img);

    lv_timer_t *timer = lv_timer_create(img_timer, 100, img);
}
