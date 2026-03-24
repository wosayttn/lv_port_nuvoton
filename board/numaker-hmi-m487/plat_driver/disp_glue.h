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
#include "drv_pdma.h"

#if !defined(PORT_OFFSET)
    #define PORT_OFFSET                0x40
#endif

#if !defined(GPIO_PIN_DATA)
    #define GPIO_PIN_DATA              GPIO_PIN_DATA_S
#endif

#if !defined(CONFIG_DISP_USE_PDMA)
    #define CONFIG_DISP_USE_PDMA       1
#endif

#define CONFIG_DISP_EBI            EBI_BANK0
#define CONFIG_DISP_EBI_ADDR       (EBI_BANK0_BASE_ADDR+(CONFIG_DISP_EBI*EBI_MAX_SIZE))
#define CONFIG_DISP_CMD_ADDR       (CONFIG_DISP_EBI_ADDR+0x0)
#define CONFIG_DISP_DAT_ADDR       (CONFIG_DISP_EBI_ADDR+0x20)
#define CONFIG_DISP_PIN_BACKLIGHT  NU_GET_PININDEX(evGB, 7)   //23
#define CONFIG_DISP_PIN_RESET      NU_GET_PININDEX(evGB, 6)   //22

#define DISP_SET_RST                 GPIO_PIN_DATA(NU_GET_PORT(CONFIG_DISP_PIN_RESET),     NU_GET_PIN(CONFIG_DISP_PIN_RESET)) = 1
#define DISP_CLR_RST                 GPIO_PIN_DATA(NU_GET_PORT(CONFIG_DISP_PIN_RESET),     NU_GET_PIN(CONFIG_DISP_PIN_RESET)) = 0
#define DISP_SET_BACKLIGHT           GPIO_PIN_DATA(NU_GET_PORT(CONFIG_DISP_PIN_BACKLIGHT), NU_GET_PIN(CONFIG_DISP_PIN_BACKLIGHT)) = 1
#define DISP_CLR_BACKLIGHT           GPIO_PIN_DATA(NU_GET_PORT(CONFIG_DISP_PIN_BACKLIGHT), NU_GET_PIN(CONFIG_DISP_PIN_BACKLIGHT)) = 0

#define CONFIG_DISP_LINE_BUFFER_NUMBER      (DISP_VER_RES_MAX/8)

int EBI_ApplyTiming(int acc_ns, int wr_idle_ns, int wr_ahd_ns, int rd_ahd_ns, int rd_idle_ns);


void sysDelay(uint32_t ms);

#endif /* __DISP_GLUE_H__ */
