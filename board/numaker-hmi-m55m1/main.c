/**************************************************************************//**
 * @file     main.c
 * @brief    M55M1 Driver Sample Code
 *
 * @note
 * Copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/

#include "lv_glue.h"
#include <arm_cmse.h>

#if defined(NVT_DCACHE_ON)
typedef enum
{
    evMR_CODE,       /* 0x00000000 ~ 0x1FFFFFFF, Normal, WT RA */
    evMR_SRAM,       /* 0x20000000 ~ 0x3FFFFFFF, Normal, WB WA RA */
    evMR_PERIPHERAL, /* 0x40000000 ~ 0x5FFFFFFF, Device, nGnRE, XN */
    evMR_EBI,        /* 0x60000000 ~ 0x7FFFFFFF, Normal, WB WA RA */
    evMR_SPIM,       /* 0x80000000 ~ 0x9FFFFFFF, Normal, WT RA */
    evMR_DEVICE1,    /* 0xA0000000 ~ 0xBFFFFFFF, Device, nGnRE, XN */
    evMR_DEVICE2,    /* 0xC0000000 ~ 0xDFFFFFFF, Device, nGnRE, XN */
    evMR_SYS_PPB,    /* 0xE0000000 ~ 0xE00FFFFF, Device, nGnRE, XN */
    evMR_SYS_VENDOR, /* 0xE0100000 ~ 0xFFFFFFFF, Device, nGnRE, XN */
} E_MEM_REGION;

/* Cache policy function */
static ARM_MPU_Region_t const mpuConfig[] =
{
    {
        /* EBI address space. */
        ARM_MPU_RBAR((uint32_t)0x60000000, // Base
                     ARM_MPU_SH_NON,       // Non-shareable
                     0,                    // Read-only
                     1,                    // Non-Privileged
                     1),                   // eXecute Never enabled
        ARM_MPU_RLAR((uint32_t)0x6FFFFFFF, // Limit
                     evMR_EBI)             // DEVICE_nGnRnE
    },
    {
        /* Change Cache policy to WBWARA from WTRA by default. */
        /* SPIM address space. */
        ARM_MPU_RBAR((uint32_t)0x80000000, // Base
                     ARM_MPU_SH_NON,       // Non-shareable
                     0,                    // Read-only
                     1,                    // Non-Privileged
                     1),                   // eXecute Never enabled
        ARM_MPU_RLAR((uint32_t)0x8FFFFFFF, // Limit
                     evMR_SPIM)            // WBWARA
    }
};

static void mpu_init(void)
{
    /* Initialize attributes corresponding to the enums defined in mpu.hpp */
    const uint8_t WTRA = ARM_MPU_ATTR_MEMORY_(1, 0, 1, 0); // Non-transient, Write-Through, Read-allocate, Not Write-allocate
    const uint8_t WBWARA = ARM_MPU_ATTR_MEMORY_(1, 1, 1, 1); // Non-transient, Write-Back, Read-allocate, Write-allocate

    ARM_MPU_Disable();

    ARM_MPU_SetMemAttr(evMR_EBI, ARM_MPU_ATTR(ARM_MPU_ATTR_DEVICE_nGnRnE, ARM_MPU_ATTR_DEVICE_nGnRnE));
    ARM_MPU_SetMemAttr(evMR_SPIM, ARM_MPU_ATTR(WBWARA, WBWARA));
    /* See https://developer.arm.com/documentation/den0024/a/Memory-Ordering/Memory-types/Device-memory */

    ARM_MPU_Load(evMR_EBI, &mpuConfig[0], sizeof(mpuConfig) / sizeof(ARM_MPU_Region_t));

    /* Enable MPU with default priv access to all other regions */
    ARM_MPU_Enable(MPU_CTRL_PRIVDEFENA_Msk);
}
#endif

static void sys_init(void)
{
    /* Unlock protected registers */
    SYS_UnlockReg();

#if defined(NVT_DCACHE_ON)
    mpu_init();
#endif

    /* Enable PLL0 180MHz clock from HIRC and switch SCLK clock source to PLL0 */
    CLK_SetBusClock(CLK_SCLKSEL_SCLKSEL_APLL0, CLK_APLLCTL_APLLSRC_HXT, FREQ_200MHZ);

    /* Update System Core Clock */
    /* User can use SystemCoreClockUpdate() to calculate SystemCoreClock. */
    SystemCoreClockUpdate();

    /* Enable UART0 module clock */
    SetDebugUartCLK();

    CLK_EnableModuleClock(GPIOA_MODULE);
    CLK_EnableModuleClock(GPIOB_MODULE);
    CLK_EnableModuleClock(GPIOC_MODULE);
    CLK_EnableModuleClock(GPIOD_MODULE);
    CLK_EnableModuleClock(GPIOE_MODULE);
    CLK_EnableModuleClock(GPIOF_MODULE);
    CLK_EnableModuleClock(GPIOG_MODULE);
    CLK_EnableModuleClock(GPIOH_MODULE);
    CLK_EnableModuleClock(GPIOI_MODULE);
    CLK_EnableModuleClock(GPIOJ_MODULE);

    /* Enable EBI clock */
    CLK_EnableModuleClock(EBI0_MODULE);
    SET_EBI_AD0_PC0();
    SET_EBI_AD1_PC1();
    SET_EBI_AD2_PC2();
    SET_EBI_AD3_PC3();
    SET_EBI_AD4_PC4();
    SET_EBI_AD5_PC5();
    SET_EBI_AD6_PD8();
    SET_EBI_AD7_PD9();
    SET_EBI_AD8_PE14();
    SET_EBI_AD9_PE15();
    SET_EBI_AD10_PE1();
    SET_EBI_AD11_PE0();
    SET_EBI_AD12_PH8();
    SET_EBI_AD13_PH9();
    SET_EBI_AD14_PH10();
    SET_EBI_AD15_PH11();
    SET_EBI_nWR_PJ9();
    SET_EBI_nRD_PJ8();
    SET_EBI_nCS0_PD14();
    SET_EBI_ADR0_PH7();

    GPIO_SetSlewCtl(PC, (BIT0 | BIT1 | BIT2 | BIT3 | BIT4 | BIT5), GPIO_SLEWCTL_HIGH);
    GPIO_SetSlewCtl(PD, (BIT8 | BIT9), GPIO_SLEWCTL_HIGH);
    GPIO_SetSlewCtl(PE, (BIT14 | BIT15), GPIO_SLEWCTL_HIGH);
    GPIO_SetSlewCtl(PE, (BIT0 | BIT1), GPIO_SLEWCTL_HIGH);
    GPIO_SetSlewCtl(PH, (BIT7 | BIT8 | BIT9 | BIT10 | BIT11), GPIO_SLEWCTL_HIGH);
    GPIO_SetSlewCtl(PJ, (BIT8 | BIT9), GPIO_SLEWCTL_HIGH);
    GPIO_SetSlewCtl(PD, BIT14, GPIO_SLEWCTL_HIGH);

    /* Enable I2C1 clock */
    CLK_EnableModuleClock(I2C1_MODULE);
    SET_I2C1_SDA_PB10();
    SET_I2C1_SCL_PB11();

    /* Enable PDMA clock */
    CLK_EnableModuleClock(PDMA0_MODULE);
    CLK_EnableModuleClock(PDMA1_MODULE);

    /* Enable SysTick clock */
    CLK_EnableSysTick(CLK_STSEL_ST0SEL_HIRC_DIV2, 0);

    /*---------------------------------------------------------------------------------------------------------*/
    /* Init I/O Multi-function                                                                                 */
    /*---------------------------------------------------------------------------------------------------------*/
    SetDebugUartMFP();

    InitDebugUart();

#if defined(USE_HYPERRAM_AS_FRAMEBUFFER)
    /* Enable SPIM module clock */
    CLK_EnableModuleClock(SPIM1_MODULE);

    /* Enable SPIM module clock */
    CLK_EnableModuleClock(OTFC1_MODULE);

    /* Init SPIM multi-function pins */
    SET_SPIM1_D5_PD7();
    SET_SPIM1_D6_PD6();
    SET_SPIM1_D7_PD5();
    SET_SPIM1_D4_PH15();
    SET_SPIM1_RWDS_PH14();
    SET_SPIM1_CLK_PH13();
    SET_SPIM1_CLKN_PH12();
    SET_SPIM1_D3_PJ3();
    SET_SPIM1_RESETN_PJ2();
    SET_SPIM1_SS_PJ7();
    SET_SPIM1_MOSI_PJ6();
    SET_SPIM1_MISO_PJ5();
    SET_SPIM1_D2_PJ4();

    PD->SMTEN |= (GPIO_SMTEN_SMTEN5_Msk |
                  GPIO_SMTEN_SMTEN6_Msk |
                  GPIO_SMTEN_SMTEN7_Msk);
    PH->SMTEN |= (GPIO_SMTEN_SMTEN12_Msk |
                  GPIO_SMTEN_SMTEN13_Msk |
                  GPIO_SMTEN_SMTEN14_Msk |
                  GPIO_SMTEN_SMTEN15_Msk);
    PJ->SMTEN |= (GPIO_SMTEN_SMTEN2_Msk |
                  GPIO_SMTEN_SMTEN3_Msk |
                  GPIO_SMTEN_SMTEN4_Msk |
                  GPIO_SMTEN_SMTEN5_Msk |
                  GPIO_SMTEN_SMTEN6_Msk |
                  GPIO_SMTEN_SMTEN7_Msk);

    /* Set SPIM I/O pins as high slew rate. */
    GPIO_SetSlewCtl(PD, (BIT5 | BIT6 | BIT7), GPIO_SLEWCTL_FAST1);
    GPIO_SetSlewCtl(PH, (BIT12 | BIT13 | BIT14 | BIT15), GPIO_SLEWCTL_FAST1);
    GPIO_SetSlewCtl(PJ, (BIT2 | BIT3 | BIT4 | BIT5 | BIT6 | BIT7), GPIO_SLEWCTL_FAST1);

    extern void HyperRAM_Init(SPIM_T * spim);
    HyperRAM_Init(SPIM1);

    SPIM_HYPER_EnterDirectMapMode(SPIM1);
#endif
}

int main(void)
{
    int task_lv_init(void);

    sys_init();

    task_lv_init();

    /* Start scheduling. */
    vTaskStartScheduler();

    for (;;);

    return 0;
}
