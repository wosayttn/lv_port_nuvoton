/**************************************************************************//**
 * @file     lv_glue.c
 * @brief    lvgl glue for M480 series
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "lvgl.h"
#include "lv_glue.h"
#include "disp_ili9341.h"
#include "touch_adc.h"

#define CONFIG_VRAM_TOTAL_ALLOCATED_SIZE    NVT_ALIGN((LV_HOR_RES_MAX * CONFIG_DISP_LINE_BUFFER_NUMBER * sizeof(lv_color_t)), 4)

static uint8_t s_au8FrameBuf[CONFIG_VRAM_TOTAL_ALLOCATED_SIZE] __attribute__((aligned(4)));

/* User can define ADC touch calibration matrix in board_dev.c. */
#if defined(__320x240__)
/* Use ADVv4.1 board */
S_CALIBRATION_MATRIX g_sCalMat = { 9, 6484, -4280144, -5183, -113, 19125360, 65536 };
#endif

static volatile uint32_t s_systick_count = 0;

void SysTick_Handler(void)
{
    s_systick_count++;
}

void systick_init(void)
{
    SysTick_Config(SystemCoreClock / CONFIG_TICK_PER_SECOND);
}

void sysDelay(uint32_t ms)
{
    volatile uint32_t tgtTicks = s_systick_count + ms;
    while (s_systick_count < tgtTicks);
}

uint32_t sysGetTicks(uint32_t no)
{
    return s_systick_count;
}

static int nuvoton_fs_init(void)
{
    return 0;
}

int nuvoton_fs_write(int hFile, void *buf, int size)
{
    return size;
}

int nuvoton_fs_read(int hFile, void *buf, int size)
{
    return size;
}

static int nuvoton_fs_fini(void)
{
    return 0;
}

int lcd_device_initialize(void)
{
    GPIO_T *PORT;

    /* Set GPIO Output mode for ILI9341 pins. */
    PORT    = (GPIO_T *)(GPIOA_BASE + (NU_GET_PORT(CONFIG_ILI9341_PIN_DC) * PORT_OFFSET));
    GPIO_SetMode(PORT, NU_GET_PIN_MASK(NU_GET_PIN(CONFIG_ILI9341_PIN_DC)), GPIO_MODE_OUTPUT);

    PORT    = (GPIO_T *)(GPIOA_BASE + (NU_GET_PORT(CONFIG_ILI9341_PIN_RESET) * PORT_OFFSET));
    GPIO_SetMode(PORT, NU_GET_PIN_MASK(NU_GET_PIN(CONFIG_ILI9341_PIN_RESET)), GPIO_MODE_OUTPUT);

    PORT    = (GPIO_T *)(GPIOA_BASE + (NU_GET_PORT(CONFIG_ILI9341_PIN_BACKLIGHT) * PORT_OFFSET));
    GPIO_SetMode(PORT, NU_GET_PIN_MASK(NU_GET_PIN(CONFIG_ILI9341_PIN_BACKLIGHT)), GPIO_MODE_OUTPUT);

    /* Open EBI */
    EBI_Open(CONFIG_ILI9341_EBI, EBI_BUSWIDTH_16BIT, EBI_TIMING_NORMAL, EBI_OPMODE_NORMAL, EBI_CS_ACTIVE_LOW);
    switch (CONFIG_ILI9341_EBI)
    {
    case EBI_BANK0:
        EBI->CTL0 |= EBI_CTL0_CACCESS_Msk;
        EBI->TCTL0 |= (EBI_TCTL0_WAHDOFF_Msk | EBI_TCTL0_RAHDOFF_Msk);
        break;
    case EBI_BANK1:
        EBI->CTL1 |= EBI_CTL1_CACCESS_Msk;
        EBI->TCTL1 |= (EBI_TCTL1_WAHDOFF_Msk | EBI_TCTL1_RAHDOFF_Msk);
        break;
    case EBI_BANK2:
        EBI->CTL2 |= EBI_CTL2_CACCESS_Msk;
        EBI->TCTL2 |= (EBI_TCTL2_WAHDOFF_Msk | EBI_TCTL2_RAHDOFF_Msk);
        break;
    default:
        return -1;
    }

    disp_ili9341_init();

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
        psLCDInfo->u32BytePerPixel = sizeof(lv_color_t);
        psLCDInfo->evLCDType = evLCD_TYPE_MPU;
    }
    break;

    case evLCD_CTRL_RECT_UPDATE:
    {
        ili9341_fillrect((uint16_t *)s_au8FrameBuf, (const lv_area_t *)argv);
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

uint32_t nu_adc_sampling(uint32_t channel)
{
    EADC_ConfigSampleModule(CONFIG_AD, 0, EADC_SOFTWARE_TRIGGER, channel);

    EADC_CLR_INT_FLAG(CONFIG_AD, EADC_STATUS2_ADIF0_Msk);

    EADC_ENABLE_INT(CONFIG_AD, BIT0);

    EADC_ENABLE_SAMPLE_MODULE_INT(CONFIG_AD, 0, BIT0);

    EADC_START_CONV(CONFIG_AD, BIT0);

    while (EADC_GET_INT_FLAG(CONFIG_AD, BIT0) == 0);

    return EADC_GET_CONV_DATA(CONFIG_AD, 0) & 0x0FFF;
}

int touchpad_device_initialize(void)
{
    return 0;
}

int touchpad_device_open(void)
{
    EADC_Open(CONFIG_AD, EADC_CTL_DIFFEN_SINGLE_END);

    nuvoton_fs_init();

    extern int ad_touch_calibrate(void);
    //ad_touch_calibrate();

    nuvoton_fs_fini();

    return 0;
}

#define CONFIG_TRIGGER_PERIOD     LV_DISP_DEF_REFR_PERIOD
int touchpad_device_read(lv_indev_data_t *psInDevData)
{
    static lv_indev_data_t sLastInDevData = {0};
    static uint32_t u32NextTriggerTime = 0;

    uint32_t adc_x, adc_y;

    LV_ASSERT(psInDevData);

    psInDevData->state   = sLastInDevData.state;
    psInDevData->point.x = sLastInDevData.point.x;
    psInDevData->point.y = sLastInDevData.point.y;

    if (sysGetTicks(0) < u32NextTriggerTime)
    {
        goto exit_touchpad_device_read;
    }

    /* Get X, Y ADC converting data */
    adc_x  = indev_touch_get_x();
    adc_y  = indev_touch_get_y();
    u32NextTriggerTime = sysGetTicks(0) + CONFIG_TRIGGER_PERIOD;

    if ((adc_x < 4000) && (adc_y < 4000))
    {
        psInDevData->state = LV_INDEV_STATE_PRESSED;
    }
    else
    {
        psInDevData->state = LV_INDEV_STATE_RELEASED;
    }

    if (psInDevData->state == LV_INDEV_STATE_PRESSED)
    {
        extern int ad_touch_map(int32_t *sumx, int32_t *sumy);
        if (ad_touch_map((int32_t *)&adc_x, (int32_t *)&adc_y) == 0)
        {

            psInDevData->point.x = ((int16_t)adc_x < 0) ? 0 :
                                   ((int16_t)adc_x >= LV_HOR_RES_MAX) ? (LV_HOR_RES_MAX - 1) :
                                   adc_x;

            psInDevData->point.y = ((int16_t)adc_y < 0) ? 0 :
                                   ((int16_t)adc_y >= LV_VER_RES_MAX) ? (LV_VER_RES_MAX - 1) :
                                   adc_y;
        }
        else
        {
            psInDevData->point.x = (int16_t)adc_x;

            psInDevData->point.y = (int16_t)adc_y;
        }

        sLastInDevData.point.x  = psInDevData->point.x;
        sLastInDevData.point.y  = psInDevData->point.y;
    }

    sLastInDevData.state = psInDevData->state;

    LV_LOG_INFO("%s (%d, %d)", psInDevData->state ? "Press" : "Release", psInDevData->point.x, psInDevData->point.y);

exit_touchpad_device_read:

    return (psInDevData->state == LV_INDEV_STATE_PRESSED) ? 1 : 0;
}

int touchpad_device_control(int cmd, void *argv)
{
    return 0;
}

void touchpad_device_close(void)
{
    EADC_Close(CONFIG_AD);
}

int touchpad_device_finalize(void)
{
    return 0;
}
