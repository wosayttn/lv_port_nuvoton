/**************************************************************************//**
 * @file     input touch.h
 * @brief    for lv input touch driver
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#ifndef __INPUT_TOUCH_H__
#define __INPUT_TOUCH_H__

#include <stdint.h>
#include "lv_glue.h"

int indev_touch_get_data(lv_indev_data_t *psInDevData);
int indev_touch_init(void);

#endif /* __INPUT_TOUCH_H__*/
