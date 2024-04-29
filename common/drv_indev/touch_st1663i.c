/**************************************************************************//**
 * @file     touch_st1663i.c
 * @brief    st1663i touch driver
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include <string.h>
#include "indev_touch.h"

#define ST1663I_REGITER_LEN  1
#define ST1663I_MAX_TOUCH    5
#define ST1663I_ADDRESS      0x55

#define CONFIG_MAX_TOUCH     1

typedef struct
{
    //012H*n+0 (n=0, 1, ...,4)
    union
    {
        uint8_t m_u8XY0_H;
        struct
        {
            uint8_t u8Y0_H: 3;
            uint8_t : 1;
            uint8_t u8X0_H: 3;
            uint8_t u8Valid: 1;
        };
    };

    //012H*n+1 (n=0, 1, ...,4)
    uint8_t m_u8X0_L;

    //012H*n+2 (n=0, 1, ...,4)
    uint8_t m_u8Y0_L;

    //012H*n+3 (n=0, 1, ...,4)
    uint8_t m_u8Z;

} S_ST_TP;

typedef struct
{
    union
    {
        uint8_t m_u8TouchInfo;
        struct
        {
            uint8_t u8Fingers: 4;
            uint8_t : 4;
        };
    };

    uint8_t m_u8Keys;

    S_ST_TP m_sTP[ST1663I_MAX_TOUCH];

} S_ST_REGMAP;

static int16_t pre_x[CONFIG_MAX_TOUCH];
static int16_t pre_y[CONFIG_MAX_TOUCH];
static int16_t pre_w[CONFIG_MAX_TOUCH];
static int16_t s_tp_dowm[CONFIG_MAX_TOUCH];
static int16_t pre_id[CONFIG_MAX_TOUCH];
static uint8_t pre_touch = 0;

static S_ST_REGMAP sStRegMap;

static int st1663i_write_reg(I2C_T *i2c, uint8_t reg, uint8_t value)
{
    return -I2C_WriteByteOneReg(i2c, ST1663I_ADDRESS, reg, value);
}

static int st1663i_read_reg(I2C_T *i2c, uint8_t reg, uint8_t *data, uint32_t len)
{
    return (I2C_ReadMultiBytesOneReg(i2c, ST1663I_ADDRESS, reg, data, len) == len) ? 0 : -1;
}

static void st1663i_touch_up(lv_indev_data_t *buf, int16_t id)
{
    s_tp_dowm[id] = 0;

    buf[id].state = LV_INDEV_STATE_RELEASED;
    buf[id].point.x = pre_x[id];
    buf[id].point.y = pre_y[id];

    pre_x[id] = -1;  /* last point is none */
    pre_y[id] = -1;
    pre_w[id] = -1;
}

static void st1663i_touch_down(lv_indev_data_t *buf, int8_t id, int16_t x, int16_t y, int16_t w)
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

    memset(&sStRegMap, 0, sizeof(S_ST_REGMAP));

    error = st1663i_read_reg(CONFIG_INDEV_TOUCH_I2C, 0x10, (uint8_t *)&sStRegMap, sizeof(sStRegMap));
    if (error)
    {
        LV_LOG_ERROR("Get touch data failed, err:");
        goto exit_indev_touch_get_data;
    }

    if (sStRegMap.u8Fingers > CONFIG_MAX_TOUCH)
    {
        LV_LOG_ERROR("FW report max point:%d > panel info. max:%d", sStRegMap.u8Fingers, CONFIG_MAX_TOUCH);
        goto exit_indev_touch_get_data;
    }

    if (pre_touch > sStRegMap.u8Fingers)               /* point up */
    {
        for (i = 0; i < CONFIG_MAX_TOUCH; i++)
        {
            uint8_t j;
            for (j = 0; j < sStRegMap.u8Fingers; j++)  /* this time touch num */
            {
                if (pre_id[i] == i)                /* this id is not free */
                    break;
            }

            if ((j == sStRegMap.u8Fingers) && (pre_id[i] != -1))         /* free this node */
            {
                // LV_LOG_INFO("free %d tid=%d\n", i, pre_id[i]);
                st1663i_touch_up(psInDevData, pre_id[i]);
                pre_id[i] = -1;
            }
        }
    }

    for (i = 0; i < sStRegMap.u8Fingers; i++)
    {
        pre_id[i] = i;

        if (sStRegMap.m_sTP[i].u8Valid)
        {
            uint16_t  x, y, w;

            x = ((uint16_t)sStRegMap.m_sTP[i].u8X0_H << 8) |  sStRegMap.m_sTP[i].m_u8X0_L;
            y = ((uint16_t)sStRegMap.m_sTP[i].u8Y0_H << 8) |  sStRegMap.m_sTP[i].m_u8Y0_L;
            w = sStRegMap.m_sTP[i].m_u8Z;

            LV_LOG_INFO("[%d] (%d %d %d %d)", sStRegMap.m_sTP[i].u8Valid, i, x, y, w);
            if (x >= LV_HOR_RES_MAX || y >= LV_VER_RES_MAX)
            {
                LV_LOG_ERROR("invalid position, X[%d,%u,%d], Y[%d,%u,%d]\n",
                             0, x, LV_HOR_RES_MAX,
                             0, y, LV_VER_RES_MAX);
                continue;
            }

            st1663i_touch_down(psInDevData, i, x, y, w);
        }
        else
        {
            // Up
            st1663i_touch_up(psInDevData, i);
        }

    } // for (i = 0; i < sStRegMap.u8TDStatus; i++)

    pre_touch = sStRegMap.u8Fingers;

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

    st1663i_write_reg(CONFIG_INDEV_TOUCH_I2C, 0x0, 0);

    return 0;
}
