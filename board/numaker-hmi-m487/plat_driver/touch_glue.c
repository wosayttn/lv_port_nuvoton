/**************************************************************************//**
 * @file     touch_glue.c
 * @brief    NuMaker Display Driver glue code for EBI 8080 interface LCD panel
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include <stdint.h>
#include "numaker_touch.h"
#include "touch_adc.h"
#if defined(__FREERTOS__)
    #include "FreeRTOS.h"
    #include "task.h"
    #include "semphr.h"
#endif

#if defined(CONFIG_INDEV_TOUCH_I2C)

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

static volatile numaker_indev_data_t s_sInDevData = {0};
static volatile uint32_t s_u32LastIRQ = 0;


// GPG ISR
void GPG_IRQHandler(void)
{
    GPIO_T *PORT = (GPIO_T *)(GPIOA_BASE + (NU_GET_PORT(CONFIG_INDEV_TOUCH_PIN_IRQ) * PORT_OFFSET));

    /* To check if PC.5 interrupt occurred */
    if (GPIO_GET_INT_FLAG(PORT, NU_GET_PIN_MASK(NU_GET_PIN(CONFIG_INDEV_TOUCH_PIN_IRQ))))
    {
        GPIO_CLR_INT_FLAG(PORT, NU_GET_PIN_MASK(NU_GET_PIN(CONFIG_INDEV_TOUCH_PIN_IRQ)));
#if defined(__FREERTOS__)
        s_u32LastIRQ = xTaskGetTickCount();
#else
        s_u32LastIRQ ++;
#endif
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

#if defined(CONFIG_INDEV_TOUCH_PIN_IRQ)
    /* Set GPIO INTPUT mode for indev touch pins. */
    PORT    = (GPIO_T *)(GPIOA_BASE + (NU_GET_PORT(CONFIG_INDEV_TOUCH_PIN_IRQ) * PORT_OFFSET));
    GPIO_SetMode(PORT, NU_GET_PIN_MASK(NU_GET_PIN(CONFIG_INDEV_TOUCH_PIN_IRQ)), GPIO_MODE_INPUT);
    GPIO_SetPullCtl(PORT, NU_GET_PIN_MASK(NU_GET_PIN(CONFIG_INDEV_TOUCH_PIN_IRQ)), GPIO_PUSEL_PULL_UP);
    GPIO_EnableInt(PORT, NU_GET_PIN(CONFIG_INDEV_TOUCH_PIN_IRQ), GPIO_INT_FALLING);
    NVIC_EnableIRQ(au32GPIRQ[NU_GET_PORT(CONFIG_INDEV_TOUCH_PIN_IRQ)]);
#endif

    return indev_touch_init();
}

int touchpad_device_open(void)
{
    return 0;
}

int touchpad_device_read(numaker_indev_data_t *psInDevData)
{
#if defined(CONFIG_INDEV_TOUCH_PIN_IRQ)
    static uint32_t u32LastIRQ = 0;

    if (u32LastIRQ != s_u32LastIRQ)
    {
        indev_touch_get_data((numaker_indev_data_t *)&s_sInDevData);
        u32LastIRQ = s_u32LastIRQ ;
    }

    psInDevData->point.x = s_sInDevData.point.x;
    psInDevData->point.y = s_sInDevData.point.y;
    psInDevData->state = s_sInDevData.state;
#else
    indev_touch_get_data(psInDevData);
#endif

    return (psInDevData->state == NUMAKER_INDEV_STATE_PRESSED) ? 1 : 0;
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

int32_t touch_plat_i2c_init(S_TOUCH_IF_I2C *psIfCtx)
{
    I2C_Open((I2C_T *)psIfCtx->m_pvI2C, 400000);

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

#elif defined(CONFIG_INDEV_TOUCH_AD)

#if defined(CONFIG_DISP_ILI9341) && (CONFIG_DISP_ILI9341==1)
S_CALIBRATION_MATRIX g_sCalMat = { 9, 6484, -4280144, -5183, -113, 19125360, 65536 };
#endif

uint32_t nu_adc_sampling(uint32_t channel)
{
    EADC_ConfigSampleModule(CONFIG_INDEV_TOUCH_AD, 0, EADC_SOFTWARE_TRIGGER, channel);

    EADC_CLR_INT_FLAG(CONFIG_INDEV_TOUCH_AD, EADC_STATUS2_ADIF0_Msk);

    EADC_ENABLE_INT(CONFIG_INDEV_TOUCH_AD, BIT0);

    EADC_ENABLE_SAMPLE_MODULE_INT(CONFIG_INDEV_TOUCH_AD, 0, BIT0);

    EADC_START_CONV(CONFIG_INDEV_TOUCH_AD, BIT0);

    while (EADC_GET_INT_FLAG(CONFIG_INDEV_TOUCH_AD, BIT0) == 0);

    return EADC_GET_CONV_DATA(CONFIG_INDEV_TOUCH_AD, 0) & 0x0FFF;
}

int touchpad_device_initialize(void)
{
    return 0;
}

int touchpad_device_open(void)
{
    EADC_Open(CONFIG_INDEV_TOUCH_AD, EADC_CTL_DIFFEN_SINGLE_END);

    extern int ad_touch_calibrate(void);
    //ad_touch_calibrate();

    return 0;
}

int touchpad_device_read(numaker_indev_data_t *psInDevData)
{
#define CONFIG_TRIGGER_PERIOD     16
    static uint32_t u32CurrentTime = 0;
    static numaker_indev_data_t sLastInDevData = {0};
    static uint32_t u32NextTriggerTime = 0;

    uint32_t adc_x, adc_y;

    psInDevData->state   = sLastInDevData.state;
    psInDevData->point.x = sLastInDevData.point.x;
    psInDevData->point.y = sLastInDevData.point.y;

#if defined(__FREERTOS__)
    u32CurrentTime = xTaskGetTickCount();
#else
    u32CurrentTime++;
#endif

    if (u32CurrentTime < u32NextTriggerTime)
    {
        goto exit_touchpad_device_read;
    }

    /* Get X, Y ADC converting data */
    adc_x  = indev_touch_get_x();
    adc_y  = indev_touch_get_y();
    u32NextTriggerTime = u32CurrentTime + CONFIG_TRIGGER_PERIOD;

    if ((adc_x < 3900) && (adc_y < 3900)) //~5%
    {
        psInDevData->state = NUMAKER_INDEV_STATE_PRESSED;
    }
    else
    {
        psInDevData->state = NUMAKER_INDEV_STATE_RELEASED;
    }

    if (psInDevData->state == NUMAKER_INDEV_STATE_PRESSED)
    {
        extern int ad_touch_map(int32_t *sumx, int32_t *sumy);
        if (ad_touch_map((int32_t *)&adc_x, (int32_t *)&adc_y) == 0)
        {

            psInDevData->point.x = ((int16_t)adc_x < 0) ? 0 :
                                   ((int16_t)adc_x >= DISP_HOR_RES_MAX) ? (DISP_HOR_RES_MAX - 1) :
                                   adc_x;

            psInDevData->point.y = ((int16_t)adc_y < 0) ? 0 :
                                   ((int16_t)adc_y >= DISP_VER_RES_MAX) ? (DISP_VER_RES_MAX - 1) :
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

exit_touchpad_device_read:

    return (psInDevData->state == NUMAKER_INDEV_STATE_PRESSED) ? 1 : 0;
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

#endif
