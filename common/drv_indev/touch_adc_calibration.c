/**************************************************************************//**
 * @file     adc_touch.c
 * @brief    adc touch driver and utility
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include <string.h>
#include "numaker_disp.h"
#include "numaker_touch.h"

#define DEF_CAL_POINT_NUM  5
#define DEF_DOT_NUMBER    9
#define DOTS_NUMBER       (DEF_DOT_NUMBER*DEF_DOT_NUMBER)

extern S_CALIBRATION_MATRIX g_sCalMat;

static const S_CALIBRATION_MATRIX g_sCalZero = { 1, 0, 0, 0, 1, 0, 1 };

/**
 * @brief Reset touch calibration matrix to identity/zero state.
 *
 * Resets the global calibration matrix to default (no transformation).
 * This effectively disables any coordinate mapping.
 */
void ad_touch_reset_calmat(void)
{
    memcpy(&g_sCalMat, &g_sCalZero, sizeof(S_CALIBRATION_MATRIX));
}

/**
 * @brief Update touch calibration matrix with new calibration data.
 *
 * Applies new calibration coefficients if valid (non-zero divisor).
 * Prints confirmation message showing the applied calibration values.
 *
 * @param psNewCalMat[in]  Pointer to new calibration matrix structure
 */
void ad_touch_update_calmat(S_CALIBRATION_MATRIX *psNewCalMat)
{
    if (psNewCalMat->div != 0)
    {
        memcpy(&g_sCalMat, psNewCalMat, sizeof(S_CALIBRATION_MATRIX));
//        printf("Applied calibration data: %d, %d, %d, %d, %d, %d, %d\n",
//               g_sCalMat.a,
//               g_sCalMat.b,
//               g_sCalMat.c,
//               g_sCalMat.d,
//               g_sCalMat.e,
//               g_sCalMat.f,
//               g_sCalMat.div);

    }
}

/**
 * @brief Calculate touch calibration matrix from reference points.
 *
 * Computes transformation matrix coefficients using either:
 * - 3-point method: Fast, but may have lower accuracy
 * - 5-point method: Uses least-squares fitting for better accuracy
 *
 * The calibration matrix maps raw ADC coordinates to display coordinates:
 * display_x = (a*adc_x + b*adc_y + c) / div
 * display_y = (d*adc_x + e*adc_y + f) / div
 *
 * @param psDispCP[in]   Pointer to display (reference) calibration points
 * @param psADCCP[in]    Pointer to raw ADC calibration points
 * @param psCM[out]      Pointer to calculated calibration matrix
 * @return               0 on success, -1 if calibration points are degenerate
 */
int ad_cal_mat_get(const numaker_point_t *psDispCP, numaker_point_t *psADCCP, S_CALIBRATION_MATRIX *psCM)
{
#if (DEF_CAL_POINT_NUM==3)
    /* Calculate determinant for matrix inversion */
    psCM->div = ((psADCCP[0].x - psADCCP[2].x) * (psADCCP[1].y - psADCCP[2].y)) -
                ((psADCCP[1].x - psADCCP[2].x) * (psADCCP[0].y - psADCCP[2].y)) ;

    if (psCM->div == 0)
    {
        /* Calibration points are collinear - cannot calculate transformation */
        return -1;
    }
    else
    {
        /* Calculate transformation coefficients using Cramer's rule */
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
    /* Use least-squares fitting for 5-point calibration (more accurate) */
    int i;
    float n, x, y, xx, yy, xy, z, zx, zy;
    float a, b, c, d, e, f, g;
    float scaling = 65536.0f;

    /* Calculate sums for least-squares matrix */
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

    /* Calculate least-squares determinant */
    d = n * (xx * yy - xy * xy) + x * (xy * y - x * yy) + y * (x * xy - y * xx);
    if (d < (float)0.1 && d > (float) -0.1)
    {
        /* Calibration points are degenerate - cannot calculate transformation */
        return -1;
    }

    /* Calculate inverse matrix elements */
    a = (xx * yy - xy * xy) / d;
    b = (xy * y  - x * yy)  / d;
    c = (x * xy  - y * xx)  / d;
    e = (n * yy  - y * y)   / d;
    f = (x * y   - n * xy)  / d;
    g = (n * xx  - x * x)   / d;

    /* Calculate X-axis coefficients */
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

    /* Calculate Y-axis coefficients */
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
    {DISP_HOR_RES_MAX / 4, DISP_VER_RES_MAX / 2},
    {DISP_HOR_RES_MAX - DISP_HOR_RES_MAX / 4, DISP_VER_RES_MAX / 4},
    {DISP_HOR_RES_MAX / 2, DISP_VER_RES_MAX - DISP_VER_RES_MAX / 4}
};
#elif (DEF_CAL_POINT_NUM==5)
const static numaker_point_t sDispPoints[DEF_CAL_POINT_NUM] =
{
#define DEF_CUT_PIECES 8
    {DISP_HOR_RES_MAX / DEF_CUT_PIECES, DISP_VER_RES_MAX / DEF_CUT_PIECES},
    {DISP_HOR_RES_MAX - DISP_HOR_RES_MAX / DEF_CUT_PIECES, DISP_VER_RES_MAX / DEF_CUT_PIECES},
    {DISP_HOR_RES_MAX - DISP_HOR_RES_MAX / DEF_CUT_PIECES, DISP_VER_RES_MAX - DISP_VER_RES_MAX / DEF_CUT_PIECES},
    {DISP_HOR_RES_MAX / DEF_CUT_PIECES, DISP_VER_RES_MAX - DISP_VER_RES_MAX / DEF_CUT_PIECES},

    {DISP_HOR_RES_MAX / 2, DISP_VER_RES_MAX / 2}
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
    int i, line = 0;
    S_LCD_INFO sLcdInfo = {0};
    disp_area_t area;

    if (lcd_device_control(evLCD_CTRL_GET_INFO, (void *)&sLcdInfo) != 0)
    {
        while (1);
    }

    uint32_t u32LineBufNum = sLcdInfo.u32VramSize / sLcdInfo.u32BytePerPixel / sLcdInfo.u32ResWidth;
    u32LineBufNum = (u32LineBufNum > DISP_VER_RES_MAX) ? DISP_VER_RES_MAX : u32LineBufNum;

    switch (sLcdInfo.evLCDType)
    {
    case evLCD_TYPE_SYNC:
    {
#if (DISP_COLOR_DEPTH == 16)  //RGB565
        volatile uint16_t *plvColorStart = (volatile uint16_t *)sLcdInfo.pvVramStartAddr;
#elif (DISP_COLOR_DEPTH == 32)  //ARGB888
        volatile uint32_t *plvColorStart = (volatile uint32_t *)sLcdInfo.pvVramStartAddr;
#endif
        for (i = 0; i < DISP_HOR_RES_MAX * DISP_VER_RES_MAX; i++)
        {
#if (DISP_COLOR_DEPTH == 16)  //RGB565
            *((uint16_t *)plvColorStart) = 0xCADB;
#elif (DISP_COLOR_DEPTH == 32)  //ARGB888
            *((uint32_t *)plvColorStart) = (uint32_t)0xFF97CADB;
#endif
            plvColorStart++;
        }
    }
    break;

    case evLCD_TYPE_MPU:
    {
        while (line < DISP_VER_RES_MAX)
        {
#if (DISP_COLOR_DEPTH == 16)  //RGB565
            volatile uint16_t *plvColorStart = (volatile uint16_t *)sLcdInfo.pvVramStartAddr;
#elif (DISP_COLOR_DEPTH == 32)  //ARGB888
            volatile uint32_t *plvColorStart = (volatile uint32_t *)sLcdInfo.pvVramStartAddr;
#endif

            for (i = 0; i < DISP_HOR_RES_MAX * u32LineBufNum; i++)
            {
#if (DISP_COLOR_DEPTH == 16)  //RGB565
                *((uint16_t *)plvColorStart) = 0xCADB;
#elif (DISP_COLOR_DEPTH == 32)  //ARGB888
                *((uint32_t *)plvColorStart) = (uint32_t)0xFF97CADB;
#endif
                plvColorStart++;
            }

            area.x1 = 0;
            area.y1 = line;
            area.x2 = DISP_HOR_RES_MAX - 1;
            area.y2 = ((line + u32LineBufNum) < DISP_VER_RES_MAX) ? (line + u32LineBufNum) - 1 : DISP_VER_RES_MAX - 1;

            /* Update dirty region. */
            if (lcd_device_control(evLCD_CTRL_RECT_UPDATE, (void *)&area) != 0)
            {
                while (1);
            }

            line += u32LineBufNum;
        }
    }
    break;

    default:
        break;
    }
}

static void _draw_bots(int x, int y)
{
    disp_area_t area;

    /* Rendering */
    int start_x = x - (DEF_DOT_NUMBER / 2);
    int start_y = y - (DEF_DOT_NUMBER / 2);

    S_LCD_INFO sLcdInfo = {0};
    if (lcd_device_control(evLCD_CTRL_GET_INFO, (void *)&sLcdInfo) != 0)
    {
        while (1);
    }

    switch (sLcdInfo.evLCDType)
    {
    case evLCD_TYPE_SYNC:
    {
        int i, j;
#if (DISP_COLOR_DEPTH == 16)  //RGB565
        volatile uint16_t *plvColorStart = (volatile uint16_t *)((uintptr_t)sLcdInfo.pvVramStartAddr + (start_y) * (DISP_HOR_RES_MAX * (DISP_COLOR_DEPTH / 8)) + (start_x * 2));
#elif (DISP_COLOR_DEPTH == 32)  //ARGB888
        volatile uint32_t *plvColorStart = (volatile uint32_t *)((uintptr_t)sLcdInfo.pvVramStartAddr + (start_y) * (DISP_HOR_RES_MAX * (DISP_COLOR_DEPTH / 8)) + (start_x * 4));
#endif
        for (i = 0; i < DEF_DOT_NUMBER; i++)
        {
            for (j = 0; j < DEF_DOT_NUMBER; j++)
            {
#if (DISP_COLOR_DEPTH == 16)  //RGB565
                *plvColorStart = 0x07E0; //RGB565
#elif (DISP_COLOR_DEPTH == 32)  //ARGB888
                *plvColorStart = 0xFF001B48; //ARGB888
#endif
                plvColorStart++;
            }
            plvColorStart += (DISP_HOR_RES_MAX - DEF_DOT_NUMBER);
        }
    }
    break;

    case evLCD_TYPE_MPU:
    {
        int i;
#if (DISP_COLOR_DEPTH == 16)  //RGB565
        volatile uint16_t *plvColorStart = (volatile uint16_t *)sLcdInfo.pvVramStartAddr;
#elif (DISP_COLOR_DEPTH == 32)  //ARGB888
        volatile uint32_t *plvColorStart = (volatile uint32_t *)sLcdInfo.pvVramStartAddr;
#endif
        for (i = 0; i < DEF_DOT_NUMBER * DEF_DOT_NUMBER; i++)
        {
#if (DISP_COLOR_DEPTH == 16)  //RGB565
            *plvColorStart = 0x07E0; //RGB565
#elif (DISP_COLOR_DEPTH == 32)  //ARGB888
            *plvColorStart = 0xFF001B48; //ARGB888
#endif
            plvColorStart++;
        }

        area.x1 = start_x;
        area.y1 = start_y;
        area.x2 = start_x + DEF_DOT_NUMBER;
        area.y2 = start_y + DEF_DOT_NUMBER;
    }
    break;
    default:
        break;
    }

    /* Update dirty region. */
    lcd_device_control(evLCD_CTRL_RECT_UPDATE, (void *)&area);


    return;
}


int ad_touch_calibrate(void)
{
    int i;
    S_CALIBRATION_MATRIX sCalMat;
    numaker_point_t sADCPoints[DEF_CAL_POINT_NUM];
    numaker_indev_data_t sInDevData;

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
                u32SumX += (uint32_t)sInDevData.point.x;
                u32SumY += (uint32_t)sInDevData.point.y;
//                printf("[%d %d] - Disp:[%d, %d] -> ADC:[%d, %d], SUM:[%d, %d]\n",
//                       i,
//                       count,
//                       sDispPoints[i].x,
//                       sDispPoints[i].y,
//                       sInDevData.point.x,
//                       sInDevData.point.y,
//                       u32SumX,
//                       u32SumY);
                count++;
            }
        }

        sADCPoints[i].x = (uint32_t)((float)u32SumX / DEF_MAX_GET_POINT_NUM);
        sADCPoints[i].y = (uint32_t)((float)u32SumY / DEF_MAX_GET_POINT_NUM);
//        printf("[%d] - Disp:[%d, %d], AVG-ADC:[%d, %d]\n", i, sDispPoints[i].x, sDispPoints[i].y, sADCPoints[i].x, sADCPoints[i].y);

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
            int32_t sumx, sumy;
            sumx = (int32_t)sADCPoints[i].x;
            sumy = (int32_t)sADCPoints[i].y;

            ad_touch_map(&sumx, &sumy);

//            printf("[%d] - Disp:[%d, %d], ADC:[%d, %d] -> map[%d, %d]\n", i, sDispPoints[i].x, sDispPoints[i].y, sADCPoints[i].x, sADCPoints[i].y, sumx, sumy);
        }
    }
    else
    {
//        printf("Failed to calibrate.");
    }

    return 0;
}
