/**************************************************************************//**
 * @file     ui_lang.h
 * @brief    Rice Cooker UI - Multi-language support (EN / JA / ZH-TW)
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#ifndef __UI_LANG_H__
#define __UI_LANG_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 * Language enum
 *============================================================================*/
typedef enum
{
    UI_LANG_EN = 0,     /* English */
    UI_LANG_JA,         /* Japanese */
    UI_LANG_ZH_TW,      /* Traditional Chinese */
    UI_LANG_MAX
} ui_lang_t;

/*============================================================================
 * String ID enum
 *============================================================================*/
typedef enum
{
    STR_APP_TITLE = 0,
    STR_READY,
    STR_SELECT_MODE,
    STR_COOKING,
    STR_COMPLETE,
    STR_KEEP_WARM,
    STR_PAUSED,

    /* Cooking modes */
    STR_MODE_WHITE_RICE,
    STR_MODE_QUICK_COOK,
    STR_MODE_PORRIDGE,
    STR_MODE_STEAM,
    STR_MODE_KEEP_WARM,

    /* Buttons / hints */
    STR_MENU_ENTER,
    STR_BTN_PAUSE,
    STR_BTN_STOP,
    STR_BTN_HOME,
    STR_BTN_BACK,
    STR_BTN_MENU,
    STR_HINT_HOME,
    STR_HINT_MENU,
    STR_HINT_COOKING,
    STR_HINT_FINISH,

    /* Cooking done */
    STR_COOKING_DONE,
    STR_KEEP_WARM_TEMP,

    STR_ID_MAX
} ui_str_id_t;

/*============================================================================
 * Public API
 *============================================================================*/

/**
 * @brief Set active language.
 */
void ui_lang_set(ui_lang_t lang);

/**
 * @brief Get active language.
 */
ui_lang_t ui_lang_get(void);

/**
 * @brief Cycle to next language (EN -> JA -> ZH_TW -> EN ...).
 */
void ui_lang_next(void);

/**
 * @brief Get a translated string by ID.
 */
const char *ui_str(ui_str_id_t id);

/**
 * @brief Get the CJK-capable font for the current language.
 *        Returns the appropriate font (built-in for EN, custom for JA/ZH).
 */
const lv_font_t *ui_lang_font_normal(void);
const lv_font_t *ui_lang_font_large(void);

#ifdef __cplusplus
}
#endif

#endif /* __UI_LANG_H__ */
