/**************************************************************************//**
 * @file     lt7381_ebi.c
 * @brief    lt7381 ebi interface glue
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
}

void disp_set_column(uint16_t StartCol, uint16_t EndCol)
{
    uint16_t ActiveX = EndCol - StartCol + 1;

    disp_write_reg(0x56, StartCol);
    disp_write_reg(0x57, StartCol >> 8);
    disp_write_reg(0x5A, ActiveX);
    disp_write_reg(0x5B, ActiveX >> 8);
}

void disp_set_page(uint16_t StartPage, uint16_t EndPage)
{
    uint16_t ActiveY = EndPage - StartPage + 1;

    disp_write_reg(0x58, StartPage);
    disp_write_reg(0x59, StartPage >> 8);
    disp_write_reg(0x5C, ActiveY);
    disp_write_reg(0x5D, ActiveY >> 8);
}

/*
  0: Memory Write FIFO is not full.
  1: Memory Write FIFO is full.
*/
static uint32_t lt7381_vram_fifo_isfull(void)
{
    return ((DISP_READ_STATUS() & BIT7) != 0) ? 1 : 0;
}

/*
  0: Memory FIFO is not empty.
  1: Memory FIFO is empty.
*/
static uint32_t lt7381_vram_fifo_isempty(void)
{
    return ((DISP_READ_STATUS() & 0x40) != 0) ? 1 : 0;
}

void disp_send_pixels(uint16_t *pixels, int byte_len)
{
    int count = byte_len / sizeof(uint16_t);

    /* Set Graphic Read/Write position */
    disp_write_reg(0x5F, 0);
    disp_write_reg(0x60, 0);
    disp_write_reg(0x61, 0);
    disp_write_reg(0x62, 0);

    /* Memory Data Read/Write Port */
    DISP_WRITE_REG(0x04);

#if defined(CONFIG_DISP_USE_PDMA)
    // PDMA-M2M feed
    if (count > 1024)
    {
        nu_pdma_mempush((void *)CONFIG_DISP_DAT_ADDR, (void *)pixels, 16, count);
    }
    else
#endif
    {
        int i = 0;
        // Just support CPU-feed only
        while (i < count)
        {
            DISP_WRITE_DATA(pixels[i]);
            i++;
        }
    }

    while (!lt7381_vram_fifo_isempty());
}
