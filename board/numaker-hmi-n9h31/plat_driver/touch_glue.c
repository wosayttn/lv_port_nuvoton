/**************************************************************************//**
 * @file     touch_glue.c
 * @brief    NuMaker Display Driver glue code for EBI 8080 interface LCD panel
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include "NuMicro.h"
#include "numaker_touch.h"
#include "touch_adc.h"
#include "plat_touch.h"

#if defined(__FREERTOS__)
    #include "FreeRTOS.h"
    #include "task.h"
    #include "semphr.h"
#endif

#if defined(__800x480__)

#define USE_DISP_PANEL      5

#if defined(CONFIG_INDEV_TOUCH_ADC)
#if (USE_DISP_PANEL==5)
S_CALIBRATION_MATRIX g_sCalMat = { 13742, -81, -1906400, -103, -8624, 33161548, 65536  };
#elif (USE_DISP_PANEL==7)
S_CALIBRATION_MATRIX g_sCalMat = { 13321, -53, -1069280, 96, 8461, -1863312, 65536 };
#endif
#endif

#endif

#if defined(CONFIG_INDEV_TOUCH_PIN_IRQ)

static volatile numaker_indev_data_t s_sInDevData = {0};
static volatile uint32_t s_u32LastIRQ = 0;

//  ISR
static INT32 Touch_ISR(UINT32 status, UINT32 userData)
{
    uint32_t PORT = (NU_GET_PORT(CONFIG_INDEV_TOUCH_PIN_IRQ) * PORT_OFFSET);
    uint32_t PIN_MASK = NU_GET_PIN_MASK(NU_GET_PIN(CONFIG_INDEV_TOUCH_PIN_IRQ));

    /* To check if PC.5 interrupt occurred */
    if (status & PIN_MASK)
    {
        /* In ISR Callback. */
        s_u32LastIRQ = xTaskGetTickCountFromISR();
    }

    GPIO_ClrISRBit(PORT, status);
}
#endif

int touchpad_device_initialize(void)
{
#if defined(CONFIG_INDEV_TOUCH_I2C)
    uint32_t PORT;

    /* Set GPIO OUTPUT mode for indev touch pins. */
    PORT    =  NU_GET_PORT(CONFIG_INDEV_TOUCH_PIN_RESET) * PORT_OFFSET;
    sysprintf("PORT: %08x, %d\n", PORT, NU_GET_PORT(CONFIG_INDEV_TOUCH_PIN_RESET));

    GPIO_OpenBit(PORT, NU_GET_PIN_MASK(NU_GET_PIN(CONFIG_INDEV_TOUCH_PIN_RESET)), DIR_OUTPUT, NO_PULL_UP);
    sysprintf("PORT: %08x, %d\n", PORT, NU_GET_PIN_MASK(NU_GET_PIN(CONFIG_INDEV_TOUCH_PIN_RESET)));

#if defined(CONFIG_INDEV_TOUCH_PIN_IRQ)
    /* Set GPIO INTPUT mode for indev touch pins. */
    PORT    =  NU_GET_PORT(CONFIG_INDEV_TOUCH_PIN_IRQ) * PORT_OFFSET;
    sysprintf("PORT: %08x, %d\n", PORT, NU_GET_PIN_MASK(NU_GET_PIN(CONFIG_INDEV_TOUCH_PIN_IRQ)));

#if defined(CONFIG_INDEV_TOUCH_GT911)

    GPIO_OpenBit(PORT, NU_GET_PIN_MASK(NU_GET_PIN(CONFIG_INDEV_TOUCH_PIN_IRQ)), DIR_OUTPUT, NO_PULL_UP);
    INDEV_TOUCH_CLR_IRQ; // IRQ - Pull low
    INDEV_TOUCH_SET_RST; // Reset - Pull low
    sysDelay(1);
    INDEV_TOUCH_SET_IRQ; // IRQ - Pull high
    sysDelay(1);
    INDEV_TOUCH_CLR_RST; // Reset - Pull high
    sysDelay(10);
    INDEV_TOUCH_CLR_IRQ; // IRQ - Pull low
    sysDelay(60);
    GPIO_CloseBit(PORT, NU_GET_PIN_MASK(NU_GET_PIN(CONFIG_INDEV_TOUCH_PIN_IRQ)));

    GPIO_OpenBit(PORT, NU_GET_PIN_MASK(NU_GET_PIN(CONFIG_INDEV_TOUCH_PIN_IRQ)), DIR_INPUT, NO_PULL_UP);
    GPIO_EnableTriggerType(PORT, NU_GET_PIN_MASK(NU_GET_PIN(CONFIG_INDEV_TOUCH_PIN_IRQ)), RISING);
    GPIO_EnableInt(PORT, (GPIO_CALLBACK)Touch_ISR, 0);

#else
    GPIO_OpenBit(PORT, NU_GET_PIN_MASK(NU_GET_PIN(CONFIG_INDEV_TOUCH_PIN_IRQ)), DIR_INPUT, NO_PULL_UP);
    GPIO_EnableTriggerType(PORT, NU_GET_PIN_MASK(NU_GET_PIN(CONFIG_INDEV_TOUCH_PIN_IRQ)), FALLING);
    GPIO_EnableInt(PORT, (GPIO_CALLBACK)Touch_ISR, 0);
#endif

#endif

    return indev_touch_init();

#else

    return 0;

#endif
}

#if defined(CONFIG_INDEV_TOUCH_ADC)
static volatile bool s_bPenDown = false;
int32_t PenDownCallback(UINT32 status, UINT32 userData)
{
    if (!s_bPenDown)
        adcIoctl(PEPOWER_OFF, 0, 0);

    s_bPenDown = true;

    return 0;
}
#endif

int touchpad_device_open(void)
{
#if defined(CONFIG_INDEV_TOUCH_I2C)

#elif defined(CONFIG_INDEV_TOUCH_ADC)

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

#endif

    return 0;
}

int touchpad_device_read(numaker_indev_data_t *psInDevData)
{

#if defined(CONFIG_INDEV_TOUCH_I2C)

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
    // polling
    indev_touch_get_data(psInDevData);
#endif

#elif defined(CONFIG_INDEV_TOUCH_ADC)

#define ADC_TOUCH_Z0_ACTIVE 20
    static numaker_indev_data_t sLastInDevData = {0};

    int32_t adc_x, adc_y, z0, z1;

    adc_x = adc_y = z0 = z1 = 0;

    psInDevData->state = NUMAKER_INDEV_STATE_RELEASED;

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
            psInDevData->state = NUMAKER_INDEV_STATE_PRESSED;
        }
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
    else
    {
        psInDevData->point.x = sLastInDevData.point.x;
        psInDevData->point.y = sLastInDevData.point.y;
    }
#endif

    return (psInDevData->state == NUMAKER_INDEV_STATE_PRESSED) ? 1 : 0;
}

int touchpad_device_control(int cmd, void *argv)
{
    return 0;
}

void touchpad_device_close(void)
{
#if defined(CONFIG_INDEV_TOUCH_I2C)

#elif defined(CONFIG_INDEV_TOUCH_ADC)
    adcClose();
#endif
}

int touchpad_device_finalize(void)
{
    return 0;
}

int32_t touch_plat_i2c_init(S_TOUCH_IF_I2C *psIfCtx)
{
    int32_t ret;

    i2cInit((int32_t)psIfCtx->m_pvI2C);

    ret = i2cOpen((PVOID)psIfCtx->m_pvI2C);
    if (ret < 0)
    {
        sysprintf("Failed to initial I2C device.\n");
        return -1;
    }

    ret = i2cIoctl((int32_t)psIfCtx->m_pvI2C, I2C_IOC_SET_SPEED, 400, 0);

    return 0;
}

int32_t touch_plat_i2c_read(S_TOUCH_IF_I2C *psIfCtx)
{
    int32_t ret = -1;

    if (psIfCtx != NULL)
    {
        i2cIoctl((int32_t)psIfCtx->m_pvI2C, I2C_IOC_SET_DEV_ADDRESS, psIfCtx->m_u8DevAddr, 0);

        switch (psIfCtx->m_u32RegLen)
        {
        case 1:
            i2cIoctl((int32_t)psIfCtx->m_pvI2C, I2C_IOC_SET_SUB_ADDRESS, (uint32_t)(*(uint8_t *)psIfCtx->m_pu8Reg), psIfCtx->m_u32RegLen);
            break;
        case 2:
            i2cIoctl((int32_t)psIfCtx->m_pvI2C, I2C_IOC_SET_SUB_ADDRESS, (uint32_t)(*(uint16_t *)psIfCtx->m_pu8Reg), psIfCtx->m_u32RegLen);
            break;
        }

        ret = (i2cRead((int32_t)psIfCtx->m_pvI2C,
                       psIfCtx->m_pu8Data,
                       psIfCtx->m_u32DataLen) == (int32_t)psIfCtx->m_u32DataLen) ? 0 : -1;
    }

    return ret;
}

int32_t touch_plat_i2c_write(S_TOUCH_IF_I2C *psIfCtx)
{
    int32_t ret = -1;

    if (psIfCtx != NULL)
    {
        i2cIoctl((int32_t)psIfCtx->m_pvI2C, I2C_IOC_SET_DEV_ADDRESS, psIfCtx->m_u8DevAddr, 0);

        switch (psIfCtx->m_u32RegLen)
        {
        case 1:
            i2cIoctl((int32_t)psIfCtx->m_pvI2C, I2C_IOC_SET_SUB_ADDRESS, (uint32_t)(*(uint8_t *)psIfCtx->m_pu8Reg), psIfCtx->m_u32RegLen);
            break;
        case 2:
            i2cIoctl((int32_t)psIfCtx->m_pvI2C, I2C_IOC_SET_SUB_ADDRESS, (uint32_t)(*(uint16_t *)psIfCtx->m_pu8Reg), psIfCtx->m_u32RegLen);
            break;
        }

        ret = (i2cWrite((int32_t)psIfCtx->m_pvI2C,
                        psIfCtx->m_pu8Data,
                        psIfCtx->m_u32DataLen) == (int32_t)psIfCtx->m_u32DataLen) ? 0 : -1;
    }

    return ret;
}

void touch_plat_i2c_fini(S_TOUCH_IF_I2C *psIfCtx)
{
    i2cClose((int32_t)psIfCtx->m_pvI2C);
}
