
// Icons from https://iconscout.com
#ifndef _LV_APP_H
#define _LV_APP_H
#include "colors.h"
#include "lvgl.h"
#include <stdio.h>
#include <string>

#ifdef PICO_BOARD
#include "pico/stdlib.h"
#include <AT24C16.h>
#include "globals.h"
extern AT24C16 EEPROM;
#endif

#define USE_INTRO 1
#define ENTER_CRITICAL_SECTION
#define EXIT_CRITICAL_SECTION
#define INIT_CRITICAL_SECTION

LV_IMG_DECLARE(hotberry_logo);
LV_IMG_DECLARE(robot_icon);
LV_IMG_DECLARE(setting_icon);
LV_IMG_DECLARE(finger_icon);
LV_IMG_DECLARE(documents_icon);
LV_IMG_DECLARE(temperature_icon);

static constexpr uint8_t profile_maximumDataPoint = 20;
struct Profile
{
    uint8_t dataPoint = 1;
    int targetTemperature[profile_maximumDataPoint];
    uint16_t targetSecond[profile_maximumDataPoint];
    uint16_t startTopHeaterAt = 0;
    Profile()
    {
        std::fill(targetTemperature, &targetTemperature[0] + profile_maximumDataPoint, 0);
        std::fill(targetSecond, &targetSecond[0] + profile_maximumDataPoint, 0);
        targetTemperature[0] = 30;
        targetSecond[0] = 0;
    }
};

static constexpr uint32_t app_display_width = 480;
static constexpr uint32_t app_display_height = 320;
static constexpr bool pidIsFloat = true;

namespace lv_app_pointers
{
// Read-only pointers
extern uint32_t *pBottomHeaterPV;
extern uint32_t *pTopHeaterPV;
extern uint32_t *pSecondsRunning;

// Read and write pointers
extern Profile (*pProfileLists)[10];
extern uint16_t *pSelectedProfile;
extern uint32_t *pBottomHeaterSV;
extern uint32_t *pTopHeaterSV;
extern float (*pTopHeaterPID)[3];
extern float (*pBottomHeaterPID)[3];
extern bool *pStartedAuto;
extern bool *pStartedManual;
} // namespace lv_app_pointers

extern void (*home_btn_press_cb[])(uint32_t);

struct WidgetParameterData
{
    lv_obj_t *issuer;
    void *param;
};

void lv_app_entry();

namespace AppHomeVar
{
extern const lv_color_t boxColors[];
extern const lv_color_t gradColors[];
extern const char *message[];
extern const void *iconSrc[];
extern const lv_align_t align[];
extern const lv_coord_t x_offs[];
extern const lv_coord_t y_offs[];
extern const bool reverseAnim[];
} // namespace AppHomeVar
void app_home(uint32_t delay);

namespace AppAutoVar
{
extern lv_coord_t elem_y_offset[];
extern lv_obj_t *chart, *auto_label, *run_btn, *profile_btn, *back_btn, *heater[2];
extern WidgetParameterData profile_wpd;
} // namespace AppAutoVar
void app_auto(uint32_t delay);

namespace AppManualVar
{
extern lv_coord_t elem_y_offset[];
extern lv_obj_t *chart, *manual_label, *run_btn, *back_btn, *heater[2];
extern WidgetParameterData topHeater_wpd, bottomHeater_wpd;
} // namespace AppManualVar
void app_manual(uint32_t delay);

namespace AppProfilesVar
{
extern WidgetParameterData profile_wpd; // Used for rollpick
extern WidgetParameterData confirm_wpd; // Used for modal confirm
extern lv_obj_t *header, *back, *logo, *profile_btn;
extern lv_obj_t *box, *label, *saveBtn, *drawBtn, *dataPointTA, *startTopHeaterAtTA, *grid_box;
extern lv_obj_t **targetSecondTA, **targetTemperatureTA;
extern bool modified;
} // namespace AppProfilesVar
void app_profiles(uint32_t delay);

namespace AppVarSettings
{
extern lv_obj_t *header, *topHeater_cont, *bottomHeater_cont;
extern lv_coord_t elem_y_offset[];
} // namespace AppVarSettings
void app_settings(uint32_t delay);

lv_obj_t *app_create_chart(lv_obj_t *_parent, bool profileGraph, uint8_t _selectedProfile, bool createLegend,
                           lv_coord_t width, lv_coord_t height);
void app_anim_y(lv_obj_t *obj, uint32_t delay, lv_coord_t offs, bool reverse, bool out = false);
lv_obj_t *rollpick_create(WidgetParameterData *wpd, const char *headerTitle, const char *options,
                          const lv_font_t *headerFont = &lv_font_montserrat_20, lv_coord_t width = lv_pct(70),
                          lv_coord_t height = lv_pct(70));
lv_obj_t *lvc_create_overlay();
lv_obj_t *modal_create_alert(const char *message, const char *headerText = "Warning!",
                             const lv_font_t *headerFont = &lv_font_montserrat_20,
                             const lv_font_t *messageFont = &lv_font_montserrat_14,
                             lv_color_t headerTextColor = bs_dark, lv_color_t textColor = bs_white,
                             lv_color_t headerColor = bs_warning, const char *buttonText = "Ok",
                             lv_coord_t xSize = (app_display_width * 0.7),
                             lv_coord_t ySize = (app_display_height * 0.7));
lv_obj_t *modal_create_confirm(WidgetParameterData *modalConfirmData, const char *message,
                               const char *headerText = "Warning!",
                               const lv_font_t *headerFont = &lv_font_montserrat_20,
                               const lv_font_t *messageFont = &lv_font_montserrat_14,
                               lv_color_t headerTextColor = bs_dark, lv_color_t textColor = bs_white,
                               lv_color_t headerColor = bs_warning, const char *confirmButtonText = "Confirm",
                               const char *cancelButtonText = "Cancel", lv_coord_t xSize = (app_display_width * 0.7),
                               lv_coord_t ySize = (app_display_height * 0.7));
lv_obj_t *modal_create_input_number(WidgetParameterData *modalConfirmData, bool isFloat, uint8_t maxLen,
                                    const char *headerText, uint16_t textAreaLen = 100,
                                    const lv_font_t *headerFont = &lv_font_montserrat_20,
                                    lv_color_t headerTextColor = bs_white, lv_color_t headerColor = bs_indigo_700,
                                    const char *confirmButtonText = "Confirm", const char *cancelButtonText = "Cancel",
                                    lv_coord_t xSize = (app_display_width * 0.7),
                                    lv_coord_t ySize = (app_display_height * 0.7));
void lvc_label_init(lv_obj_t *label, const lv_font_t *font = &lv_font_montserrat_14,
                    lv_align_t align = LV_ALIGN_DEFAULT, lv_coord_t offsetX = 0, lv_coord_t offsetY = 0,
                    lv_color_t textColor = bs_white, lv_text_align_t alignText = LV_TEXT_ALIGN_CENTER,
                    lv_label_long_mode_t longMode = LV_LABEL_LONG_WRAP, lv_coord_t textWidth = 0);
lv_obj_t *lvc_btn_init(lv_obj_t *btn, const char *labelText, lv_align_t align = LV_ALIGN_DEFAULT,
                       lv_coord_t offsetX = 0, lv_coord_t offsetY = 0, const lv_font_t *font = &lv_font_montserrat_14,
                       lv_color_t bgColor = bs_indigo_500, lv_color_t textColor = bs_white,
                       lv_text_align_t alignText = LV_TEXT_ALIGN_CENTER,
                       lv_label_long_mode_t longMode = LV_LABEL_LONG_WRAP, lv_coord_t labelWidth = 0,
                       lv_coord_t btnSizeX = 0, lv_coord_t btnSizeY = 0);

#endif