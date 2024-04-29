/**************************************************************************//**
 * @file     touch_ft5316.c
 * @brief    ft5316 touch driver
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2024 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include <string.h>
#include "indev_touch.h"

#define FT5316_REGITER_LEN   1
#define FT5316_MAX_TOUCH     5
#define FT5316_ADDRESS       0x38

#define CONFIG_MAX_TOUCH     1

typedef struct
{
    //03H
    union
    {
        uint8_t m_u8XH;
        struct
        {
            uint8_t u8X_11_8: 4;
            uint8_t : 2;
            uint8_t u8EvtFlag: 2;
#define FT_EVENTFLAG_PRESS_DOWN   0x0
#define FT_EVENTFLAG_LIFT_UP      0x1
#define FT_EVENTFLAG_CONTACT      0x2
#define FT_EVENTFLAG_NONE         0x3
        };
    };

    //04H
    union
    {
        uint8_t m_u8XL;
        struct
        {
            uint8_t u8X_7_0;
        };
    };

    //05H
    union
    {
        uint8_t m_u8YH;
        struct
        {
            uint8_t u8Y_11_8: 4;
            uint8_t u8TouchID: 4;     /* Touch ID of Touch Point, this value is 0x0F when the ID is invalid */
        };
    };

    //06H
    union
    {
        uint8_t m_u8YL;
        struct
        {
            uint8_t u8Y_7_0;
        };
    };

    //07H
    uint8_t m_u8Weight;  /* Touch pressure value */

    //08H
    union
    {
        uint8_t m_u8Misc;
        struct
        {
            uint8_t u8TouchSpeed: 2;      /* Touch Speed value */
            uint8_t u8TouchDirection: 2;  /* Touch Direction value */
            uint8_t u8TouchArea: 4;       /* Touch area value */
        };
    };

} S_FT_TP;

typedef struct
{
    union
    {
        uint8_t m_u8ModeSwitch;
#define FT_DEVICE_MODE_WORKING   0x0
#define FT_DEVICE_MODE_TEST      0x4

        struct
        {
            uint8_t : 4;
            uint8_t u8DevMode: 3;
            uint8_t : 1;
        };
    };

    uint8_t m_u8Guesture;
#define FT_GESTURE_ID_MOVE_UP       0x10
#define FT_GESTURE_ID_MOVE_RIGHT    0x14
#define FT_GESTURE_ID_MOVE_DOWN     0x18
#define FT_GESTURE_ID_MOVE_LEFT     0x1C
#define FT_GESTURE_ID_MOVE_IN       0x48
#define FT_GESTURE_ID_MOVE_OUT      0x49
#define FT_GESTURE_ID_MOVE_NONE     0x00

    union
    {
        uint8_t m_u8Status;

        struct
        {
            uint8_t u8TDStatus: 4;
            uint8_t : 4;
        };
    };

    S_FT_TP m_sTP[FT5316_MAX_TOUCH];

} S_FT_REGMAP;

static int16_t pre_x[CONFIG_MAX_TOUCH];
static int16_t pre_y[CONFIG_MAX_TOUCH];
static int16_t pre_w[CONFIG_MAX_TOUCH];
static int16_t s_tp_dowm[CONFIG_MAX_TOUCH];
static int16_t pre_id[CONFIG_MAX_TOUCH];
static uint8_t pre_touch = 0;

static S_FT_REGMAP sFtRegMap;

static int ft5316_write_reg(I2C_T *i2c, uint8_t reg, uint8_t value)
{
    return -I2C_WriteByteOneReg(i2c, FT5316_ADDRESS, reg, value);
}

static int ft5316_read_reg(I2C_T *i2c, uint8_t reg, uint8_t *data, uint32_t len)
{
    return (I2C_ReadMultiBytesOneReg(i2c, FT5316_ADDRESS, reg, data, len) == len) ? 0 : -1;
}

static void ft5316_touch_up(lv_indev_data_t *buf, int16_t id)
{
    s_tp_dowm[id] = 0;

    buf[id].state = LV_INDEV_STATE_RELEASED;
    buf[id].point.x = pre_x[id];
    buf[id].point.y = pre_y[id];

    pre_x[id] = -1;  /* last point is none */
    pre_y[id] = -1;
    pre_w[id] = -1;
}

static void ft5316_touch_down(lv_indev_data_t *buf, int8_t id, int16_t x, int16_t y, int16_t w)
{
    s_tp_dowm[id] = 1;

    buf[id].point.x = x;
    buf[id].point.y = y;
    buf[id].state = LV_INDEV_STATE_PRESSED;

    pre_x[id] = x; /* save last point */
    pre_y[id] = y;
    pre_w[id] = w;
}

int indev_touch_get_data(lv_indev_data_t *psInDevData)
{
    int i, error = 0;
    int32_t   touch_event, touchid;

    memset(&sFtRegMap, 0, sizeof(S_FT_REGMAP));

    error = ft5316_read_reg(CONFIG_INDEV_TOUCH_I2C, 0, (uint8_t *)&sFtRegMap, sizeof(sFtRegMap));
    if (error)
    {
        LV_LOG_ERROR("Get touch data failed, err:");
        goto exit_indev_touch_get_data;
    }

    if (sFtRegMap.u8TDStatus > CONFIG_MAX_TOUCH)
    {
        LV_LOG_ERROR("FW report max point:%d > panel info. max:%d", sFtRegMap.u8TDStatus, CONFIG_MAX_TOUCH);
        goto exit_indev_touch_get_data;
    }

    if (pre_touch > sFtRegMap.u8TDStatus)               /* point up */
    {
        for (i = 0; i < CONFIG_MAX_TOUCH; i++)
        {
            uint8_t j;
            for (j = 0; j < sFtRegMap.u8TDStatus; j++)  /* this time touch num */
            {
                if (pre_id[i] == i)                /* this id is not free */
                    break;
            }

            if ((j == sFtRegMap.u8TDStatus) && (pre_id[i] != -1))         /* free this node */
            {
                // LV_LOG_INFO("free %d tid=%d\n", i, pre_id[i]);
                ft5316_touch_up(psInDevData, pre_id[i]);
                pre_id[i] = -1;
            }
        }
    }

    for (i = 0; i < sFtRegMap.u8TDStatus; i++)
    {
        touch_event = sFtRegMap.m_sTP[i].u8EvtFlag;
        touchid = sFtRegMap.m_sTP[i].u8TouchID;

        LV_LOG_INFO("(%d/%d) %d %d", i, sFtRegMap.u8TDStatus, touchid, touch_event);

        if (touchid >= 0x0f)
            continue;

        pre_id[i] = touchid;

        if ((touch_event == FT_EVENTFLAG_PRESS_DOWN) || (touch_event == FT_EVENTFLAG_CONTACT))
        {
            uint16_t  x, y, w;

            x = ((uint16_t)sFtRegMap.m_sTP[i].u8X_11_8 << 8) |  sFtRegMap.m_sTP[i].u8X_7_0;
            y = ((uint16_t)sFtRegMap.m_sTP[i].u8Y_11_8 << 8) |  sFtRegMap.m_sTP[i].u8Y_7_0;
            w = sFtRegMap.m_sTP[i].m_u8Weight;

            LV_LOG_INFO("[%d] (%d %d %d %d)", touch_event, touchid, x, y, w);
            if (x >= LV_HOR_RES_MAX || y >= LV_VER_RES_MAX)
            {
                LV_LOG_ERROR("invalid position, X[%d,%u,%d], Y[%d,%u,%d]",
                             0, x, LV_HOR_RES_MAX,
                             0, y, LV_VER_RES_MAX);
                continue;
            }

            ft5316_touch_down(psInDevData, touchid, x, y, w);
        }
        else
        {
            // Up
            ft5316_touch_up(psInDevData, touchid);
        }

    } // for (i = 0; i < sFtRegMap.u8TDStatus; i++)

    pre_touch = sFtRegMap.u8TDStatus;

    LV_LOG_INFO("%s (%d, %d)", psInDevData->state ? "Press" : "Release", psInDevData->point.x, psInDevData->point.y);

    return (psInDevData->state == LV_INDEV_STATE_PRESSED) ? 1 : 0;

exit_indev_touch_get_data:

    pre_touch = 0;

    return 0;
}

int indev_touch_init(void)
{
    memset(&pre_x[0], 0xff,   CONFIG_MAX_TOUCH * sizeof(int16_t));
    memset(&pre_y[0], 0xff,   CONFIG_MAX_TOUCH * sizeof(int16_t));
    memset(&pre_w[0], 0xff,   CONFIG_MAX_TOUCH * sizeof(int16_t));
    memset(&s_tp_dowm[0], 0,  CONFIG_MAX_TOUCH * sizeof(int16_t));
    memset(&pre_id[0], 0xff,  CONFIG_MAX_TOUCH * sizeof(int16_t));

    /* Hardware reset */
    INDEV_TOUCH_SET_RST;
    sysDelay(5);
    INDEV_TOUCH_CLR_RST;
    sysDelay(200);

    I2C_Open(CONFIG_INDEV_TOUCH_I2C, 400000);

    ft5316_write_reg(CONFIG_INDEV_TOUCH_I2C, 0x0, 0);

    return 0;
}
