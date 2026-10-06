/**************************************************************************//**
 * @file     lv_conf.h
 * @brief    lvgl configuration
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#ifndef LV_CONF_H
#define LV_CONF_H

#define LV_USE_OS   LV_OS_FREERTOS
#define LV_DRAW_THREAD_STACK_SIZE       (2 * 1024)

#if defined(__320x240__)
    #define LV_HOR_RES_MAX                  320
    #define LV_VER_RES_MAX                  240
    #define CONFIG_LV_DEF_REFR_PERIOD       16
#endif

#define LV_COLOR_DEPTH                  16

#define LV_FONT_MONTSERRAT_12           1
#define LV_FONT_MONTSERRAT_16           1
#define LV_FONT_MONTSERRAT_20           1
#define LV_FONT_MONTSERRAT_24           1
#define UI_FONT_CJK_AVAILABLE            1

#define LV_USE_SYSMON                   1
#define LV_USE_PERF_MONITOR             1
#define LV_USE_MEM_MONITOR              1
#if LV_USE_MEM_MONITOR
    #define LV_USE_MEM_MONITOR_POS          LV_ALIGN_BOTTOM_LEFT
#endif
#define LV_USE_LOG                      0

/* Memory manager: use standard C library malloc/free/realloc to unify with C Runtime & FreeRTOS Heap */
#define LV_USE_STDLIB_MALLOC            LV_STDLIB_CLIB

#if LV_USE_LOG == 1
    #define LV_LOG_LEVEL                    LV_LOG_LEVEL_INFO
    //#define LV_LOG_LEVEL                    LV_LOG_LEVEL_TRACE
    //#define LV_LOG_LEVEL                    LV_LOG_LEVEL_WARN
    //#define LV_LOG_LEVEL                    LV_LOG_LEVEL_ERROR
    //#define LV_LOG_LEVEL                    LV_LOG_LEVEL_USER
    //#define LV_LOG_LEVEL                    LV_LOG_LEVEL_NONE
#endif

#endif
