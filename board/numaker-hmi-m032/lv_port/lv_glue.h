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
#include "nu_bitutil.h"
#include "nu_pin.h"

#define NVT_ALIGN(size, align)         (((size) + (align) - 1) & ~((align) - 1))
#define NVT_ALIGN_DOWN(size, align)    ((size) & ~((align) - 1))

#define CONFIG_TICK_PER_SECOND         1000

/* Define off-screen line buffer number,  Range: 1~LV_VER_RES_MAX */
#define CONFIG_DISP_LINE_BUFFER_NUMBER  24

#if (CONFIG_DISP_LINE_BUFFER_NUMBER < 1) || (CONFIG_DISP_LINE_BUFFER_NUMBER > LV_VER_RES_MAX)
    #error "Wrong CONFIG_DISP_LINE_BUFFER_NUMBER definition"
#endif

#define GPIOA_BASE                      PA_BASE
#define PORT_OFFSET                     0x40

/* ILI9341 SPI */
#define CONFIG_ILI9341_USPI             USPI0
#define CONFIG_ILI9341_USPI_CLOCK       24000000
#define CONFIG_ILI9341_USPI_SS_PIN      NU_GET_PININDEX(evGA, 8)
#define CONFIG_ILI9341_PIN_BACKLIGHT    NU_GET_PININDEX(evGA, 6)  //6
#define CONFIG_ILI9341_PIN_DC           NU_GET_PININDEX(evGB, 2)  //18
#define CONFIG_ILI9341_PIN_RESET        NU_GET_PININDEX(evGB, 3)  //19

/* SW ADC PINs */
#define CONFIG_AD                       ADC
#define CONFIG_AD_PIN_XL                NU_GET_PININDEX(evGB, 6)
#define CONFIG_AD_PIN_YU                NU_GET_PININDEX(evGB, 7)
#define CONFIG_AD_PIN_XR                NU_GET_PININDEX(evGB, 4)
#define CONFIG_AD_PIN_YD                NU_GET_PININDEX(evGB, 5)

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
    evLCD_CTRL_RECT_UPDATE,
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
void systick_init(void);
void sysDelay(uint32_t ms);
uint32_t sysGetCurrentTime(void);

#endif /* __LV_GLUE_H__ */
