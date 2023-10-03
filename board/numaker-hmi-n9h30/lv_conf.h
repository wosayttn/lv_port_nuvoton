/**************************************************************************//**
 * @file     lv_conf.h
 * @brief    lvgl configuration
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#ifndef LV_CONF_H
#define LV_CONF_H

#define LV_NUVOTON_INCLUDE     "N9H30.h"
#include LV_NUVOTON_INCLUDE
#include "sys.h"

#if defined(__800x480__)
    #define LV_HOR_RES_MAX              800
    #define LV_VER_RES_MAX              480
    #define LV_COLOR_DEPTH              16

    #define LV_DISP_DEF_REFR_PERIOD     30
#elif defined(__480x272__)
    #define LV_HOR_RES_MAX              480
    #define LV_VER_RES_MAX              272
    #define LV_COLOR_DEPTH              16

    #define LV_DISP_DEF_REFR_PERIOD     16
#else
    #define LV_HOR_RES_MAX              320
    #define LV_VER_RES_MAX              240
    #define LV_COLOR_DEPTH              16

    #define LV_DISP_DEF_REFR_PERIOD     16
#endif

/*
(1) Apply partial update:
    i. Higher FPS.
   ii. Easy tearing when sliding.
  iii. Copy offline surface buffer to on-screen buffer.

(2) Apply full-refresh:
    i. Tearingless by switching VRAM start address after receiving Vsync of VPOST.
   ii. lvgl keeps previous surface and redraw new dirty joined area, the do vsync-switch.
*/
#define CONFIG_LV_DISP_FULL_REFRESH      0
#define CONFIG_LV_GPU_USE_N9H30_2DGE     1

#if CONFIG_LV_DISP_FULL_REFRESH
    #define CONFIG_LCD_FB_NUM            3
#else
    #define CONFIG_LCD_FB_NUM            2
#endif

#define CONFIG_SYS_TIMER                TIMER0

#define LV_TICK_CUSTOM                  1
#define LV_TICK_CUSTOM_INCLUDE          LV_NUVOTON_INCLUDE
#define LV_TICK_CUSTOM_SYS_TIME_EXPR    (sysGetTicks(CONFIG_SYS_TIMER))    /*Expression evaluating to current system time in ms*/

#define LV_MEMCPY_MEMSET_STD            1

#define LV_SPRINTF_CUSTOM               1
#define LV_SPRINTF_INCLUDE              LV_NUVOTON_INCLUDE
#define lv_snprintf                     snprintf
#define lv_vsnprintf                    vsnprintf
#define LV_SPRINTF_USE_FLOAT            0

#define LV_FONT_MONTSERRAT_12           1
#define LV_FONT_MONTSERRAT_14           1
#define LV_FONT_MONTSERRAT_16           1
#define LV_FONT_MONTSERRAT_18           1
#define LV_FONT_MONTSERRAT_20           1
#define LV_FONT_MONTSERRAT_22           1
#define LV_FONT_MONTSERRAT_24           1
#define LV_FONT_MONTSERRAT_26           1
#define LV_FONT_MONTSERRAT_28           1
#define LV_FONT_MONTSERRAT_30           1
#define LV_FONT_MONTSERRAT_32           1
#define LV_FONT_MONTSERRAT_34           1
#define LV_FONT_MONTSERRAT_36           1
#define LV_FONT_MONTSERRAT_38           1
#define LV_FONT_MONTSERRAT_40           1
#define LV_FONT_MONTSERRAT_42           1
#define LV_FONT_MONTSERRAT_44           1
#define LV_FONT_MONTSERRAT_46           1
#define LV_FONT_MONTSERRAT_48           1

#define CONFIG_LV_MEM_SIZE              (512*1024)
#define CONFIG_LV_LAYER_SIMPLE_BUF_SIZE (256*1024)

/* Please comment LV_USE_DEMO_MUSIC declaration before un-comment below */
//#define LV_USE_DEMO_WIDGETS             1
#define LV_USE_DEMO_MUSIC               1

#if LV_USE_DEMO_MUSIC
    #define LV_COLOR_SCREEN_TRANSP        1
    #define LV_DEMO_MUSIC_AUTO_PLAY       1
#endif

#define LV_USE_PERF_MONITOR             1
#define LV_USE_LOG                      0

#if LV_USE_LOG == 1
    //#define LV_LOG_LEVEL                    LV_LOG_LEVEL_TRACE
    #define LV_LOG_LEVEL                    LV_LOG_LEVEL_INFO
    //#define LV_LOG_LEVEL                    LV_LOG_LEVEL_WARN
    //#define LV_LOG_LEVEL                    LV_LOG_LEVEL_ERROR
    //#define LV_LOG_LEVEL                    LV_LOG_LEVEL_USER
    //#define LV_LOG_LEVEL                    LV_LOG_LEVEL_NONE
#endif

#endif
