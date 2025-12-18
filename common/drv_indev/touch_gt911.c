/**************************************************************************//**
 * @file     touch_gt911.c
 * @brief    gt911 touch driver
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2024 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include <string.h>
#include "indev_touch.h"
#include "plat_touch.h"

#define GT911_REGITER_LEN     2
#define GT911_MAX_TOUCH       10
#define GT911_ADDRESS1        0x5D
#define GT911_ADDRESS2        0x14

#define CONFIG_MAX_TOUCH                     10
//#define CONFIG_XY_REVERSED                   1

#define GT911_COMMAND_REG                    0x8040

#define GT911_CONFIG_START                   0x8047
#define GT911_CONFIG_VERION                  GT911_CONFIG_START
#define GT911_CONFIG_X_OUTPUT_MAX_LOW        0x8048
#define GT911_CONFIG_X_OUTPUT_MAX_HIGH       0x8049
#define GT911_CONFIG_Y_OUTPUT_MAX_LOW        0x804A
#define GT911_CONFIG_Y_OUTPUT_MAX_HIGH       0x804B
#define GT911_CONFIG_TOUCH_NUMBER            0x804C
#define GT911_CONFIG_MODULE_SWITCH1          0x804D
#define GT911_CONFIG_MODULE_SWITCH2          0x804E
#define GT911_CONFIG_SHAKE_COUNT             0x804F
#define GT911_CONFIG_FILTER                  0x8050
#define GT911_CONFIG_LARGE_TOUCH             0x8051
#define GT911_CONFIG_NOISE_REDUCTION         0x8052
#define GT911_CONFIG_SCREEN_TOUCH_LEVEL      0x8053
#define GT911_CONFIG_SCREEN_LEAVE_LEVEL      0x8054
#define GT911_CONFIG_LOWER_POWER_CONTROL     0x8055
#define GT911_CONFIG_REFRESH_RATE            0x8056
#define GT911_CONFIG_X_THRESHOLD             0x8057
#define GT911_CONFIG_Y_THRESHOLD             0x8058
#define GT911_CONFIG_X_SPEED_LIMIT           0x8059
#define GT911_CONFIG_Y_SPEED_LIMIT           0x805A
#define GT911_CONFIG_SPEED_TOP_BOTTOM        0x805B
#define GT911_CONFIG_SPEED_LEFT_RIGHT        0x805C
#define GT911_CONFIG_MINI_FILTER             0x805D
#define GT911_CONFIG_STRETCH_R0              0x805E
#define GT911_CONFIG_STRETCH_R1              0x805F
#define GT911_CONFIG_STRETCH_R2              0x8060
#define GT911_CONFIG_STRETCH_RM              0x8061
#define GT911_CONFIG_DRV_GROUP_A_NUM         0x8062
#define GT911_CONFIG_DRV_GROUP_B_NUM         0x8063
#define GT911_CONFIG_SENSOR_NUM              0x8064
#define GT911_CONFIG_FREQ_A_FACTOR           0x8065
#define GT911_CONFIG_FREQ_B_FACTOR           0x8066
#define GT911_CONFIG_PANEL_BIT_FREQ_L        0x8067
#define GT911_CONFIG_PANEL_BIT_FREQ_H        0x8068
#define GT911_CONFIG_PANEL_SENSOR_TIME_L     0x8069
#define GT911_CONFIG_PANEL_SENSOR_TIME_H     0x806A
#define GT911_CONFIG_PANEL_TX_GAIN           0x806B
#define GT911_CONFIG_PANEL_RX_GAIN           0x806C
#define GT911_CONFIG_PANEL_DUMP_SHIFT        0x806D
#define GT911_CONFIG_DRV_FRAME_CONTROL       0x806E
#define GT911_CONFIG_CHARGING_LEVEL_UP       0x806F
#define GT911_CONFIG_MODULE_SWITCH3          0x8070
#define GT911_CONFIG_GESTURE_DIS             0x8071
#define GT911_CONFIG_GESTURE_LONG_PRESS_TIME 0x8072
#define GT911_CONFIG_XY_SLOPE_ADJUST         0x8073
#define GT911_CONFIG_GESTURE_CONTROL         0x8074
#define GT911_CONFIG_GESTURE_SWITCH1         0x8075
#define GT911_CONFIG_GESTURE_SWITCH2         0x8076
#define GT911_CONFIG_GESTURE_REFRESH_RATE    0x8077
#define GT911_CONFIG_GESTURE_TOUCH_LEVEL     0x8078
#define GT911_CONFIG_NEW_GREEN_WAKEUP_LEVEL  0x8079
#define GT911_CONFIG_FREQ_HOPPING_START      0x807A
#define GT911_CONFIG_FREQ_HOPPING_END        0x807B
#define GT911_CONFIG_NOISE_DETECT_TIMES      0x807C
#define GT911_CONFIG_HOPPING_FLAG            0x807D
#define GT911_CONFIG_HOPPING_THRESHOLD       0x807E
#define GT911_CONFIG_NOISE_THRESHOLD         0x807F
#define GT911_CONFIG_NOISE_MIN_THRESHOLD     0x8080
#define GT911_CONFIG_HOPPING_SENSOR_GROUP    0x8082
#define GT911_CONFIG_HOPPING_SEG1_NORMALIZE  0x8083
#define GT911_CONFIG_HOPPING_SEG1_FACTOR     0x8084
#define GT911_CONFIG_MAIN_CLOCK_ADJUST       0x8085
#define GT911_CONFIG_HOPPING_SEG2_NORMALIZE  0x8086
#define GT911_CONFIG_HOPPING_SEG2_FACTOR     0x8087
#define GT911_CONFIG_HOPPING_SEG3_NORMALIZE  0x8089
#define GT911_CONFIG_HOPPING_SEG3_FACTOR     0x808A
#define GT911_CONFIG_HOPPING_SEG4_NORMALIZE  0x808C
#define GT911_CONFIG_HOPPING_SEG4_FACTOR     0x808D
#define GT911_CONFIG_HOPPING_SEG5_NORMALIZE  0x808F
#define GT911_CONFIG_HOPPING_SEG5_FACTOR     0x8090
#define GT911_CONFIG_HOPPING_SEG6_NORMALIZE  0x8092
#define GT911_CONFIG_KEY_1                   0x8093
#define GT911_CONFIG_KEY_2                   0x8094
#define GT911_CONFIG_KEY_3                   0x8095
#define GT911_CONFIG_KEY_4                   0x8096
#define GT911_CONFIG_KEY_AREA                0x8097
#define GT911_CONFIG_KEY_TOUCH_LEVEL         0x8098
#define GT911_CONFIG_KEY_LEAVE_LEVEL         0x8099
#define GT911_CONFIG_KEY_SENS_1_2            0x809A
#define GT911_CONFIG_KEY_SENS_3_4            0x809B
#define GT911_CONFIG_KEY_RESTRAIN            0x809C
#define GT911_CONFIG_KEY_RESTRAIN_TIME       0x809D
#define GT911_CONFIG_GESTURE_LARGE_TOUCH     0x809E
#define GT911_CONFIG_COMBINE_DIS             0x80B3
#define GT911_CONFIG_SPLIT_SET               0x80B4
#define GT911_CONFIG_SENSOR_CH0              0x80B7  // ~ 0x80C4
#define GT911_CONFIG_DRIVER_CH0              0x80D5  // ~ 0x80EE
#define GT911_CONFIG_STOP                    0x80FE  // Config STOP

#define GT911_CONFIG_CHECKSUM                0x80FF  // Config checksum
#define GT911_CONFIG_REFRESH_FLAG            0x8100  // Config updated flag

#define GT911_CONFIG_BUFFER_SIZE             (GT911_CONFIG_CHECKSUM-GT911_CONFIG_START)

#define GT911_PRODUCT_ID                     0x8140
#define GT911_VENDOR_ID                      0x814A

#define GT911_READ_STATUS                    0x814E
#define GT911_POINT1_REG                     0x814F

/* hardware section */
static uint8_t GT911_CFG_TBL[GT911_CONFIG_BUFFER_SIZE];
static int16_t pre_x[CONFIG_MAX_TOUCH];
static int16_t pre_y[CONFIG_MAX_TOUCH];
static int16_t pre_w[CONFIG_MAX_TOUCH];
static int16_t s_tp_dowm[CONFIG_MAX_TOUCH];
static int16_t pre_id[CONFIG_MAX_TOUCH];
static uint8_t pre_touch = 0;

static S_TOUCH_IF_I2C s_gt911_i2c_if =
{
    .m_pvI2C      = CONFIG_INDEV_TOUCH_I2C,
    .m_pu8Reg     = NULL,
    .m_u32RegLen  = GT911_REGITER_LEN,
    .m_pu8Data    = NULL,
    .m_u32DataLen = 0,
    .m_pvPrivate  = NULL,
    .m_u8DevAddr  = GT911_ADDRESS2,
};

static uint8_t gt911_calculate_checksum(uint8_t *config, uint16_t len)
{
    uint16_t sum = 0;

    for (uint16_t i = 0; i < len; i++)
    {
        sum += config[i];
    }

    return (~(sum & 0xFF)) + 1;
}

static int gt911_write_reg(uint16_t reg, uint8_t data[], uint32_t len)
{
    S_TOUCH_IF_I2C *psIfCtx = &s_gt911_i2c_if;

    psIfCtx->m_pu8Reg = (uint8_t *)&reg;
    psIfCtx->m_u32RegLen = sizeof(uint16_t);
    psIfCtx->m_pu8Data = data;
    psIfCtx->m_u32DataLen = len;

    return touch_plat_i2c_write(psIfCtx);
}

static int gt911_read_reg(uint16_t reg, uint8_t data[], uint32_t len)
{
    S_TOUCH_IF_I2C *psIfCtx = &s_gt911_i2c_if;

    psIfCtx->m_pu8Reg = (uint8_t *)&reg;
    psIfCtx->m_u32RegLen = sizeof(uint16_t);
    psIfCtx->m_pu8Data = data;
    psIfCtx->m_u32DataLen = len;

    return touch_plat_i2c_read(psIfCtx);
}

static int gt911_get_product_id(void)
{
    typedef struct
    {
        uint8_t  u8PID1;
        uint8_t  u8PID2;
        uint8_t  u8PID3;
        uint8_t  u8PID4;
        uint16_t u16FWVersion;
    } S_GT911_PRODUCT_ID;

    S_GT911_PRODUCT_ID sPID = {0};

    if (gt911_read_reg(GT911_PRODUCT_ID, (uint8_t *)&sPID, sizeof(sPID)) != 0)
    {
        LV_LOG_ERROR("read id failed");
        return -1;
    }

    LV_LOG_INFO("Product ID: GT%c%c%c%c", sPID.u8PID1, sPID.u8PID2, sPID.u8PID3, sPID.u8PID4);
    LV_LOG_INFO("Firmware Version: %04X", sPID.u16FWVersion);

    return 0;
}

static int gt911_get_info(void)
{
    uint8_t out_info[7];

    if (gt911_read_reg(GT911_CONFIG_START, out_info, sizeof(out_info)) != 0)
    {
        LV_LOG_ERROR("read info failed");
        return -1;
    }

    LV_LOG_INFO("X range: %d", (out_info[2] << 8) | out_info[1]);
    LV_LOG_INFO("Y range: %d", (out_info[4] << 8) | out_info[3]);
    LV_LOG_INFO("Point number: %d", out_info[5] & 0x0f);

    return 0;
}

static int gt911_soft_reset(void)
{
    uint8_t u8Data = 0x02;

    if (gt911_write_reg(GT911_COMMAND_REG, &u8Data, sizeof(u8Data)) != 0)
    {
        LV_LOG_ERROR("soft reset failed");
        return -1;
    }

    return 0;
}

static void gt911_touch_up(lv_indev_data_t *buf, int16_t id)
{
    s_tp_dowm[id] = 0;

    if (id == 0)
    {
        buf[id].state = LV_INDEV_STATE_RELEASED;
        buf[id].point.x = pre_x[id];
        buf[id].point.y = pre_y[id];
    }

    pre_x[id] = -1;  /* last point is none */
    pre_y[id] = -1;
    pre_w[id] = -1;
}

static void gt911_touch_down(lv_indev_data_t *buf, int8_t id, int16_t x, int16_t y, int16_t w)
{
    s_tp_dowm[id] = 1;

    if (id == 0)
    {
        buf[id].point.x = x;
        buf[id].point.y = y;
        buf[id].state = LV_INDEV_STATE_PRESSED;
    }

    pre_x[id] = x; /* save last point */
    pre_y[id] = y;
    pre_w[id] = w;
}

int indev_touch_get_data(lv_indev_data_t *psInDevData)
{
    int i, error = 0;
    int32_t touch_event, touchid;
    uint8_t point_status = 0;
    uint8_t touch_num = 0;
    uint8_t read_buf[8 * GT911_MAX_TOUCH] = {0};

    /* point status register */
    error = gt911_read_reg(GT911_READ_STATUS, &point_status, 1);
    if (error)
    {
        goto exit_indev_touch_get_data;
    }

    touch_num = point_status & 0x0F;

    /* data is not ready */
    if (((point_status & 0x80) == 0) ||
            (touch_num > CONFIG_MAX_TOUCH))
    {
        goto exit_indev_touch_get_data;
    }

    if (touch_num > 0)
    {
        error = gt911_read_reg(GT911_POINT1_REG, &read_buf[0], 8 * touch_num);
        if (error)
        {
            goto exit_indev_touch_get_data;
        }
    }

    if (pre_touch > touch_num)               /* point up */
    {
        for (i = 0; i < CONFIG_MAX_TOUCH; i++)
        {
            uint8_t j;

            for (j = 0; j < touch_num; j++)  /* this time touch num */
            {
                touchid = read_buf[j * 8] & 0x0F;

                if (pre_id[i] == touchid)    /* this id is not free */
                    break;
            }

            if ((j == touch_num) && (pre_id[i] != -1))         /* free this node */
            {
                gt911_touch_up(psInDevData, pre_id[i]);
                pre_id[i] = -1;
            }
        }
    }

    if (touch_num > 0)
    {
        for (i = 0; i < touch_num; i++)
        {
            uint16_t  x, y, w;
            uint8_t off_set = i * 8;
            pre_id[i] = touchid = read_buf[off_set] & 0x0f;

            x = read_buf[off_set + 1] | (read_buf[off_set + 2] << 8); /* x */
            y = read_buf[off_set + 3] | (read_buf[off_set + 4] << 8); /* y */
            w = read_buf[off_set + 5] | (read_buf[off_set + 6] << 8); /* size */

#if defined(CONFIG_XY_REVERSED)
            gt911_touch_down(psInDevData, touchid, (LV_HOR_RES_MAX - 1) - x, (LV_VER_RES_MAX - 1) - y, w);
#else
            gt911_touch_down(psInDevData, touchid, x, y, w);
#endif
        }
    }

    pre_touch = touch_num;

exit_indev_touch_get_data:

    {
        uint8_t u8Data = 0x00;
        gt911_write_reg(GT911_READ_STATUS, &u8Data, sizeof(u8Data));
    }

    // LV_LOG_TRACE("%s (%d, %d)\n", psInDevData->state ? "Press" : "Release", psInDevData->point.x, psInDevData->point.y);

    return (psInDevData->state == LV_INDEV_STATE_PRESSED) ? 1 : 0;
}

static void gt911_dump_config(void)
{
    // Update configuration to GT911
    gt911_read_reg(GT911_CONFIG_START, GT911_CFG_TBL, sizeof(GT911_CFG_TBL));

    LV_LOG_INFO("========================================================");
    for (int i = 0; i < sizeof(GT911_CFG_TBL); i += 8)
    {
        LV_LOG_INFO("0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X",
                    GT911_CFG_TBL[i + 0],
                    GT911_CFG_TBL[i + 1],
                    GT911_CFG_TBL[i + 2],
                    GT911_CFG_TBL[i + 3],
                    GT911_CFG_TBL[i + 4],
                    GT911_CFG_TBL[i + 5],
                    GT911_CFG_TBL[i + 6],
                    GT911_CFG_TBL[i + 7]);
    }
    LV_LOG_INFO("========================================================");
}

int indev_touch_init(void)
{
    int ret;

    touch_plat_i2c_init(&s_gt911_i2c_if);

    memset(&pre_x[0], 0xff,   CONFIG_MAX_TOUCH * sizeof(int16_t));
    memset(&pre_y[0], 0xff,   CONFIG_MAX_TOUCH * sizeof(int16_t));
    memset(&pre_w[0], 0xff,   CONFIG_MAX_TOUCH * sizeof(int16_t));
    memset(&s_tp_dowm[0], 0,  CONFIG_MAX_TOUCH * sizeof(int16_t));
    memset(&pre_id[0], 0xff,  CONFIG_MAX_TOUCH * sizeof(int16_t));

    ret = gt911_soft_reset();
    if (ret)
    {
        s_gt911_i2c_if.m_u8DevAddr = GT911_ADDRESS1;
        ret = gt911_soft_reset();
        if (ret)
        {
            touch_plat_i2c_fini(&s_gt911_i2c_if);
            return -1;
        }
    }

    gt911_get_product_id();
    gt911_get_info();

    // Update configuration to GT911
    gt911_dump_config();

    GT911_CFG_TBL[GT911_CONFIG_START - GT911_CONFIG_START] = 0x6B;

    // Update PANEL - Width.
    GT911_CFG_TBL[GT911_CONFIG_X_OUTPUT_MAX_LOW - GT911_CONFIG_START] = (LV_HOR_RES_MAX & 0xFF); // X output max low byte
    GT911_CFG_TBL[GT911_CONFIG_X_OUTPUT_MAX_HIGH - GT911_CONFIG_START] = (LV_HOR_RES_MAX >> 8) & 0xFF; // X output max high byte

    // Update PANEL - Height.
    GT911_CFG_TBL[GT911_CONFIG_Y_OUTPUT_MAX_LOW - GT911_CONFIG_START] = (LV_VER_RES_MAX & 0xFF); // Y output max low byte
    GT911_CFG_TBL[GT911_CONFIG_Y_OUTPUT_MAX_HIGH - GT911_CONFIG_START] = (LV_VER_RES_MAX >> 8) & 0xFF; // Y output max high byte

    /* INT mode, Rising triggering */
    GT911_CFG_TBL[GT911_CONFIG_MODULE_SWITCH1 - GT911_CONFIG_START] = 0x04;

    // Calculate checksum.
    GT911_CFG_TBL[GT911_CONFIG_CHECKSUM - GT911_CONFIG_START - 1 ] = gt911_calculate_checksum(GT911_CFG_TBL, GT911_CONFIG_STOP - GT911_CONFIG_START);

    // Update configuration to GT911
    gt911_write_reg(GT911_CONFIG_START, GT911_CFG_TBL, sizeof(GT911_CFG_TBL));

    // Set refresh flag to 1, to make GT911 read the new configuration.
    {
        uint8_t u8Data = 1;
        gt911_write_reg(GT911_CONFIG_REFRESH_FLAG, &u8Data, sizeof(u8Data));
    }

    gt911_dump_config();

    return 0;
}
