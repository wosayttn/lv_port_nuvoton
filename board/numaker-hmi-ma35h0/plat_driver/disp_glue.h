/**************************************************************************//**
 * @file     disp_glue.h
 * @brief    NuMaker Display Driver glue code for EBI 8080 interface LCD panel
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#ifndef __DISP_GLUE_H__
#define __DISP_GLUE_H__

#include <stdio.h>
#include <string.h>
#include "NuMicro.h"
#include "nu_misc.h"
#include "lv_conf.h"

#define CONFIG_VRAM_BUFFER_NUM              3
#define CONFIG_DISP_LINE_BUFFER_NUMBER      (DISP_VER_RES_MAX)
#define CONFIG_VRAM_TOTAL_ALLOCATED_SIZE    NVT_ALIGN((CONFIG_VRAM_BUFFER_NUM * DISP_HOR_RES_MAX * CONFIG_DISP_LINE_BUFFER_NUMBER * (DISP_COLOR_DEPTH/8)), 32)

#if defined(CONFIG_LV_DISP_FULL_REFRESH)
    #define CONFIG_DISP_FULL_REFRESH    CONFIG_LV_DISP_FULL_REFRESH
#endif

#if !defined(CONFIG_DISP_FULL_REFRESH)
    #define CONFIG_DISP_FULL_REFRESH    0
#endif

#define DISP_HOR_RES_MAX          LV_HOR_RES_MAX
#define DISP_VER_RES_MAX          LV_VER_RES_MAX
#define DISP_COLOR_DEPTH          LV_COLOR_DEPTH

#define CONFIG_DISPLAY_BOARD_NAME   "MA35H0 display board"


void sysDelay(uint32_t ms);
#endif /* __DISP_GLUE_H__ */
