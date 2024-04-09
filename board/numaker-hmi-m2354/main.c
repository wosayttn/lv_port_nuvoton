/**************************************************************************//**
 * @file     main.c
 * @brief    M2354 Driver Sample Code
 *
 * @note
 * Copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/

#include "lv_glue.h"
#include "ff.h"
#include "diskio.h"

static uint32_t get_systick(void)
{
    return sysGetTicks(0);
}

static void sys_init(void)
{
    SYS_UnlockReg();

    /* LXT source from external LXT */
    CLK_EnableModuleClock(RTC_MODULE);
    RTC->LXTCTL &= ~(RTC_LXTCTL_LIRC32KEN_Msk | RTC_LXTCTL_C32KSEL_Msk);
    CLK_DisableModuleClock(RTC_MODULE);

    /* Enable clock source */
    CLK_EnableXtalRC(CLK_PWRCTL_HIRCEN_Msk | CLK_PWRCTL_LXTEN_Msk | CLK_PWRCTL_HXTEN_Msk | CLK_PWRCTL_HIRC48EN_Msk | CLK_PWRCTL_MIRCEN_Msk);

    /* Waiting for clock source ready */
    CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk | CLK_STATUS_LXTSTB_Msk | CLK_STATUS_HXTSTB_Msk | CLK_STATUS_HIRC48STB_Msk | CLK_STATUS_MIRCSTB_Msk);

    /* Disable PLL first to avoid unstable when setting PLL */
    CLK_DisablePLL();

    /* Set PLL frequency */
    /* Set core clock as PLL_CLOCK from PLL */
    CLK_SetCoreClock(FREQ_96MHZ);

    /* Waiting for PLL ready */
    CLK_WaitClockReady(CLK_STATUS_PLLSTB_Msk);

    /* If the defines do not exist in your project, please refer to the related clk.h in the Header folder appended to the tool package. */
    /* Set HCLK clock */
    CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_PLL, CLK_CLKDIV0_HCLK(1));

    /* Update System Core Clock */
    SystemCoreClockUpdate();

    /* Enable UART0 module clock */
    CLK_EnableModuleClock(UART0_MODULE);
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL2_UART0SEL_HXT, CLK_CLKDIV0_UART0(1));
    SYS->GPA_MFPL &= ~(SYS_GPA_MFPL_PA7MFP_Msk | SYS_GPA_MFPL_PA6MFP_Msk);
    SYS->GPA_MFPL |= (SYS_GPA_MFPL_PA7MFP_UART0_TXD | SYS_GPA_MFPL_PA6MFP_UART0_RXD);

    /* Enable EADC0 module clock/PIN */
    CLK_EnableModuleClock(EADC_MODULE);
    CLK_SetModuleClock(EADC_MODULE, MODULE_NoMsk, CLK_CLKDIV0_EADC(8));
    SYS->GPB_MFPH &= ~(SYS_GPB_MFPH_PB11MFP_Msk | SYS_GPB_MFPH_PB10MFP_Msk | SYS_GPB_MFPH_PB9MFP_Msk | SYS_GPB_MFPH_PB8MFP_Msk);
    SYS->GPB_MFPL &= ~(SYS_GPB_MFPL_PB7MFP_Msk | SYS_GPB_MFPL_PB6MFP_Msk);

    /* Enable digital path on these EADC pins */
    GPIO_ENABLE_DIGITAL_PATH(PB, BIT6 | BIT7 | BIT8 | BIT9 | BIT10 | BIT11);

    /* Enable SPI0 module clock */
    CLK_EnableModuleClock(SPI0_MODULE);
    CLK_SetModuleClock(SPI0_MODULE, CLK_CLKSEL2_SPI0SEL_PCLK1, MODULE_NoMsk);
    SYS_ResetModule(SPI0_RST);
    SYS->GPA_MFPL &= ~(SYS_GPA_MFPL_PA0MFP_Msk | SYS_GPA_MFPL_PA1MFP_Msk | SYS_GPA_MFPL_PA2MFP_Msk | SYS_GPA_MFPL_PA3MFP_Msk);
    SYS->GPA_MFPL |= (SYS_GPA_MFPL_PA0MFP_SPI0_MOSI | SYS_GPA_MFPL_PA1MFP_SPI0_MISO | SYS_GPA_MFPL_PA2MFP_SPI0_CLK | SYS_GPA_MFPL_PA3MFP_SPI0_SS);
    GPIO_SetSlewCtl(PA, BIT0 | BIT1 | BIT3, GPIO_SLEWCTL_HIGH);

    /* Pull-up MISO1 & MOSI1 pins */
    SYS->GPC_MFPH &= ~(SYS_GPC_MFPH_PC9MFP_Msk | SYS_GPC_MFPH_PC10MFP_Msk);
    GPIO_SetMode(PC, BIT9 | BIT10, GPIO_MODE_INPUT);
    GPIO_SetPullCtl(PC, BIT9 | BIT10, GPIO_PUSEL_PULL_UP);

    /* RESET ESP12 to avoid power issue. */
    SYS->GPC_MFPH &= ~(SYS_GPC_MFPH_PC13MFP_Msk);
    GPIO_SetMode(PC, BIT13, GPIO_MODE_OUTPUT);
    PC13 = 0;

    /* Enable SPI1 module clock */
    CLK_EnableModuleClock(SPI1_MODULE);
    CLK_SetModuleClock(SPI1_MODULE, CLK_CLKSEL2_SPI1SEL_PCLK0, MODULE_NoMsk);
    SYS_ResetModule(SPI1_RST);

    SYS->GPE_MFPL &= ~(SYS_GPE_MFPL_PE1MFP_Msk | SYS_GPE_MFPL_PE0MFP_Msk);
    SYS->GPE_MFPL |= (SYS_GPE_MFPL_PE1MFP_SPI1_MISO | SYS_GPE_MFPL_PE0MFP_SPI1_MOSI);
    SYS->GPH_MFPH &= ~(SYS_GPH_MFPH_PH9MFP_Msk | SYS_GPH_MFPH_PH8MFP_Msk);
    SYS->GPH_MFPH |= (SYS_GPH_MFPH_PH9MFP_SPI1_SS | SYS_GPH_MFPH_PH8MFP_SPI1_CLK);

    /* Enable SysTick module clock */
    CLK_EnableSysTick(CLK_CLKSEL0_STCLKSEL_HXT_DIV2, 0);

    /* Enable PDMA0/PDMA1 module clock */
    CLK_EnableModuleClock(PDMA0_MODULE);
    CLK_EnableModuleClock(PDMA1_MODULE);

    UART_Open(UART0, 115200);

    /* Vref connect to internal */
    SYS_SetVRef(SYS_VREFCTL_VREF_3_0V);

    systick_init();
}

#if LV_USE_LOG
static void lv_nuvoton_log(const char *buf)
{
    printf("%s", buf);
}
#endif /* LV_USE_LOG */



FATFS FatFs[FF_VOLUMES];       /* File system object for logical drive */

static void put_rc(FRESULT rc)
{
    const TCHAR *p =
        _T("OK\0DISK_ERR\0INT_ERR\0NOT_READY\0NO_FILE\0NO_PATH\0INVALID_NAME\0")
        _T("DENIED\0EXIST\0INVALID_OBJECT\0WRITE_PROTECTED\0INVALID_DRIVE\0")
        _T("NOT_ENABLED\0NO_FILE_SYSTEM\0MKFS_ABORTED\0TIMEOUT\0LOCKED\0")
        _T("NOT_ENOUGH_CORE\0TOO_MANY_OPEN_FILES\0INVALID_PARAMETER\0");

    uint32_t i;

    for (i = 0; (i != (UINT)rc) && *p; i++)
    {
        while (*p++) ;
    }

    printf(_T("rc=%u FR_%s\n"), (UINT)rc, p);
}

int main(void)
{
    sys_init();

#if 1
    lv_init();

#if LV_USE_LOG
    lv_log_register_print_cb(lv_nuvoton_log);
#endif /* LV_USE_LOG */

    lv_tick_set_cb(sysGetTicks);          /*Expression evaluating to current system time in ms*/
    lv_delay_set_cb(sysDelay);

    extern void lv_port_disp_init(void);
    lv_port_disp_init();

    extern void lv_port_indev_init(void);
    lv_port_indev_init();

    extern void ui_init(void);
    ui_init();

    while (1)
    {
        lv_task_handler();
        // Put your code here  __start.
        // Put your code here  __eend.
    }

#elif 0


#define SFUD_DEMO_TEST_BUFFER_SIZE                     1024
    static uint8_t sfud_demo_test_buf[SFUD_DEMO_TEST_BUFFER_SIZE];
    void sfud_demo(uint32_t addr, size_t size, uint8_t *data);

    sfud_init();
    sfud_demo(0, sizeof(sfud_demo_test_buf), sfud_demo_test_buf);
#else
    {
        DIR dir;                /* Directory object */
        FRESULT res;

        char logic_nbr[3] = {'0', ':', 0};

        /* Mount a logical drive */
        if ((res = f_mount(&FatFs[0], logic_nbr, 1)) != 0)
        {
            put_rc(res);
#if (FF_USE_MKFS)
            {
                BYTE Buff[FF_MAX_SS];          /* Working buffer */

                MKFS_PARM opt = {0};
                opt.fmt = FM_ANY | FM_SFD;

                /* Create a FAT volume */
                res = f_mkfs(logic_nbr, &opt, Buff, sizeof(Buff));
                if (res)
                {
                    put_rc(res);
                    goto _Exit;
                }
								else if ((res = f_mount(&FatFs[0], logic_nbr, 1)) != 0)
								{
									  goto _Exit;
								}
            }
#else
            goto _Exit:						
#endif
        }

        /* List directory information */
        if ((res = f_opendir(&dir, logic_nbr)) != 0)
        {
            put_rc(res);
        }
        else
        {
            FILINFO Finfo;
            FATFS *fs;              /* Pointer to file system object */
            uint32_t p1, s1, s2;
            p1 = s1 = s2 = 0;

            for (;;)
            {
                /* Read directory entries in sequence */
                res = f_readdir(&dir, &Finfo);

                if ((res != FR_OK) || !Finfo.fname[0])
                    break;

                if (Finfo.fattrib & AM_DIR)
                {
                    s2++;
                }
                else
                {
                    s1++;
                    p1 += Finfo.fsize;
                }

                printf("%c%c%c%c%c %u/%02u/%02u %02u:%02u %9lu  %s\n",
                       (Finfo.fattrib & AM_DIR) ? 'D' : '-',
                       (Finfo.fattrib & AM_RDO) ? 'R' : '-',
                       (Finfo.fattrib & AM_HID) ? 'H' : '-',
                       (Finfo.fattrib & AM_SYS) ? 'S' : '-',
                       (Finfo.fattrib & AM_ARC) ? 'A' : '-',
                       (Finfo.fdate >> 9) + 1980, (Finfo.fdate >> 5) & 15, Finfo.fdate & 31,
                       (Finfo.ftime >> 11), (Finfo.ftime >> 5) & 63, Finfo.fsize, Finfo.fname);
            }

            printf("%4u File(s),%10u bytes total\n%4u Dir(s)", s1, p1, s2);

            /* Get number of free clusters */
            if (f_getfree(logic_nbr, (DWORD *)&p1, &fs) == FR_OK)
                printf(", %10u bytes free\n", p1 * fs->csize * 512);
        }
    }
_Exit:

#endif
    while (1);
    return 0;
}
