#ifndef __NU_MISC_H__
#define __NU_MISC_H__

#include <stdint.h>
#include "nu_bitutil.h"
#include "nu_pin.h"

typedef enum
{
    evLCD_CTRL_GET_INFO,
    evLCD_CTRL_PAN_DISPLAY,
    evLCD_CTRL_WAIT_VSYNC,
    evLCD_CTRL_RECT_UPDATE,
    evLCD_CTRL_CNT
} E_LCD_CTRL;

typedef enum
{
    evLCD_TYPE_SYNC,
    evLCD_TYPE_MPU,
    evLCD_TYPE_CNT
} E_LCD_TYPE;

struct nu_module
{
    char      *name;
    void      *m_pvBase;
    uint32_t  u32RstId;
    uint32_t  u32ClkEnId;
    int       eIRQn;
} ;
typedef struct nu_module *nu_module_t;

typedef struct
{
    int32_t   a;
    int32_t   b;
    int32_t   c;
    int32_t   d;
    int32_t   e;
    int32_t   f;
    int32_t   div;
} S_CALIBRATION_MATRIX;

typedef struct
{
    void *pvVramStartAddr;  // VRAM Start address
    uint32_t u32VramSize;   // VRAM total size in byte
    uint32_t u32ResWidth;   // Resolution - Width
    uint32_t u32ResHeight;  // Resolution - Height
    uint32_t u32BytePerPixel;  // Byte per Pixel
    E_LCD_TYPE evLCDType;
} S_LCD_INFO;

#define NVT_ALIGN(size, align)        (((size) + (align) - 1) & ~((align) - 1))
#define NVT_ALIGN_DOWN(size, align)   ((size) & ~((align) - 1))
#define CONFIG_TICK_PER_SECOND        1000

void *nvt_malloc_align(uint32_t size, uint32_t align);
void nvt_free_align(void *ptr);

#endif //__NU_MISC_H__
