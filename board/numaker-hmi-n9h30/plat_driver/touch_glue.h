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

#define CONFIG_INDEV_TOUCH_ADC      1

#endif /* __TOUCH_GLUE_H__ */
