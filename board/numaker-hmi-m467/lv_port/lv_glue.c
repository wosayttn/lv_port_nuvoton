/**************************************************************************//**
 * @file     lv_glue.c
 * @brief    lvgl glue for M460 series
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "lvgl.h"
#include "lv_glue.h"
#include "disp.h"
#include "indev_touch.h"

#define CONFIG_VRAM_TOTAL_ALLOCATED_SIZE    NVT_ALIGN((LV_HOR_RES_MAX * CONFIG_DISP_LINE_BUFFER_NUMBER * (LV_COLOR_DEPTH/8)), 4)

static uint8_t s_au8FrameBuf[CONFIG_VRAM_TOTAL_ALLOCATED_SIZE] __attribute__((aligned(4)));

void sysDelay(uint32_t ms)
{
    vTaskDelay(ms / portTICK_PERIOD_MS);
}

int lcd_device_initialize(void)
{
    GPIO_T *PORT;

    /* Set GPIO Output mode for display pins. */
    PORT    = (GPIO_T *)(GPIOA_BASE + (NU_GET_PORT(CONFIG_DISP_PIN_RESET) * PORT_OFFSET));
    GPIO_SetMode(PORT, NU_GET_PIN_MASK(NU_GET_PIN(CONFIG_DISP_PIN_RESET)), GPIO_MODE_OUTPUT);

    PORT    = (GPIO_T *)(GPIOA_BASE + (NU_GET_PORT(CONFIG_DISP_PIN_BACKLIGHT) * PORT_OFFSET));
    GPIO_SetMode(PORT, NU_GET_PIN_MASK(NU_GET_PIN(CONFIG_DISP_PIN_BACKLIGHT)), GPIO_MODE_OUTPUT);

    /* Open EBI  */
    EBI_Open(CONFIG_DISP_EBI, EBI_BUSWIDTH_16BIT, EBI_TIMING_SLOW, EBI_OPMODE_CACCESS | EBI_OPMODE_ADSEPARATE, EBI_CS_ACTIVE_LOW);

    disp_init();

    return 0;
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

        LV_ASSERT(argv != NULL);

        psLCDInfo->pvVramStartAddr = (void *)s_au8FrameBuf;
        psLCDInfo->u32VramSize = CONFIG_VRAM_TOTAL_ALLOCATED_SIZE;
        psLCDInfo->u32ResWidth = LV_HOR_RES_MAX;
        psLCDInfo->u32ResHeight = LV_VER_RES_MAX;
        psLCDInfo->u32BytePerPixel = (LV_COLOR_DEPTH / 8);
        psLCDInfo->evLCDType = evLCD_TYPE_MPU;
    }
    break;

    case evLCD_CTRL_RECT_UPDATE:
    {
        disp_fillrect((uint16_t *)s_au8FrameBuf, (const lv_area_t *)argv);
    }
    break;

    default:
        LV_ASSERT(0);
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

#if defined(CONFIG_INDEV_TOUCH_PIN_IRQ)

static IRQn_Type au32GPIRQ[] =
{
    GPA_IRQn,
    GPB_IRQn,
    GPC_IRQn,
    GPD_IRQn,
    GPE_IRQn,
    GPF_IRQn,
    GPG_IRQn,
    GPH_IRQn,
    GPI_IRQn,
    GPJ_IRQn,
};

static volatile lv_indev_data_t s_sInDevData = {0};
static volatile uint32_t s_u32LastIRQ = 0;

// GPG ISR
void GPG_IRQHandler(void)
{
    GPIO_T *PORT = (GPIO_T *)(GPIOA_BASE + (NU_GET_PORT(CONFIG_INDEV_TOUCH_PIN_IRQ) * PORT_OFFSET));

    /* To check if PC.5 interrupt occurred */
    if (GPIO_GET_INT_FLAG(PORT, NU_GET_PIN_MASK(NU_GET_PIN(CONFIG_INDEV_TOUCH_PIN_IRQ))))
    {
        GPIO_CLR_INT_FLAG(PORT, NU_GET_PIN_MASK(NU_GET_PIN(CONFIG_INDEV_TOUCH_PIN_IRQ)));
        s_u32LastIRQ = xTaskGetTickCount();
    }
    else
    {
        /* Un-expected interrupt. Just clear all PD interrupts */
        volatile uint32_t u32temp = PORT->INTSRC;
        PORT->INTSRC = u32temp;
    }
}
#endif

int touchpad_device_initialize(void)
{
#if defined(CONFIG_INDEV_TOUCH_I2C)
    GPIO_T *PORT;

    /* Set GPIO OUTPUT mode for indev touch pins. */
    PORT    = (GPIO_T *)(GPIOA_BASE + (NU_GET_PORT(CONFIG_INDEV_TOUCH_PIN_RESET) * PORT_OFFSET));
    GPIO_SetMode(PORT, NU_GET_PIN_MASK(NU_GET_PIN(CONFIG_INDEV_TOUCH_PIN_RESET)), GPIO_MODE_OUTPUT);

#if defined(CONFIG_INDEV_TOUCH_PIN_IRQ)
    /* Set GPIO INTPUT mode for indev touch pins. */
    PORT    = (GPIO_T *)(GPIOA_BASE + (NU_GET_PORT(CONFIG_INDEV_TOUCH_PIN_IRQ) * PORT_OFFSET));
    GPIO_SetMode(PORT, NU_GET_PIN_MASK(NU_GET_PIN(CONFIG_INDEV_TOUCH_PIN_IRQ)), GPIO_MODE_INPUT);
    GPIO_SetPullCtl(PORT, NU_GET_PIN_MASK(NU_GET_PIN(CONFIG_INDEV_TOUCH_PIN_IRQ)), GPIO_PUSEL_PULL_UP);
    GPIO_EnableInt(PORT, NU_GET_PIN(CONFIG_INDEV_TOUCH_PIN_IRQ), GPIO_INT_FALLING);
    NVIC_EnableIRQ(au32GPIRQ[NU_GET_PORT(CONFIG_INDEV_TOUCH_PIN_IRQ)]);
#endif

    return indev_touch_init();

#else

    return 0;

#endif

}

int touchpad_device_open(void)
{
    return 0;
}

int touchpad_device_read(lv_indev_data_t *psInDevData)
{
#if defined(CONFIG_INDEV_TOUCH_I2C)

#if defined(CONFIG_INDEV_TOUCH_PIN_IRQ)
    static uint32_t u32LastIRQ = 0;

    if (u32LastIRQ != s_u32LastIRQ)
    {
        indev_touch_get_data((lv_indev_data_t *)&s_sInDevData);
        u32LastIRQ = s_u32LastIRQ ;
    }

    psInDevData->point.x = s_sInDevData.point.x;
    psInDevData->point.y = s_sInDevData.point.y;
    psInDevData->state = s_sInDevData.state;
#else
    indev_touch_get_data(psInDevData);
#endif

    return (psInDevData->state == LV_INDEV_STATE_PRESSED) ? 1 : 0;

#else

    return LV_INDEV_STATE_RELEASED;

#endif
}

int touchpad_device_control(int cmd, void *argv)
{
    return 0;
}

void touchpad_device_close(void)
{
}

int touchpad_device_finalize(void)
{
    return 0;
}
