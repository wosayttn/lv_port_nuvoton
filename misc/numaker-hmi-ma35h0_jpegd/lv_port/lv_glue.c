/**************************************************************************//**
 * @file     lv_glue.c
 * @brief    lvgl glue for MA35H0 series
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "lvgl.h"
#include "lv_glue.h"
#include "indev_touch.h"
#include "plat_touch.h"

#define CONFIG_VRAM_TOTAL_ALLOCATED_SIZE    NVT_ALIGN((LV_HOR_RES_MAX * LV_VER_RES_MAX * (LV_COLOR_DEPTH/8) * CONFIG_LCD_FB_NUM), 128)

static uint8_t s_au8FrameBuf[CONFIG_VRAM_TOTAL_ALLOCATED_SIZE] __attribute__((aligned(128)));

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
    vTaskDelay(pdMS_TO_TICKS(ms));
}
#if (CONFIG_LV_DISP_FULL_REFRESH==1)

#define DISP_ENABLE_INT()     (DISP->DisplayIntrEnable |=  DISP_DisplayIntrEnable_DISP0_Msk)
#define DISP_GET_INTSTS()     (DISP->DisplayIntr & DISP_DisplayIntr_DISP0_Msk)

static volatile uint32_t s_vu32Displayblank = 0;
static void lcd_disp_handler(void)
{
    /* Get DISP INTSTS */
    if (DISP_GET_INTSTS())
    {
        s_vu32Displayblank++;
    }
}
#endif

int lcd_device_initialize(void)
{
    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Select DISP pixel clock source to VPLL */
    DISP_GeneratePixelClk(LcdPanelInfo.sLcdTiming.u32PCF);

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
    DISP_ENABLE_INT();
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
        psLCDInfo->u32BytePerPixel = (LV_COLOR_DEPTH / 8);
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
        while (s_vu32Displayblank <= next)
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


#if defined(CONFIG_INDEV_TOUCH_ADC)

S_CALIBRATION_MATRIX g_sCalMat = { -17558, 1, 69298832, -10, 11142, -2549195, 65536 };

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

    extern int ad_touch_calibrate(void);
    //ad_touch_calibrate();

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

    if (xTaskGetTickCount() < u32NextTriggerTime)
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
        u32NextTriggerTime = xTaskGetTickCount() + CONFIG_TRIGGER_PERIOD;
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
            u32NextTriggerTime = xTaskGetTickCount() + CONFIG_TRIGGER_PERIOD;
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

#elif defined(CONFIG_INDEV_TOUCH_I2C)

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
    GPK_IRQn,
    GPL_IRQn,
    GPM_IRQn,
    GPN_IRQn,
};

static volatile lv_indev_data_t s_sInDevData = {0};
static volatile uint32_t s_u32LastIRQ = 0;

// GT911 ISR
static void GT911_ISR(void)
{
    GPIO_T *PORT = (GPIO_T *)(GPIOA_BASE + (NU_GET_PORT(CONFIG_INDEV_TOUCH_PIN_IRQ) * PORT_OFFSET));

    /* To check if interrupt occurred */
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
    GPIO_T *PORT;

    /* Set GPIO OUTPUT mode for indev touch pins. */
    PORT    = (GPIO_T *)(GPIOA_BASE + (NU_GET_PORT(CONFIG_INDEV_TOUCH_PIN_RESET) * PORT_OFFSET));
    GPIO_SetMode(PORT, NU_GET_PIN_MASK(NU_GET_PIN(CONFIG_INDEV_TOUCH_PIN_RESET)), GPIO_MODE_OUTPUT);

#if defined(CONFIG_INDEV_TOUCH_GT911)
    /* Set GPIO INTPUT mode for indev touch pins. */
    PORT    = (GPIO_T *)(GPIOA_BASE + (NU_GET_PORT(CONFIG_INDEV_TOUCH_PIN_IRQ) * PORT_OFFSET));

    GPIO_SetMode(PORT, NU_GET_PIN_MASK(NU_GET_PIN(CONFIG_INDEV_TOUCH_PIN_IRQ)), GPIO_MODE_OUTPUT);
    INDEV_TOUCH_CLR_IRQ; // IRQ - Pull low
    INDEV_TOUCH_SET_RST; // Reset - Pull low
    sysDelay(1);
    INDEV_TOUCH_SET_IRQ; // IRQ - Pull high
    sysDelay(1);
    INDEV_TOUCH_CLR_RST; // Reset - Pull high
    sysDelay(10);
    INDEV_TOUCH_CLR_IRQ; // IRQ - Pull low
    sysDelay(60);

#endif

#if defined(CONFIG_INDEV_TOUCH_PIN_IRQ)

    PORT    = (GPIO_T *)(GPIOA_BASE + (NU_GET_PORT(CONFIG_INDEV_TOUCH_PIN_IRQ) * PORT_OFFSET));
    GPIO_SetMode(PORT, NU_GET_PIN_MASK(NU_GET_PIN(CONFIG_INDEV_TOUCH_PIN_IRQ)), GPIO_MODE_INPUT);

#if defined(CONFIG_INDEV_TOUCH_GT911)
    GPIO_SetPullCtl(PORT, NU_GET_PIN_MASK(NU_GET_PIN(CONFIG_INDEV_TOUCH_PIN_IRQ)), GPIO_PUSEL_PULL_DOWN);
    GPIO_EnableInt(PORT, NU_GET_PIN(CONFIG_INDEV_TOUCH_PIN_IRQ), GPIO_INT_RISING);
#else
    GPIO_SetPullCtl(PORT, NU_GET_PIN_MASK(NU_GET_PIN(CONFIG_INDEV_TOUCH_PIN_IRQ)), GPIO_PUSEL_PULL_UP);
    GPIO_EnableInt(PORT, NU_GET_PIN(CONFIG_INDEV_TOUCH_PIN_IRQ), GPIO_INT_FALLING);
#endif

    sysprintf("%08x\n", CONFIG_INDEV_TOUCH_PIN_IRQ);
    sysprintf("PORT > %08x\n", PORT);
    sysprintf("PIN > %08x\n", NU_GET_PIN(CONFIG_INDEV_TOUCH_PIN_IRQ));
    sysprintf("(IRQn_ID_t)au32GPIRQ[NU_GET_PORT(CONFIG_INDEV_TOUCH_PIN_IRQ)]: %08x\n", (IRQn_ID_t)au32GPIRQ[NU_GET_PORT(CONFIG_INDEV_TOUCH_PIN_IRQ)]);

    IRQ_SetHandler((IRQn_ID_t)au32GPIRQ[NU_GET_PORT(CONFIG_INDEV_TOUCH_PIN_IRQ)], GT911_ISR);
    IRQ_Enable((IRQn_ID_t)au32GPIRQ[NU_GET_PORT(CONFIG_INDEV_TOUCH_PIN_IRQ)]);

#endif

    return indev_touch_init();
}

int touchpad_device_finalize(void)
{

}

int touchpad_device_open(void)
{

}

int touchpad_device_read(lv_indev_data_t *psInDevData)
{
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
}

int touchpad_device_control(int cmd, void *argv)
{
    return 0;
}

void touchpad_device_close(void)
{
}

int32_t touch_plat_i2c_init(S_TOUCH_IF_I2C *psIfCtx)
{
    I2C_Open((I2C_T *)psIfCtx->m_pvI2C, 400000);

    LV_LOG_INFO("I2C clock %d Hz", I2C_GetBusClockFreq((I2C_T *)psIfCtx->m_pvI2C));

    return 0;
}

int32_t touch_plat_i2c_read(S_TOUCH_IF_I2C *psIfCtx)
{
    int32_t ret = -1;

    if (psIfCtx != NULL)
    {
        switch (psIfCtx->m_u32RegLen)
        {
        case 1:
            ret = (I2C_ReadMultiBytesOneReg((I2C_T *)psIfCtx->m_pvI2C,
                                            psIfCtx->m_u8DevAddr,
                                            *((uint8_t *)psIfCtx->m_pu8Reg),
                                            psIfCtx->m_pu8Data,
                                            psIfCtx->m_u32DataLen) == psIfCtx->m_u32DataLen) ? 0 : -1;
            break;

        case 2:
            ret = (I2C_ReadMultiBytesTwoRegs((I2C_T *)psIfCtx->m_pvI2C,
                                             psIfCtx->m_u8DevAddr,
                                             *((uint16_t *)psIfCtx->m_pu8Reg),
                                             psIfCtx->m_pu8Data,
                                             psIfCtx->m_u32DataLen) == psIfCtx->m_u32DataLen) ? 0 : -1;
            break;

        default:
            break;
        }
    }

    return ret;
}

int32_t touch_plat_i2c_write(S_TOUCH_IF_I2C *psIfCtx)
{
    int32_t ret = -1;

    if (psIfCtx != NULL)
    {
        switch (psIfCtx->m_u32RegLen)
        {
        case 1:
            ret = (I2C_WriteMultiBytesOneReg((I2C_T *)psIfCtx->m_pvI2C,
                                             psIfCtx->m_u8DevAddr,
                                             *((uint8_t *)psIfCtx->m_pu8Reg),
                                             psIfCtx->m_pu8Data,
                                             psIfCtx->m_u32DataLen) == psIfCtx->m_u32DataLen) ? 0 : -1;
            break;

        case 2:
            ret = (I2C_WriteMultiBytesTwoRegs((I2C_T *)psIfCtx->m_pvI2C,
                                              psIfCtx->m_u8DevAddr,
                                              *((uint16_t *)psIfCtx->m_pu8Reg),
                                              psIfCtx->m_pu8Data,
                                              psIfCtx->m_u32DataLen) == psIfCtx->m_u32DataLen) ? 0 : -1;
            break;

        default:
            break;
        }
    }

    return ret;
}

void touch_plat_i2c_fini(S_TOUCH_IF_I2C *psIfCtx)
{
    I2C_Close((I2C_T *)psIfCtx->m_pvI2C);
}

#endif
