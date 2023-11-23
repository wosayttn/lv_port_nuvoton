/**************************************************************************//**
 * @file     disp_fsa506.h
 * @brief    ili9431 display panel driver
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#ifndef __DISP_FSA506_H__
#define __DISP_FSA506_H__

#include <stdint.h>
#include "lv_glue.h"

#if defined(CONFIG_FSA506_EBI)

#define SET_RS                   GPIO_PIN_DATA(NU_GET_PORT(CONFIG_FSA506_PIN_DC), NU_GET_PIN(CONFIG_FSA506_PIN_DC)) = 1
#define CLR_RS                   GPIO_PIN_DATA(NU_GET_PORT(CONFIG_FSA506_PIN_DC), NU_GET_PIN(CONFIG_FSA506_PIN_DC)) = 0
#define SET_RST                  GPIO_PIN_DATA(NU_GET_PORT(CONFIG_FSA506_PIN_RESET), NU_GET_PIN(CONFIG_FSA506_PIN_RESET)) = 1
#define CLR_RST                  GPIO_PIN_DATA(NU_GET_PORT(CONFIG_FSA506_PIN_RESET), NU_GET_PIN(CONFIG_FSA506_PIN_RESET)) = 0
#define SET_BACKLIGHT_ON         GPIO_PIN_DATA(NU_GET_PORT(CONFIG_FSA506_PIN_BACKLIGHT), NU_GET_PIN(CONFIG_FSA506_PIN_BACKLIGHT)) = 1
#define SET_BACKLIGHT_OFF        GPIO_PIN_DATA(NU_GET_PORT(CONFIG_FSA506_PIN_BACKLIGHT), NU_GET_PIN(CONFIG_FSA506_PIN_BACKLIGHT)) = 0
#define fsa506_delay_ms(ms)      sysDelay(ms)

void fsa506_write_reg(uint16_t reg, uint16_t data);
void fsa506_set_column(uint16_t StartCol, uint16_t EndCol);
void fsa506_set_page(uint16_t StartPage, uint16_t EndPage);
void fsa506_send_pixels(uint16_t *pixels, int byte_len);
void fsa506_fillrect(uint16_t *pixels, const lv_area_t *area);

int disp_fsa506_init(void);

#endif

#endif /* __DISP_FSA506_H__ */
