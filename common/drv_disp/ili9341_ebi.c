/**************************************************************************//**
 * @file     ili9341_ebi.c
 * @brief    ili9431 ebi interface glue
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2024 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "disp.h"

void disp_set_column(uint16_t StartCol, uint16_t EndCol)
{
    DISP_WRITE_REG(0x2A);

    DISP_WRITE_DATA((StartCol >> 8) & 0xFF);
    DISP_WRITE_DATA(StartCol & 0xFF);
    DISP_WRITE_DATA((EndCol >> 8) & 0xFF);
    DISP_WRITE_DATA(EndCol & 0xFF);
}

void disp_set_page(uint16_t StartPage, uint16_t EndPage)
{
    DISP_WRITE_REG(0x2B);

    DISP_WRITE_DATA((StartPage >> 8) & 0xFF);
    DISP_WRITE_DATA(StartPage & 0xFF);
    DISP_WRITE_DATA((EndPage >> 8) & 0xFF);
    DISP_WRITE_DATA(EndPage & 0xFF);
}

void disp_send_pixels(uint16_t *pixels, int byte_len)
{
    int count = byte_len / sizeof(uint16_t);
#if defined(CONFIG_DISP_USE_PDMA)
    // PDMA-M2M feed
    if (count > 1024)
        nu_pdma_mempush((void *)CONFIG_DISP_DAT_ADDR, (void *)pixels, 16, count);
    else
#endif
    {
        // CPU feed
        int i = 0;
        while (i < count)
        {
            DISP_WRITE_DATA(pixels[i]);
            i++;
        }
    }
}
