
// Icons from https://iconscout.com
#ifndef _LV_APP_H
#define _LV_APP_H
#include "colors.h"
#include "lvgl.h"
#include <stdio.h>
#include <unistd.h>

LV_IMG_DECLARE(hotberry_logo);
LV_IMG_DECLARE(robot_icon);
LV_IMG_DECLARE(setting_icon);
LV_IMG_DECLARE(finger_icon);
LV_IMG_DECLARE(documents_icon);
LV_IMG_DECLARE(temperature_icon);

void lv_app_entry();
void app_home(uint32_t delay);
void app_auto(uint32_t delay);
void app_manual(uint32_t delay);
void app_profiles(uint32_t delay);
void app_settings(uint32_t delay);
void app_anim_y(lv_obj_t *obj, uint32_t delay, lv_coord_t offs, bool reverse, bool out = false);
void lvc_label_init(lv_obj_t *label, const lv_font_t *font = &lv_font_montserrat_14,
                    lv_align_t align = LV_ALIGN_DEFAULT, lv_coord_t offsetX = 0, lv_coord_t offsetY = 0,
                    lv_color_t textColor = bs_dark, lv_text_align_t alignText = LV_TEXT_ALIGN_CENTER,
                    lv_label_long_mode_t longMode = LV_LABEL_LONG_WRAP, lv_coord_t textWidth = 0);
lv_obj_t *lvc_btn_init(lv_obj_t *btn, const char *labelText, lv_align_t align = LV_ALIGN_DEFAULT,
                       lv_coord_t offsetX = 0, lv_coord_t offsetY = 0, const lv_font_t *font = &lv_font_montserrat_14,
                       lv_color_t bgColor = bs_indigo_500, lv_color_t textColor = bs_white,
                       lv_text_align_t alignText = LV_TEXT_ALIGN_CENTER,
                       lv_label_long_mode_t longMode = LV_LABEL_LONG_WRAP, lv_coord_t labelWidth = 0,
                       lv_coord_t btnSizeX = 0, lv_coord_t btnSizeY = 0);

extern void (*home_btn_press_cb[])(uint32_t);

#endif