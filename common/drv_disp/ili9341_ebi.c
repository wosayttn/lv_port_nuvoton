/**************************************************************************//**
 * @file     ili9341_ebi.c
 * @brief    ili9431 ebi interface glue
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2024 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "numaker_disp.h"

/**
 * @brief Set column address window for ILI9341 via EBI interface.
 *
 * Sends the column address set command and the start/end column coordinates
 * to configure the horizontal address window.
 *
 * @param StartCol[in]  Starting column address
 * @param EndCol[in]    Ending column address
 */
void disp_set_column(uint16_t StartCol, uint16_t EndCol)
{
    /* Send column address set command */
    DISP_WRITE_REG(0x2A);

    /* Send column address range: 4 bytes (StartCol Hi, Lo, EndCol Hi, Lo) */
    DISP_WRITE_DATA((StartCol >> 8) & 0xFF);
    DISP_WRITE_DATA(StartCol & 0xFF);
    DISP_WRITE_DATA((EndCol >> 8) & 0xFF);
    DISP_WRITE_DATA(EndCol & 0xFF);
}

/**
 * @brief Set row/page address window for ILI9341 via EBI interface.
 *
 * Sends the row address set command and the start/end row coordinates
 * to configure the vertical address window.
 *
 * @param StartPage[in]  Starting row address
 * @param EndPage[in]    Ending row address
 */
void disp_set_page(uint16_t StartPage, uint16_t EndPage)
{
    /* Send row address set command */
    DISP_WRITE_REG(0x2B);

    /* Send row address range: 4 bytes (StartRow Hi, Lo, EndRow Hi, Lo) */
    DISP_WRITE_DATA((StartPage >> 8) & 0xFF);
    DISP_WRITE_DATA(StartPage & 0xFF);
    DISP_WRITE_DATA((EndPage >> 8) & 0xFF);
    DISP_WRITE_DATA(EndPage & 0xFF);
}

/**
 * @brief Send pixel data to ILI9341 display memory.
 *
 * Transfers pixel color data to the display controller. Uses PDMA for large
 * transfers (>1024 pixels) if available, otherwise uses CPU-driven writes.
 *
 * @param pixels[in]    Pointer to pixel data array (16-bit RGB565)
 * @param byte_len[in]  Total length of data in bytes
 */
void disp_send_pixels(uint16_t *pixels, int byte_len)
{
    /* Convert byte length to pixel count (16-bit pixels) */
    int count = byte_len / sizeof(uint16_t);

    /* Use PDMA for large data transfers if available */
#if defined(CONFIG_DISP_USE_PDMA)
    if (count > 1024)
        /* Use PDMA to efficiently transfer pixels */
        nu_pdma_mempush((void *)CONFIG_DISP_DAT_ADDR, (void *)pixels, 16, count);
    else
#endif
    {
        /* Fall back to CPU-driven pixel writes */
        int i = 0;
        while (i < count)
        {
            DISP_WRITE_DATA(pixels[i]);
            i++;
        }
    }
}

/* Static storage for display read area */
static disp_area_t s_receive_area = {0};

/**
 * @brief Store display area for subsequent pixel read operation.
 *
 * Saves the rectangular area configuration for use by disp_receive_pixels().
 *
 * @param area[in]  Pointer to area structure defining read region
 */
void disp_ili9341_set_area(const disp_area_t *area)
{
    /* Save area configuration for read operation */
    s_receive_area = *area;
}

/**
 * @brief Read pixel data from ILI9341 display memory.
 *
 * Reads pixel data from the previously configured display area.
 *
 * @param pixels[out]   Pointer to buffer to receive pixel data
 * @param byte_len[in]  Length of data to read in bytes
 */
void disp_receive_pixels(uint16_t *pixels, int byte_len)
{
    int i = 0, count;

    int32_t w = (int32_t)(s_receive_area.x2 - s_receive_area.x1 + 1);
    int32_t h = (int32_t)(s_receive_area.y2 - s_receive_area.y1 + 1);

    disp_set_column(s_receive_area.x1, s_receive_area.x2);
    disp_set_page(s_receive_area.y1, s_receive_area.y2);
    DISP_WRITE_REG(0x2E);

    DISP_READ_DATA(); //dummy

    count = h * w;

    while (i < count)
    {
        uint8_t r, g, b;

        uint16_t d0 = DISP_READ_DATA();
        uint16_t d1 = DISP_READ_DATA();
        uint16_t d2 = DISP_READ_DATA();

        r = d0 >> 8;
        g = d0 & 0xFF;
        b = d1 >> 8;

        pixels[i++] = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);

        r = d1 & 0xFF;
        g = d2 >> 8;
        b = d2 & 0xFF;

        if (i < count)
            pixels[i++] = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    }
}