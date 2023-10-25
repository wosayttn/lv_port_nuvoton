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
#include "lvgl.h"
#include "sys.h"
#include "lcd.h"
#include "adc.h"
#include "nu_misc.h"

#if CONFIG_LV_DISP_FULL_REFRESH
    #define CONFIG_LCD_FB_NUM            3
#else
    #define CONFIG_LCD_FB_NUM            2
#endif
#define CONFIG_DISP_LINE_BUFFER_NUMBER   (CONFIG_LCD_FB_NUM * LV_VER_RES_MAX)

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

#endif /* __LV_GLUE_H__ */
