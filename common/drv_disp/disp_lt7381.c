/**************************************************************************//**
 * @file     disp_lt7381.c
 * @brief    lt7381 driver
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "disp.h"

#define LCD_VBPD        20
#define LCD_VFPD        12
#define LCD_VSPW        3
#define LCD_HBPD        140
#define LCD_HFPD        160
#define LCD_HSPW        20

#define PIXEL_CLOCK ((LCD_HBPD + LCD_HFPD + LCD_HSPW + LV_HOR_RES_MAX) * \
                    (LCD_VBPD + LCD_VFPD + LCD_VSPW + LV_VER_RES_MAX) * 60)

#define PIXEL_CLOCK_MZ (PIXEL_CLOCK/1000000)

#if ((((PIXEL_CLOCK) % 1000000)/100000) > 5)
    #define DIVIDER   (PIXEL_CLOCK_MZ+1)
#else
    #define DIVIDER   (PIXEL_CLOCK_MZ)
#endif

#define SCLK   DIVIDER
#define MCLK   (5*DIVIDER)
#define CCLK   (5*DIVIDER)

#if (SCLK>65)
    #undef SCLK
    #define SCLK   65
#endif

#if (MCLK>100)
    #undef MCLK
    #define MCLK   100
#endif

#if (CCLK>100)
    #undef CCLK
    #define CCLK   100
#endif

#define SDRAM_ITV ((64000000 / 8192) / (1000 / MCLK) - 2)

#define XI_IN    XI_12M
typedef enum
{
    XI_4M,
    XI_5M,
    XI_8M,
    XI_10M,
    XI_12M,
    XI_CNT
} lt7381_xi_opt;

typedef struct
{
    uint16_t lpllOD_sclk;
    uint16_t lpllOD_cclk;
    uint16_t lpllOD_mclk;
    uint16_t lpllR_sclk;
    uint16_t lpllR_cclk;
    uint16_t lpllR_mclk;
    uint16_t lpllN_mclk;
    uint16_t lpllN_cclk;
    uint16_t lpllN_sclk;
} lt7381_pll_t;

static const lt7381_pll_t s_PllSettings[XI_CNT] =
{
    {3, 3, 3, 2, 2, 2, (2 * MCLK), (2 * CCLK), (2 * SCLK) }, // XI_4M
    {3, 3, 3, 5, 5, 5, (4 * MCLK), (4 * CCLK), (4 * SCLK) }, // XI_5M
    {3, 3, 3, 2, 2, 2, (1 * MCLK), (1 * CCLK), (1 * SCLK) }, // XI_8M
    {3, 3, 3, 5, 5, 5, (2 * MCLK), (2 * CCLK), (2 * SCLK) }, // XI_10M
    {3, 3, 3, 3, 3, 3, (1 * MCLK), (1 * CCLK), (1 * SCLK) }  // XI_12M
};

static void lt7381_hw_reset(void)
{
    /* Hardware reset */
    DISP_SET_RST;
    disp_delay_ms(50);
    DISP_CLR_RST;
    disp_delay_ms(50);
    DISP_SET_RST;
    disp_delay_ms(50);
}

static void lt7381_sw_reset(void)
{
    DISP_WRITE_REG(0x00);
    DISP_WRITE_DATA(0x01);
    disp_delay_ms(100);
}

static void lt7381_wait_ready(void)
{
    uint8_t i = 0;
    uint8_t system_ok = 0;

    while (1)
    {
        if ((DISP_READ_STATUS() & BIT1) == 0x00)
        {
            disp_delay_ms(2);
            DISP_WRITE_REG(0x01);
            if ((DISP_READ_DATA() & BIT7) == BIT7)
            {
                break;
            }
            else
            {
                disp_delay_ms(2);
                DISP_WRITE_REG(0x01);
                DISP_WRITE_DATA(BIT7);
            }
        }

        if ((i % 5) == 0)
            lt7381_hw_reset();

        i++;
    }

    disp_delay_ms(100);

    while (DISP_READ_STATUS() & BIT1);
}

/*
Display ON/OFF
0b: Display Off.
1b: Display On.
*/
static void lt7381_display_on(void)
{
    volatile uint16_t temp;

    DISP_WRITE_REG(0x12);
    temp = DISP_READ_DATA();
    temp |= BIT6;
    DISP_WRITE_DATA(temp);
}

static void lt7381_initial_pll(void)
{
    DISP_WRITE_REG(0x05);
    DISP_WRITE_DATA((s_PllSettings[XI_IN].lpllOD_sclk << 6) |
                    (s_PllSettings[XI_IN].lpllR_sclk << 1) |
                    (s_PllSettings[XI_IN].lpllN_sclk >> 8) & 0x1);

    DISP_WRITE_REG(0x07);
    DISP_WRITE_DATA((s_PllSettings[XI_IN].lpllOD_mclk << 6) |
                    (s_PllSettings[XI_IN].lpllR_mclk << 1) |
                    (s_PllSettings[XI_IN].lpllN_mclk >> 8) & 0x1);

    DISP_WRITE_REG(0x09);
    DISP_WRITE_DATA((s_PllSettings[XI_IN].lpllOD_cclk << 6) |
                    (s_PllSettings[XI_IN].lpllR_cclk << 1) |
                    (s_PllSettings[XI_IN].lpllN_cclk >> 8) & 0x1);

    DISP_WRITE_REG(0x06);
    DISP_WRITE_DATA(s_PllSettings[XI_IN].lpllN_sclk);

    DISP_WRITE_REG(0x08);
    DISP_WRITE_DATA(s_PllSettings[XI_IN].lpllN_mclk);

    DISP_WRITE_REG(0x0a);
    DISP_WRITE_DATA(s_PllSettings[XI_IN].lpllN_cclk);

    DISP_WRITE_REG(0x00);
    disp_delay_ms(1);
    DISP_WRITE_DATA(0x80);
    disp_delay_ms(1);
}

static void lt7381_initial_sdram(void)
{
    disp_write_reg(0xe0, 0x20);
    disp_write_reg(0xe1, 0x03);  //CAS:2=0x02，CAS:3=0x03

    disp_write_reg(0xe2, SDRAM_ITV);
    disp_write_reg(0xe3, SDRAM_ITV >> 8);
    disp_write_reg(0xe4, 0x01);

    /*  0: SDRAM is not ready for access
        1: SDRAM is ready for access        */
    while ((DISP_READ_STATUS() & BIT2) == 0x00);

    disp_delay_ms(1);
}

static void lt7381_initial_panel(void)
{
    //**[01h]**//
    /*
        00b: 24-bits output.
        01b: 18-bits output, unused pins are set as GPIO.
        10b: 16-bits output, unused pins are set as GPIO.
        11b: LVDS, all 24-bits unused output pins are set as GPIO.
    */
    DISP_WRITE_REG(0x01);
    DISP_WRITE_DATA((DISP_READ_DATA() & ~BIT3) | BIT4);

    /*
    Parallel Host Data Bus Width Selection
        0: 8-bit Parallel Host Data Bus.
        1: 16-bit Parallel Host Data Bus.*/
    DISP_WRITE_REG(0x01);
    DISP_WRITE_DATA(DISP_READ_DATA() | BIT0);

    //**[02h]**//
    /* RGB_16bpp, RGB565 */
    DISP_WRITE_REG(0x02);
    DISP_WRITE_DATA((DISP_READ_DATA() & ~BIT7) | BIT6);

    /* MemWrite_Left_Right_Top_Down */
    DISP_WRITE_REG(0x02);
    DISP_WRITE_DATA(DISP_READ_DATA() & ~(BIT2 | BIT1));

    //**[03h]**//
    /* Set graphics mode */
    DISP_WRITE_REG(0x03);
    DISP_WRITE_DATA(DISP_READ_DATA() & ~BIT2);

    /* Set memory using sdram */
    DISP_WRITE_REG(0x03);
    DISP_WRITE_DATA(DISP_READ_DATA() & ~(BIT1 | BIT0));

    //**[12h]**//
    /*
    PCLK Inversion
    0: PDAT, DE, HSYNC etc. Drive(/ change) at PCLK falling edge.
    1: PDAT, DE, HSYNC etc. Drive(/ change) at PCLK rising edge.
    */
    DISP_WRITE_REG(0x12);
    DISP_WRITE_DATA(DISP_READ_DATA() | BIT7);

    /*
    Vertical Scan direction
    0 : From Top to Bottom
    1 : From bottom to Top
    PIP window will be disabled when VDIR set as 1.
    */
    DISP_WRITE_REG(0x12);
    DISP_WRITE_DATA(DISP_READ_DATA() & ~BIT3);

    /*
    parallel PDATA[23:0] Output Sequence
    000b : RGB.
    001b : RBG.
    010b : GRB.
    011b : GBR.
    100b : BRG.
    101b : BGR.
    */
    DISP_WRITE_REG(0x12);
    DISP_WRITE_DATA(DISP_READ_DATA() & ~(BIT0 | BIT1 | BIT2));

    //**[13h]**//
    /*
    HSYNC Polarity
    0 : Low active.
    1 : High active.
    */
    DISP_WRITE_REG(0x13);
    DISP_WRITE_DATA(DISP_READ_DATA() & ~(BIT7));

    /*
    VSYNC Polarity
    0 : Low active.
    1 : High active.
    */
    DISP_WRITE_REG(0x13);
    DISP_WRITE_DATA(DISP_READ_DATA() & ~(BIT6));

    /*
    DE Polarity
    0 : High active.
    1 : Low active.
    */
    DISP_WRITE_REG(0x13);
    DISP_WRITE_DATA(DISP_READ_DATA() & ~(BIT5));

    //**[14h][15h][1Ah][1Bh]**//
    /*
    [14h] Horizontal Display Width Setting Bit[7:0]
    [15h] Horizontal Display Width Fine Tuning (HDWFT) [3:0]
    The register specifies the LCD panel horizontal display width in
    the unit of 8 pixels resolution.
    Horizontal display width(pixels) = (HDWR + 1) * 8 + HDWFTR

    [1Ah] Vertical Display Height Bit[7:0]
    Vertical Display Height(Line) = VDHR + 1
    [1Bh] Vertical Display Height Bit[10:8]
    Vertical Display Height(Line) = VDHR + 1
    */
    disp_write_reg(0x14, (LV_HOR_RES_MAX < 8) ? 0 : ((LV_HOR_RES_MAX / 8) - 1));
    disp_write_reg(0x15, (LV_HOR_RES_MAX < 8) ? LV_HOR_RES_MAX : LV_HOR_RES_MAX % 8);
    disp_write_reg(0x1A, (LV_VER_RES_MAX - 1));
    disp_write_reg(0x1B, (LV_VER_RES_MAX - 1) >> 8);

    //**[16h][17h][18h][19]**//
    /*
    [16h] Horizontal Non-Display Period(HNDR) Bit[4:0]
    This register specifies the horizontal non-display period. Also
    called back porch.
    Horizontal non-display period(pixels) = (HNDR + 1) * 8 + HNDFTR
    [17h] Horizontal Non-Display Period Fine Tuning(HNDFT) [3:0]
    This register specifies the fine tuning for horizontal non-display
    period; it is used to support the SYNC mode panel. Each level of
    this modulation is 1-pixel.
    Horizontal non-display period(pixels) = (HNDR + 1) * 8 + HNDFTR
    [18h] HSYNC Start Position[4:0]
    The starting position from the end of display area to the
    beginning of HSYNC. Each level of this modulation is 8-pixel.
    Also called front porch.
    HSYNC Start Position(pixels) = (HSTR + 1)x8
    [19h] HSYNC Pulse Width(HPW) [4:0]
    The period width of HSYNC.
    HSYNC Pulse Width(pixels) = (HPW + 1)x8
    */
    disp_write_reg(0x16, (LCD_HBPD < 8) ? 0 : (LCD_HBPD / 8) - 1);
    disp_write_reg(0x17, (LCD_HBPD < 8) ? LCD_HBPD : (LCD_HBPD % 8));

    disp_write_reg(0x18, (LCD_HFPD < 8) ? 0 : ((LCD_HFPD / 8) - 1));
    disp_write_reg(0x19, (LCD_HSPW < 8) ? 0 : ((LCD_HSPW / 8) - 1));

    //**[1Ch][1Dh][1Eh][1Fh]**//
    /*
    [1Ch] Vertical Non-Display Period Bit[7:0]
    Vertical Non-Display Period(Line) = (VNDR + 1)
    [1Dh] Vertical Non-Display Period Bit[9:8]
    Vertical Non-Display Period(Line) = (VNDR + 1)
    [1Eh] VSYNC Start Position[7:0]
    The starting position from the end of display area to the beginning of VSYNC.
    VSYNC Start Position(Line) = (VSTR + 1)
    [1Fh] VSYNC Pulse Width[5:0]
    The pulse width of VSYNC in lines.
    VSYNC Pulse Width(Line) = (VPWR + 1)
    */
    disp_write_reg(0x1C, (LCD_VBPD - 1));
    disp_write_reg(0x1D, (LCD_VBPD - 1) >> 8);

    disp_write_reg(0x1E, LCD_VFPD  - 1);
    disp_write_reg(0x1F, LCD_VSPW  - 1);

    //**[5Eh]**//
    /*
    Canvas addressing mode
    0: Block mode (X-Y coordination addressing)
    1: linear mode
    */
    DISP_WRITE_REG(0x5E);
    DISP_WRITE_DATA(DISP_READ_DATA() & ~(BIT2));

    /*
    Canvas image color depth & memory R/W data width
    In Block Mode:
    00: 8bpp
    01: 16bpp
    1x: 24bpp

    In Linear Mode:
    X0: 8-bits memory data read/write.
    X1: 16-bits memory data read/write
    */
    DISP_WRITE_REG(0x5E);
    DISP_WRITE_DATA((DISP_READ_DATA() & ~(BIT1)) | BIT0);
}

static void lt7381_initial_main_window(void)
{
    //**[10h]**//
    /*
    Main Window Color Depth Setting
    00b: 8-bpp generic TFT, i.e. 256 colors.
    01b: 16-bpp generic TFT, i.e. 65K colors.
    1xb: 24-bpp generic TFT, i.e. 1.67M colors.
    */
    DISP_WRITE_REG(0x10);
    DISP_WRITE_DATA((DISP_READ_DATA() & ~(BIT3)) | BIT2);

    //**[20h][21h][22h][23h]**//
    /*
    [20h] Main Image Start Address[7:2]
    [21h] Main Image Start Address[15:8]
    [22h] Main Image Start Address [23:16]
    [23h] Main Image Start Address [31:24]
    */
    disp_write_reg(0x20, 0x0);
    disp_write_reg(0x21, 0x0 >> 8);
    disp_write_reg(0x22, 0x0 >> 16);
    disp_write_reg(0x23, 0x0 >> 24);

    //**[24h][25h]**//
    /*
    [24h] Main Image Width [7:0]
    [25h] Main Image Width [12:8]
    Unit: Pixel.
    It must be divisible by 4. MIW Bit [1:0] tie to ?? internally.
    The value is physical pixel number. Maximum value is 8188 pixels
    */
    disp_write_reg(0x24, LV_HOR_RES_MAX);
    disp_write_reg(0x25, LV_HOR_RES_MAX >> 8);

    //**[26h][27h]**//
    /*
    [26h] Main Window Upper-Left corner X-coordination [7:0]
    [27h] Main Window Upper-Left corner X-coordination [12:8]
    Reference Main Image coordination.
    Unit: Pixel
    It must be divisible by 4. MWULX Bit [1:0] tie to ?? internally.
    X-axis coordination plus Horizontal display width cannot large than 8188.

    [28h] Main Window Upper-Left corner Y-coordination [7:0]
    [29h] Main Window Upper-Left corner Y-coordination [12:8]
    Reference Main Image coordination.
    Unit: Pixel
    Range is between 0 and 8191.
    */
    disp_write_reg(0x26, 0x0);
    disp_write_reg(0x27, 0x0 >> 8);
    disp_write_reg(0x28, 0x0);
    disp_write_reg(0x29, 0x0 >> 8);

    //**[50h][51h][52h][53h][54h][55h]**//
    /*
    [50h] Start address of Canvas [7:0]
    [51h] Start address of Canvas [15:8]
    [52h] Start address of Canvas [23:16]
    [53h] Start address of Canvas [31:24]
    [54h] Canvas image width [7:2]
    [55h] Canvas image width [12:8]
    */
    disp_write_reg(0x50, 0x0);
    disp_write_reg(0x51, 0x0 >> 8);
    disp_write_reg(0x52, 0x0 >> 16);
    disp_write_reg(0x53, 0x0 >> 24);
    disp_write_reg(0x54, LV_HOR_RES_MAX);
    disp_write_reg(0x55, LV_HOR_RES_MAX >> 8);

    //**[56h][57h][58h][59h][5Ah][5Bh][5Ch][5Dh]**//
    /*
    [56h] Active Window Upper-Left corner X-coordination [7:0]
    [57h] Active Window Upper-Left corner X-coordination [12:8]
    [58h] Active Window Upper-Left corner Y-coordination [7:0]
    [59h] Active Window Upper-Left corner Y-coordination [12:8]
    [5Ah] Width of Active Window [7:0]
    [5Bh] Width of Active Window [12:8]
    [5Ch] Height of Active Window [7:0]
    [5Dh] Height of Active Window [12:8]
    */
    disp_write_reg(0x56, 0x0);
    disp_write_reg(0x57, 0x0 >> 8);
    disp_write_reg(0x58, 0x0);
    disp_write_reg(0x59, 0x0 >> 8);
    disp_write_reg(0x5A, LV_HOR_RES_MAX);
    disp_write_reg(0x5B, LV_HOR_RES_MAX >> 8);
    disp_write_reg(0x5C, LV_VER_RES_MAX);
    disp_write_reg(0x5D, LV_VER_RES_MAX >> 8);
}

int disp_init(void)
{
    DISP_SET_RST;
    DISP_CLR_BACKLIGHT;

    /* Hardware reset */
    disp_delay_ms(100);
    lt7381_hw_reset();
    disp_delay_ms(100);
	
    lt7381_wait_ready();

    /* Software reset */	
    lt7381_sw_reset();
	
    lt7381_initial_pll();

    lt7381_initial_sdram();

    lt7381_initial_panel();

    lt7381_display_on();

    lt7381_initial_main_window();

    DISP_SET_BACKLIGHT;

    return 0;
}

void disp_fillrect(uint16_t *pixels, const lv_area_t *area)
{
    int32_t w = lv_area_get_width(area);
    int32_t h = lv_area_get_height(area);

    LV_LOG_INFO("%08x WxH=%dx%d (%d, %d) (%d, %d)",
                pixels,
                lv_area_get_width(area),
                lv_area_get_height(area),
                area->x1,
                area->y1,
                area->x2,
                area->y2);

    disp_set_column(area->x1, area->x2);
    disp_set_page(area->y1, area->y2);
    disp_send_pixels(pixels, h * w * sizeof(uint16_t));
}
