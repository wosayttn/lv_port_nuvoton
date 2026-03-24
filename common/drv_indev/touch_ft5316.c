/**************************************************************************//**
 * @file     touch_ft5316.c
 * @brief    ft5316 touch driver
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2024 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include <string.h>
#include "numaker_touch.h"

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

static S_TOUCH_IF_I2C s_ft5316_i2c_if =
{
    .m_pvI2C      = CONFIG_INDEV_TOUCH_I2C,
    .m_pu8Reg     = NULL,
    .m_u32RegLen  = 1,
    .m_pu8Data    = NULL,
    .m_u32DataLen = 0,
    .m_pvPrivate  = NULL,
    .m_u8DevAddr  = FT5316_ADDRESS,
};

static int ft5316_write_reg(uint8_t reg, uint8_t data[], uint32_t len)
{
    S_TOUCH_IF_I2C *psIfCtx = &s_ft5316_i2c_if;

    psIfCtx->m_pu8Reg = &reg;
    psIfCtx->m_u32RegLen = sizeof(uint8_t);
    psIfCtx->m_pu8Data = data;
    psIfCtx->m_u32DataLen = len;

    return touch_plat_i2c_write(psIfCtx);
}

static int ft5316_read_reg(uint8_t reg, uint8_t data[], uint32_t len)
{
    S_TOUCH_IF_I2C *psIfCtx = &s_ft5316_i2c_if;

    psIfCtx->m_pu8Reg = &reg;
    psIfCtx->m_u32RegLen = sizeof(uint8_t);
    psIfCtx->m_pu8Data = data;
    psIfCtx->m_u32DataLen = len;

    return touch_plat_i2c_read(psIfCtx);
}

static void ft5316_touch_up(numaker_indev_data_t *buf, int16_t id)
{
    s_tp_dowm[id] = 0;

    buf[id].state = NUMAKER_INDEV_STATE_RELEASED;
    buf[id].point.x = pre_x[id];
    buf[id].point.y = pre_y[id];

    pre_x[id] = -1;  /* last point is none */
    pre_y[id] = -1;
    pre_w[id] = -1;
}

static void ft5316_touch_down(numaker_indev_data_t *buf, int8_t id, int16_t x, int16_t y, int16_t w)
{
    s_tp_dowm[id] = 1;

    buf[id].point.x = x;
    buf[id].point.y = y;
    buf[id].state = NUMAKER_INDEV_STATE_PRESSED;

    pre_x[id] = x; /* save last point */
    pre_y[id] = y;
    pre_w[id] = w;
}

int indev_touch_get_data(numaker_indev_data_t *psInDevData)
{
    int i, error = 0;
    int32_t   touch_event, touchid;

    memset(&sFtRegMap, 0, sizeof(S_FT_REGMAP));

    error = ft5316_read_reg(0, (uint8_t *)&sFtRegMap, sizeof(sFtRegMap));
    if (error)
    {
        goto exit_indev_touch_get_data;
    }

    if (sFtRegMap.u8TDStatus > CONFIG_MAX_TOUCH)
    {
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
                ft5316_touch_up(psInDevData, pre_id[i]);
                pre_id[i] = -1;
            }
        }
    }

    for (i = 0; i < sFtRegMap.u8TDStatus; i++)
    {
        touch_event = sFtRegMap.m_sTP[i].u8EvtFlag;
        touchid = sFtRegMap.m_sTP[i].u8TouchID;

        if (touchid >= 0x0f)
            continue;

        pre_id[i] = touchid;

        if ((touch_event == FT_EVENTFLAG_PRESS_DOWN) || (touch_event == FT_EVENTFLAG_CONTACT))
        {
            uint16_t  x, y, w;

            x = ((uint16_t)sFtRegMap.m_sTP[i].u8X_11_8 << 8) |  sFtRegMap.m_sTP[i].u8X_7_0;
            y = ((uint16_t)sFtRegMap.m_sTP[i].u8Y_11_8 << 8) |  sFtRegMap.m_sTP[i].u8Y_7_0;
            w = sFtRegMap.m_sTP[i].m_u8Weight;

            if (x >= DISP_HOR_RES_MAX || y >= DISP_VER_RES_MAX)
            {
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

    return (psInDevData->state == NUMAKER_INDEV_STATE_PRESSED) ? 1 : 0;

exit_indev_touch_get_data:

    pre_touch = 0;

    return 0;
}

int indev_touch_init(void)
{
    uint8_t data = 0;

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

    touch_plat_i2c_init(&s_ft5316_i2c_if);

    ft5316_write_reg(0x0, &data, 1);

    return 0;
}
