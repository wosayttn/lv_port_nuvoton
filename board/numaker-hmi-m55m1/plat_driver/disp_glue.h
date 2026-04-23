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

#if defined(CONFIG_DISP_ILI9341) && (CONFIG_DISP_ILI9341==1)
    /* ILI9341 SPI */
    #define CONFIG_DISP_SPI            SPI2
    #define CONFIG_DISP_SPI_CLOCK      74000000
    #if defined(CONFIG_DISP_USE_PDMA)
        #define CONFIG_PDMA_SPI_TX     PDMA_SPI2_TX
        #define CONFIG_PDMA_SPI_RX     PDMA_SPI2_RX
        #define CONFIG_SPI_USE_PDMA
    #endif
    #define CONFIG_DISP_PIN_DC         NU_GET_PININDEX(evGB, 2)
    #define CONFIG_DISP_PIN_RESET      NU_GET_PININDEX(evGB, 3)
    #define CONFIG_DISP_PIN_BACKLIGHT  NU_GET_PININDEX(evGB, 5)
    #define DISP_SET_RS                {GPIO_PIN_DATA(NU_GET_PORT(CONFIG_DISP_PIN_DC),        NU_GET_PIN(CONFIG_DISP_PIN_DC)) = 1; __DSB();}
    #define DISP_CLR_RS                {GPIO_PIN_DATA(NU_GET_PORT(CONFIG_DISP_PIN_DC),        NU_GET_PIN(CONFIG_DISP_PIN_DC)) = 0; __DSB();}
#else
    /* FSA506/LT7381/NV3041A EBI */
    #define CONFIG_DISP_EBI            EBI_BANK0
    #define CONFIG_DISP_EBI_ADDR       (EBI_BANK0_BASE_ADDR+(CONFIG_DISP_EBI*EBI_MAX_SIZE))
    #define CONFIG_DISP_CMD_ADDR       (CONFIG_DISP_EBI_ADDR+0x0)
    #define CONFIG_DISP_DAT_ADDR       (CONFIG_DISP_EBI_ADDR+0x2)
    #define CONFIG_DISP_PIN_BACKLIGHT  NU_GET_PININDEX(evGG, 5)
    #define CONFIG_DISP_PIN_RESET      NU_GET_PININDEX(evGH, 6)
#endif

#define DISP_SET_RST               GPIO_PIN_DATA(NU_GET_PORT(CONFIG_DISP_PIN_RESET),     NU_GET_PIN(CONFIG_DISP_PIN_RESET)) = 1
#define DISP_CLR_RST               GPIO_PIN_DATA(NU_GET_PORT(CONFIG_DISP_PIN_RESET),     NU_GET_PIN(CONFIG_DISP_PIN_RESET)) = 0
#define DISP_SET_BACKLIGHT         GPIO_PIN_DATA(NU_GET_PORT(CONFIG_DISP_PIN_BACKLIGHT), NU_GET_PIN(CONFIG_DISP_PIN_BACKLIGHT)) = 1
#define DISP_CLR_BACKLIGHT         GPIO_PIN_DATA(NU_GET_PORT(CONFIG_DISP_PIN_BACKLIGHT), NU_GET_PIN(CONFIG_DISP_PIN_BACKLIGHT)) = 0

int EBI_ApplyTiming(int acc_ns, int wr_idle_ns, int wr_ahd_ns, int rd_ahd_ns, int rd_idle_ns);


void sysDelay(uint32_t ms);

#endif /* __DISP_GLUE_H__ */
