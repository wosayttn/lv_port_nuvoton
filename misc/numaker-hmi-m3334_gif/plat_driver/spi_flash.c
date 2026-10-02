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

void SpiFlash_WriteStatusReg(uint8_t u8Value1, uint8_t u8Value2)
{
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

int32_t SpiFlash_WaitReady(void)
{
    uint32_t u32Timeout = SystemCoreClock / 2;

    while (u32Timeout--)
    {
        uint8_t status = SpiFlash_ReadStatusReg();
        if ((status & 0x01) == 0) /* BUSY bit is 0 */
            return 0;
    }

    printf("[SPI_FLASH] Timeout waiting for flash ready\n");
    return -1;
}

void SpiFlash_Read(uint32_t u32Addr, uint8_t *pu8Buf, uint32_t u32Len)
{
    uint32_t u32TxCnt = 0;
    uint32_t u32RxCnt = 0;

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

    /* Read stream pipelined using FIFO */
    while (u32RxCnt < u32Len)
    {
        while (u32RxCnt < u32Len)
        {
					  if ((u32TxCnt - u32RxCnt < 8) && 
							  !QSPI_GET_TX_FIFO_FULL_FLAG(SPI_FLASH_PORT) && 
						    (u32TxCnt < u32Len))
						{
                QSPI_WRITE_TX(SPI_FLASH_PORT, 0x00);
                u32TxCnt++;
						}

            if (!QSPI_GET_RX_FIFO_EMPTY_FLAG(SPI_FLASH_PORT))
            {
                pu8Buf[u32RxCnt++] = (uint8_t)QSPI_READ_RX(SPI_FLASH_PORT);
            }
        }
    }

    spi_flash_wait_busy();
    QSPI_SET_SS_HIGH(SPI_FLASH_PORT);
    QSPI_ClearRxFIFO(SPI_FLASH_PORT);
}

void SpiFlash_QPI_FastRead(uint32_t u32Addr, uint8_t *pu8Buf, uint32_t u32Len)
{
    uint32_t u32TxCnt = 0;
    uint32_t u32RxCnt = 0;

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

		/* Read stream pipelined using FIFO (Safe & High-throughput) */
		while (u32RxCnt < u32Len)
		{
				if ((u32TxCnt - u32RxCnt < 8) && 
						!QSPI_GET_TX_FIFO_FULL_FLAG(SPI_FLASH_PORT) && 
						(u32TxCnt < u32Len))
				{
						QSPI_WRITE_TX(SPI_FLASH_PORT, 0x00);
						u32TxCnt++;
				}

				if (!QSPI_GET_RX_FIFO_EMPTY_FLAG(SPI_FLASH_PORT))
				{
						pu8Buf[u32RxCnt++] = (uint8_t)QSPI_READ_RX(SPI_FLASH_PORT);
				}
		}

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
    uint32_t i;

    SpiFlash_WriteEnable();

    QSPI_SET_DATA_WIDTH(SPI_FLASH_PORT, 8);
    QSPI_ClearRxFIFO(SPI_FLASH_PORT);

    QSPI_SET_SS_LOW(SPI_FLASH_PORT);

    /* Command 0x02: Page Program */
    QSPI_WRITE_TX(SPI_FLASH_PORT, 0x02);
    QSPI_WRITE_TX(SPI_FLASH_PORT, (u32Addr >> 16) & 0xFF);
    QSPI_WRITE_TX(SPI_FLASH_PORT, (u32Addr >> 8) & 0xFF);
    QSPI_WRITE_TX(SPI_FLASH_PORT, u32Addr & 0xFF);

    for (i = 0; i < u32Len; i++)
    {
        while (QSPI_GET_TX_FIFO_FULL_FLAG(SPI_FLASH_PORT));
        QSPI_WRITE_TX(SPI_FLASH_PORT, pu8Buf[i]);
    }

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

    s_bInitialized = true;
    return 0;
}
