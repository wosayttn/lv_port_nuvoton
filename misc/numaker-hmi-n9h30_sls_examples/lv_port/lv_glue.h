/**************************************************************************//**
 * @file     lv_glue.c
 * @brief    lvgl glue header
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#ifndef __LV_GLUE_H__
#define __LV_GLUE_H__

#include "lvgl.h"
#include "sys.h"
#include "lcd.h"
#include "adc.h"
#include "gpio.h"
#include "nu_misc.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#define CONFIG_LCD_FB_NUM            3
#define CONFIG_DISP_LINE_BUFFER_NUMBER  (LV_VER_RES_MAX)

typedef struct
{
    uint32_t u32DevWidth;           /*!< Panel width */
    uint32_t u32DevHeight;          /*!< Panel height */
    uint32_t u32CmdLow;             /*!< MPU command line low indicator */
    uint32_t u32Cmd16t18;           /*!< MPU command width */
    uint32_t u32CmdBusWidth;        /*!< MPU bus width */
    uint32_t u32DataBusWidth;       /*!< Display bus width */
    uint32_t u32MPU_Mode;           /*!< MPU mode */
    uint32_t u32DisplayColors;      /*!< Display colors */
    uint32_t u32DevType;            /*!< Type of display panel */
    union
    {
        uint32_t u32Reg_CRTCSIZE;   /*!< CRTCSIZE register value */
        struct
        {
            uint32_t HTT: 11;       /*!< Horizontal Total Pixels */
            uint32_t : 5;
            uint32_t VTT: 11;       /*!< Vertical Total Scan Lines */
            uint32_t : 5;
        } sCRTCSIZE;
    };
    union
    {
        uint32_t u32Reg_CRTCDEND;   /*!< CRTCDEND register value */
        struct
        {
            uint32_t HDEND: 11;     /*!< Horizontal Display Enable End */
            uint32_t : 5;
            uint32_t VDEND: 11;     /*!< Vertical Display Enable End */
            uint32_t : 5;
        } sCRTCDEND;
    };
    union
    {
        uint32_t u32Reg_CRTCHR;     /*!< CRTCHR register value */
        struct
        {
            uint32_t HRS: 11;       /*!< Internal Horizontal Retrace Start Timing */
            uint32_t : 5;
            uint32_t HRE: 11;       /*!< Internal Horizontal Retrace End Low */
            uint32_t : 5;
        } sCRTCHR;
    };
    union
    {
        uint32_t u32Reg_CRTCHSYNC;  /*!< CRTCHSYNC register value */
        struct
        {
            uint32_t HSYNC_S: 11;   /*!< Horizontal Sync Start Timing */
            uint32_t : 5;
            uint32_t HSYNC_E: 11;   /*!< Horizontal Sync End Timing */
            uint32_t : 3;
            uint32_t HSYNC_SHIFT: 2; /*!< Hsync Signal Adjustment For Multi-Cycles Per Pixel Mode Of Sync-Based Unipac-LCD */
        } sCRTCHSYNC;
    };
    union
    {
        uint32_t u32Reg_CRTCVR;     /*!< CRTCVR register value */
        struct
        {
            uint32_t VRS: 11;       /*!< Vertical Internal Retrace Start Timing */
            uint32_t : 5;
            uint32_t VRE: 11;       /*!< Vertical Internal Retrace End Low */
            uint32_t : 5;
        } sCRTCVR;
    };
} VPOST_BF_T;


/**
  * @brief      Disable register write-protection function
  * @param      None
  * @return     None
  * @details    This function disable register write-protection function.
  *             To unlock the protected register to allow write access.
  */
static __inline void SYS_UnlockReg(void)
{
    do
    {
        outpw(0xB00001FC, 0x59UL);
        outpw(0xB00001FC, 0x16UL);
        outpw(0xB00001FC, 0x88UL);
    }
    while (inpw(0xB00001FC) == 0UL);
}

/**
  * @brief      Enable register write-protection function
  * @param      None
  * @return     None
  * @details    This function is used to enable register write-protection function.
  *             To lock the protected register to forbid write access.
  */
static __inline void SYS_LockReg(void)
{
    outpw(0xB00001FC, 0);
}

int lcd_device_initialize(void);
int lcd_device_finalize(void);
int lcd_device_open(void);
void lcd_device_close(void);
int lcd_device_control(int cmd, void *argv);

int touchpad_device_initialize(void);
int touchpad_device_finalize(void);
int touchpad_device_open(void);
int touchpad_device_read(lv_indev_data_t *psInDevData);
void touchpad_device_close(void);
int touchpad_device_control(int cmd, void *argv);

#endif /* __LV_GLUE_H__ */
