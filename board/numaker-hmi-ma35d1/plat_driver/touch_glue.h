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
#include "nu_misc.h"

#if !defined(PORT_OFFSET)
    #define PORT_OFFSET                0x40
#endif

#if defined(USE_1024x600_GT911)
    #define CONFIG_INDEV_TOUCH_GT911    1
    #define CONFIG_XY_REVERSED          1

    #define CONFIG_INDEV_TOUCH_I2C       I2C5
    #define CONFIG_INDEV_TOUCH_PIN_IRQ   NU_GET_PININDEX(evGK, 12)
    #define CONFIG_INDEV_TOUCH_PIN_RESET NU_GET_PININDEX(evGF, 1)
    #define INDEV_TOUCH_SET_RST          GPIO_PIN_DATA(NU_GET_PORT(CONFIG_INDEV_TOUCH_PIN_RESET), NU_GET_PIN(CONFIG_INDEV_TOUCH_PIN_RESET)) = 0
    #define INDEV_TOUCH_CLR_RST          GPIO_PIN_DATA(NU_GET_PORT(CONFIG_INDEV_TOUCH_PIN_RESET), NU_GET_PIN(CONFIG_INDEV_TOUCH_PIN_RESET)) = 1
    #define INDEV_TOUCH_SET_IRQ          GPIO_PIN_DATA(NU_GET_PORT(CONFIG_INDEV_TOUCH_PIN_IRQ), NU_GET_PIN(CONFIG_INDEV_TOUCH_PIN_IRQ)) = 1
    #define INDEV_TOUCH_CLR_IRQ          GPIO_PIN_DATA(NU_GET_PORT(CONFIG_INDEV_TOUCH_PIN_IRQ), NU_GET_PIN(CONFIG_INDEV_TOUCH_PIN_IRQ)) = 0
#else
    #define CONFIG_INDEV_TOUCH_ADC      1
#endif

#endif /* __TOUCH_GLUE_H__ */
