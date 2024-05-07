/**************************************************************************//**
 * @file     lv_conf.h
 * @brief    lvgl configuration
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#ifndef LV_CONF_H
#define LV_CONF_H

#define LV_USE_OS   LV_OS_NONE

#define LV_HOR_RES_MAX                  480
#define LV_VER_RES_MAX                  272
#define LV_COLOR_DEPTH                  16

#define LV_FONT_MONTSERRAT_12           1
#define LV_FONT_MONTSERRAT_16           1

/* Please comment LV_USE_DEMO_MUSIC declaration before un-comment below */
#define LV_USE_DEMO_WIDGETS             1
//#define LV_USE_DEMO_MUSIC             1
#if LV_USE_DEMO_MUSIC
    #define LV_DEMO_MUSIC_AUTO_PLAY     1
#endif

#define LV_USE_SYSMON                   1
#define LV_USE_PERF_MONITOR             1
#define LV_USE_LOG                      0

#define CONFIG_LV_MEM_SIZE              (64*1024U)

#if LV_USE_LOG == 1
    //#define LV_LOG_LEVEL                    LV_LOG_LEVEL_TRACE
    //#define LV_LOG_LEVEL                    LV_LOG_LEVEL_INFO
    //#define LV_LOG_LEVEL                    LV_LOG_LEVEL_WARN
    #define LV_LOG_LEVEL                    LV_LOG_LEVEL_ERROR
    //#define LV_LOG_LEVEL                    LV_LOG_LEVEL_USER
    //#define LV_LOG_LEVEL                    LV_LOG_LEVEL_NONE
#endif

#endif
