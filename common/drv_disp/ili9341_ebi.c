/**************************************************************************//**
 * @file     ili9341_ebi.c
 * @brief    ili9431 ebi interface glue
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "disp_ili9341.h"
#if defined(CONFIG_ILI9341_EBI_USE_PDMA)
    #include "drv_pdma.h"
#endif

#if !defined(CONFIG_ILI9341_EBI_ADDR)
    #error "Please specify CONFIG_ILI9341_EBI_ADDR in lv_conf.h"
#endif

#define ILI9341_ADDR_CMD  0x0
#define ILI9341_ADDR_DATA 0x0030000

#define ILI9341_WRITE_REG(u32RegAddr)   (*((volatile uint16_t *)(CONFIG_ILI9341_EBI_ADDR+(ILI9341_ADDR_CMD))) = (u32RegAddr))
#define ILI9341_WRITE_DATA(u32Data)     (*((volatile uint16_t *)(CONFIG_ILI9341_EBI_ADDR+(ILI9341_ADDR_DATA))) = (u32Data))

void ili9341_send_cmd(uint8_t cmd)
{
    CLR_RS;
    ILI9341_WRITE_REG(cmd);
    SET_RS;
}

void ili9341_send_cmd_parameter(uint8_t data)
{
    ILI9341_WRITE_DATA(data);
}

void ili9341_send_pixels(uint16_t *pixels, int byte_len)
{
    int count = byte_len / sizeof(uint16_t);
#if defined(CONFIG_ILI9341_EBI_USE_PDMA)
    // PDMA-M2M feed
    if (count > 1024)
        nu_pdma_mempush((void *)(CONFIG_ILI9341_EBI_ADDR + (ILI9341_ADDR_DATA)), (void *)pixels, 16, count);
    else
#endif
    {
        // CPU feed
        int i = 0;
        while (i < count)
        {
            ILI9341_WRITE_DATA(pixels[i]);
            i++;
        }
    }
}

void ili9341_set_column(uint16_t StartCol, uint16_t EndCol)
{
    ili9341_send_cmd(0x2A);
    ILI9341_WRITE_DATA((StartCol >> 8) & 0xFF);
    ILI9341_WRITE_DATA(StartCol & 0xFF);
    ILI9341_WRITE_DATA((EndCol >> 8) & 0xFF);
    ILI9341_WRITE_DATA(EndCol & 0xFF);
}

void ili9341_set_page(uint16_t StartPage, uint16_t EndPage)
{
    ili9341_send_cmd(0x2B);
    ILI9341_WRITE_DATA((StartPage >> 8) & 0xFF);
    ILI9341_WRITE_DATA(StartPage & 0xFF);
    ILI9341_WRITE_DATA((EndPage >> 8) & 0xFF);
    ILI9341_WRITE_DATA(EndPage & 0xFF);
}
