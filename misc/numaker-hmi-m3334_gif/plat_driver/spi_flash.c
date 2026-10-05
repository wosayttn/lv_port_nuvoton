/**************************************************************************//**
 * @file     spi_flash.c
 * @brief    QSPI0 NOR Flash Driver for NuMaker-M3334 (PC0~PC5)
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include <stdio.h>
#include "NuMicro.h"
#include "spi_flash.h"
#include "drv_qspi.h"

static struct nu_qspi s_NuQSPI =
{
    .base           = SPI_FLASH_PORT,
    .ss_pin         = -1,
#if defined(CONFIG_QSPI_USE_PDMA)
    .pdma_perp_tx   = PDMA_QSPI0_TX,
    .pdma_chanid_tx = -1,
    .pdma_perp_rx   = PDMA_QSPI0_RX,
    .pdma_chanid_rx = -1,
#if defined(__FREERTOS__)
    .m_psSemBus     = NULL,
#endif
    .m_u32Done      = 0,
#endif
};

static uint32_t s_u32FlashCapacity = 4 * 1024 * 1024; /* Default 4MB (W25Q32) */
static bool s_bInitialized = false;

__STATIC_INLINE void spi_flash_wait_busy(void)
{
    uint32_t u32Timeout = SystemCoreClock / 10;
    while (QSPI_IS_BUSY(SPI_FLASH_PORT))
    {
        if (--u32Timeout == 0)
        {
            printf("[SPI_FLASH] Timeout waiting for QSPI busy flag\n");
            break;
        }
    }
}

uint32_t SpiFlash_ReadJedecID(void)
{
    uint8_t u8RxData[4];
    uint32_t u8IDCnt = 0;

    QSPI_SET_DATA_WIDTH(SPI_FLASH_PORT, 8);
    QSPI_ClearRxFIFO(SPI_FLASH_PORT);

    QSPI_SET_SS_LOW(SPI_FLASH_PORT);

    /* Command 0x9F: Read JEDEC ID */
    QSPI_WRITE_TX(SPI_FLASH_PORT, 0x9F);
    QSPI_WRITE_TX(SPI_FLASH_PORT, 0x00);
    QSPI_WRITE_TX(SPI_FLASH_PORT, 0x00);
    QSPI_WRITE_TX(SPI_FLASH_PORT, 0x00);

    spi_flash_wait_busy();

    QSPI_SET_SS_HIGH(SPI_FLASH_PORT);

    while (!QSPI_GET_RX_FIFO_EMPTY_FLAG(SPI_FLASH_PORT) && u8IDCnt < 4)
    {
        u8RxData[u8IDCnt++] = (uint8_t)QSPI_READ_RX(SPI_FLASH_PORT);
    }

    if (u8IDCnt >= 4)
    {
        return (uint32_t)((u8RxData[1] << 16) | (u8RxData[2] << 8) | u8RxData[3]);
    }

    return 0;
}

uint8_t SpiFlash_ReadStatusReg(void)
{
    uint8_t u8Val = 0xFF;

    QSPI_SET_DATA_WIDTH(SPI_FLASH_PORT, 8);
    QSPI_ClearRxFIFO(SPI_FLASH_PORT);

    QSPI_SET_SS_LOW(SPI_FLASH_PORT);

    /* Command 0x05: Read Status Register */
    QSPI_WRITE_TX(SPI_FLASH_PORT, 0x05);
    QSPI_WRITE_TX(SPI_FLASH_PORT, 0x00);

    spi_flash_wait_busy();

    QSPI_SET_SS_HIGH(SPI_FLASH_PORT);

    /* Discard dummy byte received during command byte */
    if (!QSPI_GET_RX_FIFO_EMPTY_FLAG(SPI_FLASH_PORT))
        QSPI_READ_RX(SPI_FLASH_PORT);

    if (!QSPI_GET_RX_FIFO_EMPTY_FLAG(SPI_FLASH_PORT))
        u8Val = (uint8_t)QSPI_READ_RX(SPI_FLASH_PORT);

    return u8Val;
}

void SpiFlash_WriteEnable(void)
{
    QSPI_SET_DATA_WIDTH(SPI_FLASH_PORT, 8);
    QSPI_ClearRxFIFO(SPI_FLASH_PORT);

    QSPI_SET_SS_LOW(SPI_FLASH_PORT);

    /* Command 0x06: Write Enable */
    QSPI_WRITE_TX(SPI_FLASH_PORT, 0x06);

    spi_flash_wait_busy();

    QSPI_SET_SS_HIGH(SPI_FLASH_PORT);
    QSPI_ClearRxFIFO(SPI_FLASH_PORT);
}

uint8_t SpiFlash_ReadStatusReg2(void)
{
    uint8_t u8Val = 0xFF;

    QSPI_SET_DATA_WIDTH(SPI_FLASH_PORT, 8);
    QSPI_ClearRxFIFO(SPI_FLASH_PORT);

    QSPI_SET_SS_LOW(SPI_FLASH_PORT);

    /* Command 0x35: Read Status Register-2 */
    QSPI_WRITE_TX(SPI_FLASH_PORT, 0x35);
    QSPI_WRITE_TX(SPI_FLASH_PORT, 0x00);

    spi_flash_wait_busy();

    QSPI_SET_SS_HIGH(SPI_FLASH_PORT);

    /* Discard dummy byte received during command byte */
    if (!QSPI_GET_RX_FIFO_EMPTY_FLAG(SPI_FLASH_PORT))
        QSPI_READ_RX(SPI_FLASH_PORT);

    if (!QSPI_GET_RX_FIFO_EMPTY_FLAG(SPI_FLASH_PORT))
        u8Val = (uint8_t)QSPI_READ_RX(SPI_FLASH_PORT);

    return u8Val;
}

static void spi_flash_drive_wp_high(void)
{
#if defined(NUFUN) && (NUFUN==1)
    /* NuFUN: PC4 is QSPI0_MOSI1 / WP */
    SET_GPIO_PC4();
    GPIO_SetMode(PC, BIT4, GPIO_MODE_OUTPUT);
    PC4 = 1;
#else
    /* NuTFT: PA4 is QSPI0_MOSI1 / WP */
    SET_GPIO_PA4();
    GPIO_SetMode(PA, BIT4, GPIO_MODE_OUTPUT);
    PA4 = 1;
#endif
}

static void spi_flash_restore_qspi_pins(void)
{
#if defined(NUFUN) && (NUFUN==1)
    SET_QSPI0_MOSI1_PC4();
#else
    SET_QSPI0_MOSI1_PA4();
#endif
}

void SpiFlash_WriteStatusReg(uint8_t u8Value1, uint8_t u8Value2)
{
    /* Actively drive /WP (PA4/PC4) HIGH so hardware protection does not block write */
    spi_flash_drive_wp_high();

    /* Method 1: Non-Volatile Write Enable (0x06) + 2-byte Write Status Register (0x01) */
    SpiFlash_WriteEnable();

    QSPI_SET_DATA_WIDTH(SPI_FLASH_PORT, 8);
    QSPI_ClearRxFIFO(SPI_FLASH_PORT);

    QSPI_SET_SS_LOW(SPI_FLASH_PORT);

    /* Command 0x01: Write Status Register (SR1, SR2) */
    QSPI_WRITE_TX(SPI_FLASH_PORT, 0x01);
    QSPI_WRITE_TX(SPI_FLASH_PORT, u8Value1);
    QSPI_WRITE_TX(SPI_FLASH_PORT, u8Value2);

    spi_flash_wait_busy();

    QSPI_SET_SS_HIGH(SPI_FLASH_PORT);
    QSPI_ClearRxFIFO(SPI_FLASH_PORT);

    SpiFlash_WaitReady();

    /* If SR1 or SR2 did not match, try individual write commands (0x01 for SR1, 0x31 for SR2) */
    uint8_t cur1 = SpiFlash_ReadStatusReg();
    uint8_t cur2 = SpiFlash_ReadStatusReg2();
    if ((cur1 & 0x7C) != (u8Value1 & 0x7C) || (cur2 & 0x02) != (u8Value2 & 0x02))
    {
        /* Write SR1 individually via 0x01 */
        SpiFlash_WriteEnable();
        QSPI_SET_SS_LOW(SPI_FLASH_PORT);
        QSPI_WRITE_TX(SPI_FLASH_PORT, 0x01);
        QSPI_WRITE_TX(SPI_FLASH_PORT, u8Value1);
        spi_flash_wait_busy();
        QSPI_SET_SS_HIGH(SPI_FLASH_PORT);
        QSPI_ClearRxFIFO(SPI_FLASH_PORT);
        SpiFlash_WaitReady();

        /* Write SR2 individually via 0x31 */
        SpiFlash_WriteEnable();
        QSPI_SET_SS_LOW(SPI_FLASH_PORT);
        QSPI_WRITE_TX(SPI_FLASH_PORT, 0x31);
        QSPI_WRITE_TX(SPI_FLASH_PORT, u8Value2);
        spi_flash_wait_busy();
        QSPI_SET_SS_HIGH(SPI_FLASH_PORT);
        QSPI_ClearRxFIFO(SPI_FLASH_PORT);
        SpiFlash_WaitReady();
    }

    /* Method 2: If BP bits still active, try Volatile Status Register Write Enable (0x50) */
    cur1 = SpiFlash_ReadStatusReg();
    if ((cur1 & 0x7C) != (u8Value1 & 0x7C))
    {
        /* Command 0x50: Write Enable for Volatile Status Register */
        QSPI_SET_DATA_WIDTH(SPI_FLASH_PORT, 8);
        QSPI_ClearRxFIFO(SPI_FLASH_PORT);
        QSPI_SET_SS_LOW(SPI_FLASH_PORT);
        QSPI_WRITE_TX(SPI_FLASH_PORT, 0x50);
        spi_flash_wait_busy();
        QSPI_SET_SS_HIGH(SPI_FLASH_PORT);
        QSPI_ClearRxFIFO(SPI_FLASH_PORT);

        /* Write Volatile Status Register (0x01) */
        QSPI_SET_SS_LOW(SPI_FLASH_PORT);
        QSPI_WRITE_TX(SPI_FLASH_PORT, 0x01);
        QSPI_WRITE_TX(SPI_FLASH_PORT, u8Value1);
        QSPI_WRITE_TX(SPI_FLASH_PORT, u8Value2);
        spi_flash_wait_busy();
        QSPI_SET_SS_HIGH(SPI_FLASH_PORT);
        QSPI_ClearRxFIFO(SPI_FLASH_PORT);
        SpiFlash_WaitReady();
    }

    /* Restore QSPI multi-function pin */
    spi_flash_restore_qspi_pins();
}

static bool s_bQEEnabled = false;

void SpiFlash_EnableQE(void)
{
    if (s_bQEEnabled)
        return;

    uint8_t u8Status2 = SpiFlash_ReadStatusReg2();
    if ((u8Status2 & 0x02) == 0)
    {
        uint8_t u8Status1 = SpiFlash_ReadStatusReg();
        SpiFlash_WriteStatusReg(u8Status1, u8Status2 | 0x02);
    }
    s_bQEEnabled = true;
}

void SpiFlash_Unprotect(void)
{
    uint8_t u8Status1 = SpiFlash_ReadStatusReg();
    uint8_t u8Status2 = SpiFlash_ReadStatusReg2();

    /* Block protection bits in Status Register 1: BP0, BP1, BP2, BP3/TB, BP4/SEC (bits 2..6, mask 0x7C)
     * Status Register Protect bit in Status Register 1: SRP0 (bit 7, mask 0x80)
     * Complement protection bit in Status Register 2: CMP (bit 6, mask 0x40) */
    if ((u8Status1 & 0xFC) != 0 || (u8Status2 & 0x40) != 0)
    {
        printf("[SPI_FLASH] Block protection active (SR1=0x%02X, SR2=0x%02X). Unprotecting...\n",
               u8Status1, u8Status2);

        /* Clear all BP bits and SRP0 in SR1 (0x00); clear CMP in SR2 while preserving/enabling QE (bit 1) */
        uint8_t u8NewStatus1 = 0x00;
        uint8_t u8NewStatus2 = (u8Status2 & ~0x40) | 0x02;

        SpiFlash_WriteStatusReg(u8NewStatus1, u8NewStatus2);

        u8Status1 = SpiFlash_ReadStatusReg();
        u8Status2 = SpiFlash_ReadStatusReg2();
        printf("[SPI_FLASH] Unprotected status: SR1=0x%02X, SR2=0x%02X\n", u8Status1, u8Status2);

        if ((u8Status1 & 0x7C) != 0)
        {
            printf("[SPI_FLASH] WARNING: Block protection still active (SR1=0x%02X)! Writes may fail.\n", u8Status1);
        }
        else
        {
            printf("[SPI_FLASH] Success: All flash blocks unprotected!\n");
        }
    }
    else
    {
        printf("[SPI_FLASH] Block protection: None (SR1=0x%02X, SR2=0x%02X)\n",
               u8Status1, u8Status2);
    }

    /* Command 0x98: Global Block/Sector Unlock (ULBPR) for chips supporting Individual Block Lock (WPS) */
    SpiFlash_WriteEnable();

    QSPI_SET_DATA_WIDTH(SPI_FLASH_PORT, 8);
    QSPI_ClearRxFIFO(SPI_FLASH_PORT);

    QSPI_SET_SS_LOW(SPI_FLASH_PORT);

    QSPI_WRITE_TX(SPI_FLASH_PORT, 0x98);

    spi_flash_wait_busy();

    QSPI_SET_SS_HIGH(SPI_FLASH_PORT);
    QSPI_ClearRxFIFO(SPI_FLASH_PORT);

    SpiFlash_WaitReady();
}

int32_t SpiFlash_WaitReady(void)
{
    uint32_t u32Timeout = 5000;

    while (u32Timeout--)
    {
        uint8_t status = SpiFlash_ReadStatusReg();
        if ((status & 0x01) == 0) /* BUSY bit is 0 */
            return 0;

#if defined(__FREERTOS__)
        if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
        {
            vTaskDelay(pdMS_TO_TICKS(1));
        }
        else
        {
            for (volatile int i = 0; i < 0x2000; i++);
        }
#else
        for (volatile int i = 0; i < 0x2000; i++);
#endif
    }

    printf("[SPI_FLASH] Timeout waiting for flash ready\n");
    return -1;
}

void SpiFlash_Read(uint32_t u32Addr, uint8_t *pu8Buf, uint32_t u32Len)
{
    if (u32Len == 0 || pu8Buf == NULL)
        return;

    QSPI_SET_DATA_WIDTH(SPI_FLASH_PORT, 8);
    QSPI_ClearRxFIFO(SPI_FLASH_PORT);

    QSPI_SET_SS_LOW(SPI_FLASH_PORT);

    /* Command 0x03: Standard Read Data */
    QSPI_WRITE_TX(SPI_FLASH_PORT, 0x03);
    QSPI_WRITE_TX(SPI_FLASH_PORT, (u32Addr >> 16) & 0xFF);
    QSPI_WRITE_TX(SPI_FLASH_PORT, (u32Addr >> 8) & 0xFF);
    QSPI_WRITE_TX(SPI_FLASH_PORT, u32Addr & 0xFF);

    spi_flash_wait_busy();
    QSPI_ClearRxFIFO(SPI_FLASH_PORT);

    /* Read stream using QSPI PDMA */
    nu_qspi_read_pdma(&s_NuQSPI, pu8Buf, u32Len);

    spi_flash_wait_busy();
    QSPI_SET_SS_HIGH(SPI_FLASH_PORT);
    QSPI_ClearRxFIFO(SPI_FLASH_PORT);
}

void SpiFlash_QPI_FastRead(uint32_t u32Addr, uint8_t *pu8Buf, uint32_t u32Len)
{
    if (u32Len == 0 || pu8Buf == NULL)
        return;

    QSPI_SET_DATA_WIDTH(SPI_FLASH_PORT, 8);
    QSPI_ClearRxFIFO(SPI_FLASH_PORT);

    /* /CS: active */
    QSPI_SET_SS_LOW(SPI_FLASH_PORT);

    /* Command 0xEB: Fast Read Quad I/O */
    QSPI_WRITE_TX(SPI_FLASH_PORT, 0xEB);
    spi_flash_wait_busy();

    /* Switch to Quad output mode for sending 24-bit address and dummy bytes */
    QSPI_ENABLE_QUAD_OUTPUT_MODE(SPI_FLASH_PORT);

    /* Send 24-bit start address in Quad mode */
    QSPI_WRITE_TX(SPI_FLASH_PORT, (u32Addr >> 16) & 0xFF);
    QSPI_WRITE_TX(SPI_FLASH_PORT, (u32Addr >> 8)  & 0xFF);
    QSPI_WRITE_TX(SPI_FLASH_PORT, u32Addr         & 0xFF);

    /* Send 3 dummy bytes (M7-M0 mode bits + dummy clocks) in Quad mode */
    QSPI_WRITE_TX(SPI_FLASH_PORT, 0x00);
    QSPI_WRITE_TX(SPI_FLASH_PORT, 0x00);
    QSPI_WRITE_TX(SPI_FLASH_PORT, 0x00);

    spi_flash_wait_busy();

    /* Switch to Quad input mode for receiving data */
    QSPI_ENABLE_QUAD_INPUT_MODE(SPI_FLASH_PORT);
    QSPI_ClearRxFIFO(SPI_FLASH_PORT);

    /* Read stream using QSPI PDMA in Quad Mode */
    nu_qspi_read_pdma(&s_NuQSPI, pu8Buf, u32Len);

    spi_flash_wait_busy();

    /* /CS: de-active */
    QSPI_SET_SS_HIGH(SPI_FLASH_PORT);

    /* Restore QSPI to standard 1-bit SPI mode */
    QSPI_DISABLE_QUAD_MODE(SPI_FLASH_PORT);
    QSPI_ClearRxFIFO(SPI_FLASH_PORT);
}

void SpiFlash_SectorErase(uint32_t u32SectorAddr)
{
    SpiFlash_WriteEnable();

    QSPI_SET_DATA_WIDTH(SPI_FLASH_PORT, 8);
    QSPI_ClearRxFIFO(SPI_FLASH_PORT);

    QSPI_SET_SS_LOW(SPI_FLASH_PORT);

    /* Command 0x20: 4KB Sector Erase */
    QSPI_WRITE_TX(SPI_FLASH_PORT, 0x20);
    QSPI_WRITE_TX(SPI_FLASH_PORT, (u32SectorAddr >> 16) & 0xFF);
    QSPI_WRITE_TX(SPI_FLASH_PORT, (u32SectorAddr >> 8) & 0xFF);
    QSPI_WRITE_TX(SPI_FLASH_PORT, u32SectorAddr & 0xFF);

    spi_flash_wait_busy();

    QSPI_SET_SS_HIGH(SPI_FLASH_PORT);
    QSPI_ClearRxFIFO(SPI_FLASH_PORT);

    SpiFlash_WaitReady();
}

void SpiFlash_PageProgram(uint32_t u32Addr, const uint8_t *pu8Buf, uint32_t u32Len)
{
    if (u32Len == 0 || pu8Buf == NULL)
        return;

    SpiFlash_WriteEnable();

    QSPI_SET_DATA_WIDTH(SPI_FLASH_PORT, 8);
    QSPI_ClearRxFIFO(SPI_FLASH_PORT);

    QSPI_SET_SS_LOW(SPI_FLASH_PORT);

    /* Command 0x02: Page Program */
    QSPI_WRITE_TX(SPI_FLASH_PORT, 0x02);
    QSPI_WRITE_TX(SPI_FLASH_PORT, (u32Addr >> 16) & 0xFF);
    QSPI_WRITE_TX(SPI_FLASH_PORT, (u32Addr >> 8) & 0xFF);
    QSPI_WRITE_TX(SPI_FLASH_PORT, u32Addr & 0xFF);

    spi_flash_wait_busy();
    QSPI_ClearRxFIFO(SPI_FLASH_PORT);

    /* Write data using QSPI PDMA */
    nu_qspi_write_pdma(&s_NuQSPI, pu8Buf, u32Len);

    spi_flash_wait_busy();

    QSPI_SET_SS_HIGH(SPI_FLASH_PORT);
    QSPI_ClearRxFIFO(SPI_FLASH_PORT);

    SpiFlash_WaitReady();
}

void SpiFlash_WriteSector(uint32_t u32SectorAddr, const uint8_t *pu8Buf)
{
    uint32_t p;

    SpiFlash_SectorErase(u32SectorAddr);

    for (p = 0; p < 16; p++)
    {
        SpiFlash_PageProgram(u32SectorAddr + p * SPI_FLASH_PAGE_SIZE,
                             pu8Buf + p * SPI_FLASH_PAGE_SIZE,
                             SPI_FLASH_PAGE_SIZE);
    }
}

uint32_t SpiFlash_GetCapacity(void)
{
    return s_u32FlashCapacity;
}

int SpiFlash_Init(void)
{
    if (s_bInitialized)
        return 0;

    /* Enable QSPI0 module clock */
    CLK_EnableModuleClock(QSPI0_MODULE);
    CLK_SetModuleClock(QSPI0_MODULE, CLK_CLKSEL2_QSPI0SEL_PCLK0, MODULE_NoMsk);

#if defined(NUFUN) && (NUFUN==1)
    /* NuFUN: Setup QSPI0 multi-function pins on PC0..PC5 */
    SET_QSPI0_MOSI0_PC0();
    SET_QSPI0_MISO0_PC1();
    SET_QSPI0_CLK_PC2();
    SET_QSPI0_SS_PC3();
    SET_QSPI0_MOSI1_PC4();
    SET_QSPI0_MISO1_PC5();

    /* Enable pull-up on QSPI lines, especially MOSI1 (/WP) and MISO1 (/HOLD) */
    GPIO_SetPullCtl(PC, BIT0 | BIT1 | BIT2 | BIT3 | BIT4 | BIT5, GPIO_PUSEL_PULL_UP);

    /* Enable Schmitt trigger on PC2 (CLK) */
    PC->SMTEN |= GPIO_SMTEN_SMTEN2_Msk;

    /* Enable Fast slew rate for QSPI0 pins */
    GPIO_SetSlewCtl(PC, BIT0 | BIT1 | BIT2 | BIT3 | BIT4 | BIT5, GPIO_SLEWCTL_FAST);
#else
    /* NuTFT: Setup QSPI0 multi-function pins on PA0..PA5 */
    SET_QSPI0_MOSI0_PA0();
    SET_QSPI0_MISO0_PA1();
    SET_QSPI0_CLK_PA2();
    SET_QSPI0_SS_PA3();
    SET_QSPI0_MOSI1_PA4();
    SET_QSPI0_MISO1_PA5();

    /* Enable pull-up on QSPI lines, especially MOSI1 (/WP) and MISO1 (/HOLD) */
    GPIO_SetPullCtl(PA, BIT0 | BIT1 | BIT2 | BIT3 | BIT4 | BIT5, GPIO_PUSEL_PULL_UP);

    /* Enable Schmitt trigger on PA2 (CLK) */
    PA->SMTEN |= GPIO_SMTEN_SMTEN2_Msk;

    /* Enable Fast slew rate for QSPI0 pins */
    GPIO_SetSlewCtl(PA, BIT0 | BIT1 | BIT2 | BIT3 | BIT4 | BIT5, GPIO_SLEWCTL_FAST);
#endif

    /* Open QSPI0 as Master at 60 MHz */
    QSPI_Open(SPI_FLASH_PORT, QSPI_MASTER, QSPI_MODE_0, 8, 60000000);
    QSPI_SET_MSB_FIRST(SPI_FLASH_PORT);
    QSPI_DisableAutoSS(SPI_FLASH_PORT);
    QSPI_SET_SS_HIGH(SPI_FLASH_PORT);

    /* Initialize QSPI PDMA channels */
    nu_qspi_init_pdma(&s_NuQSPI);

    /* Wait for any previous in-progress flash write/erase operation to complete safely */
    SpiFlash_WaitReady();

    /* Read JEDEC ID to identify flash */
    uint32_t u32Id = SpiFlash_ReadJedecID();
    printf("[SPI_FLASH] JEDEC ID = 0x%06X\n", u32Id);

    if (u32Id != 0xFFFFFF && u32Id != 0x000000)
    {
        uint8_t u8CapCode = (uint8_t)(u32Id & 0xFF);
        if (u8CapCode >= 0x10 && u8CapCode <= 0x22)
        {
            s_u32FlashCapacity = (1UL << u8CapCode);
        }
        else
        {
            s_u32FlashCapacity = 4 * 1024 * 1024;
        }
    }
    else
    {
        /* Fallback default to 4MB */
        s_u32FlashCapacity = 4 * 1024 * 1024;
    }

    /* Ensure Quad Enable bit (QE) is set in flash status register */
    SpiFlash_EnableQE();

    /* Unprotect block protection to ensure all sectors are writable and erasable */
    SpiFlash_Unprotect();

    s_bInitialized = true;
    return 0;
}
