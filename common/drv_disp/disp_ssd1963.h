/**************************************************************************//**
 * @file     disp_ssd1963.h
 * @brief    ili9431 display panel driver
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#ifndef __DISP_SSD1963_H__
#define __DISP_SSD1963_H__

#include <stdint.h>
#include "lv_glue.h"

#if defined(CONFIG_SSD1963_EBI)

    #define SET_RS               GPIO_PIN_DATA(NU_GET_PORT(CONFIG_SSD1963_PIN_DC), NU_GET_PIN(CONFIG_SSD1963_PIN_DC)) = 1
    #define CLR_RS               GPIO_PIN_DATA(NU_GET_PORT(CONFIG_SSD1963_PIN_DC), NU_GET_PIN(CONFIG_SSD1963_PIN_DC)) = 0
    #define SET_RST              GPIO_PIN_DATA(NU_GET_PORT(CONFIG_SSD1963_PIN_RESET), NU_GET_PIN(CONFIG_SSD1963_PIN_RESET)) = 1
    #define CLR_RST              GPIO_PIN_DATA(NU_GET_PORT(CONFIG_SSD1963_PIN_RESET), NU_GET_PIN(CONFIG_SSD1963_PIN_RESET)) = 0
    #define SET_BACKLIGHT_ON     GPIO_PIN_DATA(NU_GET_PORT(CONFIG_SSD1963_PIN_BACKLIGHT), NU_GET_PIN(CONFIG_SSD1963_PIN_BACKLIGHT)) = 1
    #define SET_BACKLIGHT_OFF    GPIO_PIN_DATA(NU_GET_PORT(CONFIG_SSD1963_PIN_BACKLIGHT), NU_GET_PIN(CONFIG_SSD1963_PIN_BACKLIGHT)) = 0
    #define SET_DISP_ON          GPIO_PIN_DATA(NU_GET_PORT(CONFIG_SSD1963_PIN_DISPLAY), NU_GET_PIN(CONFIG_SSD1963_PIN_DISPLAY)) = 1
    #define SET_DISP_OFF         GPIO_PIN_DATA(NU_GET_PORT(CONFIG_SSD1963_PIN_DISPLAY), NU_GET_PIN(CONFIG_SSD1963_PIN_DISPLAY)) = 0

    #define ssd1963_delay_ms(ms)     sysDelay(ms)

    void ssd1963_send_cmd(uint8_t cmd);
    void ssd1963_send_cmd_parameter(uint8_t data);
    void ssd1963_set_column(uint16_t StartCol, uint16_t EndCol);
    void ssd1963_set_page(uint16_t StartPage, uint16_t EndPage);
    void ssd1963_send_pixel_data(uint16_t color);
    void ssd1963_lcd_get_pixel(uint8_t *color, int x, int y);
    void ssd1963_send_pixels(uint16_t *pixels, int len);

    void ssd1963_fillrect(uint16_t *pixels, const lv_area_t *area);
    int disp_ssd1963_init(void);

#endif

#endif /* __DISP_SSD1963_H__ */
