/**************************************************************************//**
 * @file     tamper.c
 * @version  V1.00
 * @brief    TAMPER driver source file
 *
 * @copyright SPDX-License-Identifier: Apache-2.0
 * @copyright Copyright (C) 2023 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "NuMicro.h"

/** @addtogroup Standard_Driver Standard Driver
  @{
*/

/** @addtogroup TAMPER_Driver TAMPER Driver
  @{
*/

/** @addtogroup TAMPER_EXPORTED_FUNCTIONS TAMPER Exported Functions
  @{
*/

/**
  * @brief      Enable Tamper Interrupt
  *
  * @param[in]  u32IntFlagMask      Specify the interrupt source. It consists of:
  *                                 - \ref TAMPER_INTEN_TAMP0IEN_Msk
  *                                 - \ref TAMPER_INTEN_TAMP1IEN_Msk
  *                                 - \ref TAMPER_INTEN_TAMP2IEN_Msk
  *                                 - \ref TAMPER_INTEN_TAMP3IEN_Msk
  *                                 - \ref TAMPER_INTEN_TAMP4IEN_Msk
  *                                 - \ref TAMPER_INTEN_TAMP5IEN_Msk
  *                                 - \ref TAMPER_INTEN_CLKFIEN_Msk
  *                                 - \ref TAMPER_INTEN_CLKSTOPIEN_Msk
  *                                 - \ref TAMPER_INTEN_OVPIEN_Msk
  *                                 - \ref TAMPER_INTEN_VGPIEN_Msk
  *                                 - \ref TAMPER_INTEN_VGNIEN_Msk
  *                                 - \ref TAMPER_INTEN_RTCLVRIEN_Msk
  *                                 - \ref TAMPER_INTEN_RTCIOIEN_Msk
  *                                 - \ref TAMPER_INTEN_RTCLKIEN_Msk
  *                                 - \ref TAMPER_INTEN_VLOSSIEN_Msk
  *                                 - \ref TAMPER_INTEN_WDTIEN_Msk
  *                                 - \ref TAMPER_INTEN_BODIEN_Msk
  *
  *
  * @details    This API is used to enable the specify tamper interrupt function.
  */
void TAMPER_EnableInt(uint32_t u32IntFlagMask)
{
    TAMPER->INTEN |= u32IntFlagMask;
}

/**
  * @brief      Disable Tamper Interrupt
  *
  * @param[in]  u32IntFlagMask      Specify the interrupt source. It consists of:
  *                                 - \ref TAMPER_INTEN_TAMP0IEN_Msk
  *                                 - \ref TAMPER_INTEN_TAMP1IEN_Msk
  *                                 - \ref TAMPER_INTEN_TAMP2IEN_Msk
  *                                 - \ref TAMPER_INTEN_TAMP3IEN_Msk
  *                                 - \ref TAMPER_INTEN_TAMP4IEN_Msk
  *                                 - \ref TAMPER_INTEN_TAMP5IEN_Msk
  *                                 - \ref TAMPER_INTEN_CLKFIEN_Msk
  *                                 - \ref TAMPER_INTEN_CLKSTOPIEN_Msk
  *                                 - \ref TAMPER_INTEN_OVPIEN_Msk
  *                                 - \ref TAMPER_INTEN_VGPIEN_Msk
  *                                 - \ref TAMPER_INTEN_VGNIEN_Msk
  *                                 - \ref TAMPER_INTEN_RTCLVRIEN_Msk
  *                                 - \ref TAMPER_INTEN_RTCIOIEN_Msk
  *                                 - \ref TAMPER_INTEN_RTCLKIEN_Msk
  *                                 - \ref TAMPER_INTEN_VLOSSIEN_Msk
  *                                 - \ref TAMPER_INTEN_WDTIEN_Msk
  *                                 - \ref TAMPER_INTEN_BODIEN_Msk
  *
  *
  * @details    This API is used to disable the specify tamper interrupt function.
  */
void TAMPER_DisableInt(uint32_t u32IntFlagMask)
{
    TAMPER->INTEN &= ~u32IntFlagMask;
}

/**
  * @brief      Static Tamper Detect
  *
  * @param[in]  u32TamperSelect     Tamper pin select. Possible options are
  *                                 - \ref TAMPER_TAMPER0_SELECT
  *                                 - \ref TAMPER_TAMPER1_SELECT
  *                                 - \ref TAMPER_TAMPER2_SELECT
  *                                 - \ref TAMPER_TAMPER3_SELECT
  *                                 - \ref TAMPER_TAMPER4_SELECT
  *                                 - \ref TAMPER_TAMPER5_SELECT
  *
  * @param[in]  u32DetecLevel       Tamper pin detection level select. Possible options are
  *                                 - \ref TAMPER_TAMPER_HIGH_LEVEL_DETECT
  *                                 - \ref TAMPER_TAMPER_LOW_LEVEL_DETECT
  *
  * @param[in]  u32DebounceEn       Tamper pin de-bounce enable
  *                                 - \ref TAMPER_TAMPER_DEBOUNCE_ENABLE
  *                                 - \ref TAMPER_TAMPER_DEBOUNCE_DISABLE
  *
  *
  * @details    This API is used to enable the tamper pin detect function with specify trigger condition.
  *             User needs to disable dynamic tamper function before use this API.
  */
void TAMPER_StaticTamperEnable(uint32_t u32TamperSelect, uint32_t u32DetecLevel, uint32_t u32DebounceEn)
{
    uint32_t i;
    uint32_t u32Reg;
    uint32_t u32TmpReg;

    u32Reg = TAMPER->TIOCTL;

    u32TmpReg = (TAMPER_TIOCTL_TAMP0EN_Msk | (u32DetecLevel << TAMPER_TIOCTL_TAMP0LV_Pos) |
                 (u32DebounceEn << TAMPER_TIOCTL_TAMP0DBEN_Pos));

    for (i = 0UL; i < (uint32_t)TAMPER_MAX_TAMPER_PIN_NUM; i++)
    {
        if (u32TamperSelect & (0x1UL << i))
        {
            u32Reg &= ~((TAMPER_TIOCTL_TAMP0EN_Msk | TAMPER_TIOCTL_TAMP0LV_Msk | TAMPER_TIOCTL_TAMP0DBEN_Msk) << (i * 4UL));
            u32Reg |= (u32TmpReg << (i * 4UL));
        }
    }

    TAMPER->TIOCTL = u32Reg;
}

/**
  * @brief      Static Tamper Disable
  *
  * @param[in]  u32TamperSelect     Tamper pin select. Possible options are
  *                                 - \ref TAMPER_TAMPER0_SELECT
  *                                 - \ref TAMPER_TAMPER1_SELECT
  *                                 - \ref TAMPER_TAMPER2_SELECT
  *                                 - \ref TAMPER_TAMPER3_SELECT
  *                                 - \ref TAMPER_TAMPER4_SELECT
  *                                 - \ref TAMPER_TAMPER5_SELECT
  *
  *
  * @details    This API is used to disable the static tamper pin detect.
  */
void TAMPER_StaticTamperDisable(uint32_t u32TamperSelect)
{
    uint32_t i;
    uint32_t u32Reg;
    uint32_t u32TmpReg;

    u32Reg = TAMPER->TIOCTL;

    u32TmpReg = TAMPER_TIOCTL_TAMP0EN_Msk;

    for (i = 0UL; i < (uint32_t)TAMPER_MAX_TAMPER_PIN_NUM; i++)
    {
        if (u32TamperSelect & (0x1UL << i))
        {
            u32Reg &= ~(u32TmpReg << (i * 4UL));
        }
    }

    TAMPER->TIOCTL = u32Reg;
}

/**
  * @brief      Dynamic Tamper Detect
  *
  * @param[in]  u32PairSel          Tamper pin detection enable. Possible options are
  *                                 - \ref TAMPER_PAIR0_SELECT
  *                                 - \ref TAMPER_PAIR1_SELECT
  *                                 - \ref TAMPER_PAIR2_SELECT
  *
  * @param[in]  u32DebounceEn       Tamper pin de-bounce enable
  *                                 - \ref TAMPER_TAMPER_DEBOUNCE_ENABLE
  *                                 - \ref TAMPER_TAMPER_DEBOUNCE_DISABLE
  *
  *  @param[in]  u32Pair1Source     Dynamic pair 1 input source select
  *                                 0: Pair 1 source select tamper 2
  *                                 1: Pair 1 source select tamper 0
  *
  *  @param[in]  u32Pair2Source     Dynamic pair 2 input source select
  *                                 0: Pair 2 source select tamper 4
  *                                 1: Pair 2 source select tamper 0
  *
  *
  * @details    This API is used to enable the dynamic tamper.
  */
void TAMPER_DynamicTamperEnable(uint32_t u32PairSel, uint32_t u32DebounceEn, uint32_t u32Pair1Source, uint32_t u32Pair2Source)
{
    uint32_t i;
    uint32_t u32Reg;
    uint32_t u32TmpReg;
    uint32_t u32Tamper2Debounce, u32Tamper4Debounce;

    u32Reg = TAMPER->TIOCTL;
    u32Reg &= ~(TAMPER_TIOCTL_TAMP0EN_Msk | TAMPER_TIOCTL_TAMP1EN_Msk | TAMPER_TIOCTL_TAMP2EN_Msk |
                TAMPER_TIOCTL_TAMP3EN_Msk | TAMPER_TIOCTL_TAMP4EN_Msk | TAMPER_TIOCTL_TAMP5EN_Msk);

    u32Tamper2Debounce = u32Reg & TAMPER_TIOCTL_TAMP2DBEN_Msk;
    u32Tamper4Debounce = u32Reg & TAMPER_TIOCTL_TAMP4DBEN_Msk;

    u32Reg &= ~(TAMPER_TIOCTL_DYN1ISS_Msk | TAMPER_TIOCTL_DYN2ISS_Msk);
    u32Reg |= ((u32Pair1Source & 0x1UL) << TAMPER_TIOCTL_DYN1ISS_Pos) | ((u32Pair2Source & 0x1UL) << TAMPER_TIOCTL_DYN2ISS_Pos);

    if (u32DebounceEn)
    {
        u32TmpReg = (TAMPER_TIOCTL_TAMP0EN_Msk | TAMPER_TIOCTL_TAMP1EN_Msk |
                     TAMPER_TIOCTL_TAMP0DBEN_Msk | TAMPER_TIOCTL_TAMP1DBEN_Msk | TAMPER_TIOCTL_DYNPR0EN_Msk);
    }
    else
    {
        u32TmpReg = (TAMPER_TIOCTL_TAMP0EN_Msk | TAMPER_TIOCTL_TAMP1EN_Msk | TAMPER_TIOCTL_DYNPR0EN_Msk);
    }

    for (i = 0UL; i < (uint32_t)TAMPER_MAX_PAIR_NUM; i++)
    {
        if (u32PairSel & (0x1UL << i))
        {
            u32Reg &= ~((TAMPER_TIOCTL_TAMP0DBEN_Msk | TAMPER_TIOCTL_TAMP1DBEN_Msk) << (i * 8UL));
            u32Reg |= (u32TmpReg << (i * 8UL));
        }
    }

    if ((u32Pair1Source) && (u32PairSel & (uint32_t)TAMPER_PAIR1_SELECT))
    {
        u32Reg &= ~TAMPER_TIOCTL_TAMP2EN_Msk;
        u32Reg |= u32Tamper2Debounce;
    }

    if ((u32Pair2Source) && (u32PairSel & (uint32_t)TAMPER_PAIR2_SELECT))
    {
        u32Reg &= ~TAMPER_TIOCTL_TAMP4EN_Msk;
        u32Reg |= u32Tamper4Debounce;
    }

    TAMPER->TIOCTL = u32Reg;
}

/**
  * @brief      Dynamic Tamper Disable
  *
  * @param[in]  u32PairSel          Tamper pin detection disable. Possible options are
  *                                 - \ref TAMPER_PAIR0_SELECT
  *                                 - \ref TAMPER_PAIR1_SELECT
  *                                 - \ref TAMPER_PAIR2_SELECT
  *
  *
  * @details    This API is used to disable the dynamic tamper.
  */
void TAMPER_DynamicTamperDisable(uint32_t u32PairSel)
{
    uint32_t i;
    uint32_t u32Reg;
    uint32_t u32TmpReg;
    uint32_t u32Tamper2En = 0UL, u32Tamper4En = 0UL;

    u32Reg = TAMPER->TIOCTL;

    if ((u32Reg & (uint32_t)TAMPER_TIOCTL_DYN1ISS_Msk) && (u32PairSel & (uint32_t)TAMPER_PAIR1_SELECT))
    {
        u32Tamper2En = u32Reg & TAMPER_TIOCTL_TAMP2EN_Msk;
    }

    if ((u32Reg & (uint32_t)TAMPER_TIOCTL_DYN2ISS_Msk) && (u32PairSel & (uint32_t)TAMPER_PAIR2_SELECT))
    {
        u32Tamper4En = u32Reg & TAMPER_TIOCTL_TAMP4EN_Msk;
    }

    u32TmpReg = (TAMPER_TIOCTL_TAMP0EN_Msk | TAMPER_TIOCTL_TAMP1EN_Msk | TAMPER_TIOCTL_DYNPR0EN_Msk);

    for (i = 0UL; i < (uint32_t)TAMPER_MAX_PAIR_NUM; i++)
    {
        if (u32PairSel & (0x1UL << i))
        {
            u32Reg &= ~(u32TmpReg << ((i * 8UL)));
        }
    }

    u32Reg |= (u32Tamper2En | u32Tamper4En);

    TAMPER->TIOCTL = u32Reg;
}

/**
  * @brief      Config Dynamic Tamper
  *
  * @param[in]  u32ChangeRate       The dynamic tamper output change rate
  *                                 - \ref TAMPER_2POW6_CLK
  *                                 - \ref TAMPER_2POW7_CLK
  *                                 - \ref TAMPER_2POW8_CLK
  *                                 - \ref TAMPER_2POW9_CLK
  *                                 - \ref TAMPER_2POW10_CLK
  *                                 - \ref TAMPER_2POW11_CLK
  *                                 - \ref TAMPER_2POW12_CLK
  *                                 - \ref TAMPER_2POW13_CLK
  *
  * @param[in]  u32SeedReload       Reload new seed or not
  *                                 0: not reload new seed
  *                                 1: reload new seed
  *
  * @param[in]  u32RefPattern       Reference pattern
  *                                 - \ref TAMPER_REF_RANDOM_PATTERN
  *                                 - \ref TAMPER_REF_SEED
  *
  * @param[in]  u32Seed             Seed Value (0x0 ~ 0xFFFFFFFF)
  *
  *
  * @details    This API is used to config dynamic tamper setting.
  */
void TAMPER_DynamicTamperConfig(uint32_t u32ChangeRate, uint32_t u32SeedReload, uint32_t u32RefPattern, uint32_t u32Seed)
{
    uint32_t u32Reg;

    u32Reg = TAMPER->TIOCTL;

    u32Reg &= ~(TAMPER_TIOCTL_DYNSRC_Msk | TAMPER_TIOCTL_SEEDRLD_Msk | TAMPER_TIOCTL_DYNRATE_Msk);

    u32Reg |= (u32ChangeRate) | ((u32SeedReload & 0x1UL) << TAMPER_TIOCTL_SEEDRLD_Pos) |
              ((u32RefPattern & 0x1UL) << TAMPER_TIOCTL_DYNSRC_Pos);

    TAMPER->SEED = u32Seed; /* Need to set seed value before reloaded seed */
    TAMPER->TIOCTL = u32Reg;
}

/** @} end of group TAMPER_EXPORTED_FUNCTIONS */
/** @} end of group TAMPER_Driver */
/** @} end of group Standard_Driver */
