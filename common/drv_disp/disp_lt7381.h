/**************************************************************************//**
 * @file     disp_lt7381.h
 * @brief    lt7381 display panel driver
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#ifndef __DISP_LT7381_H__
#define __DISP_LT7381_H__

#include <stdint.h>
#include "lv_glue.h"

#if defined(CONFIG_LT7381_EBI)

#if defined(CONFIG_LT7381_PIN_DC)
    #define SET_RS                   GPIO_PIN_DATA(NU_GET_PORT(CONFIG_LT7381_PIN_DC), NU_GET_PIN(CONFIG_LT7381_PIN_DC)) = 1
    #define CLR_RS                   GPIO_PIN_DATA(NU_GET_PORT(CONFIG_LT7381_PIN_DC), NU_GET_PIN(CONFIG_LT7381_PIN_DC)) = 0

    #define LT7381_ADDR_CMD          0x0
    #define LT7381_ADDR_DATA         0x0

#else
    #define LT7381_ADDR_CMD          0x0
    /*ADR7*/
    //#define LT7381_ADDR_DATA         (BIT11|BIT10|BIT9|BIT8|BIT6|BIT5|BIT4|BIT3|BIT2|BIT1)
    #define LT7381_ADDR_DATA         (BIT1)
#endif

#define SET_RST                  GPIO_PIN_DATA(NU_GET_PORT(CONFIG_LT7381_PIN_RESET), NU_GET_PIN(CONFIG_LT7381_PIN_RESET)) = 1
#define CLR_RST                  GPIO_PIN_DATA(NU_GET_PORT(CONFIG_LT7381_PIN_RESET), NU_GET_PIN(CONFIG_LT7381_PIN_RESET)) = 0
#define SET_BACKLIGHT_ON         GPIO_PIN_DATA(NU_GET_PORT(CONFIG_LT7381_PIN_BACKLIGHT), NU_GET_PIN(CONFIG_LT7381_PIN_BACKLIGHT)) = 1
#define SET_BACKLIGHT_OFF        GPIO_PIN_DATA(NU_GET_PORT(CONFIG_LT7381_PIN_BACKLIGHT), NU_GET_PIN(CONFIG_LT7381_PIN_BACKLIGHT)) = 0
#define lt7381_delay_ms(ms)      sysDelay(ms)

#if !defined(CONFIG_LT7381_EBI_ADDR)
    #error "Please specify CONFIG_LT7381_EBI_ADDR in lv_conf.h"
#endif

#if !defined(CONFIG_LT7381_PIN_DC)
#define LT7381_WRITE_REG(u16RegAddr) \
        { \
            *((volatile uint16_t *)(CONFIG_LT7381_EBI_ADDR+LT7381_ADDR_CMD)) = u16RegAddr; \
            __DSB(); \
        }
#define LT7381_WRITE_DATA(u16Data) \
        { \
            *((volatile uint16_t *)(CONFIG_LT7381_EBI_ADDR+LT7381_ADDR_DATA)) = u16Data; \
            __DSB(); \
        }
#define LT7381_READ_STATUS()   *((volatile uint16_t *)(CONFIG_LT7381_EBI_ADDR+LT7381_ADDR_CMD))
#define LT7381_READ_DATA()     *((volatile uint16_t *)(CONFIG_LT7381_EBI_ADDR+LT7381_ADDR_DATA))

#else
#endif

void lt7381_write_reg(uint16_t reg, uint16_t data);
void lt7381_set_column(uint16_t StartCol, uint16_t EndCol);
void lt7381_set_page(uint16_t StartPage, uint16_t EndPage);
void lt7381_send_pixels(uint16_t *pixels, int byte_len);
void lt7381_fillrect(uint16_t *pixels, const lv_area_t *area);

int disp_lt7381_init(void);

#endif

#endif /* __DISP_LT7381_H__ */
