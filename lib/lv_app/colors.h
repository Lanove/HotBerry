#ifndef _COLORS_H
#define _COLORS_H

#include "lvgl.h"

// Boostrap colors
const static lv_color_t bs_blue = lv_color_make(0x0d, 0x6e, 0xfd);
const static lv_color_t bs_indigo_100 = lv_color_make(0xe0, 0xcf, 0xfc);
const static lv_color_t bs_indigo_200 = lv_color_make(0xc2, 0x9f, 0xfa);
const static lv_color_t bs_indigo_300 = lv_color_make(0xa3, 0x70, 0xf7);
const static lv_color_t bs_indigo_400 = lv_color_make(0x85, 0x40, 0xf5);
const static lv_color_t bs_indigo_500 = lv_color_make(0x66, 0x10, 0xf2);
const static lv_color_t bs_indigo_600 = lv_color_make(0x52, 0x0d, 0xc2);
const static lv_color_t bs_indigo_700 = lv_color_make(0x3d, 0x0a, 0x91);
const static lv_color_t bs_indigo_800 = lv_color_make(0x29, 0x06, 0x61);
const static lv_color_t bs_indigo_900 = lv_color_make(0x14, 0x03, 0x30);
const static lv_color_t bs_purple = lv_color_make(0x6f, 0x42, 0xc1);
const static lv_color_t bs_pink = lv_color_make(0xd6, 0x33, 0x84);
const static lv_color_t bs_red = lv_color_make(0xdc, 0x35, 0x45);
const static lv_color_t bs_orange = lv_color_make(0xfd, 0x7e, 0x14);
const static lv_color_t bs_yellow = lv_color_make(0xff, 0xc1, 0x07);
const static lv_color_t bs_green = lv_color_make(0x19, 0x87, 0x54);
const static lv_color_t bs_teal = lv_color_make(0x20, 0xc9, 0x97);
const static lv_color_t bs_cyan = lv_color_make(0x0d, 0xca, 0xf0);
const static lv_color_t bs_cyan_100 = lv_color_make(0xe6, 0xfb, 0xff);
const static lv_color_t bs_white = lv_color_make(0xff, 0xff, 0xff);
const static lv_color_t bs_gray = lv_color_make(0x6c, 0x75, 0x7d);
const static lv_color_t bs_gray_dark = lv_color_make(0x34, 0x3a, 0x40);
const static lv_color_t bs_gray_100 = lv_color_make(0xf8, 0xf9, 0xfa);
const static lv_color_t bs_gray_200 = lv_color_make(0xe9, 0xec, 0xef);
const static lv_color_t bs_gray_300 = lv_color_make(0xde, 0xe2, 0xe6);
const static lv_color_t bs_gray_400 = lv_color_make(0xce, 0xd4, 0xda);
const static lv_color_t bs_gray_500 = lv_color_make(0xad, 0xb5, 0xbd);
const static lv_color_t bs_gray_600 = lv_color_make(0x6c, 0x75, 0x7d);
const static lv_color_t bs_gray_700 = lv_color_make(0x49, 0x50, 0x57);
const static lv_color_t bs_gray_800 = lv_color_make(0x34, 0x3a, 0x40);
const static lv_color_t bs_gray_900 = lv_color_make(0x21, 0x25, 0x29);
const static lv_color_t bs_primary = lv_color_make(0x0d, 0x6e, 0xfd);
const static lv_color_t bs_secondary = lv_color_make(0x6c, 0x75, 0x7d);
const static lv_color_t bs_success = lv_color_make(0x19, 0x87, 0x54);
const static lv_color_t bs_info = lv_color_make(0x0d, 0xca, 0xf0);
const static lv_color_t bs_warning = lv_color_make(0xff, 0xc1, 0x07);
const static lv_color_t bs_danger = lv_color_make(0xdc, 0x35, 0x45);
const static lv_color_t bs_light = lv_color_make(0xf8, 0xf9, 0xfa);
const static lv_color_t bs_dark = lv_color_make(0x21, 0x25, 0x29);
const static lv_color_t bs_dark_333 = lv_color_make(0x33, 0x33, 0x33);

const static lv_color_t md_red = lv_color_make(0xF4,0x43,0x36);
const static lv_color_t md_grad_red = lv_color_make(0xf4,0x72,0x36);
const static lv_color_t md_blue = lv_color_make(0x21,0x96,0xf3);
const static lv_color_t md_grad_blue = lv_color_make(0x21,0xad,0xf3);
const static lv_color_t md_purple = lv_color_make(0xc6,0x36,0xe0);
const static lv_color_t md_grad_purple = lv_color_make(0xd5,0x36,0xe0);
const static lv_color_t md_teal = lv_color_make(0x00,0xb0,0x9e);
const static lv_color_t md_grad_teal = lv_color_make(0x00,0xb0,0x6f);

const static lv_color_t md_pink = lv_color_make(0xe9,0x1e,0x63);
const static lv_color_t md_indigo = lv_color_make(0x3f,0x51,0xb5);
const static lv_color_t md_light_blue = lv_color_make(0x03,0xa9,0xf4);
const static lv_color_t md_deep_purple = lv_color_make(0x67,0x3a,0xb7);
#endif