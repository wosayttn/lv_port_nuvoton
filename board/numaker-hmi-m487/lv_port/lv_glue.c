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

#define CONFIG_VRAM_TOTAL_ALLOCATED_SIZE    NVT_ALIGN((LV_HOR_RES_MAX * CONFIG_DISP_LINE_BUFFER_NUMBER * sizeof(lv_color_t)), 4)

static uint8_t s_au8FrameBuf[CONFIG_VRAM_TOTAL_ALLOCATED_SIZE] __attribute__((aligned(4)));

extern S_CALIBRATION_MATRIX g_sCalMat;

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


static void ili9341_fillrect(uint16_t *pixels, const lv_area_t *area)
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

    ili9341_set_column(area->x1, area->x2);
    ili9341_set_page(area->y1, area->y2);
    ili9341_send_cmd(0x2c);

    ili9341_send_pixels(pixels, h * w * sizeof(uint16_t));
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

#define NU_MFP_POS(PIN)                ((PIN % 8) * 4)
#define NU_MFP_MSK(PIN)                (0xful << NU_MFP_POS(PIN))

static void nu_pin_func(uint32_t pin, int data)
{
    uint32_t pin_index      = NU_GET_PIN(pin);
    uint32_t port_index     = NU_GET_PORT(pin);
    __IO uint32_t *GPx_MFPx = ((__IO uint32_t *) &SYS->GPA_MFPL) + port_index * 2 + (pin_index / 8);
    uint32_t MFP_Msk        = NU_MFP_MSK(pin_index);

    *GPx_MFPx  = (*GPx_MFPx & (~MFP_Msk)) | data;
}

static void tp_switch_to_analog(uint32_t pin)
{
    GPIO_T *port = (GPIO_T *)(GPIOA_BASE + PORT_OFFSET * NU_GET_PORT(pin));

    if ((pin == CONFIG_AD_PIN_YU) || (pin == CONFIG_AD_PIN_XR))
        nu_pin_func(pin, (1 << NU_MFP_POS(NU_GET_PIN(pin))));

    GPIO_DISABLE_DIGITAL_PATH(port, NU_GET_PIN_MASK(NU_GET_PIN(pin)));
}

static void tp_switch_to_digital(uint32_t pin)
{
    GPIO_T *port = (GPIO_T *)(GPIOA_BASE + PORT_OFFSET * NU_GET_PORT(pin));

    nu_pin_func(pin, 0);

    /* Enable digital path on these EADC pins */
    GPIO_ENABLE_DIGITAL_PATH(port, NU_GET_PIN_MASK(NU_GET_PIN(pin)));
}

static uint32_t nu_adc_sampling(uint32_t channel)
{
    EADC_ConfigSampleModule(CONFIG_AD, 0, EADC_SOFTWARE_TRIGGER, channel);

    EADC_CLR_INT_FLAG(CONFIG_AD, EADC_STATUS2_ADIF0_Msk);

    EADC_ENABLE_INT(CONFIG_AD, BIT0);

    EADC_ENABLE_SAMPLE_MODULE_INT(CONFIG_AD, 0, BIT0);

    EADC_START_CONV(CONFIG_AD, BIT0);

    while (EADC_GET_INT_FLAG(CONFIG_AD, BIT0) == 0);

    return EADC_GET_CONV_DATA(CONFIG_AD, 0) & 0x0FFF;
}

static uint32_t get_adc_x(void)
{
    GPIO_T *PORT;

    /*=== Get X from ADC input ===*/
    PORT    = (GPIO_T *)(GPIOA_BASE + (NU_GET_PORT(CONFIG_AD_PIN_XR) * PORT_OFFSET));
    GPIO_SetMode(PORT, NU_GET_PIN_MASK(NU_GET_PIN(CONFIG_AD_PIN_XR)), GPIO_MODE_OUTPUT);

    PORT    = (GPIO_T *)(GPIOA_BASE + (NU_GET_PORT(CONFIG_AD_PIN_YD) * PORT_OFFSET));
    GPIO_SetMode(PORT, NU_GET_PIN_MASK(NU_GET_PIN(CONFIG_AD_PIN_YD)), GPIO_MODE_INPUT);

    PORT    = (GPIO_T *)(GPIOA_BASE + (NU_GET_PORT(CONFIG_AD_PIN_XL) * PORT_OFFSET));
    GPIO_SetMode(PORT, NU_GET_PIN_MASK(NU_GET_PIN(CONFIG_AD_PIN_XL)), GPIO_MODE_OUTPUT);

    GPIO_PIN_DATA(NU_GET_PORT(CONFIG_AD_PIN_XR), NU_GET_PIN(CONFIG_AD_PIN_XR)) = 1;
    GPIO_PIN_DATA(NU_GET_PORT(CONFIG_AD_PIN_XL), NU_GET_PIN(CONFIG_AD_PIN_XL)) = 0;

    tp_switch_to_digital(CONFIG_AD_PIN_XR);
    tp_switch_to_digital(CONFIG_AD_PIN_YD);
    tp_switch_to_digital(CONFIG_AD_PIN_XL);

    /* Disable the digital input path to avoid the leakage current. */
    /* Configure the ADC analog input pins.  */
    tp_switch_to_analog(CONFIG_AD_PIN_YU);

    return nu_adc_sampling(NU_GET_PIN(CONFIG_AD_PIN_YU));
}

static uint32_t get_adc_y(void)
{
    GPIO_T *PORT;

    /*=== Get Y from ADC input ===*/
    PORT    = (GPIO_T *)(GPIOA_BASE + (NU_GET_PORT(CONFIG_AD_PIN_YU) * PORT_OFFSET));
    GPIO_SetMode(PORT, NU_GET_PIN_MASK(NU_GET_PIN(CONFIG_AD_PIN_YU)), GPIO_MODE_OUTPUT);

    PORT    = (GPIO_T *)(GPIOA_BASE + (NU_GET_PORT(CONFIG_AD_PIN_YD) * PORT_OFFSET));
    GPIO_SetMode(PORT, NU_GET_PIN_MASK(NU_GET_PIN(CONFIG_AD_PIN_YD)), GPIO_MODE_OUTPUT);

    PORT    = (GPIO_T *)(GPIOA_BASE + (NU_GET_PORT(CONFIG_AD_PIN_XL) * PORT_OFFSET));
    GPIO_SetMode(PORT, NU_GET_PIN_MASK(NU_GET_PIN(CONFIG_AD_PIN_XL)), GPIO_MODE_INPUT);

    GPIO_PIN_DATA(NU_GET_PORT(CONFIG_AD_PIN_YU), NU_GET_PIN(CONFIG_AD_PIN_YU)) = 1;
    GPIO_PIN_DATA(NU_GET_PORT(CONFIG_AD_PIN_YD), NU_GET_PIN(CONFIG_AD_PIN_YD)) = 0;

    tp_switch_to_digital(CONFIG_AD_PIN_YU);
    tp_switch_to_digital(CONFIG_AD_PIN_YD);
    tp_switch_to_digital(CONFIG_AD_PIN_XL);

    /* Disable the digital input path to avoid the leakage current. */
    /* Configure the ADC analog input pins.  */
    tp_switch_to_analog(CONFIG_AD_PIN_XR);

    return nu_adc_sampling(NU_GET_PIN(CONFIG_AD_PIN_XR));
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
    ad_touch_calibrate();

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
    adc_x  = get_adc_x();
    adc_y  = get_adc_y();
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
