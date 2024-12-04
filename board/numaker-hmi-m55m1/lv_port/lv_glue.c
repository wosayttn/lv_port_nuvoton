/**************************************************************************//**
 * @file     lv_glue.c
 * @brief    lvgl glue for M55M1 series
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "lvgl.h"
#include "lv_glue.h"
#include "disp.h"
#include "indev_touch.h"

#if defined(CONFIG_DISP_USE_EBI_SYNC)
    #define CONFIG_VRAM_BUFFER_NUM              2
#else
    #define CONFIG_VRAM_BUFFER_NUM              1
#endif
#define CONFIG_VRAM_TOTAL_ALLOCATED_SIZE    NVT_ALIGN((CONFIG_VRAM_BUFFER_NUM * LV_HOR_RES_MAX * CONFIG_DISP_LINE_BUFFER_NUMBER * (LV_COLOR_DEPTH/8)), DCACHE_LINE_SIZE)

#if defined(USE_HYPERRAM_AS_FRAMEBUFFER)
    static uint8_t *s_au8FrameBuf = (uint8_t *)SPIM_DMM0_SADDR;
#else
    static uint8_t s_au8FrameBuf[CONFIG_VRAM_TOTAL_ALLOCATED_SIZE] __attribute__((aligned(DCACHE_LINE_SIZE)));
#endif

void sysDelay(uint32_t ms)
{
    vTaskDelay(ms / portTICK_PERIOD_MS);
}

#if defined(CONFIG_DISP_USE_EBI_SYNC)

void disp_set_vrambufaddr(void *pvBufAddr);

#if (CONFIG_LV_DISP_FULL_REFRESH==1)
static volatile uint32_t s_vu32Displayblank = 0;

#if (LV_USE_OS==LV_OS_FREERTOS)
    static xQueueHandle s_VSyncQ = NULL;
    static uint8_t dummy = 0x87;
#endif

static void disp_blank_handler(void *p)
{
    s_vu32Displayblank++;

#if (LV_USE_OS==LV_OS_FREERTOS)
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    xQueueSendFromISR(s_VSyncQ, &dummy, &xHigherPriorityTaskWoken);

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
#endif
}
#endif
#endif

int lcd_device_initialize(void)
{
#if defined(CONFIG_DISP_USE_EBI_SYNC)

    /* Open EBI  */
    EBI_Open(CONFIG_DISP_EBI, EBI_BUSWIDTH_16BIT, EBI_TIMING_FASTEST, EBI_OPMODE_CACCESS | EBI_OPMODE_ADSEPARATE, EBI_CS_ACTIVE_LOW);

    /* Optimization timing. */
    EBI_SetBusTiming(CONFIG_DISP_EBI, 0, EBI_MCLKDIV_4);

    /* Set VRAM buffer address. */
    disp_set_vrambufaddr((void *)s_au8FrameBuf);


#if (CONFIG_LV_DISP_FULL_REFRESH==1)

#if (LV_USE_OS==LV_OS_FREERTOS)
    /* Create a queue of length 1 */
    s_VSyncQ = xQueueGenericCreate(1, sizeof(uint8_t), 0);
    LV_ASSERT(s_VSyncQ != NULL);
#endif

    /* Set blank callback. */
    void disp_set_blankcb(void *pvpfnBlank);
    disp_set_blankcb(disp_blank_handler);
#endif

#else

    GPIO_T *PORT;

    /* Set GPIO Output mode for display pins. */
    PORT    = (GPIO_T *)(GPIOA_BASE + (NU_GET_PORT(CONFIG_DISP_PIN_RESET) * PORT_OFFSET));
    GPIO_SetMode(PORT, NU_GET_PIN_MASK(NU_GET_PIN(CONFIG_DISP_PIN_RESET)), GPIO_MODE_OUTPUT);

    PORT    = (GPIO_T *)(GPIOA_BASE + (NU_GET_PORT(CONFIG_DISP_PIN_BACKLIGHT) * PORT_OFFSET));
    GPIO_SetMode(PORT, NU_GET_PIN_MASK(NU_GET_PIN(CONFIG_DISP_PIN_BACKLIGHT)), GPIO_MODE_OUTPUT);


#if defined(__800x480__ )
    /* Open EBI  */
    EBI_Open(CONFIG_DISP_EBI, EBI_BUSWIDTH_16BIT, EBI_TIMING_NORMAL, EBI_OPMODE_CACCESS | EBI_OPMODE_ADSEPARATE, EBI_CS_ACTIVE_LOW);

    /* Optimization timing. */
    EBI_SetBusTiming(CONFIG_DISP_EBI, EBI_TCTL_RAHDOFF_Msk | EBI_TCTL_WAHDOFF_Msk | (4 << EBI_TCTL_TACC_Pos), EBI_MCLKDIV_2);
#else
    /* Open EBI  */
    EBI_Open(CONFIG_DISP_EBI, EBI_BUSWIDTH_16BIT, EBI_TIMING_SLOW, EBI_OPMODE_CACCESS | EBI_OPMODE_ADSEPARATE, EBI_CS_ACTIVE_LOW);
#endif

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

        LV_ASSERT(argv != NULL);

        psLCDInfo->pvVramStartAddr = (void *)s_au8FrameBuf;
        psLCDInfo->u32VramSize = CONFIG_VRAM_TOTAL_ALLOCATED_SIZE;
        psLCDInfo->u32ResWidth = LV_HOR_RES_MAX;
        psLCDInfo->u32ResHeight = LV_VER_RES_MAX;
        psLCDInfo->u32BytePerPixel = (LV_COLOR_DEPTH / 8);
#if defined(CONFIG_DISP_USE_EBI_SYNC)
        psLCDInfo->evLCDType = evLCD_TYPE_SYNC;
#else
        psLCDInfo->evLCDType = evLCD_TYPE_MPU;
#endif
    }
    break;

#if defined(CONFIG_DISP_USE_EBI_SYNC)
    case evLCD_CTRL_PAN_DISPLAY:
    {
        LV_ASSERT(argv != NULL);
        disp_set_vrambufaddr(argv);
    }
    break;

#if (CONFIG_LV_DISP_FULL_REFRESH==1)
    case evLCD_CTRL_WAIT_VSYNC:
    {
        volatile uint32_t next = s_vu32Displayblank + 1;
        {
#if (LV_USE_OS==LV_OS_FREERTOS)
            /* First make sure the queue is empty, by trying to remove an element with 0 timeout. */
            xQueueReceive(s_VSyncQ, &dummy, 0);

            /* Wait for next VSYNC to occur. */
            xQueueReceive(s_VSyncQ, &dummy, portMAX_DELAY);
#else
            //Wait next blank coming;
            while (s_vu32Displayblank <  next);
#endif
        }
    }
    break;
#endif

#endif

    case evLCD_CTRL_RECT_UPDATE:
    {
#if defined(CONFIG_DISP_USE_EBI_SYNC)
        SCB_CleanDCache_by_Addr(s_au8FrameBuf, CONFIG_VRAM_TOTAL_ALLOCATED_SIZE);
#else
        disp_fillrect((uint16_t *)s_au8FrameBuf, (const lv_area_t *)argv);
#endif
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

// GPF ISR
void GPF_IRQHandler(void)
{
    GPIO_T *PORT = (GPIO_T *)(GPIOA_BASE + (NU_GET_PORT(CONFIG_INDEV_TOUCH_PIN_IRQ) * PORT_OFFSET));

    /* To check if PF.6 interrupt occurred */
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
