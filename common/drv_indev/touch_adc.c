/**************************************************************************//**
 * @file     indev_touch_adc.c
 * @brief    AD touch driver
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "numaker_touch.h"
#include "numaker_disp.h"

#if defined(CONFIG_NG_MFP)

#define NU_MFP_POS(PIN)   ((PIN % 4) * 8)
#define NU_MFP_MSK(PIN)   (0x1ful << NU_MFP_POS(PIN))

static void nu_pin_func(uint32_t pin, int data)
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

/**
 * @brief Configure a GPIO pin to operate in analog mode.
 *
 * Sets a GPIO pin to analog input mode by configuring the multi-function
 * register and disabling the digital input path to avoid leakage current.
 *
 * @param pin[in]  GPIO pin identifier (combined port and pin number)
 */
static void tp_switch_to_analog(uint32_t pin)
{
    /* Get GPIO port structure for the specified pin */
    GPIO_T *port = (GPIO_T *)(GPIOA_BASE + PORT_OFFSET * NU_GET_PORT(pin));

    /* Configure pin multi-function register for analog mode */
    nu_pin_func(pin, (1 << NU_MFP_POS(NU_GET_PIN(pin))));

    /* Disable the digital input path to avoid leakage current on ADC pins */
    GPIO_DISABLE_DIGITAL_PATH(port, NU_GET_PIN_MASK(NU_GET_PIN(pin)));
}

/**
 * @brief Configure a GPIO pin to operate in digital mode.
 *
 * Sets a GPIO pin to digital I/O mode by clearing the multi-function
 * register and enabling the digital input path.
 *
 * @param pin[in]  GPIO pin identifier (combined port and pin number)
 */
static void tp_switch_to_digital(uint32_t pin)
{
    /* Get GPIO port structure for the specified pin */
    GPIO_T *port = (GPIO_T *)(GPIOA_BASE + PORT_OFFSET * NU_GET_PORT(pin));

    /* Clear multi-function register to set pin to GPIO mode */
    nu_pin_func(pin, 0);

    /* Enable digital path on GPIO pin for digital I/O operation */
    GPIO_ENABLE_DIGITAL_PATH(port, NU_GET_PIN_MASK(NU_GET_PIN(pin)));
}

/**
 * @brief Sample ADC channel for touch input (weak definition).
 *
 * Weak function that can be overridden by platform-specific implementation
 * to perform actual ADC sampling. Default implementation returns 0.
 *
 * @param channel[in]  ADC channel number to sample
 *
 * @return ADC sample value, or 0 if not implemented
 */
#if defined (__GNUC__)
    __attribute__((weak)) uint32_t nu_adc_sampling(uint32_t channel)
#else
    __weak uint32_t nu_adc_sampling(uint32_t channel)
#endif
{
    return 0;
}

/**
 * @brief Read X-axis coordinate from touch input using ADC method.
 *
 * Reads the X-axis position by applying a voltage differential across the
 * X-axis resistive touchpad and measuring the resulting voltage via ADC.
 * This implements the standard 4-wire resistive touch panel protocol.
 *
 * @return X-coordinate value read from ADC
 */
uint32_t indev_touch_get_x(void)
{
    GPIO_T *PORT;

    /*===== Configuration for X-axis measurement =====*/
    /* XR (Right) pin - output high */
    PORT    = (GPIO_T *)(GPIOA_BASE + (NU_GET_PORT(CONFIG_AD_PIN_XR) * PORT_OFFSET));
    GPIO_SetMode(PORT, NU_GET_PIN_MASK(NU_GET_PIN(CONFIG_AD_PIN_XR)), GPIO_MODE_OUTPUT);

    /* YD (Down) pin - input (ADC will measure here) */
    PORT    = (GPIO_T *)(GPIOA_BASE + (NU_GET_PORT(CONFIG_AD_PIN_YD) * PORT_OFFSET));
    GPIO_SetMode(PORT, NU_GET_PIN_MASK(NU_GET_PIN(CONFIG_AD_PIN_YD)), GPIO_MODE_INPUT);

    /* XL (Left) pin - output low */
    PORT    = (GPIO_T *)(GPIOA_BASE + (NU_GET_PORT(CONFIG_AD_PIN_XL) * PORT_OFFSET));
    GPIO_SetMode(PORT, NU_GET_PIN_MASK(NU_GET_PIN(CONFIG_AD_PIN_XL)), GPIO_MODE_OUTPUT);

    /* Set voltage differential: XR=1 (high), XL=0 (low) to drive X-axis */
    GPIO_PIN_DATA(NU_GET_PORT(CONFIG_AD_PIN_XR), NU_GET_PIN(CONFIG_AD_PIN_XR)) = 1;
    GPIO_PIN_DATA(NU_GET_PORT(CONFIG_AD_PIN_XL), NU_GET_PIN(CONFIG_AD_PIN_XL)) = 0;

    /* Configure output pins as digital GPIO for voltage output */
    tp_switch_to_digital(CONFIG_AD_PIN_XR);
    tp_switch_to_digital(CONFIG_AD_PIN_YD);
    tp_switch_to_digital(CONFIG_AD_PIN_XL);

    /* Configure measurement input as analog for ADC sampling */
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
