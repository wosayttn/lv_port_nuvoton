/**************************************************************************//**
 * @file     lt7381_ebi.c
 * @brief    lt7381 ebi interface glue
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "disp_lt7381.h"

void lt7381_write_reg(uint16_t reg, uint16_t data)
{
    // Register
    LT7381_WRITE_REG(reg & 0xFF);

    // Data
    LT7381_WRITE_DATA(data & 0xFF);
}

void lt7381_set_column(uint16_t StartCol, uint16_t EndCol)
{
    uint16_t ActiveX = EndCol - StartCol + 1;

    lt7381_write_reg(0x56, StartCol);
    lt7381_write_reg(0x57, StartCol >> 8);
    lt7381_write_reg(0x5A, ActiveX);
    lt7381_write_reg(0x5B, ActiveX >> 8);
}

void lt7381_set_page(uint16_t StartPage, uint16_t EndPage)
{
    uint16_t ActiveY = EndPage - StartPage + 1;

    lt7381_write_reg(0x58, StartPage);
    lt7381_write_reg(0x59, StartPage >> 8);
    lt7381_write_reg(0x5C, ActiveY);
    lt7381_write_reg(0x5D, ActiveY >> 8);
}

/*
  0: Memory Write FIFO is not full.
  1: Memory Write FIFO is full.
*/
static uint32_t lt7381_vram_fifo_isfull(void)
{
    return ((LT7381_READ_STATUS() & BIT7) != 0) ? 1 : 0;
}

/*
  0: Memory FIFO is not empty.
  1: Memory FIFO is empty.
*/
static uint32_t lt7381_vram_fifo_isempty(void)
{
    return ((LT7381_READ_STATUS() & 0x40) != 0) ? 1 : 0;
}

void lt7381_send_pixels(uint16_t *pixels, int byte_len)
{
    static int bInit = 0;
    int i = 0;
    int count = byte_len / sizeof(uint16_t);

    if (!bInit)
    {

        /* REG[03h] Input Control Register（ICR）*/
        /* Select Graphics mode. */
        LT7381_WRITE_REG(0x03);
        LT7381_WRITE_DATA(LT7381_READ_STATUS() & ~BIT2);

        /* Set Graphic Read/Write position */
        lt7381_write_reg(0x5F, 0);
        lt7381_write_reg(0x60, 0);
        lt7381_write_reg(0x61, 0);
        lt7381_write_reg(0x62, 0);

        bInit = 1;
    }

    LT7381_WRITE_REG(0x04);

    // Just support CPU-feed only
    while (i < count)
    {
        if ((i % 32) == 0)
        {
            while (lt7381_vram_fifo_isfull())
                printf("full!!!\n");
        }

        LT7381_WRITE_DATA(pixels[i]);
        i++;
    }

    while (!lt7381_vram_fifo_isempty());
}
