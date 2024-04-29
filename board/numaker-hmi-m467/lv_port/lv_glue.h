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

/* FSA506/SSD1963 EBI */
#define CONFIG_DISP_EBI            EBI_BANK0
#define CONFIG_DISP_USE_PDMA
#define CONFIG_DISP_EBI_ADDR       (EBI_BANK0_BASE_ADDR+(CONFIG_DISP_EBI*EBI_MAX_SIZE))
#define CONFIG_DISP_CMD_ADDR       (CONFIG_DISP_EBI_ADDR+0x0)
#define CONFIG_DISP_DAT_ADDR       (CONFIG_DISP_EBI_ADDR+0x2)
#define CONFIG_DISP_PIN_BACKLIGHT  NU_GET_PININDEX(evGG, 5)   //101
#define CONFIG_DISP_PIN_RESET      NU_GET_PININDEX(evGG, 7)   //103
#define DISP_SET_RST               GPIO_PIN_DATA(NU_GET_PORT(CONFIG_DISP_PIN_RESET),     NU_GET_PIN(CONFIG_DISP_PIN_RESET)) = 1
#define DISP_CLR_RST               GPIO_PIN_DATA(NU_GET_PORT(CONFIG_DISP_PIN_RESET),     NU_GET_PIN(CONFIG_DISP_PIN_RESET)) = 0
#define DISP_SET_BACKLIGHT         GPIO_PIN_DATA(NU_GET_PORT(CONFIG_DISP_PIN_BACKLIGHT), NU_GET_PIN(CONFIG_DISP_PIN_BACKLIGHT)) = 1
#define DISP_CLR_BACKLIGHT         GPIO_PIN_DATA(NU_GET_PORT(CONFIG_DISP_PIN_BACKLIGHT), NU_GET_PIN(CONFIG_DISP_PIN_BACKLIGHT)) = 0

/* ST1663I I2C */
#define CONFIG_INDEV_TOUCH_I2C        I2C1
#define CONFIG_INDEV_TOUCH_PIN_IRQ    NU_GET_PININDEX(evGG, 6)
#define CONFIG_INDEV_TOUCH_PIN_RESET  NU_GET_PININDEX(evGD, 10)
#define INDEV_TOUCH_SET_RST           GPIO_PIN_DATA(NU_GET_PORT(CONFIG_INDEV_TOUCH_PIN_RESET), NU_GET_PIN(CONFIG_INDEV_TOUCH_PIN_RESET)) = 0
#define INDEV_TOUCH_CLR_RST           GPIO_PIN_DATA(NU_GET_PORT(CONFIG_INDEV_TOUCH_PIN_RESET), NU_GET_PIN(CONFIG_INDEV_TOUCH_PIN_RESET)) = 1

#define disp_delay_ms(ms)             sysDelay(ms)

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
