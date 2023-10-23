/**************************************************************************//**
 * @file     disp_ili9341.h
 * @brief    ili9431 display panel driver
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#ifndef __DISP_ILI9341_H__
#define __DISP_ILI9341_H__

#include <stdint.h>
#include "lv_glue.h"

#define SET_RS                   GPIO_PIN_DATA(NU_GET_PORT(CONFIG_ILI9341_PIN_DC), NU_GET_PIN(CONFIG_ILI9341_PIN_DC)) = 1
#define CLR_RS                   GPIO_PIN_DATA(NU_GET_PORT(CONFIG_ILI9341_PIN_DC), NU_GET_PIN(CONFIG_ILI9341_PIN_DC)) = 0
#define SET_RST                  GPIO_PIN_DATA(NU_GET_PORT(CONFIG_ILI9341_PIN_RESET), NU_GET_PIN(CONFIG_ILI9341_PIN_RESET)) = 1
#define CLR_RST                  GPIO_PIN_DATA(NU_GET_PORT(CONFIG_ILI9341_PIN_RESET), NU_GET_PIN(CONFIG_ILI9341_PIN_RESET)) = 0
#define SET_BACKLIGHT_ON         GPIO_PIN_DATA(NU_GET_PORT(CONFIG_ILI9341_PIN_BACKLIGHT), NU_GET_PIN(CONFIG_ILI9341_PIN_BACKLIGHT)) = 1
#define SET_BACKLIGHT_OFF        GPIO_PIN_DATA(NU_GET_PORT(CONFIG_ILI9341_PIN_BACKLIGHT), NU_GET_PIN(CONFIG_ILI9341_PIN_BACKLIGHT)) = 0
#define ili9341_delay_ms(ms)     sysDelay(ms)

void ili9341_send_cmd(uint8_t cmd);
void ili9341_send_cmd_parameter(uint8_t data);
void ili9341_set_column(uint16_t StartCol, uint16_t EndCol);
void ili9341_set_page(uint16_t StartPage, uint16_t EndPage);
void ili9341_send_pixels(uint16_t *pixels, int byte_len);
void ili9341_fillrect(uint16_t *pixels, const lv_area_t *area);

int disp_ili9341_init(void);

#endif /* __DISP_ILI9341_H__ */
