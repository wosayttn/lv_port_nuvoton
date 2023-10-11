/**************************************************************************//**
 * @file     lv_glue.c
 * @brief    lvgl glue header
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#ifndef __LV_GLUE_H__
#define __LV_GLUE_H__

#include "lvgl.h"
#include "displib.h"
#include "adc.h"

#define NVT_ALIGN(size, align)      (((size) + (align) - 1) & ~((align) - 1))
#define NVT_ALIGN_DOWN(size, align) ((size) & ~((align) - 1))

#define CONFIG_TICK_PER_SECOND      1000

typedef struct
{
    int32_t   a;
    int32_t   b;
    int32_t   c;
    int32_t   d;
    int32_t   e;
    int32_t   f;
    int32_t   div;
} S_CALIBRATION_MATRIX;

typedef struct
{
    void *pvVramStartAddr;  // VRAM Start address

    uint32_t u32VramSize;   // VRAM total size in byte

    uint32_t u32ResWidth;   // Resolution - Width

    uint32_t u32ResHeight;  // Resolution - Height

    uint32_t u32BytePerPixel;  // Byte per Pixel

} S_LCD_INFO;

typedef enum
{
    evLCD_CTRL_GET_INFO,
    evLCD_CTRL_PAN_DISPLAY,
    evLCD_CTRL_WAIT_VSYNC,
    evLCD_CTRL_CNT
} E_LCD_CTRL;

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
