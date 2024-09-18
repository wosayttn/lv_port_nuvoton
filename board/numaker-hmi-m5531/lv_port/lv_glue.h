/**************************************************************************//**
 * @file     lv_glue.c
 * @brief    lvgl glue header
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#ifndef __LV_GLUE_H__
#define __LV_GLUE_H__

#include <stdio.h>
#include "NuMicro.h"
#include "lvgl.h"
#include "nu_misc.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#define PORT_OFFSET   0x40

/* Define off-screen line buffer number,  Range: 1~LV_VER_RES_MAX */
#define CONFIG_DISP_LINE_BUFFER_NUMBER  (LV_VER_RES_MAX)

#if (CONFIG_DISP_LINE_BUFFER_NUMBER < 1) || (CONFIG_DISP_LINE_BUFFER_NUMBER > LV_VER_RES_MAX)
    #error "Wrong CONFIG_DISP_LINE_BUFFER_NUMBER definition"
#endif

#define GPIO_PIN_DATA              GPIO_PIN_DATA_S

#define CONFIG_DISP_EBI            EBI_BANK0

/* ST1663I/FT5316 I2C */
#define CONFIG_INDEV_TOUCH_I2C        I2C1
#define CONFIG_INDEV_TOUCH_PIN_IRQ    NU_GET_PININDEX(evGF, 6)
#define CONFIG_INDEV_TOUCH_PIN_RESET  NU_GET_PININDEX(evGD, 10)
#define INDEV_TOUCH_SET_RST           GPIO_PIN_DATA(NU_GET_PORT(CONFIG_INDEV_TOUCH_PIN_RESET), NU_GET_PIN(CONFIG_INDEV_TOUCH_PIN_RESET)) = 0
#define INDEV_TOUCH_CLR_RST           GPIO_PIN_DATA(NU_GET_PORT(CONFIG_INDEV_TOUCH_PIN_RESET), NU_GET_PIN(CONFIG_INDEV_TOUCH_PIN_RESET)) = 1

int lcd_device_initialize(void);
int lcd_device_finalize(void);
int lcd_device_open(void);
void lcd_device_close(void);
int lcd_device_control(int cmd, void *argv);

int touchpad_device_initialize(void);
int touchpad_device_finalize(void);
int touchpad_device_open(void);
int touchpad_device_read(lv_indev_data_t *psInDevData);
void touchpad_device_close(void);
int touchpad_device_control(int cmd, void *argv);
void sysDelay(uint32_t ms);

#endif /* __LV_GLUE_H__ */
