/**************************************************************************//**
 * @file     disp.h
 * @brief    for lv display driver
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2024 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#ifndef __DISP_H__
#define __DISP_H__

#include <stdint.h>
#include "lv_glue.h"

#if defined(CONFIG_DISP_USE_PDMA)
    #include "drv_pdma.h"
#endif

#if defined(CONFIG_DISP_EBI)
    #define DISP_WRITE_REG(u16RegAddr)  *((volatile uint16_t *)CONFIG_DISP_CMD_ADDR) = u16RegAddr
    #define DISP_WRITE_DATA(u16Data)    *((volatile uint16_t *)CONFIG_DISP_DAT_ADDR) = u16Data
    #define DISP_READ_STATUS()          *((volatile uint16_t *)CONFIG_DISP_CMD_ADDR)
    #define DISP_READ_DATA()            *((volatile uint16_t *)CONFIG_DISP_DAT_ADDR)
#endif

void disp_write_reg(uint16_t reg, uint16_t data);
void disp_set_column(uint16_t StartCol, uint16_t EndCol);
void disp_set_page(uint16_t StartPage, uint16_t EndPage);
void disp_send_pixels(uint16_t *pixels, int byte_len);
void disp_fillrect(uint16_t *pixels, const lv_area_t *area);
int  disp_init(void);

#endif /* __DISP_H__ */
