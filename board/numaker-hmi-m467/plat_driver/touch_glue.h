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

#if defined(CONFIG_DISP_ILI9341) && (CONFIG_DISP_ILI9341==1)
    /* TouchPad over A/D converting */
    #define CONFIG_INDEV_TOUCH_AD         EADC0
    #define CONFIG_NG_MFP                 1
    #define CONFIG_AD_PIN_XL              NU_GET_PININDEX(evGB, 7)
    #define CONFIG_AD_PIN_YU              NU_GET_PININDEX(evGB, 6)
    #define CONFIG_AD_PIN_XR              NU_GET_PININDEX(evGB, 9)
    #define CONFIG_AD_PIN_YD              NU_GET_PININDEX(evGB, 8)
#else
    /* TouchPad over I2C */
    #define CONFIG_INDEV_TOUCH_I2C       I2C1
    #define CONFIG_INDEV_TOUCH_PIN_IRQ   NU_GET_PININDEX(evGG, 6)
    #define CONFIG_INDEV_TOUCH_PIN_RESET NU_GET_PININDEX(evGD, 10)
    #define INDEV_TOUCH_SET_RST          GPIO_PIN_DATA(NU_GET_PORT(CONFIG_INDEV_TOUCH_PIN_RESET), NU_GET_PIN(CONFIG_INDEV_TOUCH_PIN_RESET)) = 0
    #define INDEV_TOUCH_CLR_RST          GPIO_PIN_DATA(NU_GET_PORT(CONFIG_INDEV_TOUCH_PIN_RESET), NU_GET_PIN(CONFIG_INDEV_TOUCH_PIN_RESET)) = 1
    #define INDEV_TOUCH_SET_IRQ          GPIO_PIN_DATA(NU_GET_PORT(CONFIG_INDEV_TOUCH_PIN_IRQ), NU_GET_PIN(CONFIG_INDEV_TOUCH_PIN_IRQ)) = 1
    #define INDEV_TOUCH_CLR_IRQ          GPIO_PIN_DATA(NU_GET_PORT(CONFIG_INDEV_TOUCH_PIN_IRQ), NU_GET_PIN(CONFIG_INDEV_TOUCH_PIN_IRQ)) = 0
    #define CONFIG_XY_REVERSED           1
#endif

#endif /* __TOUCH_GLUE_H__ */
