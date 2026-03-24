/**************************************************************************//**
 * @file     drv_sys.c
 * @brief    SYS high level driver
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "NuMicro.h"

void nu_dump_clocks(void)
{
    printf("SYS_UPLL = %d MHz\n", sysGetClock(SYS_UPLL));
    printf("SYS_APLL = %d MHz\n", sysGetClock(SYS_APLL));
    printf("SYS_SYSTEM = %d MHz\n", sysGetClock(SYS_SYSTEM));
    printf("SYS_HCLK = %d MHz\n", sysGetClock(SYS_HCLK));
    printf("SYS_PCLK01 = %d MHz\n", sysGetClock(SYS_PCLK01));
    printf("SYS_PCLK2 = %d MHz\n", sysGetClock(SYS_PCLK2));
    printf("SYS_CPU = %d MHz\n", sysGetClock(SYS_CPU));

    printf("CLK_HCLKEN = %08X\n", inpw(REG_CLK_HCLKEN));
    printf("CLK_PCLKEN0 = %08X\n", inpw(REG_CLK_PCLKEN0));
    printf("CLK_PCLKEN1 = %08X\n", inpw(REG_CLK_PCLKEN1));

    printf("AIC_INTMSK0 = %08X\n", inpw(REG_AIC_INTMSK0));
    printf("AIC_INTMSK1 = %08X\n", inpw(REG_AIC_INTMSK1));

    printf("AIC_INTEN0 = %08X\n", inpw(REG_AIC_INTEN0));
    printf("AIC_INTEN1 = %08X\n", inpw(REG_AIC_INTEN1));

    printf("AIC_INTDIS0 = %08X\n", inpw(REG_AIC_INTDIS0));
    printf("AIC_INTDIS1 = %08X\n", inpw(REG_AIC_INTDIS1));
}

void nu_sys_ip_reset(E_SYS_IPRST eIPRstIdx)
{
    uint32_t volatile u32IPRSTRegAddr;
    uint32_t u32IPRSTRegBit;

    if (eIPRstIdx >= SYS_IPRST_CNT)
        return;

    u32IPRSTRegAddr = REG_SYS_AHBIPRST + (4ul * (eIPRstIdx / 32));
    u32IPRSTRegBit  = eIPRstIdx % 32;

    /* Unlock write-protect */
    SYS_UnlockReg();

    /* Enable IP reset */
    outpw(u32IPRSTRegAddr, inpw(u32IPRSTRegAddr) | (1 << u32IPRSTRegBit));

    /* Disable IP reset */
    outpw(u32IPRSTRegAddr, inpw(u32IPRSTRegAddr) & ~(1 << u32IPRSTRegBit));

    /* Wait it done. */
    while (inpw(u32IPRSTRegAddr) & (1 << u32IPRSTRegBit)) {}

    /* Lock write protect */
    //SYS_LockReg();
}

static void _nu_sys_ipclk(E_SYS_IPCLK eIPClkIdx, uint32_t bEnable)
{
    uint32_t volatile u32IPCLKRegAddr;
    uint32_t u32IPCLKRegBit;

    if (eIPClkIdx >= SYS_IPCLK_CNT)
        return;

    u32IPCLKRegAddr = REG_CLK_HCLKEN + (4ul * (eIPClkIdx / 32));
    u32IPCLKRegBit  = eIPClkIdx % 32;

    SYS_UnlockReg();

    if (bEnable)
    {
        /* Enable IP CLK */
        outpw(u32IPCLKRegAddr, inpw(u32IPCLKRegAddr) | (1 << u32IPCLKRegBit));
    }
    else
    {
        /* Disable IP CLK */
        outpw(u32IPCLKRegAddr, inpw(u32IPCLKRegAddr) & ~(1 << u32IPCLKRegBit));
    }

    //SYS_LockReg();
}


void nu_sys_ipclk_enable(E_SYS_IPCLK eIPClkIdx)
{
    _nu_sys_ipclk(eIPClkIdx, 1);
}

void nu_sys_ipclk_disable(E_SYS_IPCLK eIPClkIdx)
{
    _nu_sys_ipclk(eIPClkIdx, 0);
}

uint32_t SPI_SetBusClock(SPI_T *spi, uint32_t hz)
{
    // Missing in BSP.
    return 0;
}


