/**************************************************************************//**
 * @file     lv_port_disp.c
 * @brief    lvgl input device porting
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
#include "lvgl.h"
#include "lv_glue.h"

static void input_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
    touchpad_device_read(data);
}

void lv_port_indev_init(void)
{
    static lv_indev_drv_t indev_drv;

    LV_ASSERT(touchpad_device_initialize() == 0);
    LV_ASSERT(touchpad_device_open() == 0);

    /* Basic initialization */
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = input_read;

    /* Register the driver in LVGL and save the created input device object */
    lv_indev_drv_register(&indev_drv);
}
