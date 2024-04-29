/**************************************************************************//**
 * @file     fsa506_ebi.c
 * @brief    fsa506 ebi interface glue
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "disp.h"

void disp_write_reg(uint16_t reg, uint16_t data)
{
    // Register
    DISP_WRITE_REG(reg & 0xFF);

    // Data
    DISP_WRITE_DATA(data & 0xFF);

    // Done
    DISP_WRITE_REG(0x80);
}

void disp_set_column(uint16_t StartCol, uint16_t EndCol)
{
    disp_write_reg(0x0, (StartCol >> 8) & 0xFF);
    disp_write_reg(0x1, StartCol & 0xFF);
    disp_write_reg(0x2, (EndCol >> 8) & 0xFF);
    disp_write_reg(0x3, EndCol & 0xFF);
}

void disp_set_page(uint16_t StartPage, uint16_t EndPage)
{
    disp_write_reg(0x4, (StartPage >> 8) & 0xFF);
    disp_write_reg(0x5, StartPage & 0xFF);
    disp_write_reg(0x6, (EndPage >> 8) & 0xFF);
    disp_write_reg(0x7, EndPage & 0xFF);
}

void disp_send_pixels(uint16_t *pixels, int byte_len)
{
    int count = byte_len / sizeof(uint16_t);

    DISP_WRITE_REG(0xC1);

#if defined(CONFIG_DISP_USE_PDMA)
    // PDMA-M2M feed
    if (count > 1024)
    {
        nu_pdma_mempush((void *)CONFIG_DISP_DAT_ADDR, (void *)pixels, 16, count);
    }
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

    DISP_WRITE_REG(0x80);
}
