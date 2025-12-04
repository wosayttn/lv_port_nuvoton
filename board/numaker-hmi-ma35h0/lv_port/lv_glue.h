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
#include <stdint.h>
#include "MA35H0.h"
#include "lvgl.h"
#include "displib.h"
#include "adc.h"
#include "nu_misc.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#define CONFIG_LCD_FB_NUM               3
#define CONFIG_DISP_LINE_BUFFER_NUMBER  (LV_VER_RES_MAX)
#define PORT_OFFSET                     0x40

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
