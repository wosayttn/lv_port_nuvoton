/**************************************************************************//**
 * @file     lv_glue.c
 * @brief    lvgl glue for NUC980 series
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "lvgl.h"
#include "lv_glue.h"
#include "disp.h"
#include "touch_adc.h"


#define CONFIG_VRAM_TOTAL_ALLOCATED_SIZE    NVT_ALIGN((LV_HOR_RES_MAX * CONFIG_DISP_LINE_BUFFER_NUMBER * (LV_COLOR_DEPTH/8)), DEF_CACHE_LINE_SIZE)

static uint8_t s_au8FrameBuf[CONFIG_VRAM_TOTAL_ALLOCATED_SIZE] __attribute__((aligned(DEF_CACHE_LINE_SIZE)));

#if defined(__320x240__)
S_CALIBRATION_MATRIX g_sCalMat = { 43, -5839, 21672848, 4193, -11, -747882, 65536 };
#endif

void sysDelay(uint32_t ms)
{
    vTaskDelay(ms / portTICK_PERIOD_MS);
}

void nu_dump_clocks(void)
{
    LV_LOG_INFO("SYS_UPLL = %d MHz", sysGetClock(SYS_UPLL));
    LV_LOG_INFO("SYS_APLL = %d MHz", sysGetClock(SYS_APLL));
    LV_LOG_INFO("SYS_SYSTEM = %d MHz", sysGetClock(SYS_SYSTEM));
    LV_LOG_INFO("SYS_HCLK = %d MHz", sysGetClock(SYS_HCLK));
    LV_LOG_INFO("SYS_PCLK01 = %d MHz", sysGetClock(SYS_PCLK01));
    LV_LOG_INFO("SYS_PCLK2 = %d MHz", sysGetClock(SYS_PCLK2));
    LV_LOG_INFO("SYS_CPU = %d MHz", sysGetClock(SYS_CPU));

    LV_LOG_INFO("CLK_HCLKEN = %08X", inpw(REG_CLK_HCLKEN));
    LV_LOG_INFO("CLK_PCLKEN0 = %08X", inpw(REG_CLK_PCLKEN0));
    LV_LOG_INFO("CLK_PCLKEN1 = %08X", inpw(REG_CLK_PCLKEN1));

    LV_LOG_INFO("AIC_INTMSK0 = %08X", inpw(REG_AIC_INTMSK0));
    LV_LOG_INFO("AIC_INTMSK1 = %08X", inpw(REG_AIC_INTMSK1));

    LV_LOG_INFO("AIC_INTEN0 = %08X", inpw(REG_AIC_INTEN0));
    LV_LOG_INFO("AIC_INTEN1 = %08X", inpw(REG_AIC_INTEN1));

    LV_LOG_INFO("AIC_INTDIS0 = %08X", inpw(REG_AIC_INTDIS0));
    LV_LOG_INFO("AIC_INTDIS1 = %08X", inpw(REG_AIC_INTDIS1));
}

void nu_sys_ip_reset(E_SYS_IPRST eIPRstIdx)
{
    uint32_t volatile u32IPRSTRegAddr;
    uint32_t u32IPRSTRegBit;

    if (eIPRstIdx >= SYS_IPRST_CNT)
        return;

    u32IPRSTRegAddr = REG_SYS_AHBIPRST + (4ul * (eIPRstIdx / 32));
    u32IPRSTRegBit  = eIPRstIdx % 32;

    /* Unlock write-protect */
    SYS_UnlockReg();

    /* Enable IP reset */
    outpw(u32IPRSTRegAddr, inpw(u32IPRSTRegAddr) | (1 << u32IPRSTRegBit));

    /* Disable IP reset */
    outpw(u32IPRSTRegAddr, inpw(u32IPRSTRegAddr) & ~(1 << u32IPRSTRegBit));

    /* Wait it done. */
    while (inpw(u32IPRSTRegAddr) & (1 << u32IPRSTRegBit)) {}

    /* Lock write protect */
    //SYS_LockReg();
}

static void _nu_sys_ipclk(E_SYS_IPCLK eIPClkIdx, uint32_t bEnable)
{
    uint32_t volatile u32IPCLKRegAddr;
    uint32_t u32IPCLKRegBit;

    if (eIPClkIdx >= SYS_IPCLK_CNT)
        return;

    u32IPCLKRegAddr = REG_CLK_HCLKEN + (4ul * (eIPClkIdx / 32));
    u32IPCLKRegBit  = eIPClkIdx % 32;

    SYS_UnlockReg();

    if (bEnable)
    {
        /* Enable IP CLK */
        outpw(u32IPCLKRegAddr, inpw(u32IPCLKRegAddr) | (1 << u32IPCLKRegBit));
    }
    else
    {
        /* Disable IP CLK */
        outpw(u32IPCLKRegAddr, inpw(u32IPCLKRegAddr) & ~(1 << u32IPCLKRegBit));
    }

    //SYS_LockReg();
}


void nu_sys_ipclk_enable(E_SYS_IPCLK eIPClkIdx)
{
    _nu_sys_ipclk(eIPClkIdx, 1);
}

void nu_sys_ipclk_disable(E_SYS_IPCLK eIPClkIdx)
{
    _nu_sys_ipclk(eIPClkIdx, 0);
}

int lcd_device_initialize(void)
{
    GPIO_T *PORT;

    /* Set GPIO Output mode for ILI9341 pins. */
    PORT    = (GPIO_T *)(PA_BA + (NU_GET_PORT(CONFIG_DISP_PIN_DC) * PORT_OFFSET));
    GPIO_SetMode(PORT, NU_GET_PIN_MASK(NU_GET_PIN(CONFIG_DISP_PIN_DC)), GPIO_MODE_OUTPUT);

    PORT    = (GPIO_T *)(PA_BA + (NU_GET_PORT(CONFIG_DISP_PIN_RESET) * PORT_OFFSET));
    GPIO_SetMode(PORT, NU_GET_PIN_MASK(NU_GET_PIN(CONFIG_DISP_PIN_RESET)), GPIO_MODE_OUTPUT);

    PORT    = (GPIO_T *)(PA_BA + (NU_GET_PORT(CONFIG_DISP_PIN_BACKLIGHT) * PORT_OFFSET));
    GPIO_SetMode(PORT, NU_GET_PIN_MASK(NU_GET_PIN(CONFIG_DISP_PIN_BACKLIGHT)), GPIO_MODE_OUTPUT);

    /* Open SPI */
    SPI_Open(CONFIG_DISP_SPI, SPI_MASTER, SPI_MODE_0, 8, CONFIG_DISP_SPI_CLOCK);

    /* Set sequence to MSB first */
    SPI_SET_MSB_FIRST(CONFIG_DISP_SPI);

    /* Set CS pin to HIGH */
    SPI_SET_SS_HIGH(CONFIG_DISP_SPI);

    /* Set sequence to MSB first */
    SPI_SET_MSB_FIRST(CONFIG_DISP_SPI);

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

int touchpad_device_initialize(void)
{
    GPIO_SetMode(PB, BIT4 | BIT5 | BIT6 | BIT7, GPIO_MODE_INPUT);
    GPIO_DISABLE_DIGITAL_PATH(PB, BIT4 | BIT5 | BIT6 | BIT7);

    return 0;
}

static volatile bool s_bPenDown = false;
int32_t PenDownCallback(UINT32 status, UINT32 userData)
{
    if (!s_bPenDown)
        adcIoctl(PEPOWER_OFF, 0, 0);

    s_bPenDown = true;

    return 0;
}

int touchpad_device_open(void)
{
    adcOpen();

    /* ADC Sample Counter - Set the counter value to extend the ADC start signal period to get more sampling time for precise conversion. */
    outpw(REG_ADC_CONF, (inpw(REG_ADC_CONF) & ~(0xfful << 24)) | 0xfful << 24);

    adcIoctl(T_ON, 0, 0);
    adcIoctl(Z_ON, 0, 0);
    adcIoctl(PEDEF_ON, (UINT32)PenDownCallback, 0);

    s_bPenDown = false;
    adcIoctl(PEPOWER_ON, 0, 0);

    extern int ad_touch_calibrate(void);
    //ad_touch_calibrate();

    return 0;
}

int touchpad_device_read(lv_indev_data_t *psInDevData)
{
#define ADC_TOUCH_Z0_ACTIVE 20
    static lv_indev_data_t sLastInDevData = {0};

    int32_t adc_x, adc_y, z0, z1;

    LV_ASSERT(psInDevData);

    adc_x = adc_y = z0 = z1 = 0;

    psInDevData->state = LV_INDEV_STATE_RELEASED;

    if (s_bPenDown)
    {
        adcIoctl(START_MST, 0, 0);
        adcReadXY((short *)&adc_x, (short *)&adc_y, 1);
        adcReadZ((short *)&z0, (short *)&z1, 1);

        if (z0 < ADC_TOUCH_Z0_ACTIVE)
        {
            s_bPenDown = false;

            adcIoctl(PEPOWER_ON, 0, 0);
        }
        else
        {
            psInDevData->state = LV_INDEV_STATE_PRESSED;
        }
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
    else
    {
        psInDevData->point.x = sLastInDevData.point.x;
        psInDevData->point.y = sLastInDevData.point.y;
    }

    LV_LOG_INFO("%s (%d, %d)", psInDevData->state ? "Press" : "Release", psInDevData->point.x, psInDevData->point.y);

    return (psInDevData->state == LV_INDEV_STATE_PRESSED) ? 1 : 0;
}

int touchpad_device_control(int cmd, void *argv)
{
    return 0;
}

void touchpad_device_close(void)
{
    adcClose();
}

int touchpad_device_finalize(void)
{
    return 0;
}
