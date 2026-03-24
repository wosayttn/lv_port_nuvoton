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

#if defined(USE_NUMAKER_HMI_N9H31_V1_1)
    //For V1.1 board
    #define CONFIG_INDEV_TOUCH_GT911    1

    /* GT911 I2C */
    #define CONFIG_INDEV_TOUCH_I2C       0  //I2CNUM_0
    #define CONFIG_INDEV_TOUCH_PIN_IRQ   NU_GET_PININDEX(evGI, 1)
    #define CONFIG_INDEV_TOUCH_PIN_RESET NU_GET_PININDEX(evGI, 11)
    #define INDEV_TOUCH_SET_RST          GPIO_ClrBit((NU_GET_PORT(CONFIG_INDEV_TOUCH_PIN_RESET)*PORT_OFFSET), NU_GET_PIN_MASK(NU_GET_PIN(CONFIG_INDEV_TOUCH_PIN_RESET)))
    #define INDEV_TOUCH_CLR_RST          GPIO_SetBit((NU_GET_PORT(CONFIG_INDEV_TOUCH_PIN_RESET)*PORT_OFFSET), NU_GET_PIN_MASK(NU_GET_PIN(CONFIG_INDEV_TOUCH_PIN_RESET)))
    #define INDEV_TOUCH_SET_IRQ          GPIO_SetBit((NU_GET_PORT(CONFIG_INDEV_TOUCH_PIN_IRQ)*PORT_OFFSET), NU_GET_PIN_MASK(NU_GET_PIN(CONFIG_INDEV_TOUCH_PIN_IRQ)))
    #define INDEV_TOUCH_CLR_IRQ          GPIO_ClrBit((NU_GET_PORT(CONFIG_INDEV_TOUCH_PIN_IRQ)*PORT_OFFSET), NU_GET_PIN_MASK(NU_GET_PIN(CONFIG_INDEV_TOUCH_PIN_IRQ)))
#else
    //For V1.0 board
    #define CONFIG_INDEV_TOUCH_ADC      1
#endif

#endif /* __TOUCH_GLUE_H__ */
