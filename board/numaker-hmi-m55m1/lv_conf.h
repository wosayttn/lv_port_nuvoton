/**************************************************************************//**
 * @file     lv_conf.h
 * @brief    lvgl configuration
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#ifndef LV_CONF_H
#define LV_CONF_H


#define LV_HOR_RES_MAX                  480
#define LV_VER_RES_MAX                  272
#define LV_COLOR_DEPTH                  16
#define LV_DISP_DEF_REFR_PERIOD         30

#define LV_USE_GPU_ARM2D                1

#define LV_TICK_CUSTOM                  1
#define LV_TICK_CUSTOM_INCLUDE          "lv_glue.h"
#define LV_TICK_CUSTOM_SYS_TIME_EXPR    (sysGetTicks(0))    /*Expression evaluating to current system time in ms*/

#define LV_MEMCPY_MEMSET_STD            1

#define LV_FONT_MONTSERRAT_12           1
#define LV_FONT_MONTSERRAT_16           1
#define LV_USE_FONT_COMPRESSED          1

/* Please comment LV_USE_DEMO_MUSIC declaration before un-comment below */
//#define LV_USE_DEMO_WIDGETS             1
#define LV_USE_DEMO_MUSIC               1
//#define LV_USE_DEMO_BENCHMARK           1

#define LV_MEM_SIZE                 (64*1024)

#if LV_USE_DEMO_MUSIC
    #define LV_DEMO_MUSIC_AUTO_PLAY       1
    #define LV_COLOR_SCREEN_TRANSP        1
#endif

#define LV_USE_PERF_MONITOR             1
#define LV_USE_LOG                      1
#if LV_USE_LOG == 1
    //#define LV_LOG_LEVEL                    LV_LOG_LEVEL_TRACE
    //#define LV_LOG_LEVEL                    LV_LOG_LEVEL_INFO
    //#define LV_LOG_LEVEL                    LV_LOG_LEVEL_WARN
    #define LV_LOG_LEVEL                    LV_LOG_LEVEL_ERROR
    //#define LV_LOG_LEVEL                    LV_LOG_LEVEL_USER
    //#define LV_LOG_LEVEL                    LV_LOG_LEVEL_NONE
#endif

#endif
