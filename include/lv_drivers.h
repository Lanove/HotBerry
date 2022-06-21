#ifndef _LV_DRIVERS_H
#define _LV_DRIVERS_H
#include "globals.h"
#include "lvgl.h"
#include "ili9486_drivers.h"
#include "hardware/clocks.h"

extern ili9486_drivers *tft;

extern void init_display();
extern void lv_display_flush_cb(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);
extern void lv_input_touch_cb(lv_indev_drv_t *indev_driver, lv_indev_data_t *data);
#endif