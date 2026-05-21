/**************************************************************************//**
 * @file     ui_lang.c
 * @brief    Rice Cooker UI - Multi-language string tables
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#include "ui_common.h"
#include "ui_lang.h"

/*============================================================================
 * Custom CJK font declarations
 * Generated with lv_font_conv using SourceHanSansSC-Normal.otf
 * Contains all JA/ZH/KO/DE special characters used in UI strings.
 *============================================================================*/
LV_FONT_DECLARE(ui_font_cjk_16);
LV_FONT_DECLARE(ui_font_cjk_20);

static ui_lang_t s_lang = UI_LANG_EN;

/*============================================================================
 * String tables
 *============================================================================*/
static const char *s_strings_en[STR_ID_MAX] =
{
    [STR_APP_TITLE]       = "Rice Cooker",
    [STR_READY]           = LV_SYMBOL_OK "  Ready",
    [STR_SELECT_MODE]     = "Select Mode",
    [STR_COOKING]         = "Cooking",
    [STR_COMPLETE]        = "Complete!",
    [STR_KEEP_WARM]       = "Keep Warm",
    [STR_PAUSED]          = "PAUSED",

    [STR_MODE_WHITE_RICE] = "White Rice",
    [STR_MODE_QUICK_COOK] = "Quick Cook",
    [STR_MODE_PORRIDGE]   = "Porridge",
    [STR_MODE_STEAM]      = "Steam",
    [STR_MODE_KEEP_WARM]  = "Keep Warm",

    [STR_MENU_ENTER]      = "Menu",
    [STR_BTN_PAUSE]       = LV_SYMBOL_PAUSE " Pause",
    [STR_BTN_STOP]        = LV_SYMBOL_CLOSE " Stop",
    [STR_BTN_HOME]        = LV_SYMBOL_HOME "  Home",
    [STR_BTN_BACK]        = LV_SYMBOL_LEFT " Back",
    [STR_BTN_MENU]        = LV_SYMBOL_LIST "  Menu",
    [STR_HINT_HOME]       = "[MENU] Enter Menu",
    [STR_HINT_MENU]       = "[UP/DN] Select  [START] Confirm  [MENU] Back",
    [STR_HINT_COOKING]    = "[START] Pause/Resume  [MENU] Stop",
    [STR_HINT_FINISH]     = "[MENU] or [START] Return Home",

    [STR_COOKING_DONE]    = LV_SYMBOL_OK "  Cooking Done!",
    [STR_KEEP_WARM_TEMP]  = "Keep Warm: %d.%d " LV_SYMBOL_CHARGE "C",
};

static const char *s_strings_ja[STR_ID_MAX] =
{
    [STR_APP_TITLE]       = "\xe7\x82\x8a\xe9\xa3\xaf\xe5\x99\xa8",                             /* 炊飯器 */
    [STR_READY]           = LV_SYMBOL_OK "  \xe6\xba\x96\xe5\x82\x99\xe5\xae\x8c\xe4\xba\x86", /* 準備完了 */
    [STR_SELECT_MODE]     = "\xe3\x83\xa2\xe3\x83\xbc\xe3\x83\x89\xe9\x81\xb8\xe6\x8a\x9e",    /* モード選択 */
    [STR_COOKING]         = "\xe7\x82\x8a\xe9\xa3\xaf\xe4\xb8\xad",                             /* 炊飯中 */
    [STR_COMPLETE]        = "\xe5\xae\x8c\xe4\xba\x86\xef\xbc\x81",                             /* 完了！ */
    [STR_KEEP_WARM]       = "\xe4\xbf\x9d\xe6\xb8\xa9",                                         /* 保温 */
    [STR_PAUSED]          = "\xe4\xb8\x80\xe6\x99\x82\xe5\x81\x9c\xe6\xad\xa2",                 /* 一時停止 */

    [STR_MODE_WHITE_RICE] = "\xe7\x99\xbd\xe7\xb1\xb3",                                         /* 白米 */
    [STR_MODE_QUICK_COOK] = "\xe6\x97\xa9\xe7\x82\x8a\xe3\x81\x8d",                             /* 早炊き */
    [STR_MODE_PORRIDGE]   = "\xe3\x81\x8a\xe3\x81\x8b\xe3\x82\x86",                             /* おかゆ */
    [STR_MODE_STEAM]      = "\xe8\x92\xb8\xe3\x81\x97",                                         /* 蒸し */
    [STR_MODE_KEEP_WARM]  = "\xe4\xbf\x9d\xe6\xb8\xa9",                                         /* 保温 */

    [STR_MENU_ENTER]      = "\xe3\x83\xa1\xe3\x83\x8b\xe3\x83\xa5\xe3\x83\xbc",                 /* メニュー */
    [STR_BTN_PAUSE]       = LV_SYMBOL_PAUSE " \xe4\xb8\x80\xe6\x99\x82\xe5\x81\x9c\xe6\xad\xa2", /* 一時停止 */
    [STR_BTN_STOP]        = LV_SYMBOL_CLOSE " \xe5\x81\x9c\xe6\xad\xa2",                         /* 停止 */
    [STR_BTN_HOME]        = LV_SYMBOL_HOME "  \xe3\x83\x9b\xe3\x83\xbc\xe3\x83\xa0",            /* ホーム */
    [STR_BTN_BACK]        = LV_SYMBOL_LEFT " \xe6\x88\xbb\xe3\x82\x8b",                          /* 戻る */
    [STR_BTN_MENU]        = LV_SYMBOL_LIST "  \xe3\x83\xa1\xe3\x83\x8b\xe3\x83\xa5\xe3\x83\xbc",/* メニュー */
    [STR_HINT_HOME]       = "[MENU] \xe3\x83\xa1\xe3\x83\x8b\xe3\x83\xa5\xe3\x83\xbc\xe3\x81\xb8", /* メニューへ */
    [STR_HINT_MENU]       = "[UP/DN] \xe9\x81\xb8\xe6\x8a\x9e  [START] \xe6\xb1\xba\xe5\xae\x9a  [MENU] \xe6\x88\xbb\xe3\x82\x8b", /* 選択 決定 戻る */
    [STR_HINT_COOKING]    = "[START] \xe4\xb8\x80\xe6\x99\x82\xe5\x81\x9c\xe6\xad\xa2  [MENU] \xe5\x81\x9c\xe6\xad\xa2", /* 一時停止 停止 */
    [STR_HINT_FINISH]     = "[MENU]/[START] \xe3\x83\x9b\xe3\x83\xbc\xe3\x83\xa0\xe3\x81\xb8",  /* ホームへ */

    [STR_COOKING_DONE]    = LV_SYMBOL_OK "  \xe7\x82\x8a\xe9\xa3\xaf\xe5\xae\x8c\xe4\xba\x86\xef\xbc\x81", /* 炊飯完了！ */
    [STR_KEEP_WARM_TEMP]  = "\xe4\xbf\x9d\xe6\xb8\xa9: %d.%d " LV_SYMBOL_CHARGE "C",           /* 保温: */
};

static const char *s_strings_zh[STR_ID_MAX] =
{
    [STR_APP_TITLE]       = "\xe9\x9b\xbb\xe5\xad\x90\xe9\x8d\x8b",                             /* 電子鍋 */
    [STR_READY]           = LV_SYMBOL_OK "  \xe5\xb0\xb1\xe7\xb7\x92",                          /* 就緒 */
    [STR_SELECT_MODE]     = "\xe9\x81\xb8\xe6\x93\x87\xe6\xa8\xa1\xe5\xbc\x8f",                 /* 選擇模式 */
    [STR_COOKING]         = "\xe7\x83\xb9\xe7\x85\xae\xe4\xb8\xad",                             /* 烹煮中 */
    [STR_COMPLETE]        = "\xe5\xae\x8c\xe6\x88\x90\xef\xbc\x81",                             /* 完成！ */
    [STR_KEEP_WARM]       = "\xe4\xbf\x9d\xe6\xba\xab",                                         /* 保溫 */
    [STR_PAUSED]          = "\xe5\xb7\xb2\xe6\x9a\xab\xe5\x81\x9c",                             /* 已暫停 */

    [STR_MODE_WHITE_RICE] = "\xe7\x99\xbd\xe7\xb1\xb3",                                         /* 白米 */
    [STR_MODE_QUICK_COOK] = "\xe5\xbf\xab\xe7\x85\xae",                                         /* 快煮 */
    [STR_MODE_PORRIDGE]   = "\xe7\xa8\x80\xe9\xa3\xaf",                                         /* 稀飯 */
    [STR_MODE_STEAM]      = "\xe8\x92\xb8\xe7\x85\xae",                                         /* 蒸煮 */
    [STR_MODE_KEEP_WARM]  = "\xe4\xbf\x9d\xe6\xba\xab",                                         /* 保溫 */

    [STR_MENU_ENTER]      = "\xe9\x81\xb8\xe5\x96\xae",                                         /* 選單 */
    [STR_BTN_PAUSE]       = LV_SYMBOL_PAUSE " \xe6\x9a\xab\xe5\x81\x9c",                        /* 暫停 */
    [STR_BTN_STOP]        = LV_SYMBOL_CLOSE " \xe5\x81\x9c\xe6\xad\xa2",                        /* 停止 */
    [STR_BTN_HOME]        = LV_SYMBOL_HOME "  \xe9\xa6\x96\xe9\xa0\x81",                         /* 首頁 */
    [STR_BTN_BACK]        = LV_SYMBOL_LEFT " \xe8\xbf\x94\xe5\x9b\x9e",                          /* 返回 */
    [STR_BTN_MENU]        = LV_SYMBOL_LIST "  \xe9\x81\xb8\xe5\x96\xae",                        /* 選單 */
    [STR_HINT_HOME]       = "[MENU] \xe9\x80\xb2\xe5\x85\xa5\xe9\x81\xb8\xe5\x96\xae",         /* 進入選單 */
    [STR_HINT_MENU]       = "[UP/DN] \xe9\x81\xb8\xe6\x93\x87  [START] \xe7\xa2\xba\xe8\xaa\x8d  [MENU] \xe8\xbf\x94\xe5\x9b\x9e", /* 選擇 確認 返回 */
    [STR_HINT_COOKING]    = "[START] \xe6\x9a\xab\xe5\x81\x9c/\xe7\xb9\xbc\xe7\xba\x8c  [MENU] \xe5\x81\x9c\xe6\xad\xa2", /* 暫停/繼續 停止 */
    [STR_HINT_FINISH]     = "[MENU]/[START] \xe8\xbf\x94\xe5\x9b\x9e\xe9\xa6\x96\xe9\xa0\x81", /* 返回首頁 */

    [STR_COOKING_DONE]    = LV_SYMBOL_OK "  \xe7\x83\xb9\xe7\x85\xae\xe5\xae\x8c\xe6\x88\x90\xef\xbc\x81", /* 烹煮完成！ */
    [STR_KEEP_WARM_TEMP]  = "\xe4\xbf\x9d\xe6\xba\xab: %d.%d " LV_SYMBOL_CHARGE "C",           /* 保溫: */
};

static const char *s_strings_ko[STR_ID_MAX] =
{
    [STR_APP_TITLE]       = "\xec\xa0\x84\xea\xb8\xb0\xeb\xb0\xa5\xec\x86\xa5",                               /* 전기밥솥 */
    [STR_READY]           = LV_SYMBOL_OK "  \xec\xa4\x80\xeb\xb9\x84 \xec\x99\x84\xeb\xa3\x8c",              /* 준비 완료 */
    [STR_SELECT_MODE]     = "\xeb\xaa\xa8\xeb\x93\x9c \xec\x84\xa0\xed\x83\x9d",                             /* 모드 선택 */
    [STR_COOKING]         = "\xec\xb7\xa8\xec\x82\xac \xec\xa4\x91",                                         /* 취사 중 */
    [STR_COMPLETE]        = "\xec\x99\x84\xeb\xa3\x8c!",                                                     /* 완료! */
    [STR_KEEP_WARM]       = "\xeb\xb3\xb4\xec\x98\xa8",                                                     /* 보온 */
    [STR_PAUSED]          = "\xec\x9d\xbc\xec\x8b\x9c\xec\xa0\x95\xec\xa7\x80",                             /* 일시정지 */

    [STR_MODE_WHITE_RICE] = "\xeb\xb0\xb1\xeb\xaf\xb8",                                                     /* 백미 */
    [STR_MODE_QUICK_COOK] = "\xeb\xb9\xa0\xeb\xa5\xb8 \xec\xb7\xa8\xec\x82\xac",                           /* 빠른 취사 */
    [STR_MODE_PORRIDGE]   = "\xec\xa3\xbd",                                                                 /* 죽 */
    [STR_MODE_STEAM]      = "\xec\xb0\x9c\xea\xb8\xb0",                                                     /* 찜기 */
    [STR_MODE_KEEP_WARM]  = "\xeb\xb3\xb4\xec\x98\xa8",                                                     /* 보온 */

    [STR_MENU_ENTER]      = "\xeb\xa9\x94\xeb\x89\xb4",                                                     /* 메뉴 */
    [STR_BTN_PAUSE]       = LV_SYMBOL_PAUSE " \xec\x9d\xbc\xec\x8b\x9c\xec\xa0\x95\xec\xa7\x80",           /* 일시정지 */
    [STR_BTN_STOP]        = LV_SYMBOL_CLOSE " \xec\xa0\x95\xec\xa7\x80",                                     /* 정지 */
    [STR_BTN_HOME]        = LV_SYMBOL_HOME "  \xed\x99\x88",                                                /* 홈 */
    [STR_BTN_BACK]        = LV_SYMBOL_LEFT " \xeb\x92\xa4\xeb\xa1\x9c",                                      /* 뒤로 */
    [STR_BTN_MENU]        = LV_SYMBOL_LIST "  \xeb\xa9\x94\xeb\x89\xb4",                                    /* 메뉴 */
    [STR_HINT_HOME]       = "[MENU] \xeb\xa9\x94\xeb\x89\xb4\xeb\xa1\x9c",                                  /* 메뉴로 */
    [STR_HINT_MENU]       = "[UP/DN] \xec\x84\xa0\xed\x83\x9d  [START] \xed\x99\x95\xec\x9d\xb8  [MENU] \xeb\x92\xa4\xeb\xa1\x9c", /* 선택 확인 뒤로 */
    [STR_HINT_COOKING]    = "[START] \xec\x9d\xbc\xec\x8b\x9c\xec\xa0\x95\xec\xa7\x80  [MENU] \xec\xa0\x95\xec\xa7\x80", /* 일시정지 정지 */
    [STR_HINT_FINISH]     = "[MENU]/[START] \xed\x99\x88\xec\x9c\xbc\xeb\xa1\x9c",                          /* 홈으로 */

    [STR_COOKING_DONE]    = LV_SYMBOL_OK "  \xec\xb7\xa8\xec\x82\xac \xec\x99\x84\xeb\xa3\x8c!",            /* 취사 완료! */
    [STR_KEEP_WARM_TEMP]  = "\xeb\xb3\xb4\xec\x98\xa8: %d.%d " LV_SYMBOL_CHARGE "C",                       /* 보온: */
};

static const char *s_strings_de[STR_ID_MAX] =
{
    [STR_APP_TITLE]       = "Reiskocher",
    [STR_READY]           = LV_SYMBOL_OK "  Bereit",
    [STR_SELECT_MODE]     = "Modus w\xc3\xa4hlen",                                              /* Modus wählen */
    [STR_COOKING]         = "Kocht",
    [STR_COMPLETE]        = "Fertig!",
    [STR_KEEP_WARM]       = "Warmhalten",
    [STR_PAUSED]          = "PAUSIERT",

    [STR_MODE_WHITE_RICE] = "Wei\xc3\x9f" "er Reis",                                             /* Weißer Reis */
    [STR_MODE_QUICK_COOK] = "Schnellkochen",
    [STR_MODE_PORRIDGE]   = "Brei",
    [STR_MODE_STEAM]      = "Dampfgaren",
    [STR_MODE_KEEP_WARM]  = "Warmhalten",

    [STR_MENU_ENTER]      = "Men\xc3\xbc",                                                      /* Menü */
    [STR_BTN_PAUSE]       = LV_SYMBOL_PAUSE " Pause",
    [STR_BTN_STOP]        = LV_SYMBOL_CLOSE " Stopp",
    [STR_BTN_HOME]        = LV_SYMBOL_HOME "  Start",
    [STR_BTN_BACK]        = LV_SYMBOL_LEFT " Zur\xc3\xbc" "ck",                                  /* Zurück */
    [STR_BTN_MENU]        = LV_SYMBOL_LIST "  Men\xc3\xbc",                                     /* Menü */
    [STR_HINT_HOME]       = "[MENU] Men\xc3\xbc" " \xc3\xb6" "ffnen",                           /* Menü öffnen */
    [STR_HINT_MENU]       = "[UP/DN] W\xc3\xa4" "hlen  [START] Best\xc3\xa4" "tigen  [MENU] Zur\xc3\xbc" "ck", /* Wählen Bestätigen Zurück */
    [STR_HINT_COOKING]    = "[START] Pause  [MENU] Stopp",
    [STR_HINT_FINISH]     = "[MENU]/[START] Zur\xc3\xbc" "ck zum Start",                         /* Zurück zum Start */

    [STR_COOKING_DONE]    = LV_SYMBOL_OK "  Kochen fertig!",
    [STR_KEEP_WARM_TEMP]  = "Warmhalten: %d.%d " LV_SYMBOL_CHARGE "C",
};

static const char **s_string_tables[UI_LANG_MAX] =
{
    [UI_LANG_EN]    = s_strings_en,
    [UI_LANG_JA]    = s_strings_ja,
    [UI_LANG_ZH_TW] = s_strings_zh,
    [UI_LANG_KO]    = s_strings_ko,
    [UI_LANG_DE]    = s_strings_de,
};

/*============================================================================
 * Public API
 *============================================================================*/
void ui_lang_set(ui_lang_t lang)
{
    if (lang < UI_LANG_MAX)
        s_lang = lang;
}

ui_lang_t ui_lang_get(void)
{
    return s_lang;
}

void ui_lang_next(void)
{
    s_lang = (ui_lang_t)((s_lang + 1) % UI_LANG_MAX);
}

const char *ui_str(ui_str_id_t id)
{
    if (id >= STR_ID_MAX)
        return "?";

    const char *str = s_string_tables[s_lang][id];
    return str ? str : s_strings_en[id];  /* fallback to EN */
}

const lv_font_t *ui_lang_font_normal(void)
{
    if (s_lang != UI_LANG_EN)
        return &ui_font_cjk_16;
    return &lv_font_montserrat_16;
}

const lv_font_t *ui_lang_font_large(void)
{
    if (s_lang != UI_LANG_EN)
        return &ui_font_cjk_20;
    return &lv_font_montserrat_24;
}
