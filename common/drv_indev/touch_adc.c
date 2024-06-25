/**************************************************************************//**
 * @file     indev_touch_adc.c
 * @brief    AD touch driver
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "touch_adc.h"

#if defined(CONFIG_NG_MFP)

#define NU_MFP_POS(PIN)   ((PIN % 4) * 8)
#define NU_MFP_MSK(PIN)   (0x1ful << NU_MFP_POS(PIN))

void nu_pin_func(uint32_t pin, int data)
{
    uint32_t GPx_MFPx_org;
    uint32_t pin_index      = NU_GET_PIN(pin);
    uint32_t port_index     = NU_GET_PORT(pin);
    __IO uint32_t *GPx_MFPx = ((__IO uint32_t *) &SYS->GPA_MFP0 + (port_index * 4) + (pin_index / 4));
    uint32_t MFP_Msk        = NU_MFP_MSK(pin_index);

    GPx_MFPx_org = *GPx_MFPx;
    *GPx_MFPx    = (GPx_MFPx_org & (~MFP_Msk)) | data;
}

#else

#define NU_MFP_POS(PIN)                ((PIN % 8) * 4)
#define NU_MFP_MSK(PIN)                (0xful << NU_MFP_POS(PIN))

static void nu_pin_func(uint32_t pin, int data)
{
    uint32_t pin_index      = NU_GET_PIN(pin);
    uint32_t port_index     = NU_GET_PORT(pin);
    __IO uint32_t *GPx_MFPx = ((__IO uint32_t *) &SYS->GPA_MFPL + (port_index * 2) + (pin_index / 8));
    uint32_t MFP_Msk        = NU_MFP_MSK(pin_index);

    *GPx_MFPx  = (*GPx_MFPx & (~MFP_Msk)) | data;
}

#endif

static void tp_switch_to_analog(uint32_t pin)
{
    GPIO_T *port = (GPIO_T *)(GPIOA_BASE + PORT_OFFSET * NU_GET_PORT(pin));

    nu_pin_func(pin, (1 << NU_MFP_POS(NU_GET_PIN(pin))));

    /* Disable the digital input path to avoid the leakage current. */
    /* Disable digital path on these ADC pin */
    GPIO_DISABLE_DIGITAL_PATH(port, NU_GET_PIN_MASK(NU_GET_PIN(pin)));
}

static void tp_switch_to_digital(uint32_t pin)
{
    GPIO_T *port = (GPIO_T *)(GPIOA_BASE + PORT_OFFSET * NU_GET_PORT(pin));

    nu_pin_func(pin, 0);

    /* Enable digital path on these ADC pin */
    GPIO_ENABLE_DIGITAL_PATH(port, NU_GET_PIN_MASK(NU_GET_PIN(pin)));
}

#if defined (__GNUC__)
    __attribute__((weak)) uint32_t nu_adc_sampling(uint32_t channel)
#else
    __weak uint32_t nu_adc_sampling(uint32_t channel)
#endif
{
    LV_LOG_ERROR("Implement your ad sampling routine in lv_glue.c");
    return 0;
}

uint32_t indev_touch_get_x(void)
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

    /* Configure the digital input pins.  */
    tp_switch_to_digital(CONFIG_AD_PIN_XR);
    tp_switch_to_digital(CONFIG_AD_PIN_YD);
    tp_switch_to_digital(CONFIG_AD_PIN_XL);

    /* Configure the ADC analog input pins.  */
    tp_switch_to_analog(CONFIG_AD_PIN_YU);

    return nu_adc_sampling(NU_GET_PIN(CONFIG_AD_PIN_YU));
}

uint32_t indev_touch_get_y(void)
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

    /* Configure the digital input pins.  */
    tp_switch_to_digital(CONFIG_AD_PIN_YU);
    tp_switch_to_digital(CONFIG_AD_PIN_YD);
    tp_switch_to_digital(CONFIG_AD_PIN_XL);

    /* Configure the ADC analog input pins.  */
    tp_switch_to_analog(CONFIG_AD_PIN_XR);

    return nu_adc_sampling(NU_GET_PIN(CONFIG_AD_PIN_XR));
}
