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

#define PORT_OFFSET                 0x40

/* Define off-screen line buffer number,  Range: 1~LV_VER_RES_MAX */
#define CONFIG_DISP_LINE_BUFFER_NUMBER  (LV_VER_RES_MAX/2)

#if (CONFIG_DISP_LINE_BUFFER_NUMBER < 1) || (CONFIG_DISP_LINE_BUFFER_NUMBER > LV_VER_RES_MAX)
    #error "Wrong CONFIG_DISP_LINE_BUFFER_NUMBER definition"
#endif

/* Alias */
#define GPIO_PIN_DATA                GPIO_PIN_DATA_S

/* ILI9341 SPI */
#define CONFIG_DISP_SPI              USPI0
#define CONFIG_DISP_SPI_CLOCK        48000000
#define CONFIG_DISP_SPI_SS_PIN       0
#define CONFIG_DISP_USE_PDMA         1

#if defined(CONFIG_DISP_USE_PDMA)
    #define CONFIG_PDMA_SPI_TX       PDMA_USCI0_TX
    #define CONFIG_PDMA_SPI_RX       PDMA_USCI0_RX
    #define CONFIG_SPI_USE_PDMA
#endif

#define CONFIG_DISP_PIN_DC           NU_GET_PININDEX(evGB, 2)
#define CONFIG_DISP_PIN_RESET        NU_GET_PININDEX(evGB, 3)
#define CONFIG_DISP_PIN_BACKLIGHT    NU_GET_PININDEX(evGA, 6)
#define DISP_SET_RS                  GPIO_PIN_DATA(NU_GET_PORT(CONFIG_DISP_PIN_DC),        NU_GET_PIN(CONFIG_DISP_PIN_DC)) = 1
#define DISP_CLR_RS                  GPIO_PIN_DATA(NU_GET_PORT(CONFIG_DISP_PIN_DC),        NU_GET_PIN(CONFIG_DISP_PIN_DC)) = 0
#define DISP_SET_RST                 GPIO_PIN_DATA(NU_GET_PORT(CONFIG_DISP_PIN_RESET),     NU_GET_PIN(CONFIG_DISP_PIN_RESET)) = 1
#define DISP_CLR_RST                 GPIO_PIN_DATA(NU_GET_PORT(CONFIG_DISP_PIN_RESET),     NU_GET_PIN(CONFIG_DISP_PIN_RESET)) = 0
#define DISP_SET_BACKLIGHT           GPIO_PIN_DATA(NU_GET_PORT(CONFIG_DISP_PIN_BACKLIGHT), NU_GET_PIN(CONFIG_DISP_PIN_BACKLIGHT)) = 1
#define DISP_CLR_BACKLIGHT           GPIO_PIN_DATA(NU_GET_PORT(CONFIG_DISP_PIN_BACKLIGHT), NU_GET_PIN(CONFIG_DISP_PIN_BACKLIGHT)) = 0


/* SW ADC PINs */
#define CONFIG_AD                    EADC_S
#define CONFIG_AD_PIN_XL             NU_GET_PININDEX(evGB, 6)
#define CONFIG_AD_PIN_YU             NU_GET_PININDEX(evGB, 7)
#define CONFIG_AD_PIN_XR             NU_GET_PININDEX(evGB, 4)
#define CONFIG_AD_PIN_YD             NU_GET_PININDEX(evGB, 5)

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
