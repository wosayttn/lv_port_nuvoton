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

#if defined(__320x240__)
    #define LV_HOR_RES_MAX                  320
    #define LV_VER_RES_MAX                  240
    #define CONFIG_LV_DEF_REFR_PERIOD       16
#endif

#define LV_COLOR_DEPTH                  16

#define LV_FONT_MONTSERRAT_12           1
#define LV_FONT_MONTSERRAT_16           1
#define LV_FONT_MONTSERRAT_20           1
#define LV_FONT_MONTSERRAT_24           0

#define LV_FONT_DEFAULT                 &lv_font_montserrat_16

#define UI_FONT_CJK_AVAILABLE            0

/* Core widgets */
#define LV_USE_ANIMIMG                  0
#define LV_USE_ARC                      0
#define LV_USE_ARCLABEL                 0
#define LV_USE_BAR                      0
#define LV_USE_BUTTON                   0
#define LV_USE_BUTTONMATRIX             0
#define LV_USE_CALENDAR                 0
#define LV_USE_CANVAS                   0
#define LV_USE_CHART                    0
#define LV_USE_CHECKBOX                 0
#define LV_USE_DROPDOWN                 0
#define LV_USE_IMAGE                    1
#define LV_USE_IMAGEBUTTON              0
#define LV_USE_KEYBOARD                 0
#define LV_USE_LABEL                    1
#define LV_USE_LED                      0
#define LV_USE_LINE                     0
#define LV_USE_LIST                     0
#define LV_USE_MENU                     0
#define LV_USE_MSGBOX                   0
#define LV_USE_ROLLER                   0
#define LV_USE_SCALE                    0
#define LV_USE_SLIDER                   0
#define LV_USE_SPAN                     0
#define LV_USE_SPINBOX                  0
#define LV_USE_SPINNER                  0
#define LV_USE_SWITCH                   0
#define LV_USE_TABLE                    0
#define LV_USE_TABVIEW                  0
#define LV_USE_TEXTAREA                 0
#define LV_USE_TILEVIEW                 0
#define LV_USE_WIN                      0

#define LV_USE_GIF                      1

/* Layouts & Themes */
#define LV_USE_FLEX                     0
#define LV_USE_GRID                     0
#define LV_USE_THEME_DEFAULT            1
#define LV_USE_THEME_SIMPLE             0
#define LV_USE_THEME_MONO               0

/* Feature pruning for minimal ROM */
#define LV_USE_OBJ_PROPERTY             0
#define LV_USE_OBJ_PROPERTY_NAME        0
#define LV_USE_OBJ_ID_BUILTIN           0
#define LV_USE_VECTOR_GRAPHIC           0
#define LV_USE_SNAPSHOT                 0
#define LV_USE_BIDI                     0
#define LV_USE_ARABIC_PERSIAN_CHARS     0
#define LV_USE_FLOAT                    0
#define LV_USE_LOTTIE                   0
#define LV_USE_FFMPEG                   0
#define LV_USE_FILE_EXPLORER            0
#define LV_USE_TRANSLATION              0

#define LV_USE_FS_FATFS                 1
#if LV_USE_FS_FATFS
    #define LV_FS_FATFS_LETTER          'A'
    #define LV_FS_FATFS_PATH            ""
    #define LV_FS_FATFS_CACHE_SIZE      0
#endif

#define LV_USE_DEMO_WIDGETS             0

#define LV_USE_SYSMON                   1
#define LV_USE_PERF_MONITOR             1
#define LV_USE_LOG                      0

#define CONFIG_LV_MEM_SIZE            (224*1024U)

#if LV_USE_LOG == 1
    #define LV_LOG_LEVEL                    LV_LOG_LEVEL_INFO
    //#define LV_LOG_LEVEL                    LV_LOG_LEVEL_TRACE
    //#define LV_LOG_LEVEL                    LV_LOG_LEVEL_WARN
    //#define LV_LOG_LEVEL                    LV_LOG_LEVEL_ERROR
    //#define LV_LOG_LEVEL                    LV_LOG_LEVEL_USER
    //#define LV_LOG_LEVEL                    LV_LOG_LEVEL_NONE
#endif

#endif
