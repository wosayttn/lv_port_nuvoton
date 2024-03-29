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

#define PORT_OFFSET                 0x40

/* Define off-screen line buffer number,  Range: 1~LV_VER_RES_MAX */
#define CONFIG_DISP_LINE_BUFFER_NUMBER  (LV_VER_RES_MAX)

#if (CONFIG_DISP_LINE_BUFFER_NUMBER < 1) || (CONFIG_DISP_LINE_BUFFER_NUMBER > LV_VER_RES_MAX)
    #error "Wrong CONFIG_DISP_LINE_BUFFER_NUMBER definition"
#endif

#if 1
    /* FSA506 EBI */
    #define CONFIG_FSA506_EBI              EBI_BANK0
    #define CONFIG_FSA506_EBI_USE_PDMA     1
    #define CONFIG_FSA506_EBI_ADDR         (EBI_BANK0_BASE_ADDR+(CONFIG_FSA506_EBI*EBI_MAX_SIZE))
    #define CONFIG_FSA506_PIN_BACKLIGHT    NU_GET_PININDEX(evGG, 5)   //101
    #define CONFIG_FSA506_PIN_DC           NU_GET_PININDEX(evGH, 7)   //119
    #define CONFIG_FSA506_PIN_RESET        NU_GET_PININDEX(evGG, 7)   //103

    /* ST1663I I2C */
    #define CONFIG_ST1663I_I2C             I2C1
    #define CONFIG_ST1663I_PIN_IRQ         NU_GET_PININDEX(evGG, 6)
    #define CONFIG_ST1663I_PIN_RESET       NU_GET_PININDEX(evGD, 10)

#else

    /* SSD1963 EBI */
    #define CONFIG_SSD1963_EBI              EBI_BANK0
    #define CONFIG_SSD1963_EBI_USE_PDMA     1
    #define CONFIG_SSD1963_EBI_ADDR         (EBI_BANK0_BASE_ADDR+(CONFIG_SSD1963_EBI*EBI_MAX_SIZE))
    #define CONFIG_SSD1963_PIN_BACKLIGHT    NU_GET_PININDEX(evGG, 5)   //101
    #define CONFIG_SSD1963_PIN_DC           NU_GET_PININDEX(evGH, 7)   //119
    #define CONFIG_SSD1963_PIN_RESET        NU_GET_PININDEX(evGG, 7)   //103
    #define CONFIG_SSD1963_PIN_DISPLAY      NU_GET_PININDEX(evGG, 8)

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
void systick_init(void);
void sysDelay(uint32_t ms);
uint32_t sysGetTicks(uint32_t no);

#endif /* __LV_GLUE_H__ */
