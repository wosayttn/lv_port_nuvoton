/**************************************************************************//**
 * @file     adc_touch.c
 * @brief    adc touch driver and utility
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "lvgl.h"
#include "lv_glue.h"

#define DEF_CAL_POINT_NUM  5
#define DEF_DOT_NUMBER    9
#define DOTS_NUMBER       (DEF_DOT_NUMBER*DEF_DOT_NUMBER)

/* User can define ADC touch calibration matrix in board_dev.c. */
#if defined(__320x240__)
S_CALIBRATION_MATRIX g_sCalMat = { 5873, 41, -1646858, 17, -4398, 16618786, 65536 };
#elif defined(__800x480__)
S_CALIBRATION_MATRIX g_sCalMat = { 13605, -12, -2163964, -126, -8477, 32548474, 65536 };
#endif

static const S_CALIBRATION_MATRIX g_sCalZero = { 1, 0, 0, 0, 1, 0, 1 };

void ad_touch_reset_calmat(void)
{
    memcpy(&g_sCalMat, &g_sCalZero, sizeof(S_CALIBRATION_MATRIX));
}

void ad_touch_update_calmat(S_CALIBRATION_MATRIX *psNewCalMat)
{
    LV_ASSERT(psNewCalMat);
    if (psNewCalMat->div != 0)
    {
        memcpy(&g_sCalMat, psNewCalMat, sizeof(S_CALIBRATION_MATRIX));
        LV_LOG_INFO("Applied calibration data: %d, %d, %d, %d, %d, %d, %d",
                    g_sCalMat.a,
                    g_sCalMat.b,
                    g_sCalMat.c,
                    g_sCalMat.d,
                    g_sCalMat.e,
                    g_sCalMat.f,
                    g_sCalMat.div);

    }
}

int ad_cal_mat_get(const lv_point_t *psDispCP, lv_point_t *psADCCP, S_CALIBRATION_MATRIX *psCM)
{
#if (DEF_CAL_POINT_NUM==3)

    psCM->div = ((psADCCP[0].x - psADCCP[2].x) * (psADCCP[1].y - psADCCP[2].y)) -
                ((psADCCP[1].x - psADCCP[2].x) * (psADCCP[0].y - psADCCP[2].y)) ;

    if (psCM->div == 0)
    {
        return -1;
    }
    else
    {
        psCM->a = ((psDispCP[0].x - psDispCP[2].x) * (psADCCP[1].y - psADCCP[2].y)) -
                  ((psDispCP[1].x - psDispCP[2].x) * (psADCCP[0].y - psADCCP[2].y)) ;

        psCM->b = ((psADCCP[0].x - psADCCP[2].x) * (psDispCP[1].x - psDispCP[2].x)) -
                  ((psDispCP[0].x - psDispCP[2].x) * (psADCCP[1].x - psADCCP[2].x)) ;

        psCM->c = (psADCCP[2].x * psDispCP[1].x - psADCCP[1].x * psDispCP[2].x) * psADCCP[0].y +
                  (psADCCP[0].x * psDispCP[2].x - psADCCP[2].x * psDispCP[0].x) * psADCCP[1].y +
                  (psADCCP[1].x * psDispCP[0].x - psADCCP[0].x * psDispCP[1].x) * psADCCP[2].y ;

        psCM->d = ((psDispCP[0].y - psDispCP[2].y) * (psADCCP[1].y - psADCCP[2].y)) -
                  ((psDispCP[1].y - psDispCP[2].y) * (psADCCP[0].y - psADCCP[2].y)) ;

        psCM->e = ((psADCCP[0].x - psADCCP[2].x) * (psDispCP[1].y - psDispCP[2].y)) -
                  ((psDispCP[0].y - psDispCP[2].y) * (psADCCP[1].x - psADCCP[2].x)) ;

        psCM->f = (psADCCP[2].x * psDispCP[1].y - psADCCP[1].x * psDispCP[2].y) * psADCCP[0].y +
                  (psADCCP[0].x * psDispCP[2].y - psADCCP[2].x * psDispCP[0].y) * psADCCP[1].y +
                  (psADCCP[1].x * psDispCP[0].y - psADCCP[0].x * psDispCP[1].y) * psADCCP[2].y ;
    }

#elif (DEF_CAL_POINT_NUM==5)

    int i;
    float n, x, y, xx, yy, xy, z, zx, zy;
    float a, b, c, d, e, f, g;
    float scaling = 65536.0f;

    n = x = y = xx = yy = xy = 0;
    for (i = 0; i < DEF_CAL_POINT_NUM; i++)
    {
        n  += (float)1.0;
        x  += (float)psADCCP[i].x;
        y  += (float)psADCCP[i].y;
        xx += (float)psADCCP[i].x * psADCCP[i].x;
        yy += (float)psADCCP[i].y * psADCCP[i].y;
        xy += (float)psADCCP[i].x * psADCCP[i].y;
    }

    d = n * (xx * yy - xy * xy) + x * (xy * y - x * yy) + y * (x * xy - y * xx);
    if (d < (float)0.1 && d > (float) -0.1)
    {
        return -1;
    }

    a = (xx * yy - xy * xy) / d;
    b = (xy * y  - x * yy)  / d;
    c = (x * xy  - y * xx)  / d;
    e = (n * yy  - y * y)   / d;
    f = (x * y   - n * xy)  / d;
    g = (n * xx  - x * x)   / d;

    z = zx = zy = 0;
    for (i = 0; i < DEF_CAL_POINT_NUM; i++)
    {
        z  += (float)psDispCP[i].x;
        zx += (float)psDispCP[i].x * psADCCP[i].x;
        zy += (float)psDispCP[i].x * psADCCP[i].y;
    }

    psCM->c = (int32_t)((a * z + b * zx + c * zy) * scaling);
    psCM->a = (int32_t)((b * z + e * zx + f * zy) * scaling);
    psCM->b = (int32_t)((c * z + f * zx + g * zy) * scaling);

    z = zx = zy = 0;
    for (i = 0; i < DEF_CAL_POINT_NUM; i++)
    {
        z  += (float)psDispCP[i].y;
        zx += (float)psDispCP[i].y * psADCCP[i].x;
        zy += (float)psDispCP[i].y * psADCCP[i].y;
    }

    psCM->f = (int32_t)((a * z + b * zx + c * zy) * scaling);
    psCM->d = (int32_t)((b * z + e * zx + f * zy) * scaling);
    psCM->e = (int32_t)((c * z + f * zx + g * zy) * scaling);

    psCM->div = (int32_t)scaling;

#else
#error "Not supported calibration method"
#endif
    return 0;
}

#if (DEF_CAL_POINT_NUM==3)
const lv_point_t sDispPoints[DEF_CAL_POINT_NUM] =
{
    {LV_HOR_RES_MAX / 4, LV_VER_RES_MAX / 2},
    {LV_HOR_RES_MAX - LV_HOR_RES_MAX / 4, LV_VER_RES_MAX / 4},
    {LV_HOR_RES_MAX / 2, LV_VER_RES_MAX - LV_VER_RES_MAX / 4}
};
#elif (DEF_CAL_POINT_NUM==5)
const static lv_point_t sDispPoints[DEF_CAL_POINT_NUM] =
{
#define DEF_CUT_PIECES 8
    {LV_HOR_RES_MAX / DEF_CUT_PIECES, LV_VER_RES_MAX / DEF_CUT_PIECES},
    {LV_HOR_RES_MAX - LV_HOR_RES_MAX / DEF_CUT_PIECES, LV_VER_RES_MAX / DEF_CUT_PIECES},
    {LV_HOR_RES_MAX - LV_HOR_RES_MAX / DEF_CUT_PIECES, LV_VER_RES_MAX - LV_VER_RES_MAX / DEF_CUT_PIECES},
    {LV_HOR_RES_MAX / DEF_CUT_PIECES, LV_VER_RES_MAX - LV_VER_RES_MAX / DEF_CUT_PIECES},

    {LV_HOR_RES_MAX / 2, LV_VER_RES_MAX / 2}
};
#endif

int ad_touch_map(int32_t *sumx, int32_t *sumy)
{
    int32_t xtemp, ytemp;

    if (g_sCalMat.div == 1)
        return -1;

    xtemp = *sumx;
    ytemp = *sumy;
    *sumx = (g_sCalMat.c +
             g_sCalMat.a * xtemp +
             g_sCalMat.b * ytemp) / g_sCalMat.div;
    *sumy = (g_sCalMat.f +
             g_sCalMat.d * xtemp +
             g_sCalMat.e * ytemp) / g_sCalMat.div;

    return 0;
}

static void _cleanscreen(void)
{
    /* Sync-type LCD panel, will fill to VRAM directly. */
    int i;
    static S_LCD_INFO sLcdInfo = {0};
    LV_ASSERT(lcd_device_control(evLCD_CTRL_GET_INFO, (void *)&sLcdInfo) == 0);

    volatile lv_color_t *plvColorStart = (volatile lv_color_t *)sLcdInfo.pvVramStartAddr;
    for (i = 0; i < LV_HOR_RES_MAX * LV_VER_RES_MAX; i++)
    {
#if (LV_COLOR_SIZE == 16)  //RGB565
        plvColorStart->full = 0xCADB;
#elif (LV_COLOR_SIZE == 32)  //ARGB888
        plvColorStart->full = (uint32_t)0xFF97CADB;
#endif

        plvColorStart++;
    }
}

static void _draw_bots(int x, int y)
{
    /* Rendering */
    int i, j;
    int start_x = x - (DEF_DOT_NUMBER / 2);
    int start_y = y - (DEF_DOT_NUMBER / 2);

    static S_LCD_INFO sLcdInfo = {0};
    LV_ASSERT(lcd_device_control(evLCD_CTRL_GET_INFO, (void *)&sLcdInfo) == 0);

#if (LV_COLOR_SIZE == 16)  //RGB565
    {
        uint16_t *pu16Start = (uint16_t *)((uint32_t)sLcdInfo.pvVramStartAddr + (start_y) * (LV_HOR_RES_MAX * sizeof(lv_color_t)) + (start_x * 2));
        for (i = 0; i < DEF_DOT_NUMBER; i++)
        {
            for (j = 0; j < DEF_DOT_NUMBER; j++)
            {
                *pu16Start = 0x07E0; //RGB565
                pu16Start++;
            }
            pu16Start += (LV_HOR_RES_MAX - DEF_DOT_NUMBER);
        }
    }
#elif (LV_COLOR_SIZE == 32)  //ARGB888
    {
        uint32_t *pu32Start = (uint32_t *)((uint32_t)sLcdInfo.pvVramStartAddr + (start_y) * (LV_HOR_RES_MAX * sizeof(lv_color_t)) + (start_x * 4));
        for (i = 0; i < DEF_DOT_NUMBER; i++)
        {
            for (j = 0; j < DEF_DOT_NUMBER; j++)
            {
                *pu32Start = 0xFF001B48; //ARGB888
                pu32Start++;
            }
            pu32Start += (LV_HOR_RES_MAX - DEF_DOT_NUMBER);
        }
    }
#else
    {
        LV_LOG_ERROR("Not supported.");
        return;
    }
#endif

    return;
}


int ad_touch_calibrate(void)
{
    int i;
    S_CALIBRATION_MATRIX sCalMat;
    lv_point_t sADCPoints[DEF_CAL_POINT_NUM];
    lv_indev_data_t sInDevData;

    ad_touch_reset_calmat();

    _cleanscreen();

    for (i = 0; i < DEF_CAL_POINT_NUM; i++)
    {
        int count = 0;
        uint32_t u32SumX, u32SumY;

        /* Ready to calibrate */
        _draw_bots(sDispPoints[i].x, sDispPoints[i].y);

#define DEF_MAX_GET_POINT_NUM 10

        u32SumX = 0;
        u32SumY = 0;

        while (count < DEF_MAX_GET_POINT_NUM)
        {
            if (!touchpad_device_read(&sInDevData))
            {
                continue;
            }
            else
            {
                u32SumX += (lv_coord_t)sInDevData.point.x;
                u32SumY += (lv_coord_t)sInDevData.point.y;
                LV_LOG_INFO("[%d %d] - Disp:[%d, %d] -> ADC:[%d, %d], SUM:[%d, %d]",
                            i,
                            count,
                            sDispPoints[i].x,
                            sDispPoints[i].y,
                            sInDevData.point.x,
                            sInDevData.point.y,
                            u32SumX,
                            u32SumY);
                count++;
            }
        }

        sADCPoints[i].x = (lv_coord_t)((float)u32SumX / DEF_MAX_GET_POINT_NUM);
        sADCPoints[i].y = (lv_coord_t)((float)u32SumY / DEF_MAX_GET_POINT_NUM);
        LV_LOG_INFO("[%d] - Disp:[%d, %d], AVG-ADC:[%d, %d]", i, sDispPoints[i].x, sDispPoints[i].y, sADCPoints[i].x, sADCPoints[i].y);

        _cleanscreen();

        while (touchpad_device_read(&sInDevData)); //Drain all unused points

        sysDelay(500);
    }

    _cleanscreen();

    /* Get calibration matrix. */
    if (ad_cal_mat_get(&sDispPoints[0], &sADCPoints[0], &sCalMat) == 0)
    {
        /* Finally, update calibration matrix to drivers. */
        ad_touch_update_calmat(&sCalMat);

        for (i = 0; i < DEF_CAL_POINT_NUM; i++)
        {
            LV_LOG_INFO("[%d] - Disp:[%d, %d], ADC:[%d, %d]", i, sDispPoints[i].x, sDispPoints[i].y, sADCPoints[i].x, sADCPoints[i].y);
        }
    }
    else
    {
        LV_LOG_ERROR("Failed to calibrate.");
    }

    return 0;
}

int ad_data_grab(void)
{
    uint32_t count = 0;

    lv_indev_data_t sInDevData;

    ad_touch_reset_calmat();

    while (1)
    {
        if (!touchpad_device_read(&sInDevData))
        {
            continue;
        }
        else
        {
            sysprintf("[%d] [%d %d]\n",
                      count,
                      sInDevData.point.x,
                      sInDevData.point.y);
            count++;
        }
    }
}
