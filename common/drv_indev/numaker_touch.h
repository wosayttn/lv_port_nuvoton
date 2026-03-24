/**************************************************************************//**
 * @file     numaker_touch.h
 * @brief    for touch driver
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2024 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#ifndef __NUMAKER_TOUCH_H__
#define __NUMAKER_TOUCH_H__

#include <stdint.h>
#include "plat_touch.h"
#include "touch_glue.h"
#include "numaker_disp.h"

typedef enum
{
    NUMAKER_INDEV_STATE_RELEASED = 0,
    NUMAKER_INDEV_STATE_PRESSED
} numaker_indev_state_t;

typedef struct
{
    uint32_t x;
    uint32_t y;
} numaker_point_t;

typedef struct
{
    numaker_indev_state_t state;
    numaker_point_t point;
} numaker_indev_data_t;

int touchpad_device_initialize(void);
int touchpad_device_finalize(void);
int touchpad_device_open(void);
int touchpad_device_read(numaker_indev_data_t *psInDevData);
void touchpad_device_close(void);
int touchpad_device_control(int cmd, void *argv);

int indev_touch_init(void);
int indev_touch_get_data(numaker_indev_data_t *psInDevData);

#endif /* __NUMAKER_TOUCH_H__ */
