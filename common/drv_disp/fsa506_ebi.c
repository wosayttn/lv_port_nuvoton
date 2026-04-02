/**************************************************************************//**
 * @file     fsa506_ebi.c
 * @brief    fsa506 ebi interface glue
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "numaker_disp.h"

/**
 * @brief Write a register value to FSA506 display controller via EBI.
 *
 * Sends an 8-bit register address and 8-bit data value to the FSA506
 * display controller through the EBI (External Bus Interface).
 *
 * @param reg[in]   Register address (8-bit)
 * @param data[in]  Register data value (8-bit)
 */
void disp_write_reg(uint16_t reg, uint16_t data)
{
    /* Write register address (8-bit) */
    DISP_WRITE_REG(reg & 0xFF);

    /* Write register data value (8-bit) */
    DISP_WRITE_DATA(data & 0xFF);

    /* Signal write completion */
    DISP_WRITE_REG(0x80);
}

/**
 * @brief Set horizontal display column address window for FSA506.
 *
 * Configures the start and end column addresses for the display window,
 * effectively setting the left and right boundaries of the drawing area.
 *
 * @param StartCol[in]  Starting column address
 * @param EndCol[in]    Ending column address
 */
void disp_set_column(uint16_t StartCol, uint16_t EndCol)
{
    /* Write start column high byte */
    disp_write_reg(0x0, (StartCol >> 8) & 0xFF);
    /* Write start column low byte */
    disp_write_reg(0x1, StartCol & 0xFF);
    /* Write end column high byte */
    disp_write_reg(0x2, (EndCol >> 8) & 0xFF);
    /* Write end column low byte */
    disp_write_reg(0x3, EndCol & 0xFF);
}

/**
 * @brief Set vertical display row address window for FSA506.
 *
 * Configures the start and end row addresses for the display window,
 * effectively setting the top and bottom boundaries of the drawing area.
 *
 * @param StartPage[in]  Starting row/page address
 * @param EndPage[in]    Ending row/page address
 */
void disp_set_page(uint16_t StartPage, uint16_t EndPage)
{
    /* Write start page high byte */
    disp_write_reg(0x4, (StartPage >> 8) & 0xFF);
    /* Write start page low byte */
    disp_write_reg(0x5, StartPage & 0xFF);
    /* Write end page high byte */
    disp_write_reg(0x6, (EndPage >> 8) & 0xFF);
    /* Write end page low byte */
    disp_write_reg(0x7, EndPage & 0xFF);
}

/**
 * @brief Send pixel data to FSA506 display RAM via EBI.
 *
 * Transfers pixel color data to the display controller's memory. If PDMA
 * (Programmable DMA) is enabled, uses DMA for efficient bulk transfers.
 * Otherwise uses CPU-driven memory pushes.
 *
 * @param pixels[in]    Pointer to pixel color data buffer
 * @param byte_len[in]  Length of data in bytes
 */
void disp_send_pixels(uint16_t *pixels, int byte_len)
{
    /* Calculate number of 16-bit words to transfer */
    int count = byte_len / sizeof(uint16_t);

    /* Signal pixel data write operation to controller */
    DISP_WRITE_REG(0xC1);

    /* Use DMA if available for efficient transfer of large data */
#if defined(CONFIG_DISP_USE_PDMA)
    /* Use PDMA for memory-to-memory transfer if data is large enough */
    if (count > 512)
    {
        /* Initiate PDMA transfer to display data register */
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

void disp_receive_pixels(uint16_t *pixels, int byte_len)
{
    int count = byte_len / sizeof(uint16_t);
    volatile uint16_t dummy;

    DISP_WRITE_REG(0xC1);

    /* Must do a dummy read to trigger read task. */
    dummy = DISP_READ_DATA();

#if defined(CONFIG_DISP_USE_PDMA)
    // PDMA-M2M feed
    if (count > 512)
    {
        nu_pdma_mempull((void *)pixels, (void *)CONFIG_DISP_DAT_ADDR, 16, count);
    }
    else
#endif
    {
        // CPU feed
        int i = 0;
        while (i < count)
        {
            pixels[i] = DISP_READ_DATA();
            i++;
        }
    }

    DISP_WRITE_REG(0x80);
}
