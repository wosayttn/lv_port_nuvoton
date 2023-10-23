/**************************************************************************//**
 * @file     touch_st1663i.h
 * @brief    st1663i touch driver
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#ifndef __TOUCH_ST1663I_H__
#define __TOUCH_ST1663I_H__

#include <stdint.h>
#include "lv_glue.h"

int indev_touch_get_data(lv_indev_data_t *psInDevData);
int indev_touch_init(void);

#endif /* __TOUCH_ST1663I_H__ */
