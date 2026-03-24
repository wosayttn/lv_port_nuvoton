/**************************************************************************//**
 * @file     touch_glue.c
 * @brief    NuMaker Display Driver glue code for EBI 8080 interface LCD panel
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include "numaker_touch.h"
#include "touch_adc.h"
#if defined(__FREERTOS__)
    #include "FreeRTOS.h"
    #include "task.h"
    #include "semphr.h"
#endif

#if defined(CONFIG_INDEV_TOUCH_AD)

#if defined(CONFIG_DISP_ILI9341) && (CONFIG_DISP_ILI9341==1)
S_CALIBRATION_MATRIX g_sCalMat = { 43, -5839, 21672848, 4193, -11, -747882, 65536 };
#endif

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

int touchpad_device_read(numaker_indev_data_t *psInDevData)
{
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

exit_touchpad_device_read:

    return (psInDevData->state == NUMAKER_INDEV_STATE_PRESSED) ? 1 : 0;
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

#endif
