/**************************************************************************//**
 * @file     touch_glue.h
 * @brief    NuMaker TouchPad Driver glue code
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#ifndef __TOUCH_GLUE_H__
#define __TOUCH_GLUE_H__

#include <stdio.h>
#include <string.h>
#include "NuMicro.h"
#include "nu_misc.h"

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

    #define CONFIG_INDEV_TOUCH_AD          EADC0
    #define CONFIG_NG_MFP                  1

    #if defined(NUFUN) && (NUFUN==1)
        #define CONFIG_AD_PIN_XL             NU_GET_PININDEX(evGB, 5)
        #define CONFIG_AD_PIN_YU             NU_GET_PININDEX(evGB, 4)
        #define CONFIG_AD_PIN_XR             NU_GET_PININDEX(evGB, 3)
        #define CONFIG_AD_PIN_YD             NU_GET_PININDEX(evGB, 2)
    #else
        #define CONFIG_AD_PIN_XL             NU_GET_PININDEX(evGB, 15)
        #define CONFIG_AD_PIN_YU             NU_GET_PININDEX(evGB, 14)
        #define CONFIG_AD_PIN_XR             NU_GET_PININDEX(evGB, 9)
        #define CONFIG_AD_PIN_YD             NU_GET_PININDEX(evGB, 8)
    #endif

#else

#endif

#endif /* __TOUCH_GLUE_H__ */
