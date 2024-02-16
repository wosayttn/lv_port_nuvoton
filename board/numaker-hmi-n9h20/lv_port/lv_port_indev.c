/**************************************************************************//**
 * @file     lv_port_disp.c
 * @brief    lvgl input device porting
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
#include "lvgl.h"
#include "lv_glue.h"

static void input_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    touchpad_device_read(data);
}

void lv_port_indev_init(void)
{
    static lv_indev_t* lv_indev_touch;

    LV_ASSERT(touchpad_device_initialize() == 0);
    LV_ASSERT(touchpad_device_open() == 0);

    lv_indev_touch = lv_indev_create();
    lv_indev_set_type(lv_indev_touch, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(lv_indev_touch,  input_read);
}
