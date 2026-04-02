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

/**
 * @brief Write LT7381 register with data value.
 *
 * Sends register address and 8-bit data value separately for LT7381
 * display controller configuration.
 *
 * @param reg[in]   Register address (8-bit)
 * @param data[in]  Data value to write (8-bit)
 */
void disp_write_reg(uint16_t reg, uint16_t data)
{
    /* Send register address */
    DISP_WRITE_REG(reg & 0xFF);

    /* Send data value */
    DISP_WRITE_DATA(data & 0xFF);
}

/**
 * @brief Read LT7381 register value.
 *
 * Sends register address and reads back the 8-bit data value.
 *
 * @param reg[in]  Register address (8-bit)
 * @return         Register value read from display controller
 */
uint16_t disp_read_reg(uint8_t reg)
{
    volatile uint16_t data = 0xFFFFu;

    /* Send register address to select which register to read */
    DISP_WRITE_REG(reg & 0xFF);

    /* Read data from the selected register */
    data = DISP_READ_DATA();

    return data;
}

/**
 * @brief Set column address window for LT7381.
 *
 * Configures the horizontal window using LT7381-specific registers
 * (0x56/0x57 for start column, 0x5A/0x5B for active width).
 *
 * @param StartCol[in]  Starting column address
 * @param EndCol[in]    Ending column address
 */
void disp_set_column(uint16_t StartCol, uint16_t EndCol)
{
    uint16_t ActiveX = EndCol - StartCol + 1;

    /* Set column start address (split into low/high bytes) */
    disp_write_reg(0x56, StartCol);
    disp_write_reg(0x57, StartCol >> 8);
    /* Set active column width (split into low/high bytes) */
    disp_write_reg(0x5A, ActiveX);
    disp_write_reg(0x5B, ActiveX >> 8);
}

/**
 * @brief Set row/page address window for LT7381.
 *
 * Configures the vertical window using LT7381-specific registers
 * (0x58/0x59 for start row, 0x5C/0x5D for active height).
 *
 * @param StartPage[in]  Starting row address
 * @param EndPage[in]    Ending row address
 */
void disp_set_page(uint16_t StartPage, uint16_t EndPage)
{
    uint16_t ActiveY = EndPage - StartPage + 1;

    /* Set row start address (split into low/high bytes) */
    disp_write_reg(0x58, StartPage);
    disp_write_reg(0x59, StartPage >> 8);
    /* Set active row height (split into low/high bytes) */
    disp_write_reg(0x5C, ActiveY);
    disp_write_reg(0x5D, ActiveY >> 8);
}

/**
 * @brief Send pixel data to LT7381 display VRAM.
 *
 * Writes 16-bit pixel data to the display controller's video RAM.
 * Respects FIFO status to avoid overflow.
 *
 * @param pixels[in]    Pointer to pixel data array (16-bit RGB565)
 * @param byte_len[in]  Total length of data in bytes
 */
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
