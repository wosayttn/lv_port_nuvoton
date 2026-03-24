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

/* TouchPad over A/D converting */
#define CONFIG_INDEV_TOUCH_AD         EADC0
#define CONFIG_AD_PIN_XL           NU_GET_PININDEX(evGH, 4)
#define CONFIG_AD_PIN_YU           NU_GET_PININDEX(evGB, 8)
#define CONFIG_AD_PIN_XR           NU_GET_PININDEX(evGB, 9)
#define CONFIG_AD_PIN_YD           NU_GET_PININDEX(evGH, 5)

#endif /* __TOUCH_GLUE_H__ */
