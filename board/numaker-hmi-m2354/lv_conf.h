/**************************************************************************//**
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

#define LV_HOR_RES_MAX                  320
#define LV_VER_RES_MAX                  240
#define LV_COLOR_DEPTH                  16
#define LV_DISP_DEF_REFR_PERIOD         16

#define LV_TICK_CUSTOM                  1
#define LV_TICK_CUSTOM_INCLUDE          LV_NUVOTON_INCLUDE

#define LV_TICK_CUSTOM_SYS_TIME_EXPR    (sysGetTicks(0))    /*Expression evaluating to current system time in ms*/

#define LV_MEMCPY_MEMSET_STD            1

#define LV_SPRINTF_CUSTOM               1
#define LV_SPRINTF_INCLUDE              LV_NUVOTON_INCLUDE
#define lv_snprintf                     snprintf
#define lv_vsnprintf                    vsnprintf
#define LV_SPRINTF_USE_FLOAT            0

#define LV_FONT_MONTSERRAT_12           1
#define LV_FONT_MONTSERRAT_16           1

/* Please comment LV_USE_DEMO_MUSIC declaration before un-comment below */
#define LV_USE_DEMO_WIDGETS             1
//#define LV_USE_DEMO_MUSIC               1

#if LV_USE_DEMO_MUSIC
    #define LV_COLOR_SCREEN_TRANSP        1
    #define LV_DEMO_MUSIC_AUTO_PLAY       1
#endif

#define LV_USE_PERF_MONITOR             1
#define LV_USE_LOG                      1

#if LV_USE_LOG == 1
    //#define LV_LOG_LEVEL                    LV_LOG_LEVEL_TRACE
    ////#define LV_LOG_LEVEL                    LV_LOG_LEVEL_INFO
    //#define LV_LOG_LEVEL                    LV_LOG_LEVEL_WARN
    #define LV_LOG_LEVEL                    LV_LOG_LEVEL_ERROR
    //#define LV_LOG_LEVEL                    LV_LOG_LEVEL_USER
    //#define LV_LOG_LEVEL                    LV_LOG_LEVEL_NONE
#endif

#endif
