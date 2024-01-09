/**************************************************************************//**
 * @file     fsa506_ebi.c
 * @brief    ili9431 ebi interface glue
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "disp_fsa506.h"
#if defined(CONFIG_FSA506_EBI_USE_PDMA)
    #include "drv_pdma.h"
#endif

#if !defined(CONFIG_FSA506_EBI_ADDR)
    #error "Please specify CONFIG_FSA506_EBI_ADDR in lv_conf.h"
#endif

#define FSA506_ADDR_CMD  0x0
#define FSA506_ADDR_DATA 0x0

#define FSA506_WRITE_REG(u32RegAddr) \
        { \
            (*((volatile uint16_t *)(CONFIG_FSA506_EBI_ADDR+(FSA506_ADDR_CMD))) = (u32RegAddr)); \
            __DSB(); \
        }

#define FSA506_WRITE_DATA(u32Data) \
        { \
            (*((volatile uint16_t *)(CONFIG_FSA506_EBI_ADDR+(FSA506_ADDR_DATA))) = (u32Data)); \
            __DSB(); \
        }

void fsa506_write_reg(uint16_t reg, uint16_t data)
{
    // Register
    CLR_RS;
    FSA506_WRITE_REG(reg & 0xFF);
    SET_RS;

    // Data
    FSA506_WRITE_DATA(data & 0xFF);

    // Done
    CLR_RS;
    FSA506_WRITE_REG(0x80);
    SET_RS;
}

void fsa506_set_column(uint16_t StartCol, uint16_t EndCol)
{
    fsa506_write_reg(0x0, (StartCol >> 8) & 0xFF);
    fsa506_write_reg(0x1, StartCol & 0xFF);
    fsa506_write_reg(0x2, (EndCol >> 8) & 0xFF);
    fsa506_write_reg(0x3, EndCol & 0xFF);
}

void fsa506_set_page(uint16_t StartPage, uint16_t EndPage)
{
    fsa506_write_reg(0x4, (StartPage >> 8) & 0xFF);
    fsa506_write_reg(0x5, StartPage & 0xFF);
    fsa506_write_reg(0x6, (EndPage >> 8) & 0xFF);
    fsa506_write_reg(0x7, EndPage & 0xFF);
}

void fsa506_send_pixels(uint16_t *pixels, int byte_len)
{
    int count = byte_len / sizeof(uint16_t);

    CLR_RS;
    FSA506_WRITE_REG(0xC1);
    SET_RS;

#if defined(CONFIG_FSA506_EBI_USE_PDMA)
    // PDMA-M2M feed
    if (count > 1024)
    {
        uint32_t Odd = count & 0x1;
        nu_pdma_mempush((void *)(CONFIG_FSA506_EBI_ADDR + FSA506_ADDR_DATA), (void *)pixels, (Odd ? 16:32), (Odd ? count:(count/2)) );
    }
    else
#endif
    {
        // CPU feed
        int i = 0;
        while (i < count)
        {
            FSA506_WRITE_DATA(pixels[i]);
            i++;
        }
    }

    CLR_RS;
    FSA506_WRITE_REG(0x80);
    SET_RS;
}
