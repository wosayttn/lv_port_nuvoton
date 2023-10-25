/**************************************************************************//**
 * @file     lv_glue.c
 * @brief    lvgl glue for MA35D1 series
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "lvgl.h"
#include "lv_glue.h"


#define CONFIG_VRAM_TOTAL_ALLOCATED_SIZE    NVT_ALIGN((LV_HOR_RES_MAX * LV_VER_RES_MAX * sizeof(lv_color_t) * CONFIG_LCD_FB_NUM), 64)

static uint8_t s_au8FrameBuf[CONFIG_VRAM_TOTAL_ALLOCATED_SIZE] __attribute__((aligned(128)));

#if defined(__1024x600__)
S_CALIBRATION_MATRIX g_sCalMat = { -17558, 1, 69298832, -10, 11142, -2549195, 65536 };
#endif

/* LCD attributes 1024x600 */
#if defined(__1024x600__)
const static DISP_LCD_INFO LcdPanelInfo =
{
    /* Panel Resolution */
    1024,
    600,
    /* DISP_LCD_TIMING */
    {
        51000000,
        1024,
        1,
        160,
        160,
        600,
        1,
        23,
        12,
        ePolarity_Positive,
        ePolarity_Positive
    },
    /* DISP_PANEL_CONF */
    {
        eDPIFmt_D24,
        ePolarity_Positive,
        ePolarity_Positive,
        ePolarity_Positive
    },
};
#endif

void sysDelay(uint32_t ms)
{
    volatile uint32_t tgtTicks = msTicks0 + ms;
    while (msTicks0 < tgtTicks);
}
uint32_t sysGetCurrentTime(void)
{
    return msTicks0;
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

#if (CONFIG_LV_DISP_FULL_REFRESH==1)
static volatile uint32_t s_vu32Displayblank = 0;
static void lcd_disp_handler(void)
{
    s_vu32Displayblank++;
}
#endif

int lcd_device_initialize(void)
{
    /* Set EPLL/2 as DISP Core Clock source */
    DISP_EnableDCUClk();

    /* Waiting EPLL ready */
    CLK_WaitClockReady(CLK_STATUS_STABLE_EPLL);

    /* Enable DISP Core Clock */
    CLK_EnableModuleClock(DCU_MODULE);

    /* Select DISP pixel clock source to VPLL */
    DISP_GeneratePixelClk(LcdPanelInfo.sLcdTiming.u32PCF);

    /* Select DISP pixel clock source */
    CLK_SetModuleClock(DCUP_MODULE, CLK_CLKSEL0_DCUSEL_EPLL_DIV2, 0);

    /* Set multi-function pins for LCD Display Controller */
    SYS->GPG_MFPH = SYS->GPG_MFPH & ~(SYS_GPG_MFPH_PG8MFP_Msk | SYS_GPG_MFPH_PG9MFP_Msk | SYS_GPG_MFPH_PG10MFP_Msk) |
                    (SYS_GPG_MFPH_PG8MFP_LCM_VSYNC | SYS_GPG_MFPH_PG9MFP_LCM_HSYNC | SYS_GPG_MFPH_PG10MFP_LCM_CLK);
    SYS->GPK_MFPL = SYS->GPK_MFPL & ~(SYS_GPK_MFPL_PK4MFP_Msk) | SYS_GPK_MFPL_PK4MFP_LCM_DEN;

    SYS->GPI_MFPH = SYS->GPI_MFPH & ~(SYS_GPI_MFPH_PI8MFP_Msk | SYS_GPI_MFPH_PI9MFP_Msk | SYS_GPI_MFPH_PI10MFP_Msk | SYS_GPI_MFPH_PI11MFP_Msk |
                                      SYS_GPI_MFPH_PI12MFP_Msk | SYS_GPI_MFPH_PI13MFP_Msk | SYS_GPI_MFPH_PI14MFP_Msk | SYS_GPI_MFPH_PI15MFP_Msk);
    SYS->GPI_MFPH |= (SYS_GPI_MFPH_PI8MFP_LCM_DATA0 | SYS_GPI_MFPH_PI9MFP_LCM_DATA1 | SYS_GPI_MFPH_PI10MFP_LCM_DATA2 |
                      SYS_GPI_MFPH_PI11MFP_LCM_DATA3 | SYS_GPI_MFPH_PI12MFP_LCM_DATA4 | SYS_GPI_MFPH_PI13MFP_LCM_DATA5 | SYS_GPI_MFPH_PI14MFP_LCM_DATA6 | SYS_GPI_MFPH_PI15MFP_LCM_DATA7);
    SYS->GPH_MFPL = SYS->GPH_MFPL & ~(SYS_GPH_MFPL_PH0MFP_Msk | SYS_GPH_MFPL_PH1MFP_Msk | SYS_GPH_MFPL_PH2MFP_Msk | SYS_GPH_MFPL_PH3MFP_Msk |
                                      SYS_GPH_MFPL_PH4MFP_Msk | SYS_GPH_MFPL_PH5MFP_Msk | SYS_GPH_MFPL_PH6MFP_Msk | SYS_GPH_MFPL_PH7MFP_Msk);
    SYS->GPH_MFPL |= (SYS_GPH_MFPL_PH0MFP_LCM_DATA8 | SYS_GPH_MFPL_PH1MFP_LCM_DATA9 | SYS_GPH_MFPL_PH2MFP_LCM_DATA10 | SYS_GPH_MFPL_PH3MFP_LCM_DATA11 |
                      SYS_GPH_MFPL_PH4MFP_LCM_DATA12 | SYS_GPH_MFPL_PH5MFP_LCM_DATA13 | SYS_GPH_MFPL_PH6MFP_LCM_DATA14 | SYS_GPH_MFPL_PH7MFP_LCM_DATA15);

    SYS->GPC_MFPH = SYS->GPC_MFPH & ~(SYS_GPC_MFPH_PC12MFP_Msk | SYS_GPC_MFPH_PC13MFP_Msk | SYS_GPC_MFPH_PC14MFP_Msk | SYS_GPC_MFPH_PC15MFP_Msk);
    SYS->GPC_MFPH |= (SYS_GPC_MFPH_PC12MFP_LCM_DATA16 | SYS_GPC_MFPH_PC13MFP_LCM_DATA17 | SYS_GPC_MFPH_PC14MFP_LCM_DATA18 | SYS_GPC_MFPH_PC15MFP_LCM_DATA19);
    SYS->GPH_MFPH = SYS->GPH_MFPH & ~(SYS_GPH_MFPH_PH12MFP_Msk | SYS_GPH_MFPH_PH13MFP_Msk | SYS_GPH_MFPH_PH14MFP_Msk | SYS_GPH_MFPH_PH15MFP_Msk);
    SYS->GPH_MFPH |= (SYS_GPH_MFPH_PH12MFP_LCM_DATA20 | SYS_GPH_MFPH_PH13MFP_LCM_DATA21 | SYS_GPH_MFPH_PH14MFP_LCM_DATA22 | SYS_GPH_MFPH_PH15MFP_LCM_DATA23);

    /* Assign the highest AXI port priority to Display */
    DISPLIB_DDR_AXIPort_Priority();

    /* Configure display attributes of LCD panel */
    LV_ASSERT(DISPLIB_LCDInit(LcdPanelInfo) == 0);

    /* Configure DISP Framebuffer settings  */
    LV_ASSERT(DISPLIB_SetFBConfig(
#if (LV_COLOR_DEPTH==32)
                  eFBFmt_A8R8G8B8,
#else
                  eFBFmt_RGB565,
#endif
                  LV_HOR_RES_MAX,
                  LV_VER_RES_MAX,
                  ptr_to_u32(s_au8FrameBuf)) == 0);

#if (CONFIG_LV_DISP_FULL_REFRESH==1)
    IRQ_SetHandler((IRQn_ID_t)DISP_IRQn, lcd_disp_handler);
    IRQ_Enable((IRQn_ID_t)DISP_IRQn);
#endif

    return 0;
}

int lcd_device_open(void)
{
    /* Start to display */
    LV_ASSERT(DISPLIB_EnableOutput(eLayer_Video) == 0);

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
        psLCDInfo->evLCDType = evLCD_TYPE_SYNC;
    }
    break;

    case evLCD_CTRL_PAN_DISPLAY:
    {
        LV_ASSERT(argv != NULL);
        LV_ASSERT(DISPLIB_SetFBAddr(ptr_to_u32(argv)) == 0);
    }
    break;

#if (CONFIG_LV_DISP_FULL_REFRESH==1)
    case evLCD_CTRL_WAIT_VSYNC:
    {
        volatile uint32_t next = s_vu32Displayblank + 1;
        while (s_vu32Displayblank < next)
        {
            //Wait next blank coming;
        }
    }
    break;
#endif

    case evLCD_CTRL_RECT_UPDATE:
    {
        dcache_clean_by_mva(s_au8FrameBuf, CONFIG_VRAM_TOTAL_ALLOCATED_SIZE);
    }
    break;

    default:
        LV_ASSERT(0);
    }

    return 0;
}

void lcd_device_close(void)
{
    /* Stop to display */
    LV_ASSERT(DISPLIB_DisableOutput(eLayer_Video) == 0);
}

int lcd_device_finalize(void)
{
#if (CONFIG_LV_DISP_FULL_REFRESH==1)
    IRQ_Disable((IRQn_ID_t)DISP_IRQn);
#endif

    return 0;
}

#define Z_TH    20
#define CONFIG_TRIGGER_PERIOD   20    //in ms
#define adc_touch_antiglitch()  { sysDelay(1); ADC0->ISR=ADC0->ISR; }

typedef enum
{
    evADT_Idle,
    evADT_PDDetect,  //1
    evADT_PD_GOTO_XY, //2
    evADT_XYConvert_Trigger, //3
    evADT_XYConvert_Done,
    evADT_Cnt
} E_ADT_Mode;

static volatile E_ADT_Mode s_evADTMode = evADT_Idle;

static void ADC0_IRQHandler(void)
{
    uint32_t u32ISR = ADC0->ISR;
    if ((u32ISR & ADC_ISR_PEDEF_Msk) == ADC_ISR_PEDEF_Msk)
    {
        // Clear interrupt flag
        ADC_CLR_INT_FLAG(ADC0, ADC_ISR_PEDEF_Msk);
        ADC_DisableInt(ADC0, ADC_IER_PEDEIEN_Msk);

        s_evADTMode = evADT_PD_GOTO_XY;
    }
    else if ((u32ISR & ADC_ISR_MF_Msk) == ADC_ISR_MF_Msk)
    {
        // Clear interrupt flag
        ADC_CLR_INT_FLAG(ADC0, ADC_ISR_MF_Msk);
        s_evADTMode = evADT_XYConvert_Done;
    }
}


int touchpad_device_initialize(void)
{
    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Enable IP clock */
    CLK_EnableModuleClock(ADC_MODULE);
    CLK_EnableModuleClock(GPB_MODULE);

    /* Select IP clock source */
    CLK_SetModuleClock(ADC_MODULE, 0, CLK_CLKDIV4_ADC(450));  // Set ADC clock rate to 400kHz

    /* Set PB.12~15 to input mode */
    PB->MODE &= ~(GPIO_MODE_MODE12_Msk | GPIO_MODE_MODE13_Msk | GPIO_MODE_MODE14_Msk | GPIO_MODE_MODE15_Msk);

    /* Set multi-function pin ADC channel 4~7 input*/
    SYS->GPB_MFPH &= ~(SYS_GPB_MFPH_PB12MFP_Msk | SYS_GPB_MFPH_PB13MFP_Msk | SYS_GPB_MFPH_PB14MFP_Msk | SYS_GPB_MFPH_PB15MFP_Msk);
    SYS->GPB_MFPH |= (SYS_GPB_MFPH_PB12MFP_ADC0_CH4 | SYS_GPB_MFPH_PB13MFP_ADC0_CH5 | SYS_GPB_MFPH_PB14MFP_ADC0_CH6 | SYS_GPB_MFPH_PB15MFP_ADC0_CH7);

    /* Disable digital input path to prevent leakage */
    GPIO_DISABLE_DIGITAL_PATH(PB, BIT12);
    GPIO_DISABLE_DIGITAL_PATH(PB, BIT13);
    GPIO_DISABLE_DIGITAL_PATH(PB, BIT14);
    GPIO_DISABLE_DIGITAL_PATH(PB, BIT15);

    return 0;
}

int touchpad_device_open(void)
{
    // Enable channel 0
    ADC_Open(ADC0, ADC_INPUT_MODE_4WIRE_TOUCH, ADC_HIGH_SPEED_MODE, ADC_CH_0_MASK);

    /* Enable ADC to high speed mode */
    ADC0->CONF |= ADC_CONF_SPEED_Msk;

    IRQ_SetHandler((IRQn_ID_t)ADC0_IRQn, ADC0_IRQHandler);
    IRQ_Enable((IRQn_ID_t)ADC0_IRQn);

    nuvoton_fs_init();

    extern int ad_touch_calibrate(void);
    //ad_touch_calibrate();

    nuvoton_fs_fini();

    return 0;
}

static void adc_touch_pendown_detect(bool bStartDetect)
{
    IRQ_Disable((IRQn_ID_t)ADC0_IRQn);

    // Power off ADC
    ADC_POWER_DOWN(ADC0);

    ADC_DisableInt(ADC0, ADC_IER_PEDEIEN_Msk | ADC_IER_MIEN_Msk);
    adc_touch_antiglitch();

    if (bStartDetect == true)
    {
        // Enable ADC pen down complete interrupt
        ADC_EnableInt(ADC0, ADC_IER_PEDEIEN_Msk);

        // Start to detect pen down event
        ADC_DETECT_PD_MODE(ADC0);
    }
    else
    {
        /* Enable interrupt */
        ADC_EnableInt(ADC0, ADC_IER_MIEN_Msk);

        /* Switch to XY coordination converting mode */
        ADC_CONVERT_XY_MODE(ADC0);
    }
    adc_touch_antiglitch();

    // Power on ADC
    ADC_POWER_ON(ADC0);

    IRQ_Enable((IRQn_ID_t)ADC0_IRQn);
}

int touchpad_device_read(lv_indev_data_t *psInDevData)
{
    static lv_indev_data_t sLastInDevData = {0};
    static uint32_t u32NextTriggerTime = 0;

    LV_ASSERT(psInDevData);

    psInDevData->state   = sLastInDevData.state;
    psInDevData->point.x = sLastInDevData.point.x;
    psInDevData->point.y = sLastInDevData.point.y;

    if (sysGetCurrentTime() < u32NextTriggerTime)
    {
        goto exit_touchpad_device_read;
    }

    psInDevData->state = LV_INDEV_STATE_RELEASED;
    switch (s_evADTMode)
    {
    case evADT_PD_GOTO_XY:
    {
        adc_touch_pendown_detect(false);
        s_evADTMode = evADT_XYConvert_Trigger;
        ADC_START_CONV(ADC0);
        u32NextTriggerTime = sysGetCurrentTime() + CONFIG_TRIGGER_PERIOD;
    }
    break;

    // Convert X/Y value if touch detected
    case evADT_XYConvert_Done:
    {
        uint32_t adc_x = 0, adc_y = 0;

        // Get ADC convert result
        adc_x = ADC_GET_CONVERSION_XDATA(ADC0);
        adc_y = ADC_GET_CONVERSION_YDATA(ADC0);

        if (ADC_GET_CONVERSION_Z1DATA(ADC0) < Z_TH)
        {
            ADC_DisableInt(ADC0, ADC_IER_MIEN_Msk);
            s_evADTMode = evADT_Idle;
            adc_touch_antiglitch();
        }
        else
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
            psInDevData->state = LV_INDEV_STATE_PRESSED;

            s_evADTMode = evADT_XYConvert_Trigger;
            ADC_START_CONV(ADC0);
            u32NextTriggerTime = sysGetCurrentTime() + CONFIG_TRIGGER_PERIOD;
        }
    }
    break;

    case evADT_XYConvert_Trigger:
    {
        if (ADC_GET_CONVERSION_Z1DATA(ADC0) > Z_TH)
        {
            psInDevData->state = LV_INDEV_STATE_PRESSED;
        }
    }
    break;

    case evADT_Idle:
    {
        s_evADTMode = evADT_PDDetect;
        adc_touch_pendown_detect(true);
    }
    //no break

    default: //evADT_PDDetect
        break;

    } //switch

    sLastInDevData.state = psInDevData->state;

    //LV_LOG_INFO("<-%d, %d, (%d, %d)", s_evADTMode, psInDevData->state, psInDevData->point.x, psInDevData->point.y);

exit_touchpad_device_read:
    return (psInDevData->state == LV_INDEV_STATE_PRESSED) ? 1 : 0;
}

int touchpad_device_control(int cmd, void *argv)
{
    return 0;
}

void touchpad_device_close(void)
{
    IRQ_Disable((IRQn_ID_t)ADC0_IRQn);

    // Disable channel 0
    ADC_Close(ADC0);
}

int touchpad_device_finalize(void)
{
    /* Disable digital input path to prevent leakage */
    GPIO_ENABLE_DIGITAL_PATH(PB, BIT12);
    GPIO_ENABLE_DIGITAL_PATH(PB, BIT13);
    GPIO_ENABLE_DIGITAL_PATH(PB, BIT14);
    GPIO_ENABLE_DIGITAL_PATH(PB, BIT15);

    /* Set multi-function pin to GPIO. */
    SYS->GPB_MFPH = (SYS->GPB_MFPH & ~(SYS_GPB_MFPH_PB12MFP_Msk | SYS_GPB_MFPH_PB13MFP_Msk | SYS_GPB_MFPH_PB14MFP_Msk | SYS_GPB_MFPH_PB15MFP_Msk));

    /* Disable IP clock */
    CLK_DisableModuleClock(ADC_MODULE);

    return 0;
}
