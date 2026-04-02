/**************************************************************************//**
 * @file     nv3041a_ebi.c
 * @brief    nv3041a ebi interface glue
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2025 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "numaker_disp.h"

/**
 * @brief Write NV3041A register with data value.
 *
 * Sends register address and 8-bit data value separately for NV3041A
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
 * @brief Set column address window for NV3041A.
 *
 * Sends column set command (0x2A) followed by start and end column coordinates
 * in big-endian 16-bit format.
 *
 * @param StartCol[in]  Starting column address
 * @param EndCol[in]    Ending column address
 */
void disp_set_column(uint16_t StartCol, uint16_t EndCol)
{
    DISP_WRITE_REG(0x2A);

    /* Send column address range in big-endian format */
    DISP_WRITE_DATA((StartCol >> 8) & 0xFF);
    DISP_WRITE_DATA(StartCol & 0xFF);
    DISP_WRITE_DATA((EndCol >> 8) & 0xFF);
    DISP_WRITE_DATA(EndCol & 0xFF);
}

/**
 * @brief Set row/page address window for NV3041A.
 *
 * Sends row address set command (0x2B) followed by start and end row coordinates
 * in big-endian 16-bit format.
 *
 * @param StartPage[in]  Starting row address
 * @param EndPage[in]    Ending row address
 */
void disp_set_page(uint16_t StartPage, uint16_t EndPage)
{
    DISP_WRITE_REG(0x2B);

    /* Send row address range in big-endian format */
    DISP_WRITE_DATA((StartPage >> 8) & 0xFF);
    DISP_WRITE_DATA(StartPage & 0xFF);
    DISP_WRITE_DATA((EndPage >> 8) & 0xFF);
    DISP_WRITE_DATA(EndPage & 0xFF);
}

/**
 * @brief Send pixel data to NV3041A display VRAM.
 *
 * Writes 16-bit pixel data to the display controller's video RAM.
 * Uses PDMA for large transfers (>512 pixels) if available.
 *
 * @param pixels[in]    Pointer to pixel data array (16-bit RGB565)
 * @param byte_len[in]  Total length of data in bytes
 */
void disp_send_pixels(uint16_t *pixels, int byte_len)
{
    int count = byte_len / sizeof(uint16_t);

#if defined(CONFIG_DISP_USE_PDMA)
    /* Use PDMA for efficient large data transfers */
    if (count > 512)
        nu_pdma_mempush((void *)CONFIG_DISP_DAT_ADDR, (void *)pixels, 16, count);
    else
#endif
    {
        /* Fall back to CPU-driven pixel writes for smaller transfers */
        int i = 0;
        while (i < count)
        {
            DISP_WRITE_DATA(pixels[i]);
            i++;
        }
    }
}
