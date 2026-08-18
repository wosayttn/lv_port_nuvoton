/******************************************************************************
 * @file     lv_conf.h
 * @brief    lvgl configuration
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#ifndef LV_CONF_H
#define LV_CONF_H

#define LV_NUVOTON_INCLUDE     "NuMicro.h"
#include LV_NUVOTON_INCLUDE

#define CONFIG_LV_ATTRIBUTE_MEM_ALIGN_SIZE 64
#define CONFIG_LV_ATTRIBUTE_MEM_ALIGN __attribute__((aligned(64)))
#define CONFIG_LV_DRAW_BUF_ALIGN 64

#define LV_USE_DRAW_GFX 1
#define LV_USE_OS LV_OS_FREERTOS
#define LV_USE_DRAW_SW_ASM LV_DRAW_SW_ASM_NEON

#define CONFIG_DISP_DIRECT_REFRESH 1

#define LV_DEF_REFR_PERIOD 16
#if defined(__1024x600__)
    #define LV_HOR_RES_MAX 1024
    #define LV_VER_RES_MAX 600
    #define LV_COLOR_DEPTH 32
#else
    #define LV_HOR_RES_MAX 800
    #define LV_VER_RES_MAX 480
    #define LV_COLOR_DEPTH 32
#endif

#define lv_snprintf snprintf
#define lv_vsnprintf vsnprintf

#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_18 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_22 1
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_MONTSERRAT_26 1
#define LV_FONT_MONTSERRAT_28 1
#define LV_FONT_MONTSERRAT_30 1
#define LV_FONT_MONTSERRAT_32 1
#define LV_FONT_MONTSERRAT_34 1
#define LV_FONT_MONTSERRAT_36 1
#define LV_FONT_MONTSERRAT_38 1
#define LV_FONT_MONTSERRAT_40 1
#define LV_FONT_MONTSERRAT_42 1
#define LV_FONT_MONTSERRAT_44 1
#define LV_FONT_MONTSERRAT_46 1
#define LV_FONT_MONTSERRAT_48 1

#define CONFIG_LV_MEM_SIZE (8 * 1024 * 1024)
#define CONFIG_LV_CACHE_DEF_SIZE (8 * 1024 * 1024)
#define CONFIG_LV_DRAW_BUF_ALIGN 64
#define LV_DRAW_LAYER_SIMPLE_BUF_SIZE (1024 * 1024)

#define LV_USE_DEMO_WIDGETS 1
//#define LV_USE_DEMO_BENCHMARK 1

#define LV_USE_SYSMON 1
#define LV_USE_PERF_MONITOR 1
#define LV_USE_LOG 0

#if CONFIG_APP_DEMO_IMG == 1
    //#define LV_USE_LODEPNG                1
    //#define LV_USE_GIF                    1
    //#define LV_USE_BMP                    1
    //#define LV_USE_TJPGD                  1
    #define LV_USE_HWJPGD                   1

    #define LV_USE_FS_FATFS                 1
    #define LV_FS_FATFS_LETTER              'A'
    #define LV_USE_FS_MEMFS                 1
    #define LV_FS_MEMFS_LETTER              'Z'
#endif

#if LV_USE_LOG == 1
    //#define LV_LOG_LEVEL                    LV_LOG_LEVEL_TRACE
    //#define LV_LOG_LEVEL LV_LOG_LEVEL_INFO
    //#define LV_LOG_LEVEL                    LV_LOG_LEVEL_WARN
    //#define LV_LOG_LEVEL                    LV_LOG_LEVEL_ERROR
    #define LV_LOG_LEVEL LV_LOG_LEVEL_USER
    //#define LV_LOG_LEVEL                    LV_LOG_LEVEL_NONE
#endif

#endif
