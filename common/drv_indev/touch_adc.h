/**************************************************************************//**
 * @file     indev_touch_adc.h
 * @brief    software ad touch driver
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#ifndef __INDEV_TOUCH_ADC_H__
#define __INDEV_TOUCH_ADC_H__

#include <stdint.h>
#include "lv_glue.h"

uint32_t nu_adc_sampling(uint32_t channel);
uint32_t indev_touch_get_x(void);
uint32_t indev_touch_get_y(void);

#endif /* __INDEV_TOUCH_ADC_H__ */
