// This file was generated by SquareLine Studio
// SquareLine Studio version: SquareLine Studio 1.4.0
// LVGL version: 8.3.11
// Project name: SquareLine_Project

#ifndef _SQUARELINE_PROJECT_UI_H
#define _SQUARELINE_PROJECT_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

//#include "lv_i18n.h"
#include "ui_helpers.h"
#include "ui_events.h"

#include "ui_img_manager.h"
// SCREEN: ui_Screen1
void ui_Screen1_screen_init(void);
void ui_event_Screen1(lv_event_t *e);
extern lv_obj_t *ui_Screen1;
extern lv_obj_t *ui_Image3;
// SCREEN: ui_Screen2
void ui_Screen2_screen_init(void);
void ui_event_Screen2(lv_event_t *e);
extern lv_obj_t *ui_Screen2;
extern lv_obj_t *ui_Image6;
extern lv_obj_t *ui____initial_actions0;


extern lv_img_dsc_t ui_img_2_png;   // assets/2.png
void ui_img_2_png_load();
extern lv_img_dsc_t ui_img_3_png;   // assets/3.png
void ui_img_3_png_load();






void ui_init(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif