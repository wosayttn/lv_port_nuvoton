/**************************************************************************//**
 * @file     lt7381_ebi.c
 * @brief    lt7381 ebi interface glue
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "numaker_disp.h"

/*
  0: Memory Write FIFO is not full.
  1: Memory Write FIFO is full.
*/
#define LT7381_VRAM_WF_ISFULL()    (DISP_READ_STATUS() & BIT7)

/*
  0: Memory Write FIFO is not empty.
  1: Memory Write FIFO is empty.
*/
#define LT7381_VRAM_WF_ISEMPTY()   (DISP_READ_STATUS() & BIT6)

/*
  0: Memory Read FIFO is not full.
  1: Memory Read FIFO is full.
*/
#define LT7381_VRAM_RF_ISFULL()    (DISP_READ_STATUS() & BIT5)

/*
  0: Memory Read FIFO is not empty.
  1: Memory Read FIFO is empty.
*/
#define LT7381_VRAM_RF_ISEMPTY()   (DISP_READ_STATUS() & BIT4)

void disp_write_reg(uint16_t reg, uint16_t data)
{
    // Register
    DISP_WRITE_REG(reg & 0xFF);

    // Data
    DISP_WRITE_DATA(data & 0xFF);
}

uint16_t disp_read_reg(uint8_t reg)
{
    volatile uint16_t data = 0xFFFFu;

    // Register
    DISP_WRITE_REG(reg & 0xFF);

    // Data
    data = DISP_READ_DATA();

    return data;
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


void disp_send_pixels(uint16_t *pixels, int byte_len)
{
    int count = byte_len / sizeof(uint16_t);

    while (!LT7381_VRAM_WF_ISEMPTY()) {}

#if defined(CONFIG_DISP_USE_PDMA)
    // PDMA-M2M feed
    /* FIXME */
    /* Cannot do LT7381_VRAM_WF_ISFULL during PDMA transferring. So, it need slow down timing */
    if (count > 512)
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
            /* Check VRAM FIFO is full or not. */
            while (LT7381_VRAM_WF_ISFULL());
            DISP_WRITE_DATA(pixels[i]);

            i++;
        }
    }

    while (!LT7381_VRAM_WF_ISEMPTY());
}

void disp_receive_pixels(uint16_t *pixels, int byte_len)
{
    int count = byte_len / sizeof(uint16_t);
    volatile uint16_t dummy;

    /* Must do a dummy read to trigger read task. */
    dummy = DISP_READ_DATA();

    while (!LT7381_VRAM_RF_ISFULL()) {}

#if defined(CONFIG_DISP_USE_PDMA)
    if (count > 512)
    {
        nu_pdma_mempull((void *)pixels, (void *)CONFIG_DISP_DAT_ADDR, 16, count);
    }
    else
#endif
    {
        int i = 0;

        while (i < count)
        {
            pixels[i] = DISP_READ_DATA();
            __DSB();

            i++;
        }
    }
}
