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

#if defined(CONFIG_INDEV_TOUCH_AD)

#if defined(CONFIG_DISP_ILI9341) && (CONFIG_DISP_ILI9341==1)
S_CALIBRATION_MATRIX g_sCalMat = { -105, 6354, -3362552, 5086, -24, -2489744, 65536 };
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
