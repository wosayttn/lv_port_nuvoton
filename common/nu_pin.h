#ifndef __NU_PIN_H__
#define __NU_PIN_H__

typedef enum
{
    evGA,
    evGB,
    evGC,
    evGD,
    evGE,
    evGF,
    evGG,
    evGH,
    evGI,
    evGJ,
    evGK,
    evGL,
    evGM,
    evGN,
    evGO,
    evGP,
    evG_CNT
} nu_pin_group;

#define NU_GET_PININDEX(port, pin)     ((port)*16+(pin))
#define NU_GET_PIN(pin_index)          ((pin_index) & 0x0000000F)
#define NU_GET_PORT(pin_index)         (((pin_index)>>4) & 0x0000000F)
#define NU_GET_PIN_MASK(pin)           (1 << (pin))

#endif //__NU_PIN_H__
