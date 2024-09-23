/**************************************************************************//**
 * @file     main.c
 * @brief    M55M1 Driver Sample Code
 *
 * @note
 * Copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/

#include "lv_glue.h"
#include <arm_cmse.h>

/* DMA350 driver structures */
static const struct dma350_dev_cfg_t GDMA_DEV_CFG_S =
{
    .dma_sec_cfg = (DMASECCFG_TypeDef *)(GDMA_S + 0x0UL),
    .dma_sec_ctrl = (DMASECCTRL_TypeDef *)(GDMA_S + 0x100UL),
    .dma_nsec_ctrl = (DMANSECCTRL_TypeDef *)(GDMA_S + 0x200UL),
    .dma_info = (DMAINFO_TypeDef *)(GDMA_S + 0xF00UL)
};

static struct dma350_dev_data_t GDMA_DEV_DATA_S =
{
    .state = 0
};

static struct dma350_dev_t GDMA_DEV_S =
{
    &(GDMA_DEV_CFG_S),
    &(GDMA_DEV_DATA_S)
};

static struct dma350_ch_dev_t GDMA_CH0_DEV_S =
{
    .cfg = {
        .ch_base = (DMACH_TypeDef *)(GDMA_S + 0x1000UL),
        .channel = 0
    },
    .data = {0}
};

static struct dma350_ch_dev_t GDMA_CH1_DEV_S =
{
    .cfg = {
        .ch_base = (DMACH_TypeDef *)(GDMA_S + 0x1100UL),
        .channel = 1
    },
    .data = {0}
};

struct dma350_ch_dev_t *const GDMA_CH_DEV_S[] =
{
    &GDMA_CH0_DEV_S,
    &GDMA_CH1_DEV_S	
};

static void DNA350DevInit(void)
{
    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Enable GDMA0 clock source */
    CLK_EnableModuleClock(GDMA0_MODULE);

    /* Reset GDMA module */
    SYS_ResetModule(SYS_GDMA0RST);

    dma350_init(&GDMA_DEV_S);

    NVIC_SetPriority(GDMACH0_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1);
    NVIC_SetPriority(GDMACH1_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1);

    /* Enable NVIC for GDMA CH0 */
    NVIC_EnableIRQ(GDMACH0_IRQn);

    /* Enable NVIC for GDMA CH1 */
    NVIC_EnableIRQ(GDMACH1_IRQn);    
}

static void sys_init(void)
{
    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Enable PLL0 clock from HXT and switch SCLK clock source to PLL0 */
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
    /* Notice: EBI must set to non-privileged for accessing. */
    CLK->SCUCTL |= CLK_SCUCTL_SCU0CKEN_Msk;
    SCU->D1PNP1 |= SCU_D1PNP1_EBI_Msk;		

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
#if defined(CONFIG_DISP_USE_EBI_SYNC)
    SET_EBI_ADR1_PH6();
    SET_EBI_ADR7_PH0();
    GPIO_SetSlewCtl(PH, (BIT0 | BIT6 | BIT7 | BIT8 | BIT9 | BIT10 | BIT11), GPIO_SLEWCTL_HIGH);
#else
    GPIO_SetSlewCtl(PH, (BIT7 | BIT8 | BIT9 | BIT10 | BIT11), GPIO_SLEWCTL_HIGH);
#endif

    GPIO_SetSlewCtl(PC, (BIT0 | BIT1 | BIT2 | BIT3 | BIT4 | BIT5), GPIO_SLEWCTL_HIGH);
    GPIO_SetSlewCtl(PD, (BIT8 | BIT9), GPIO_SLEWCTL_HIGH);
    GPIO_SetSlewCtl(PE, (BIT14 | BIT15), GPIO_SLEWCTL_HIGH);
    GPIO_SetSlewCtl(PE, (BIT0 | BIT1), GPIO_SLEWCTL_HIGH);
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

    DNA350DevInit();
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
