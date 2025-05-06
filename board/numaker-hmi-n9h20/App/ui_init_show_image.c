#include "lvgl.h"
#include "avilib.h"

static const uint32_t crc32_tab[] =
{
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
    0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
    0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
    0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
    0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
    0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
    0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
    0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
    0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
    0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
    0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
    0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
    0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
    0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
    0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
    0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
    0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
    0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
    0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
    0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
    0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
    0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
    0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
    0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
    0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
    0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
    0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
    0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
    0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
    0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
    0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
    0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

static uint32_t crc32(uint8_t *ptr, uint32_t len)
{
#define CRC_SEED    0xFFFFFFFF
    uint32_t crc = CRC_SEED;
    uint32_t i;
    uint8_t data8;

    for (i = 0; i < len; i++)
    {
        data8 = ptr[i];
        crc = crc32_tab[(crc ^ data8) & 0xFF] ^ (crc >> 8);
    }

    return crc ^ ~0U;
}

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

static int is_jfif(const uint8_t *jpeg_data, size_t len)
{
    if (len < 20) return 0;
    if (jpeg_data[0] != 0xFF || jpeg_data[1] != 0xD8) return 0;
    if (jpeg_data[2] != 0xFF || jpeg_data[3] != 0xE0) return 0;
    if (memcmp(&jpeg_data[6], "JFIF", 4) == 0)
    {
        return 1;
    }
    return 0;
}

static avi_t *avi = NULL;
static void img_timer(lv_timer_t *t)
{
    static lv_img_dsc_t JpgImg = {0};
    static char framebuf[1 * 1024 * 1024];
    static uint32_t idx = 0;
    lv_obj_t *img = (lv_obj_t *)lv_timer_get_user_data(t);
    uint32_t chunk = AVI_video_frames(avi);

    if (avi && img && (idx < chunk))
    {
        int key = 0;
        int framesize = AVI_read_frame(avi, &framebuf[0], &key);

        if (key && (framesize > 0))
        {
            //sysprintf("[%d/%d]framesize: %d, key: %d crc32: 0x%08x\n", idx+1, chunk, framesize, key, crc32(framebuf, framesize));

            /* Jpeg image is variable. */					
            JpgImg.header.magic = LV_IMAGE_HEADER_MAGIC;
            JpgImg.header.cf = LV_COLOR_FORMAT_RAW;
            JpgImg.header.flags = 0;
            JpgImg.header.w = avi->width;
            JpgImg.header.h = avi->height;
            JpgImg.header.stride = 0;
            JpgImg.data = (const char *)&framebuf[0];
            JpgImg.data_size = framesize;

            /* Jpeg image decoding and rendering. */
            lv_image_set_src(img, (const void *)&JpgImg);
            lv_obj_center(img);

            idx++;
        }
    }
    else if (idx == chunk)
    {
        idx = 0;
        AVI_set_video_position(avi, idx);
    }
}

/**
 * Open a AVI-MJPEG from a file
 */
static void demo_show_avi_mjpeg(void)
{
    // AVI-MJPEG Player
    if ((avi = AVI_open_input_file("0:movie.avi", 1)) == NULL)
    {
        sysprintf("[%s]Failed to open the avi file\n", __func__);
        AVI_close(avi);
        avi = NULL;
    }
    else
    {
        sysprintf("AVI chunks: %d, Width: %d, Height: %d, fps:%d\n",
                  AVI_video_frames(avi),
                  AVI_video_width(avi),
                  AVI_video_height(avi),
                  (uint32_t)AVI_frame_rate(avi));

        lv_obj_t *img = lv_image_create(lv_screen_active());
        lv_timer_t *timer = lv_timer_create(img_timer, 1000 / (AVI_frame_rate(avi) + 1), img);
    }
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
    //demo_show_jpg_image();
    demo_show_avi_mjpeg();
#endif

}
