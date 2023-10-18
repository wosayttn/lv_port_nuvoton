/**************************************************************************//**
 * @file     disp_fsa506.c
 * @brief    ili9431 driver
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "disp_fsa506.h"

int disp_fsa506_init(void)
{
    SET_RST;
    SET_BACKLIGHT_OFF;

    /* Hardware reset */
    CLR_RST;
    fsa506_delay_ms(100);    // Delay 100ms

    SET_RST;
    fsa506_delay_ms(500);    // Delay 500ms

    fsa506_write_reg(0x40, 0x12);   // [5]:PLL control 20~100MHz [2:1]:Output Driving 8mA, [0]:Output slew Fast
    fsa506_write_reg(0x41, 0x05);   // PLL Programmable pre-divider: 5
    fsa506_write_reg(0x42, 0x06);   // PLL Programmable loop divider: 6

    /* Set the panel X size */
    fsa506_write_reg(0x08, (LV_HOR_RES_MAX >> 8)); //Set the panel X size H[1.0]
    fsa506_write_reg(0x09, (LV_HOR_RES_MAX));    //Set the panel X size L[7:0]

    /* Memory write start address */
    fsa506_write_reg(0x0a, 0x00); //[17:16] bits of memory write start address
    fsa506_write_reg(0x0b, 0x00); //[15:8] bits of memory write start address
    fsa506_write_reg(0x0c, 0x00); //[7:0] bits of memory write start address

    /* Clock & format */
    fsa506_write_reg(0x10, 0x0D); //[0-1] : 20MHz, [2]: Parallel panel, [3]: Normal operation
    fsa506_write_reg(0x11, 0x05); //[3-5]: RGB, [0-2]BGR

    /* For TFT output timing adjust */
    fsa506_write_reg(0x12, 0x00);                      //Hsync start position H-Byte
    fsa506_write_reg(0x13, 0x00);                      //Hsync start position L-Byte
    fsa506_write_reg(0x14, (41 >> 8));        //Hsync pulse width H-Byte
    fsa506_write_reg(0x15, (41));             //Hsync pulse width L-Byte

    fsa506_write_reg(0x16, (43 >> 8));        //DE pulse start position H-Byte
    fsa506_write_reg(0x17, (43));             //DE pulse start position L-Byte
    fsa506_write_reg(0x18, (LV_HOR_RES_MAX >> 8)); //DE pulse width H-Byte
    fsa506_write_reg(0x19, (LV_HOR_RES_MAX));     //DE pulse width L-Byte
    fsa506_write_reg(0x1a, (525 >> 8));       //Hsync total clocks H-Byte
    fsa506_write_reg(0x1b, (525));            //Hsync total clocks H-Byte
    fsa506_write_reg(0x1c, 0x00);                      //Vsync start position H-Byte
    fsa506_write_reg(0x1d, 0x00);                      //Vsync start position L-Byte
    fsa506_write_reg(0x1e, (10 >> 8));        //Vsync pulse width H-Byte
    fsa506_write_reg(0x1f, (10));             //Vsync pulse width L-Byte
    fsa506_write_reg(0x20, (12 >> 8));        //Vertical DE pulse start position H-Byte
    fsa506_write_reg(0x21, (12));             //Vertical DE pulse start position L-Byte
    fsa506_write_reg(0x22, (LV_VER_RES_MAX >> 8)); //Vertical Active width H-Byte
    fsa506_write_reg(0x23, (LV_VER_RES_MAX));     //Vertical Active width H-Byte
    fsa506_write_reg(0x24, (286 >> 8));       //Vertical total width H-Byte
    fsa506_write_reg(0x25, (286));            //Vertical total width L-Byte

    fsa506_write_reg(0x26, 0x00);                      //Memory read start address
    fsa506_write_reg(0x27, 0x00);                      //Memory read start address
    fsa506_write_reg(0x28, 0x00);                      //Memory read start address
    fsa506_write_reg(0x29, 0x01);                      //[0] Load output timing related setting (H sync., V sync. and DE) to take effect

    //[7:4] Reserved
    //[3]   Output pin X_DCON level control
    //[2]   Output clock inversion     0: Normal 1: Inverse
    //[1:0] Image rotate
    //      00: 0? 01: 90? 10: 270?11: 180?
    fsa506_write_reg(0x2d, (1 << 2) | 0x08);

    /* Set the Horizontal offset  */
    fsa506_write_reg(0x30, 0x00);                        //_H byte H-Offset[3:0]
    fsa506_write_reg(0x31, 0x00);                        //_L byte H-Offset[7:0]
    fsa506_write_reg(0x32, 0x00);                        //_H byte V-Offset[3:0]
    fsa506_write_reg(0x33, 0x00);                        //_L byte V-Offset[7:0]
    fsa506_write_reg(0x34, (LV_HOR_RES_MAX >> 8));  //H byte H-def[3:0]
    fsa506_write_reg(0x35, (LV_HOR_RES_MAX));       //_L byte H-def[7:0]
    fsa506_write_reg(0x36, ((2 * LV_VER_RES_MAX) >> 8)); //[3:0] MSB of image vertical physical resolution in memory
    fsa506_write_reg(0x37, (2 * LV_VER_RES_MAX));   //[7:0] LSB of image vertical physical resolution in memory

    SET_BACKLIGHT_ON;

    return 0;
}
