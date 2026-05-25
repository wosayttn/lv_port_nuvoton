/**
 * @file lv_conf.h
 * @brief LVGL configuration for PC simulator (SDL2 backend)
 */
#ifndef LV_CONF_H
#define LV_CONF_H

/* No OS in simulator – use plain while-loop */
#define LV_USE_OS               LV_OS_NONE

#ifndef __320x240__
#define __320x240__
#endif
#define LV_HOR_RES_MAX          320
#define LV_VER_RES_MAX          240
#define CONFIG_LV_DEF_REFR_PERIOD   16   /* ~30 FPS is fine for sim */

#define LV_COLOR_DEPTH          16

#define LV_FONT_MONTSERRAT_12   1
#define LV_FONT_MONTSERRAT_16   1
#define LV_FONT_MONTSERRAT_20   1
#define LV_FONT_MONTSERRAT_24   1

#define UI_FONT_CJK_AVAILABLE   1

#define LV_USE_SYSMON           1
#define LV_USE_PERF_MONITOR     1
#define LV_USE_LOG              1
#define LV_LOG_LEVEL            LV_LOG_LEVEL_WARN
#define LV_LOG_PRINTF           1

/* SDL2 display driver */
#define LV_USE_SDL              1
#define LV_SDL_WINDOW_TITLE     "Rice Cooker Simulator"
#define LV_SDL_FULLSCREEN       0

/* Memory */
#define CONFIG_LV_MEM_SIZE      (256*1024U)

/* Unused demos */
#define LV_USE_DEMO_WIDGETS     0
#define LV_USE_DEMO_BENCHMARK   0

#endif /* LV_CONF_H */
