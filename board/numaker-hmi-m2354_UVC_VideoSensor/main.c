/**************************************************************************//**
 * @file     main.c
 * @version  V1.00
 * @brief    Simulate an USB Video Class
 *
 ******************************************************************************/
#include <stdio.h>
#include "NuMicro.h"
#include "uvc.h"
#include "sensor.h"

/*---------------------------------------------------------------------------------------------------------*/
/* Define global variables and constants                                                                   */
/*---------------------------------------------------------------------------------------------------------*/
#define SENSOR_IN_WIDTH             640
#define SENSOR_IN_HEIGHT            480
#define SYSTEM_WIDTH                320
#define SYSTEM_HEIGHT               240

/* Buffer for Packet & Planar format */
static uint8_t u8FrameBuffer0[SENSOR_IN_WIDTH * SENSOR_IN_HEIGHT * 2] __attribute__((aligned(32)));
static uint8_t u8FrameBuffer1[SENSOR_IN_WIDTH * SENSOR_IN_HEIGHT * 2] __attribute__((aligned(32)));

extern volatile uint8_t   gAppConnected;
static volatile uint32_t  s_u32XferSize = 0;
static volatile uint32_t  s_u32XferBufAddr = 0;

/*--------------------------------------------------------------------------*/
/* To run CCAPInterruptHandler, when CAP frame end interrupt                */
/*--------------------------------------------------------------------------*/
void CCAPInterruptHandler(void)
{
    /* UVC transfer counter */
    static uint32_t u32UVCXferIdx = 0;

    /* Current Frame Index */
    static uint32_t u32CurFrameIndex = 2;

    /* Current Format Index */
    static uint32_t u32CurFormatIndex = 1;

    /* Current Width & Height */
    static uint16_t u16CurWidth = 320, u16CurHeight = 240;

    uint32_t u32NextFrameBufferAddr;

    if (s_u32XferBufAddr == 0) // UVC ISO-IN IDLE
    {
        if (u32UVCXferIdx & 0x1u)
        {
            s_u32XferBufAddr = (uint32_t)u8FrameBuffer1;
            u32NextFrameBufferAddr = (uint32_t)u8FrameBuffer0;
        }
        else
        {
            s_u32XferBufAddr = (uint32_t)u8FrameBuffer0;
            u32NextFrameBufferAddr = (uint32_t)u8FrameBuffer1;
        }

        /* UVC_Foramt_YUY2 */
        switch (uvcStatus.FormatIndex)
        {
        case UVC_Format_YUY2:       /* UVC_Foramt_YUY2 */
        {
            s_u32XferSize = uvcStatus.MaxVideoFrameSize;
            if (u32CurFrameIndex != uvcStatus.FrameIndex || u32CurFormatIndex != uvcStatus.FormatIndex)
            {
                /* Frame Size Changed */
                switch (uvcStatus.FrameIndex)
                {
                case UVC_VGA:
                    u16CurWidth = 640;
                    u16CurHeight = 480;
                    break;
                case UVC_QVGA:
                    u16CurWidth = 320;
                    u16CurHeight = 240;
                    break;
                case UVC_QQVGA:
                    u16CurWidth = 160;
                    u16CurHeight = 120;
                    break;
                }
                /* Set VideoIn Setting for Frame Size */
                ChangeFrame(TRUE, u32NextFrameBufferAddr, u16CurWidth, u16CurHeight);
                u32CurFrameIndex = uvcStatus.FrameIndex;
                u32CurFormatIndex = uvcStatus.FormatIndex;
            }
            else
                ChangeFrame(FALSE, u32NextFrameBufferAddr, u16CurWidth, u16CurHeight);

            break;
        }
        default:
            break;
        }

        u32UVCXferIdx++;
    }
}

/*------------------------------------------------------------------------------------------*/
/*  CCAP_IRQHandler                                                                         */
/*------------------------------------------------------------------------------------------*/
NVT_ITCM void CCAP_IRQHandler(void)
{
    uint32_t u32IntStatus = CCAP_GET_INT_STS();

    if (CCAP_IsIntEnabled(CCAP_INT_VIEN_ENABLE) && (u32IntStatus & CCAP_INTSTS_VINTF_Msk) == CCAP_INTSTS_VINTF_Msk)
    {
#ifdef NVT_DCACHE_ON
        /* Invalidate data cache of received frame buffer.  */
        SCB_InvalidateDCache_by_Addr(g_au8FrameBuffer, sizeof(g_au8FrameBuffer));
#endif
        CCAPInterruptHandler();
        CCAP_CLR_INT_FLAG(CCAP_INTSTS_VINTF_Msk);   /* Clear Frame end interrupt */
    }

    if (CCAP_IsIntEnabled(CCAP_INT_ADDRMIEN_ENABLE) && (u32IntStatus & CCAP_INTSTS_ADDRMINTF_Msk) == CCAP_INTSTS_ADDRMINTF_Msk)
    {
        CCAP_CLR_INT_FLAG(CCAP_INTSTS_ADDRMINTF_Msk);   /* Clear Address match interrupt */
    }

    if (CCAP_IsIntEnabled(CCAP_INT_MEIEN_ENABLE) && (u32IntStatus & CCAP_INTSTS_MEINTF_Msk) == CCAP_INTSTS_MEINTF_Msk)
    {
        CCAP_CLR_INT_FLAG(CCAP_INTSTS_MEINTF_Msk);     /* Clear Memory error interrupt */
    }

    CCAP_EnableUpdate();

    // CPU read interrupt flag register to wait write(clear) instruction completement.
    u32IntStatus = CCAP_GET_INT_STS();
}

void CCAP_SetFreq(uint32_t u32ModFreqKHz, uint32_t u32SensorFreq)
{
    int32_t i32Div;

    NVT_UNUSED(i32Div);

    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Enable CCAP Clock */
    CLK_EnableModuleClock(CCAP0_MODULE);

    /* Reset IP */
    SYS_ResetModule(SYS_CCAP0RST);

    /* Specified sensor clock */
    CLK_SetModuleClock(CCAP0_MODULE, CLK_CCAPSEL_CCAP0SEL_HIRC, 0);
    /* Not support in TESTCHIP_ONLY
        i32Div= (CLK_GetHCLK2Freq() / u32SensorFreq) - 1;
        if(i32Div < 0)
            i32Div = 0;
        CLK->VSENSEDIV = (CLK->VSENSEDIV & ~CLK_VSENSEDIV_VSENSEDIV_Msk) | (i32Div << CLK_VSENSEDIV_VSENSEDIV_Pos);
     * Not support in TESTCHIP_ONLY */

    /* Lock protected registers */
    SYS_LockReg();
}

int32_t PacketFormatDownScale(S_SENSOR_INFO *psSensorInfo)
{
    uint32_t u32Frame;

    /* Init CCAP clock and Sensor clock */
    CCAP_SetFreq(12000000, 12000000);

    /* Initialize sensor and set output format as YUV422 */
    if (psSensorInfo->pfnInitSensor(0) == FALSE)
    {
        printf("Init sensor failed !\n");
        return -1;
    }

    /* Enable External CCAP Interrupt */
    NVIC_EnableIRQ(CCAP_IRQn);

    /* Enable CCAP Frame End Interrupt */
    CCAP_EnableInt(CCAP_INT_VIEN_ENABLE);

    /* Set Vsync polarity, Hsync polarity, pixel clock polarity, Sensor Format and Order */
    CCAP_Open(psSensorInfo->m_u32Polarity, psSensorInfo->m_u32InputFormat, CCAP_PAR_OUTFMT_YUV422);

    /* Set Cropping Window Vertical/Horizontal Starting Address and Cropping Window Size */
    CCAP_SetCroppingWindow(0, 0, psSensorInfo->m_u16Height, psSensorInfo->m_u16Width);

    /* Set System Memory Packet Base Address Register */
    CCAP_SetPacketBuf((uint32_t)u8FrameBuffer0);

    /* Set Packet Scaling Vertical/Horizontal Factor Register */
    CCAP_SetPacketScaling(SYSTEM_HEIGHT, psSensorInfo->m_u16Height, SYSTEM_WIDTH, psSensorInfo->m_u16Width);

    /* Set Packet Frame Output Pixel Stride Width */
    CCAP_SetPacketStride(SYSTEM_WIDTH);

    /* Start Image Capture Interface */
    CCAP_Start();

    return 0;
}


void SYS_Init(void)
{
    uint32_t volatile i;

    /* Unlock protected registers */
    SYS_UnlockReg();

    /*---------------------------------------------------------------------------------------------------------*/
    /* Init System Clock                                                                                       */
    /*---------------------------------------------------------------------------------------------------------*/

    /* Enable External RC 12MHz clock */
    CLK_EnableXtalRC(CLK_SRCCTL_HXTEN_Msk);

    /* Waiting for External RC clock ready */
    CLK_WaitClockReady(CLK_STATUS_HXTSTB_Msk);

    /* Switch SCLK clock source to APLL0 and Enable APLL0 180MHz clock */
    CLK_SetBusClock(CLK_SCLKSEL_SCLKSEL_APLL0, CLK_APLLCTL_APLLSRC_HXT, FREQ_180MHZ);

    /* Update System Core Clock */
    /* User can use SystemCoreClockUpdate() to calculate SystemCoreClock. */
    SystemCoreClockUpdate();

    /* Enable all GPIO clock */
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

    /* Debug UART clock setting*/
    SetDebugUartCLK();

    SYS->USBPHY &= ~SYS_USBPHY_HSUSBROLE_Msk;    /* select HSUSBD */
    /* Enable USB PHY */
    SYS->USBPHY = (SYS->USBPHY & ~(SYS_USBPHY_HSUSBROLE_Msk | SYS_USBPHY_HSUSBACT_Msk)) | SYS_USBPHY_HSOTGPHYEN_Msk;
    for (i = 0; i < 0x1000; i++);  // delay

    SYS->USBPHY |= SYS_USBPHY_HSUSBACT_Msk;

    /* Enable IP clock */
    CLK_EnableModuleClock(HSUSBD0_MODULE);

    /*---------------------------------------------------------------------------------------------------------*/
    /* Init I/O Multi-function                                                                                 */
    /*---------------------------------------------------------------------------------------------------------*/
    SetDebugUartMFP();

    /* Set multi-function pins for CCAP */
    SET_CCAP_DATA7_PG2();
    SET_CCAP_DATA6_PG3();
    SET_CCAP_DATA5_PG4();
    SET_CCAP_DATA4_PF11();
    SET_CCAP_DATA3_PF10();
    SET_CCAP_DATA2_PF9();
    SET_CCAP_DATA1_PF8();
    SET_CCAP_DATA0_PF7();
    SET_CCAP_PIXCLK_PG9();
    SET_CCAP_SCLK_PG10();
    SET_CCAP_VSYNC_PG12();
    SET_CCAP_HSYNC_PG13();

    /* Lock protected registers */
    SYS_LockReg();
}


/*------------------------------------------------------------------------------------------*/
/* Change VideoIN Setting for Frame size */
void ChangeFrame(char bChangeSize, uint32_t u32Address, uint16_t u16Width, uint16_t u16Height)
{
    if (bChangeSize)    /* Change Frame Size */
    {
        CCAP_SetPacketScaling(u16Height, SENSOR_IN_HEIGHT, u16Width, SENSOR_IN_WIDTH);
    }
    /* Set Buffer Address */
    CCAP_SetPacketBuf((uint32_t)u32Address);
    CCAP_SetPacketStride(u16Width);
}

int32_t main(void)
{
    /* Init System, peripheral clock and multi-function I/O */
    SYS_Init();

    /* Init UART to 115200-8n1 for print message */
    InitDebugUart();

    printf("NuMicro HSUSBD UVC YUV422 Streaming\n");

    /* Using Packet format to Image down scale */
    if (PacketFormatDownScale(&g_sSensorHM1055) != 0)
        printf("Capture frame failed !\n");

    HSUSBD_Open(&gsHSInfo, UVC_ClassRequest, UVC_SetInterface);

    /* Endpoint configuration */
    UVC_Init();

    /* Enable HSUSBD interrupt */
    NVIC_EnableIRQ(HSUSBD_IRQn);

    /* Start transaction */
    while (1)
    {
        if (!HSUSBD_IS_ATTACHED())
        {
            HSUSBD_Start();
            break;
        }
    }

    while (1)
    {
        if (gAppConnected == 1)
        {
            if (s_u32XferBufAddr) //Trigger ISO-IN DMA
            {
                UVC_SendImage(s_u32XferBufAddr, s_u32XferSize, uvcStatus.StillImage);
                s_u32XferBufAddr = 0;  // Set ISO-IN IDLE
            }
        }
    }

    while (1);
}

/*** (C) COPYRIGHT 2023 Nuvoton Technology Corp. ***/
