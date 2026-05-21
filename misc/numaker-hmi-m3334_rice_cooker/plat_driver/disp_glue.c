/**************************************************************************//**
 * @file     disp_glue.c
 * @brief    NuMaker Display Driver glue code for EBI 8080 interface LCD panel
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include <stdint.h>
#include <math.h>
#include "numaker_disp.h"
#if defined(__FREERTOS__)
    #include "FreeRTOS.h"
    #include "task.h"
    #include "semphr.h"
#endif

static uint8_t s_au8FrameBuf[CONFIG_VRAM_TOTAL_ALLOCATED_SIZE] __attribute__((aligned(4)));

void sysDelay(uint32_t ms)
{
#if defined(__FREERTOS__)
    vTaskDelay(pdMS_TO_TICKS(ms));
#else
    TIMER_Delay(TIMER0, ms * 1000);
#endif
}

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

#if defined(CONFIG_DISP_SPI)
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
