#include "lvgl.h"
#include <string.h>

#define MAX_JPG_FILES   64
#define SLIDE_INTERVAL  1000   // ms
static char jpg_list[MAX_JPG_FILES][128];
static uint32_t jpg_count = 0;
static uint32_t jpg_index = 0;

static lv_obj_t *img_obj;
static lv_timer_t *slide_timer;
static int is_jpg(const char *name)
{
    const char *ext = strrchr(name, '.');
    if (!ext) return 0;

    if (!strcasecmp(ext, ".jpg")) return 1;
    if (!strcasecmp(ext, ".jpeg")) return 1;

    return 0;
}

static void scan_jpg_files(void)
{
    lv_fs_dir_t dir;
    char fn[256];

    jpg_count = 0;

    if (lv_fs_dir_open(&dir, "A:/") != LV_FS_RES_OK)
        return;

    while (lv_fs_dir_read(&dir, fn, sizeof(fn)) == LV_FS_RES_OK)
    {
        if (fn[0] == '\0') break;

        if (is_jpg(fn) && jpg_count < MAX_JPG_FILES)
        {
            lv_snprintf(jpg_list[jpg_count],
                        sizeof(jpg_list[0]),
                        "A:/%s",
                        fn);
            jpg_count++;
        }
    }

    lv_fs_dir_close(&dir);
}

static void slide_next_cb(lv_timer_t *t)
{
    if (jpg_count == 0) return;

    lv_image_set_src(img_obj, jpg_list[jpg_index]);

    jpg_index++;
    if (jpg_index >= jpg_count)
        jpg_index = 0;
}

void ui_init(void)
{
    scan_jpg_files();

    if (jpg_count == 0)
    {
        LV_LOG_INFO("No JPG found in A:/");
        return;
    }

    img_obj = lv_image_create(lv_screen_active());
    lv_obj_center(img_obj);

    jpg_index = 0;
    lv_image_set_src(img_obj, jpg_list[0]);

    slide_timer = lv_timer_create(
                      slide_next_cb,
                      SLIDE_INTERVAL,
                      NULL);
}
