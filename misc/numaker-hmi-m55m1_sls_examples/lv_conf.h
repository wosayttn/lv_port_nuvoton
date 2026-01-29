/**************************************************************************//**
 * @file     lv_conf.h
 * @brief    lvgl configuration
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#ifndef LV_CONF_H
#define LV_CONF_H

#define CONFIG_LV_ATTRIBUTE_MEM_ALIGN_SIZE     32
#define CONFIG_LV_ATTRIBUTE_MEM_ALIGN          __attribute__((aligned(32)))
#define CONFIG_LV_DRAW_BUF_ALIGN               32

#define LV_USE_OS   LV_OS_FREERTOS

#define LV_USE_DRAW_GDMA                  1
#if !defined(__ICCARM__)
//    #define LV_USE_NATIVE_HELIUM_ASM          1
//    #define LV_USE_DRAW_SW_ASM                LV_DRAW_SW_ASM_HELIUM
    #define LV_USE_NATIVE_HELIUM_ASM          0
    #define LV_USE_DRAW_SW_ASM                LV_DRAW_SW_ASM_HELIUM
    #define LV_USE_DRAW_ARM2D_SYNC            1
#endif
//#define CONFIG_LV_DISP_FULL_REFRESH     1

#define LV_ATTRIBUTE_MEM_ALIGN          __attribute__((section(".spim_data"), aligned(32), used))

#if defined(__320x240__)
    #define LV_HOR_RES_MAX              320
    #define LV_VER_RES_MAX              240
#endif

#if defined(__480x272__)
    #define LV_HOR_RES_MAX              480
    #define LV_VER_RES_MAX              272
#endif

#if defined(__800x480__)
    #define LV_HOR_RES_MAX              800
    #define LV_VER_RES_MAX              480
#endif

#define LV_COLOR_DEPTH                  16

#define LV_FONT_MONTSERRAT_12           1
#define LV_FONT_MONTSERRAT_16           1
#define LV_FONT_MONTSERRAT_24           1
#define LV_FONT_MONTSERRAT_26           1
#define LV_FONT_MONTSERRAT_46           1

#define CONFIG_LV_DRAW_BUF_ALIGN        32

#define LV_USE_DEMO_WIDGETS             1
//#define LV_USE_DEMO_BENCHMARK           1

#define LV_USE_SYSMON                   1
#define LV_USE_PERF_MONITOR             1
#define LV_USE_LOG                      0

#define CONFIG_LV_MEM_SIZE              (768*1024U)

#if LV_USE_LOG == 1
    //#define LV_LOG_LEVEL                    LV_LOG_LEVEL_TRACE
    #define LV_LOG_LEVEL                    LV_LOG_LEVEL_INFO
    //#define LV_LOG_LEVEL                    LV_LOG_LEVEL_WARN
    //#define LV_LOG_LEVEL                    LV_LOG_LEVEL_ERROR
    //#define LV_LOG_LEVEL                    LV_LOG_LEVEL_USER
    //#define LV_LOG_LEVEL                    LV_LOG_LEVEL_NONE
#endif

#endif
