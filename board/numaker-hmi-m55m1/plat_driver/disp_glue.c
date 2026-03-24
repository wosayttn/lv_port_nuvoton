/**************************************************************************//**
 * @file     disp_glue.c
 * @brief    NuMaker Display Driver glue code for EBI 8080 interface LCD panel
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include <stdint.h>
#include "numaker_disp.h"
#if defined(__FREERTOS__)
    #include "FreeRTOS.h"
    #include "task.h"
    #include "semphr.h"
#endif

static uint8_t s_au8FrameBuf[CONFIG_VRAM_TOTAL_ALLOCATED_SIZE] __attribute__((aligned(DCACHE_LINE_SIZE)));

void sysDelay(uint32_t ms)
{
#if defined(__FREERTOS__)
    vTaskDelay(pdMS_TO_TICKS(ms));
#else
    TIMER_Delay(TIMER0, ms * 1000);
#endif
}

#if defined(CONFIG_DISP_EBI)

int EBI_ApplyTiming(int acc_ns,
                    int wr_idle_ns, int wr_ahd_ns,
                    int rd_ahd_ns, int rd_idle_ns)
{
#define TAHD_MAX(a, b)   ((a) > (b) ? (a) : (b))
#define TASU_CYCLE       (1)

    // Try from fastest MCLK (Div 0) to slowest
    for (int i32MCLKDiv = EBI_MCLKDIV_1; i32MCLKDiv <= EBI_MCLKDIV_8; i32MCLKDiv++)
    {

        double fEBI_MCLK_hz = (double)CLK_GetHCLK0Freq() / (i32MCLKDiv + 1);

        uint32_t TACC = ns_to_cycles_ceil(acc_ns,     fEBI_MCLK_hz) - TASU_CYCLE;
        uint32_t W2X  = ns_to_cycles_ceil(wr_idle_ns, fEBI_MCLK_hz);
        uint32_t TAHD = ns_to_cycles_ceil(TAHD_MAX(wr_ahd_ns, rd_ahd_ns), fEBI_MCLK_hz);
        uint32_t R2R  = ns_to_cycles_ceil(rd_idle_ns, fEBI_MCLK_hz);

        printf("EBI_MCLK_hz: %f\n", fEBI_MCLK_hz);
        printf("EBI_MCLK_ns: %f\n", hz_to_ns(fEBI_MCLK_hz));
        printf("acc_ns: %d ns\n", acc_ns);
        printf("wr_idle_ns: %d ns\n", wr_idle_ns);
        printf("wr_ahd_ns: %d ns\n", wr_ahd_ns);
        printf("rd_ahd_ns: %d ns\n", rd_ahd_ns);
        printf("rd_idle_ns: %d ns\n", rd_idle_ns);
        printf("Calculated: TACC:%d, W2X:%d, TAHD:%d, R2R:%d\n", TACC, W2X, TAHD, R2R);

        // Hardware register range check
        if ((TACC > 31) || (W2X > 15) || (TAHD > 7) || (R2R > 15)) continue;

        uint32_t WAHDOFF = (wr_ahd_ns == 0) ? 1 : 0;
        uint32_t RAHDOFF = (rd_ahd_ns == 0) ? 1 : 0;

        EBI_SetBusTiming(CONFIG_DISP_EBI,
                         (RAHDOFF << EBI_TCTL_RAHDOFF_Pos) |
                         (WAHDOFF << EBI_TCTL_WAHDOFF_Pos) |
                         (W2X      << EBI_TCTL_W2X_Pos)    |
                         (R2R      << EBI_TCTL_R2R_Pos)    |
                         (TAHD     << EBI_TCTL_TAHD_Pos)   |
                         (TACC     << EBI_TCTL_TACC_Pos),
                         i32MCLKDiv);

        printf("Applied: TACC:%d, W2X:%d, TAHD:%d, R2R:%d\n", TACC, W2X, TAHD, R2R);
        printf("Verify EBI->CTL0: 0x%08X\n", EBI->CTL0);
        printf("Verify EBI->TCTL0: 0x%08X\n", EBI->TCTL0);

        return 0;
    }

    return -1;
}

#endif

int lcd_device_initialize(void)
{
    GPIO_T *PORT;

    /* Set GPIO Output mode for display pins. */
#if defined(CONFIG_DISP_PIN_RESET)
    PORT    = (GPIO_T *)(GPIOA_BASE + (NU_GET_PORT(CONFIG_DISP_PIN_RESET) * PORT_OFFSET));
    GPIO_SetMode(PORT, NU_GET_PIN_MASK(NU_GET_PIN(CONFIG_DISP_PIN_RESET)), GPIO_MODE_OUTPUT);
#endif

#if defined(CONFIG_DISP_PIN_BACKLIGHT)
    PORT    = (GPIO_T *)(GPIOA_BASE + (NU_GET_PORT(CONFIG_DISP_PIN_BACKLIGHT) * PORT_OFFSET));
    GPIO_SetMode(PORT, NU_GET_PIN_MASK(NU_GET_PIN(CONFIG_DISP_PIN_BACKLIGHT)), GPIO_MODE_OUTPUT);
#endif

#if defined(CONFIG_DISP_PIN_DC)
    PORT    = (GPIO_T *)(GPIOA_BASE + (NU_GET_PORT(CONFIG_DISP_PIN_DC) * PORT_OFFSET));
    GPIO_SetMode(PORT, NU_GET_PIN_MASK(NU_GET_PIN(CONFIG_DISP_PIN_DC)), GPIO_MODE_OUTPUT);
#endif

#if defined(CONFIG_DISP_EBI)
    /* Open EBI  */
    EBI_Open(CONFIG_DISP_EBI, EBI_BUSWIDTH_16BIT, EBI_TIMING_FAST, EBI_OPMODE_ADSEPARATE, EBI_CS_ACTIVE_LOW);
    EBI_ENABLE_WRITE_BUFFER();
    EBI_ApplyTiming(EBI_8080_ACCESS_NS,
                    EBI_8080_WR_IDLE_NS,
                    EBI_8080_WR_AHD_NS,
                    EBI_8080_RD_AHD_NS,
                    EBI_8080_RD_IDLE_NS);

#elif defined(CONFIG_DISP_SPI)
    /* Open SPI */
    SPI_Open(CONFIG_DISP_SPI, SPI_MASTER, SPI_MODE_0, 8, CONFIG_DISP_SPI_CLOCK);

    /* Set sequence to MSB first */
    SPI_SET_MSB_FIRST(CONFIG_DISP_SPI);

    /* Set CS pin to HIGH */
    SPI_SET_SS_HIGH(CONFIG_DISP_SPI);

    /* Set sequence to MSB first */
    SPI_SET_MSB_FIRST(CONFIG_DISP_SPI);
#endif

    return disp_init();
}

int lcd_device_open(void)
{
    return 0;
}

int lcd_device_control(int cmd, void *argv)
{
    switch (cmd)
    {
    case evLCD_CTRL_GET_INFO:
    {
        S_LCD_INFO *psLCDInfo = (S_LCD_INFO *)argv;

        psLCDInfo->pvVramStartAddr = (void *)s_au8FrameBuf;
        psLCDInfo->u32VramSize = CONFIG_VRAM_TOTAL_ALLOCATED_SIZE;
        psLCDInfo->u32ResWidth = DISP_HOR_RES_MAX;
        psLCDInfo->u32ResHeight = DISP_VER_RES_MAX;
        psLCDInfo->u32BytePerPixel = (DISP_COLOR_DEPTH / 8);
        psLCDInfo->evLCDType = evLCD_TYPE_MPU;
    }
    break;

    case evLCD_CTRL_RECT_UPDATE:
    {
        disp_fillrect((uint16_t *)s_au8FrameBuf, (const disp_area_t *)argv);
    }
    break;

    case evLCD_CTRL_RECT_READ:
    {
        disp_readrect((uint16_t *)s_au8FrameBuf, (const disp_area_t *)argv);
    }
    break;

    default:
        while (1);
    }

    return 0;
}

void lcd_device_close(void)
{
}

int lcd_device_finalize(void)
{
    return 0;
}
