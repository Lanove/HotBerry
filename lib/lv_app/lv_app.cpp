#include "lv_app.h"
#include "keyboard.h"
#ifdef PICO_BOARD
AT24C16 EEPROM;
SemaphoreHandle_t lv_app_mutex;
#endif

static constexpr uint32_t animTime = 500;
static constexpr uint32_t animTranslationY = 150;
static constexpr uint32_t manual_max_run_seconds = 300;
static constexpr const char *profile_roller_list = "Profile 0\nProfile 1\nProfile 2\nProfile 3\nProfile 4\nProfile "
                                                   "5\nProfile 6\nProfile 7\nProfile 8\nProfile 9";

namespace lv_app_pointers
{
// Pointers of variable used on both display application and main application

// Read-only pointers
uint32_t *pBottomHeaterPV;
uint32_t *pTopHeaterPV;
uint32_t *pSecondsRunning;

// Read and write pointers
uint32_t *pBottomHeaterSV;
uint32_t *pTopHeaterSV;
bool *pStartedAuto;
bool *pStartedManual;
float (*pTopHeaterPID)[3];
uint16_t *pSelectedProfile;
float (*pBottomHeaterPID)[3];
Profile (*pProfileLists)[10];
} // namespace lv_app_pointers
using namespace lv_app_pointers;

Profile tempProfile;
uint32_t cTopHeaterPV;
uint32_t cBottomHeaterPV;
uint32_t cSecondsRunning;

// Pointer to functions of each possible destination of buttons from home screen
void (*home_btn_press_cb[])(uint32_t) = {app_auto, app_manual, app_profiles, app_settings};

static lv_obj_t *scr_home;
static lv_obj_t *scr_auto;
static lv_obj_t *scr_manual;
static lv_obj_t *scr_profiles;
static lv_obj_t *scr_settings;

namespace ChartData
{
const lv_color_t preheatColor = md_grad_red;
const lv_color_t doubleHeatColor = md_red;

enum
{
    TOP_SERIES = 0,
    BOTTOM_SERIES = 1
};
const lv_color_t legendColor[] = {bs_cyan, bs_indigo_300};
const char *legendText[] = {"Top", "Bottom"};
lv_obj_t *secondsRunningLabel = NULL;

int dataPoint;
int targetTemperatures[profile_maximumDataPoint];
uint16_t targetSeconds[profile_maximumDataPoint];
int startTopHeaterAt;
lv_point_t *coords = NULL;
int coord_counter;
int selectedProfile;
lv_obj_t *parent;
lv_obj_t *chart = NULL;
lv_obj_t *preheatLine = NULL;
lv_obj_t *doubleheatLine = NULL;
lv_chart_series_t *profileSeries = NULL;
lv_chart_series_t *topSeries = NULL;
lv_chart_series_t *bottomSeries = NULL;
lv_chart_cursor_t *cursor;
int line_point_cnt;
int32_t last_cursor_id = -1;
bool profileGraph;
bool createLegend;
int totalSecond;
int highestTemperature;
void deleteChart()
{
    if (chart)
    {
        lv_obj_del(chart);
        chart = NULL;
    }
    if (preheatLine)
    {
        lv_obj_del(preheatLine);
        preheatLine = NULL;
    }
    if (doubleheatLine)
    {
        lv_obj_del(doubleheatLine);
        doubleheatLine = NULL;
    }
}
}; // namespace ChartData
lv_obj_t *app_create_chart(lv_obj_t *_parent, bool _profileGraph, uint8_t _selectedProfile, bool _createLegend,
                           lv_coord_t width, lv_coord_t height)
{
    using namespace ChartData;
    last_cursor_id = -1;
    parent = _parent;
    profileGraph = _profileGraph;
    createLegend = _createLegend;
    if (profileGraph)
    {
        selectedProfile = _selectedProfile;
        LV_APP_MUTEX_ENTER;
        dataPoint = (*pProfileLists)[selectedProfile].dataPoint;
        memcpy(targetTemperatures, (*pProfileLists)[selectedProfile].targetTemperature,
               sizeof((*pProfileLists)[selectedProfile].targetTemperature));
        memcpy(targetSeconds, (*pProfileLists)[selectedProfile].targetSecond,
               sizeof((*pProfileLists)[selectedProfile].targetSecond));
        startTopHeaterAt = (*pProfileLists)[selectedProfile].startTopHeaterAt;
        LV_APP_MUTEX_EXIT;
        coord_counter = 0;
        if (coords == NULL && dataPoint)
            coords = (lv_point_t *)malloc(sizeof(lv_point_t) * (dataPoint));
        else
            coords = (lv_point_t *)realloc(coords, sizeof(lv_point_t) * (dataPoint));

        totalSecond = dataPoint ? targetSeconds[dataPoint - 1] : 0;
        highestTemperature = 0;
        for (int i = 0; i < dataPoint; i++)
        {
            if (targetTemperatures[i] > highestTemperature)
                highestTemperature = targetTemperatures[i];
        }
    }
    else
    {
        totalSecond = manual_max_run_seconds;
        highestTemperature = 100;
    }
    chart = lv_chart_create(parent);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_obj_set_style_pad_all(chart, 1, 0);
    lv_obj_set_style_bg_color(chart, lv_obj_get_style_bg_color(parent, 0), 0);
    lv_obj_set_style_line_color(chart, bs_gray, LV_PART_MAIN);
    lv_obj_set_style_border_side(chart, LV_BORDER_SIDE_LEFT | LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(chart, bs_gray_500, 0);
    lv_obj_set_style_radius(chart, 0, 0);
    lv_obj_set_style_size(chart, 1, LV_PART_INDICATOR);
    lv_obj_set_size(chart, width, height);
    lv_chart_set_update_mode(chart, LV_CHART_UPDATE_MODE_CIRCULAR);

    lv_chart_set_point_count(chart, totalSecond + 1);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0,
                       profileGraph ? (dataPoint ? highestTemperature + 10 : 10) : highestTemperature);

    lv_chart_set_axis_tick(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 1, 10, 1, true, 10);
    lv_chart_set_axis_tick(chart, LV_CHART_AXIS_PRIMARY_X, 0, 1, 10, 1, true, 30);

    lv_chart_set_div_line_count(chart, 10, 10);
    lv_obj_refresh_ext_draw_size(chart);

    if (createLegend)
    {
        static constexpr lv_coord_t legend_x_offset[] = {10, 60};
        for (int i = 0; i < 2; i++)
        {
            lv_obj_t *box = lv_obj_create(chart);
            lv_obj_set_scrollbar_mode(box, LV_SCROLLBAR_MODE_OFF);
            lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_size(box, 15, 15);
            lv_obj_set_style_bg_color(box, legendColor[i], 0);
            lv_obj_set_style_border_width(box, 0, 0);
            lv_obj_set_style_radius(box, 1, 0);
            lv_obj_align(box, LV_ALIGN_TOP_LEFT, legend_x_offset[i], 0);
            lv_obj_t *label = lv_label_create(chart);
            lvc_label_init(label, &lv_font_montserrat_14);
            lv_obj_align_to(label, box, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
            lv_label_set_text_static(label, legendText[i]);
        }
        secondsRunningLabel = lv_label_create(chart);
        lvc_label_init(secondsRunningLabel, &lv_font_montserrat_14, LV_ALIGN_BOTTOM_LEFT, 5, 0);
        lv_label_set_text_fmt(secondsRunningLabel, "Time Running:%ds", cSecondsRunning);
    }

    if (profileGraph && dataPoint > 0)
    {
        profileSeries = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
        cursor = lv_chart_add_cursor(chart, md_grad_red, LV_DIR_LEFT | LV_DIR_BOTTOM);
        // Add data points from profile
        for (int i = 0; i < dataPoint; i++)
            lv_chart_set_value_by_id(chart, profileSeries, targetSeconds[i], targetTemperatures[i]);
        // Draw profile graph
        lv_obj_add_event_cb(
            chart,
            [](lv_event_t *e) {
                lv_obj_draw_part_dsc_t *dsc = lv_event_get_draw_part_dsc(e);
                lv_obj_t *chart = lv_event_get_target(e);
                if (!lv_obj_draw_part_check_type(dsc, &lv_chart_class, LV_CHART_DRAW_PART_LINE_AND_POINT))
                    return;
                if (coord_counter < dataPoint && dsc->sub_part_ptr == profileSeries && dsc->draw_area->x1 > 0 &&
                    dsc->draw_area->x2 < 480 && dsc->draw_area->y1 > 0 &&
                    dsc->draw_area->y2 < 320) // Store coordinate only on valid points
                {
                    coords[coord_counter].x = dsc->draw_area->x1;
                    coords[coord_counter].y = dsc->draw_area->y1;
                    if (coord_counter ==
                        dataPoint - 1) // It seems that this is last coordinate, let's draw the profile graph then
                    {
                        // Draw the line for preheating only
                        line_point_cnt = 0;
                        preheatLine = lv_line_create(parent);
                        lv_line_set_points(preheatLine, coords, startTopHeaterAt + 1);
                        lv_obj_set_style_line_width(preheatLine, 2, 0);
                        lv_obj_set_style_line_color(preheatLine, preheatColor, 0);
                        // Continue the preheating line with double heating line
                        doubleheatLine = lv_line_create(parent);
                        lv_line_set_points(doubleheatLine, coords + startTopHeaterAt, dataPoint - startTopHeaterAt);
                        lv_obj_set_style_line_width(doubleheatLine, 2, 0);
                        lv_obj_set_style_line_color(doubleheatLine, doubleHeatColor, 0);
                        lv_event_send(chart, LV_EVENT_READY, NULL);
                    }
                    coord_counter++;
                }
            },
            LV_EVENT_DRAW_PART_END, NULL);

        // Add cursor for profile graph, click points to show profile numbers at particular point
        lv_obj_add_event_cb(
            chart,
            [](lv_event_t *e) {
                lv_event_code_t code = lv_event_get_code(e);
                lv_obj_t *obj = lv_event_get_target(e);
                LV_APP_MUTEX_ENTER;
                Profile selectedProfileList = (*pProfileLists)[*pSelectedProfile];
                LV_APP_MUTEX_EXIT;
                if (code == LV_EVENT_VALUE_CHANGED)
                {
                    last_cursor_id = lv_chart_get_pressed_point(obj);
                    if (last_cursor_id != LV_CHART_POINT_NONE)
                    {
                        for (uint8_t i = 0; i < selectedProfileList.dataPoint; i++)
                        {
                            if (last_cursor_id > selectedProfileList.targetSecond[i] - 5 &&
                                last_cursor_id < selectedProfileList.targetSecond[i] + 5)
                            {
                                last_cursor_id = selectedProfileList.targetSecond[i];
                                lv_chart_set_cursor_point(obj, cursor, profileSeries, last_cursor_id);
                                break;
                            }
                            if (i == selectedProfileList.dataPoint - 1)
                            {
                                last_cursor_id = -1;
                                lv_chart_set_cursor_point(obj, cursor, profileSeries, LV_CHART_POINT_NONE);
                            }
                        }
                    }
                    else
                    {
                        last_cursor_id = -1;
                        lv_chart_set_cursor_point(obj, cursor, profileSeries, LV_CHART_POINT_NONE);
                    }
                }
                else if (code == LV_EVENT_DRAW_PART_END)
                {
                    lv_obj_draw_part_dsc_t *dsc = lv_event_get_draw_part_dsc(e);
                    if (!lv_obj_draw_part_check_type(dsc, &lv_chart_class, LV_CHART_DRAW_PART_CURSOR))
                        return;
                    if (dsc->p1 == NULL || dsc->p2 == NULL || dsc->p1->y != dsc->p2->y || last_cursor_id < 0)
                        return;
                    lv_coord_t *data_array = lv_chart_get_y_array(chart, profileSeries);
                    lv_coord_t v = data_array[last_cursor_id];
                    char tbuf[16];
                    char sbuf[16];
                    lv_snprintf(tbuf, sizeof(tbuf), "%d°C", v);
                    lv_snprintf(sbuf, sizeof(sbuf), "at %ds", last_cursor_id);

                    lv_point_t size;
                    lv_txt_get_size(&size, sbuf, LV_FONT_DEFAULT, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);

                    lv_area_t a;
                    a.y1 = dsc->p1->y + 5;
                    a.y2 = a.y1 + size.y + 20;
                    a.x1 = dsc->p1->x + 10;
                    a.x2 = a.x1 + size.x + 20;

                    lv_draw_rect_dsc_t draw_rect_dsc;
                    lv_draw_rect_dsc_init(&draw_rect_dsc);
                    draw_rect_dsc.bg_color = last_cursor_id >= targetSeconds[startTopHeaterAt] ? md_red : md_grad_red;

                    draw_rect_dsc.radius = 3;

                    lv_draw_rect(dsc->draw_ctx, &draw_rect_dsc, &a);

                    lv_draw_label_dsc_t tlabel_dsc;
                    lv_draw_label_dsc_init(&tlabel_dsc);
                    tlabel_dsc.color = lv_color_white();
                    a.x1 += 5;
                    a.x2 -= 5;
                    a.y1 += 2;
                    a.y2 -= 2;
                    lv_draw_label(dsc->draw_ctx, &tlabel_dsc, &a, tbuf, NULL);

                    lv_draw_label_dsc_t slabel_dsc;
                    lv_draw_label_dsc_init(&slabel_dsc);
                    slabel_dsc.color = lv_color_white();
                    a.y1 += size.y;
                    a.y2 -= 5;
                    lv_draw_label(dsc->draw_ctx, &slabel_dsc, &a, sbuf, NULL);
                }
            },
            LV_EVENT_ALL, NULL);
    }

    // Redraw x axis ticks because LVGL didn't support custom x axis ticks range
    lv_obj_add_event_cb(
        chart,
        [](lv_event_t *e) {
            lv_obj_draw_part_dsc_t *dsc = lv_event_get_draw_part_dsc(e);
            if (!lv_obj_draw_part_check_type(dsc, &lv_chart_class, LV_CHART_DRAW_PART_TICK_LABEL))
                return;
            if (dsc->id == LV_CHART_AXIS_PRIMARY_X && dsc->text)
                lv_snprintf(dsc->text, dsc->text_length, "%d", dsc->value * totalSecond / 9);
        },
        LV_EVENT_DRAW_PART_BEGIN, NULL);

    return chart;
}

/**
 * @brief Entry to the display application, call once on main to run display application
 *
 */
void lv_app_entry()
{
#if USE_INTRO == 1
    static constexpr uint32_t hotberry_fadein_dur = 1000;
    static constexpr uint32_t hotberry_stay_dur = 2000;
    static constexpr uint32_t hotberry_fadeout_dur = 1000;
#else
    static constexpr uint32_t hotberry_fadein_dur = 0;
    static constexpr uint32_t hotberry_stay_dur = 0;
    static constexpr uint32_t hotberry_fadeout_dur = 0;
#endif

    lv_obj_t *scr = lv_obj_create(NULL);
    lv_scr_load(scr);
    lv_obj_t *hb_logo = lv_img_create(scr);
    lv_img_set_src(hb_logo, &hotberry_logo);
    lv_obj_align(hb_logo, LV_ALIGN_CENTER, 0, 0);

    // For some reason fade-in and fade-out using built-in function didn't work
    // so use regular animation for fade-in and use built-in function for fade-out
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, hb_logo);
    lv_anim_set_time(&a, hotberry_fadein_dur);
    lv_anim_set_exec_cb(&a, [](void *obj, int32_t v) {
        lv_obj_set_style_opa((lv_obj_t *)obj, v, 0);
        if (v == 255)
            lv_obj_fade_out((lv_obj_t *)obj, hotberry_fadeout_dur, hotberry_stay_dur);
    });
    lv_anim_set_values(&a, 0, 255);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_start(&a);

    // Open home screen after logo showcase is done
    app_home(hotberry_fadein_dur + hotberry_stay_dur + hotberry_fadeout_dur);

#ifdef PICO_BOARD
    LV_APP_MUTEX_ENTER;
    if (EEPROM.init(EEPROM_I2CBUS, EEPROM_SDA, EEPROM_SCL, EEPROM_BusSpeed))
        printf("EEPROM detected!\n");
    else
        printf("EEPROM Not detected!\n");

    EEPROM.memRead(sizeof(*pProfileLists), *pTopHeaterPID, sizeof(*pTopHeaterPID));
    EEPROM.memRead(sizeof(*pProfileLists) + sizeof(*pTopHeaterPID), *pBottomHeaterPID, sizeof(*pBottomHeaterPID));
    EEPROM.memRead(0, *pProfileLists, sizeof(*pProfileLists));
    LV_APP_MUTEX_EXIT;
#endif
    lv_timer_create(
        [](_lv_timer_t *e) {
            static uint32_t lastSecond = 0;
            LV_APP_MUTEX_ENTER;
            cTopHeaterPV = *pTopHeaterPV;
            cBottomHeaterPV = *pBottomHeaterPV;
            cSecondsRunning = *pSecondsRunning;
            bool startedAuto = *pStartedAuto;
            bool startedManual = *pStartedManual;
            uint16_t totalSecond =
                (*pProfileLists)[*pSelectedProfile].targetSecond[(*pProfileLists)[*pSelectedProfile].dataPoint - 1];
            LV_APP_MUTEX_EXIT;
            if (lv_scr_act() == scr_auto)
            {
                using namespace AppAutoVar;
                for (int i = 0; i < 2; i++)
                {
                    lv_obj_t *heater_temp = lv_obj_get_child(heater[i], 1);
                    lv_label_set_text_fmt(heater_temp, "%d°C", i == 0 ? cTopHeaterPV : cBottomHeaterPV);
                }
            }
            else if (lv_scr_act() == scr_manual)
            {
                using namespace AppManualVar;
                for (int i = 0; i < 2; i++)
                {
                    lv_obj_t *heater_temp = lv_obj_get_child(heater[i], 1);
                    lv_label_set_text_fmt(heater_temp, "%d°C", i == 0 ? cTopHeaterPV : cBottomHeaterPV);
                }
            }
            if (cSecondsRunning != lastSecond && ((startedAuto && cSecondsRunning <= totalSecond) || startedManual))
            {
                lv_chart_set_value_by_id(ChartData::chart, ChartData::topSeries,
                                         startedAuto ? cSecondsRunning : cSecondsRunning % ChartData::totalSecond,
                                         cTopHeaterPV);
                lv_chart_set_value_by_id(ChartData::chart, ChartData::bottomSeries,
                                         startedAuto ? cSecondsRunning : cSecondsRunning % ChartData::totalSecond,
                                         cBottomHeaterPV);

                if (startedManual && cSecondsRunning > ChartData::totalSecond)
                {
                    for (int i = 1; i <= 5; i++)
                    {
                        lv_chart_set_value_by_id(ChartData::chart, ChartData::topSeries,
                                                 (cSecondsRunning % ChartData::totalSecond) + i, LV_CHART_POINT_NONE);
                        lv_chart_set_value_by_id(ChartData::chart, ChartData::bottomSeries,
                                                 (cSecondsRunning % ChartData::totalSecond) + i, LV_CHART_POINT_NONE);
                    }
                }
                lv_label_set_text_fmt(ChartData::secondsRunningLabel, "Time Running:%ds", cSecondsRunning);
                // lv_chart_set_next_value(ChartData::chart, ChartData::topSeries, cTopHeaterPV);
                // lv_chart_set_next_value(ChartData::chart, ChartData::bottomSeries, cBottomHeaterPV);
            }
            lastSecond = cSecondsRunning;
        },
        200, NULL);
}

// Variables used on home screen
namespace AppHomeVar
{
const lv_color_t boxColors[] = {md_blue, md_teal, md_red, md_purple};
const lv_color_t gradColors[] = {md_grad_blue, md_grad_teal, md_grad_red, md_grad_purple};
const char *message[] = {"AUTO\nOPERATION", "MANUAL\nOPERATION", "PROFILES", "SETTINGS"};
const void *iconSrc[] = {&robot_icon, &finger_icon, &documents_icon, &setting_icon};
const lv_align_t align[] = {LV_ALIGN_TOP_LEFT, LV_ALIGN_TOP_RIGHT, LV_ALIGN_BOTTOM_LEFT, LV_ALIGN_BOTTOM_RIGHT};
const lv_coord_t x_offs[] = {10, -10, 10, -10};
const lv_coord_t y_offs[] = {5, 5, -5, -5};
const bool reverseAnim[] = {false, false, true, true};
} // namespace AppHomeVar

void app_home(uint32_t delay)
{
    using namespace AppHomeVar;
    scr_home = lv_obj_create(NULL);
    lv_scr_load_anim(scr_home, LV_SCR_LOAD_ANIM_NONE, 0, delay, true);
    lv_obj_set_scrollbar_mode(scr_home, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(scr_home, LV_OBJ_FLAG_SCROLLABLE);

    for (int i = 0; i < 4; i++)
    {
        lv_obj_t *btn = lv_btn_create(scr_home);
        lv_obj_set_size(btn, 225, 150);
        lv_obj_set_style_radius(btn, 2, 0);
        lv_obj_set_style_bg_grad_dir(btn, LV_GRAD_DIR_VER, 0);
        lv_obj_set_style_bg_grad_color(btn, gradColors[i], 0);
        lv_obj_set_style_bg_color(btn, boxColors[i], 0);
        lv_obj_align(btn, align[i], x_offs[i], y_offs[i]);
        // Go to corresponding screen on button click
        lv_obj_add_event_cb(
            btn,
            [](lv_event_t *e) {
                lv_obj_t *btn = lv_event_get_target(e);
                for (int i = 0; i < 4; i++)
                {
                    lv_obj_t *child = lv_obj_get_child(scr_home, i);
                    app_anim_y(child, 0, y_offs[i], reverseAnim[i], true);
                }
                (*home_btn_press_cb[lv_obj_get_index(btn)])(animTime);
            },
            LV_EVENT_CLICKED, NULL);

        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text_fmt(label, "%s", message[i]);
        lv_obj_align(label, LV_ALIGN_BOTTOM_LEFT, 0, 0);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_20, 0);

        lv_obj_t *icon = lv_img_create(btn);
        lv_img_set_src(icon, iconSrc[i]);
        lv_obj_align(icon, LV_ALIGN_TOP_RIGHT, 10, -10);
        app_anim_y(btn, delay, y_offs[i], reverseAnim[i]);
    }
}

namespace AppAutoVar
{
lv_coord_t elem_y_offset[] = {0, 44, 94, 134, 204, 274, -7};
lv_obj_t *chart, *auto_label, *run_btn, *profile_btn, *back_btn, *heater[2];
WidgetParameterData profile_wpd;
lv_coord_t chartWidth = 330, chartHeight = 300;
} // namespace AppAutoVar
void app_auto(uint32_t delay)
{
    using namespace AppAutoVar;
    using ChartData::bottomSeries, ChartData::topSeries, ChartData::legendColor;
    scr_auto = lv_obj_create(NULL);
    lv_scr_load_anim(scr_auto, LV_SCR_LOAD_ANIM_NONE, 0, delay, true);
    lv_obj_set_scrollbar_mode(scr_auto, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(scr_auto, LV_OBJ_FLAG_SCROLLABLE);

    auto_label = lv_label_create(scr_auto);
    lv_obj_set_width(auto_label, 100);
    lvc_label_init(auto_label, &lv_font_montserrat_18, LV_ALIGN_TOP_RIGHT, -3,
                   elem_y_offset[lv_obj_get_index(auto_label)], bs_white);
    lv_label_set_text(auto_label, "Auto\nOperation");
    app_anim_y(auto_label, delay, elem_y_offset[lv_obj_get_index(auto_label)], false);

    run_btn = lv_btn_create(scr_auto);
    lv_obj_set_width(run_btn, 100);
    lv_obj_set_style_radius(run_btn, 2, 0);
    lvc_btn_init(run_btn, LV_SYMBOL_PLAY " START", LV_ALIGN_TOP_RIGHT, -3, elem_y_offset[lv_obj_get_index(run_btn)],
                 &lv_font_montserrat_20, md_red);
    app_anim_y(run_btn, delay, elem_y_offset[lv_obj_get_index(run_btn)], false);
    // Flip the pStartedAuto (dereferenced) boolean value on click and change button color and text
    lv_obj_add_event_cb(
        run_btn,
        [](lv_event_t *e) {
            lv_obj_t *run_btn_label = lv_obj_get_child(run_btn, 0);
            LV_APP_MUTEX_ENTER;
            *pStartedAuto = !*pStartedAuto;
            bool started = *pStartedAuto;
            *pSecondsRunning = 0;
            LV_APP_MUTEX_EXIT;
            if (started == 1)
            {
                lv_chart_set_all_value(ChartData::chart, ChartData::topSeries, LV_CHART_POINT_NONE);
                lv_chart_set_all_value(ChartData::chart, ChartData::bottomSeries, LV_CHART_POINT_NONE);
            }
            lv_label_set_text(run_btn_label, started ? LV_SYMBOL_STOP " STOP" : LV_SYMBOL_PLAY " START");
            lv_obj_set_style_bg_color(run_btn, started ? md_teal : md_red, 0);
        },
        LV_EVENT_CLICKED, NULL);

    lv_obj_t *profile_btn = lv_btn_create(scr_auto);
    lv_obj_set_size(profile_btn, 100, 30);
    lv_obj_set_style_radius(profile_btn, 2, 0);
    lv_obj_t *profile_btn_label = lvc_btn_init(profile_btn, "", LV_ALIGN_TOP_RIGHT, -3,
                                               elem_y_offset[lv_obj_get_index(profile_btn)], &lv_font_montserrat_16);
    LV_APP_MUTEX_ENTER;
    uint8_t dSelectedProfile = *pSelectedProfile;
    LV_APP_MUTEX_EXIT;
    lv_label_set_text_fmt(profile_btn_label, "PROFILE %d", dSelectedProfile);

    // Create roll pick on profile button click
    lv_obj_add_event_cb(
        profile_btn,
        [](lv_event_t *e) {
            lv_obj_t *profile_btn = lv_event_get_target(e);
            LV_APP_MUTEX_ENTER;
            bool started = *pStartedAuto;
            uint8_t dSelectedProfile = *pSelectedProfile;
            profile_wpd.param = pSelectedProfile;
            profile_wpd.issuer = profile_btn;
            LV_APP_MUTEX_EXIT;
            if (started)
            {
                modal_create_alert("Can't modify profile while auto operation is still running!");
                return;
            }
            lv_obj_t *rollpick =
                rollpick_create(&profile_wpd, "Choose Profile", profile_roller_list, &lv_font_montserrat_20);
            lv_roller_set_selected(rollpick, dSelectedProfile, LV_ANIM_OFF);
        },
        LV_EVENT_CLICKED, NULL);
    // Change the button label's text to selected profile
    lv_obj_add_event_cb(
        profile_btn,
        [](lv_event_t *e) {
            lv_obj_t *profile_btn = lv_event_get_target(e);
            lv_obj_t *profile_btn_label = lv_obj_get_child(profile_btn, 0);
            LV_APP_MUTEX_ENTER;
            uint8_t dSelectedProfile = *pSelectedProfile;
            LV_APP_MUTEX_EXIT;
            lv_label_set_text_fmt(profile_btn_label, "PROFILE %d", dSelectedProfile);
            ChartData::deleteChart();
            chart = app_create_chart(scr_auto, true, dSelectedProfile, true, chartWidth, chartHeight);
            lv_obj_align(chart, LV_ALIGN_LEFT_MID, 30,
                         elem_y_offset[6]); // Can't use obj index as array index because chart draw addiotional object
                                            // to the parents of chart

            bottomSeries = lv_chart_add_series(chart, legendColor[ChartData::BOTTOM_SERIES], LV_CHART_AXIS_PRIMARY_Y);
            topSeries = lv_chart_add_series(chart, legendColor[ChartData::TOP_SERIES], LV_CHART_AXIS_PRIMARY_Y);
        },
        LV_EVENT_REFRESH, NULL);
    app_anim_y(profile_btn, delay, elem_y_offset[lv_obj_get_index(profile_btn)], false);

    const char *heater_label_msg[] = {"TOP HEATER", "BTM HEATER"};
    for (int i = 0; i < 2; i++)
    {
        heater[i] = lv_btn_create(scr_auto);
        lv_obj_set_size(heater[i], 100, 60);
        lv_obj_set_style_radius(heater[i], 2, 0);
        lv_obj_set_style_bg_color(heater[i], md_grad_red, 0);
        lv_obj_set_style_bg_grad_dir(heater[i], LV_GRAD_DIR_VER, 0);
        lv_obj_set_style_bg_grad_color(heater[i], md_red, 0);
        lv_obj_align(heater[i], LV_ALIGN_TOP_RIGHT, -3, elem_y_offset[lv_obj_get_index(heater[i])]);
        lv_obj_t *temp_icon = lv_img_create(heater[i]);
        lv_img_set_src(temp_icon, &temperature_icon);
        lv_obj_align(temp_icon, LV_ALIGN_LEFT_MID, -30, -7);
        lv_obj_t *heater_temp = lv_label_create(heater[i]);
        lv_obj_set_style_text_font(heater_temp, &lv_font_montserrat_26, 0);
        lv_label_set_text_fmt(heater_temp, "%d°C", i == 0 ? cTopHeaterPV : cBottomHeaterPV);
        lv_obj_align(heater_temp, LV_ALIGN_LEFT_MID, 5, -7);
        lv_obj_t *heater_label = lv_label_create(heater[i]);
        lv_obj_set_style_text_font(heater_label, &lv_font_montserrat_12, 0);
        lv_obj_align(heater_label, LV_ALIGN_BOTTOM_MID, 0, 7);
        lv_label_set_text(heater_label, heater_label_msg[i]);
        app_anim_y(heater[i], delay, elem_y_offset[lv_obj_get_index(heater[i])], false);
    }

    back_btn = lv_btn_create(scr_auto);
    lv_obj_set_width(back_btn, 100);
    lv_obj_set_style_radius(back_btn, 2, 0);
    lvc_btn_init(back_btn, LV_SYMBOL_HOME " HOME", LV_ALIGN_TOP_RIGHT, -3, elem_y_offset[lv_obj_get_index(back_btn)],
                 &lv_font_montserrat_20);
    app_anim_y(back_btn, delay, elem_y_offset[lv_obj_get_index(back_btn)], false);
    lv_obj_add_event_cb(
        back_btn,
        [](lv_event_t *e) {
            using namespace AppAutoVar;
            LV_APP_MUTEX_ENTER;
            bool started = *pStartedAuto;
            LV_APP_MUTEX_EXIT;
            if (started)
            {
                modal_create_alert("Can't go home while auto operation is still running!");
                return;
            }
            uint32_t child_cnt = lv_obj_get_child_cnt(scr_auto);
            for (int i = 0; i < child_cnt; i++)
            {
                lv_obj_t *child = lv_obj_get_child(scr_auto, i);
                if (child != chart && !lv_obj_check_type(child, &lv_line_class))
                    app_anim_y(child, 0, elem_y_offset[i], false, true);
            }
            app_home(animTime);
        },
        LV_EVENT_CLICKED, NULL);

    chart = app_create_chart(scr_auto, true, dSelectedProfile, true, chartWidth, chartHeight);
    lv_obj_align(chart, LV_ALIGN_LEFT_MID, 30, elem_y_offset[6]); // Can't use obj index as array index because chart
                                                                  // draw addiotional object to the parents of chart
    bottomSeries = lv_chart_add_series(chart, legendColor[ChartData::BOTTOM_SERIES], LV_CHART_AXIS_PRIMARY_Y);
    topSeries = lv_chart_add_series(chart, legendColor[ChartData::TOP_SERIES], LV_CHART_AXIS_PRIMARY_Y);

    lv_chart_set_all_value(chart, bottomSeries, LV_CHART_POINT_NONE);
    lv_chart_set_all_value(chart, topSeries, LV_CHART_POINT_NONE);
}

namespace AppManualVar
{
lv_coord_t elem_y_offset[] = {-7, 0, 44, 94, 184, 274};
lv_obj_t *chart, *manual_label, *run_btn, *back_btn, *heater[2];
WidgetParameterData topHeater_wpd, bottomHeater_wpd;
lv_coord_t chartWidth = 330, chartHeight = 300;
} // namespace AppManualVar
void app_manual(uint32_t delay)
{
    using namespace AppManualVar;
    using ChartData::bottomSeries, ChartData::topSeries, ChartData::legendColor;
    scr_manual = lv_obj_create(NULL);
    lv_scr_load_anim(scr_manual, LV_SCR_LOAD_ANIM_NONE, 0, delay, true);
    lv_obj_set_scrollbar_mode(scr_manual, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(scr_manual, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(
        scr_manual, [](lv_event_t *e) { init_keyboard(scr_manual); }, LV_EVENT_SCREEN_LOADED, NULL);

    chart = app_create_chart(scr_manual, false, 255, true, chartWidth, chartHeight);
    lv_obj_align(chart, LV_ALIGN_LEFT_MID, 30, elem_y_offset[lv_obj_get_index(chart)]);

    bottomSeries = lv_chart_add_series(chart, legendColor[ChartData::BOTTOM_SERIES], LV_CHART_AXIS_PRIMARY_Y);
    topSeries = lv_chart_add_series(chart, legendColor[ChartData::TOP_SERIES], LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_all_value(chart, bottomSeries, LV_CHART_POINT_NONE);
    lv_chart_set_all_value(chart, topSeries, LV_CHART_POINT_NONE);

    manual_label = lv_label_create(scr_manual);
    lv_obj_set_width(manual_label, 100);
    lvc_label_init(manual_label, &lv_font_montserrat_18, LV_ALIGN_TOP_RIGHT, -3,
                   elem_y_offset[lv_obj_get_index(manual_label)], bs_white);
    lv_label_set_text(manual_label, "Manual\nOperation");
    app_anim_y(manual_label, delay, elem_y_offset[lv_obj_get_index(manual_label)], false);

    run_btn = lv_btn_create(scr_manual);
    lv_obj_set_width(run_btn, 100);
    lv_obj_set_style_radius(run_btn, 2, 0);
    lvc_btn_init(run_btn, LV_SYMBOL_PLAY " START", LV_ALIGN_TOP_RIGHT, -3, elem_y_offset[lv_obj_get_index(run_btn)],
                 &lv_font_montserrat_20, md_red);
    lv_obj_add_event_cb(
        run_btn,
        [](lv_event_t *e) {
            lv_obj_t *run_btn_label = lv_obj_get_child(run_btn, 0);
            LV_APP_MUTEX_ENTER;
            *pStartedManual = !*pStartedManual;
            bool started = *pStartedManual;
            *pSecondsRunning = 0;
            LV_APP_MUTEX_EXIT;
            if (started == 1)
            {
                lv_chart_set_all_value(ChartData::chart, ChartData::topSeries, LV_CHART_POINT_NONE);
                lv_chart_set_all_value(ChartData::chart, ChartData::bottomSeries, LV_CHART_POINT_NONE);
            }
            lv_label_set_text(run_btn_label, started ? LV_SYMBOL_STOP " STOP" : LV_SYMBOL_PLAY " START");
            lv_obj_set_style_bg_color(run_btn, started ? md_teal : md_red, 0);
        },
        LV_EVENT_CLICKED, NULL);
    app_anim_y(run_btn, delay, elem_y_offset[lv_obj_get_index(run_btn)], false);

    const char *heater_label_msg[] = {"TOP HEATER", "BTM HEATER"};
    for (int i = 0; i < 2; i++)
    {
        heater[i] = lv_btn_create(scr_manual);
        lv_obj_set_size(heater[i], 100, 80);
        lv_obj_set_style_radius(heater[i], 2, 0);
        lv_obj_set_style_bg_color(heater[i], md_grad_red, 0);
        lv_obj_set_style_bg_grad_dir(heater[i], LV_GRAD_DIR_VER, 0);
        lv_obj_set_style_bg_grad_color(heater[i], md_red, 0);
        lv_obj_align(heater[i], LV_ALIGN_TOP_RIGHT, -3, elem_y_offset[lv_obj_get_index(heater[i])]);
        lv_obj_t *temp_icon = lv_img_create(heater[i]);
        lv_img_set_src(temp_icon, &temperature_icon);
        lv_obj_align(temp_icon, LV_ALIGN_LEFT_MID, -30, 0);
        lv_obj_t *heater_temp = lv_label_create(heater[i]);
        lv_obj_set_style_text_font(heater_temp, &lv_font_montserrat_26, 0);
        lv_label_set_text_fmt(heater_temp, "%d°C", i == 0 ? cTopHeaterPV : cBottomHeaterPV);
        lv_obj_align(heater_temp, LV_ALIGN_CENTER, 7, 0);
        lv_obj_t *heater_label = lv_label_create(heater[i]);
        lv_obj_set_style_text_font(heater_label, &lv_font_montserrat_12, 0);
        lv_obj_align(heater_label, LV_ALIGN_BOTTOM_MID, 0, 7);
        lv_label_set_text(heater_label, heater_label_msg[i]);
        lv_obj_t *sv_label = lv_label_create(heater[i]);
        lvc_label_init(sv_label, &lv_font_montserrat_12, LV_ALIGN_TOP_RIGHT, 10, -7, bs_white);
        LV_APP_MUTEX_ENTER;
        lv_label_set_text_fmt(sv_label, "SV : %d°C", (i == 0) ? *pTopHeaterSV : *pBottomHeaterSV);
        LV_APP_MUTEX_EXIT;
        lv_obj_add_event_cb(
            heater[i],
            [](lv_event_t *e) {
                lv_obj_t *target = lv_event_get_target(e);
                if (target == heater[0])
                { // Top heater
                    topHeater_wpd.issuer = target;
                    LV_APP_MUTEX_ENTER;
                    topHeater_wpd.param = pTopHeaterSV;
                    LV_APP_MUTEX_EXIT;
                    modal_create_input_number(&topHeater_wpd, false, 3, "Set Top Heater SV");
                }
                else
                { // Top heater
                    bottomHeater_wpd.issuer = target;
                    LV_APP_MUTEX_ENTER;
                    bottomHeater_wpd.param = pBottomHeaterSV;
                    LV_APP_MUTEX_EXIT;
                    modal_create_input_number(&bottomHeater_wpd, false, 3, "Set Bottom Heater SV");
                }
            },
            LV_EVENT_CLICKED, NULL);
        lv_obj_add_event_cb(
            heater[i],
            [](lv_event_t *e) {
                lv_obj_t *target = lv_event_get_target(e);
                lv_obj_t *sv_label = lv_obj_get_child(target, 3);
                LV_APP_MUTEX_ENTER;
                lv_label_set_text_fmt(sv_label, "SV : %d°C", (target == heater[0]) ? *pTopHeaterSV : *pBottomHeaterSV);
                LV_APP_MUTEX_EXIT;
            },
            LV_EVENT_REFRESH, NULL);
        app_anim_y(heater[i], delay, elem_y_offset[lv_obj_get_index(heater[i])], false);
    }

    back_btn = lv_btn_create(scr_manual);
    lv_obj_set_width(back_btn, 100);
    lv_obj_set_style_radius(back_btn, 2, 0);
    lvc_btn_init(back_btn, LV_SYMBOL_HOME " HOME", LV_ALIGN_TOP_RIGHT, -3, elem_y_offset[lv_obj_get_index(back_btn)],
                 &lv_font_montserrat_20);
    app_anim_y(back_btn, delay, elem_y_offset[lv_obj_get_index(back_btn)], false);
    lv_obj_add_event_cb(
        back_btn,
        [](lv_event_t *e) {
            using namespace AppManualVar;
            LV_APP_MUTEX_ENTER;
            bool started = *pStartedManual;
            LV_APP_MUTEX_EXIT;
            if (started)
            {
                modal_create_alert("Can't go home while manual operation is still running!");
                return;
            }
            uint32_t child_cnt = lv_obj_get_child_cnt(scr_manual);
            for (int i = 0; i < child_cnt; i++)
            {
                lv_obj_t *child = lv_obj_get_child(scr_manual, i);
                if (child != chart)
                    app_anim_y(child, 0, elem_y_offset[i], false, true);
            }
            app_home(animTime);
        },
        LV_EVENT_CLICKED, NULL);
}

namespace AppProfilesVar
{
WidgetParameterData profile_wpd; // Used for rollpick
WidgetParameterData confirm_wpd; // Used for modal confirm
lv_obj_t *header, *back, *logo, *profile_btn;
lv_obj_t *box, *label, *saveBtn, *drawBtn, *dataPointTA, *startTopHeaterAtTA, *grid_box;
lv_obj_t **targetSecondTA = NULL, **targetTemperatureTA = NULL;
bool modified;
auto ta_is_modified_cb = [](lv_event_t *e) { modified = true; };
} // namespace AppProfilesVar
void app_profiles(uint32_t delay)
{
    using namespace AppProfilesVar;

    static auto createTextArea = [](lv_obj_t *parent, uint16_t maxLen, uint16_t textAreaLen, bool isFloat,
                                    lv_align_t align, lv_coord_t x_offs, lv_coord_t y_offs) -> lv_obj_t * {
        lv_obj_t *textArea = lv_textarea_create(parent);
        lv_obj_set_style_bg_color(textArea, lv_palette_darken(LV_PALETTE_GREY, 4), LV_PART_MAIN);
        lv_obj_set_style_border_color(textArea, bs_indigo_300, 0);
        lv_obj_set_style_border_width(textArea, 1, 0);
        lv_textarea_set_max_length(textArea, maxLen);
        lv_textarea_set_one_line(textArea, true);
        lv_obj_align(textArea, align, x_offs, y_offs);
        lv_obj_set_width(textArea, textAreaLen);
        lv_obj_set_style_pad_all(textArea, 5, LV_PART_MAIN);
        lv_textarea_set_accepted_chars(textArea, isFloat ? "0123456789." : "0123456789");
        return textArea;
    };

    modified = false;

    LV_APP_MUTEX_ENTER;
    memcpy(&tempProfile, &(*pProfileLists)[*pSelectedProfile], sizeof(Profile));
    LV_APP_MUTEX_EXIT;

    scr_profiles = lv_obj_create(NULL);
    lv_scr_load_anim(scr_profiles, LV_SCR_LOAD_ANIM_NONE, 0, delay, true);
    init_keyboard(scr_profiles);

    lv_obj_t *scr_cont = lv_obj_create(scr_profiles);
    lv_obj_set_style_pad_all(scr_cont, 0, 0);
    lv_obj_set_style_border_width(scr_cont, 0, 0);
    lv_obj_set_style_radius(scr_cont, 0, 0);
    lv_obj_set_style_bg_color(scr_cont, lv_obj_get_style_bg_color(scr_profiles, 0), 0);
    lv_obj_set_width(scr_cont, 480);
    lv_obj_set_height(scr_cont, 320);

    header = lv_obj_create(scr_cont);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_size(header, 480, 60);
    lv_obj_set_scrollbar_mode(header, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(header, 0, 0);
    app_anim_y(header, delay, 0, false);

    back = lv_btn_create(header);
    lv_obj_remove_style_all(back);
    lv_obj_set_size(back, 50, 50);
    lvc_btn_init(back, LV_SYMBOL_LEFT, LV_ALIGN_LEFT_MID, -15, 0, &lv_font_montserrat_24,
                 lv_obj_get_style_bg_color(header, 0));
    lv_obj_clear_flag(back, LV_OBJ_FLAG_CLICK_FOCUSABLE);
    lv_obj_add_event_cb( // Press back button to go back to home screen
        back,
        [](lv_event_t *e) {
            if (modified)
            { // Show warning message before going to home screen because there is some modified changes on the profile
                confirm_wpd.issuer = lv_event_get_target(e);
                modal_create_confirm(
                    &confirm_wpd,
                    "There are some changes on the profile, pressing confirm will discard any changes made");
                return;
            }
            // Otherwise just go to home screen immediately
            lv_event_send(lv_event_get_target(e), LV_EVENT_REFRESH, NULL);
        },
        LV_EVENT_CLICKED, NULL);

    lv_obj_add_event_cb( // Sent from back button click or modal confirm click when there are some changes
        back,
        [](lv_event_t *e) {
            grid_box = NULL;
            for (uint32_t i = 0; i < lv_obj_get_child_cnt(scr_profiles); i++)
            {
                lv_obj_t *child = lv_obj_get_child(scr_profiles, i);
                app_anim_y(child, 0, 0, false, true);
            }
            app_home(animTime);
        },
        LV_EVENT_REFRESH, NULL);

    logo = lv_img_create(header);
    lv_img_set_src(logo, &hotberry_logo);
    lv_obj_align_to(logo, back, LV_ALIGN_OUT_RIGHT_MID, -20, 0);
    lv_img_set_zoom(logo, 190);

    profile_btn = lv_btn_create(header);
    profile_wpd.issuer = profile_btn;
    LV_APP_MUTEX_ENTER;
    profile_wpd.param = pSelectedProfile;
    uint8_t dSelectedProfile = *pSelectedProfile;
    LV_APP_MUTEX_EXIT;
    lv_obj_t *profile_btn_label = lvc_btn_init(profile_btn, "", LV_ALIGN_RIGHT_MID, 0, 0, &lv_font_montserrat_20);
    lv_label_set_text_fmt(profile_btn_label, "PROFILE %d", dSelectedProfile);

    lv_obj_add_event_cb( // Create profile select rollpick when button is pressed
        profile_btn,
        [](lv_event_t *e) {
            if (modified)
            { // There are some changes on the profile, so warn the user first before create profile rollpick
                confirm_wpd.issuer = profile_btn;
                modal_create_confirm(
                    &confirm_wpd,
                    "There are some changes on the profile, changing profile will discard any changes made");
                return;
            }
            // Immediately create profile rollpick by sending LV_EVENT_REFRESH with &confirm_wpd as it's parameter
            lv_event_send(profile_btn, LV_EVENT_REFRESH, &confirm_wpd);
        },
        LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb( // This callback is called when profile rollpick is selected or modal confirm is selected
        profile_btn,
        [](lv_event_t *e) {
            WidgetParameterData *eventParameter_wpd = (WidgetParameterData *)lv_event_get_param(e);
            uint8_t dSelectedProfile;
            if (eventParameter_wpd == &confirm_wpd)
            { // If the parameter is confirm_wpd then we are ordered to create rollpick
                LV_APP_MUTEX_ENTER;
                dSelectedProfile = *pSelectedProfile;
                LV_APP_MUTEX_EXIT;

                lv_obj_t *rollpick =
                    rollpick_create(&profile_wpd, "Choose Profile", profile_roller_list, &lv_font_montserrat_20);
                lv_roller_set_selected(rollpick, dSelectedProfile, LV_ANIM_OFF);
                return;
            } // Else, then the callback is sent from profile rollpick

            // Update the profile labels and text area for datapoint and startHeaterAt
            lv_obj_t *profile_btn = lv_event_get_target(e);
            lv_obj_t *profile_btn_label = lv_obj_get_child(profile_btn, 0);

            LV_APP_MUTEX_ENTER;
            memcpy(&tempProfile, &(*pProfileLists)[*pSelectedProfile], sizeof(Profile));
            dSelectedProfile = *pSelectedProfile;
            LV_APP_MUTEX_EXIT;

            lv_label_set_text_fmt(profile_btn_label, "PROFILE %d", dSelectedProfile);
            char buf[16];

            lv_label_set_text_fmt(label, "PROFILE %d", dSelectedProfile);

            sprintf(buf, "%d", tempProfile.dataPoint);
            lv_textarea_set_text(dataPointTA, buf);

            sprintf(buf, "%d", tempProfile.startTopHeaterAt);
            lv_textarea_set_text(startTopHeaterAtTA, buf);

            lv_event_send(dataPointTA, LV_EVENT_READY, NULL); // Redraw grid_box for profile parameters
            modified = false;
        },
        LV_EVENT_REFRESH, NULL);

    box = lv_obj_create(scr_cont);
    lv_obj_set_size(box, app_display_width - 20, LV_SIZE_CONTENT);
    lv_obj_align(box, LV_ALIGN_TOP_MID, 0, 70);
    app_anim_y(box, delay, 70, true);

    label = lv_label_create(box);
    lvc_label_init(label, &lv_font_montserrat_20, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_label_set_text_fmt(label, "PROFILE %d", dSelectedProfile);

    drawBtn = lv_btn_create(box);
    lvc_btn_init(drawBtn, "View Profile Graph", LV_ALIGN_TOP_RIGHT);
    lv_obj_add_event_cb( // Draw a profile view of current selected profile
        drawBtn,
        [](lv_event_t *e) {
            LV_APP_MUTEX_ENTER;
            uint8_t dSelectedProfile = *pSelectedProfile;
            LV_APP_MUTEX_EXIT;
            lv_obj_t *overlay = lvc_create_overlay();
            lv_obj_set_style_pad_all(overlay, 5, 0);
            lv_obj_t *modal = lv_obj_create(overlay);
            lv_obj_set_size(modal, lv_pct(100), lv_pct(100));
            lv_obj_center(modal);
            lv_obj_t *label = lv_label_create(modal);
            lvc_label_init(label, &lv_font_montserrat_20, LV_ALIGN_TOP_LEFT);
            lv_label_set_text_fmt(label, "Profile %d", dSelectedProfile);
            lv_obj_t *exit_btn = lv_btn_create(modal);
            lvc_btn_init(exit_btn, "Exit", LV_ALIGN_TOP_RIGHT);
            lv_obj_t *chart = app_create_chart(scr_profiles, true, dSelectedProfile, false, 380, 220);
            if (chart)
                lv_obj_align(chart, LV_ALIGN_TOP_MID, 0, 70);
            lv_obj_add_event_cb( // Delete overlay to exit from profile view
                exit_btn,
                [](lv_event_t *e) {
                    lv_obj_t *overlay = lv_obj_get_parent(lv_obj_get_parent(lv_event_get_target(e)));
                    lv_obj_del(overlay);
                    lv_obj_t *chart = (lv_obj_t *)lv_event_get_user_data(e);
                    ChartData::deleteChart();
                },
                LV_EVENT_CLICKED, chart);
        },
        LV_EVENT_CLICKED, NULL);

    saveBtn = lv_btn_create(box);
    lvc_btn_init(saveBtn, "Save");
    lv_obj_align_to(saveBtn, drawBtn, LV_ALIGN_OUT_LEFT_MID, -15, 0);
    lv_obj_add_event_cb(
        saveBtn,
        [](lv_event_t *e) {
            modified = false;
            lv_obj_t *btn = lv_event_get_target(e);
            const char *startTopHeaterAt_txt = lv_textarea_get_text(startTopHeaterAtTA);
            const char *dataPoint_txt = lv_textarea_get_text(dataPointTA);
            tempProfile.startTopHeaterAt = startTopHeaterAt_txt[0] == '\0' ? 0 : std::stol(startTopHeaterAt_txt);
            tempProfile.dataPoint = dataPoint_txt[0] == '\0' ? 0 : std::stol(dataPoint_txt);
            for (int i = 0; i < tempProfile.dataPoint; i++)
            {
                const char *targetSecond_txt = lv_textarea_get_text(targetSecondTA[i]);
                const char *targetTemperature_txt = lv_textarea_get_text(targetTemperatureTA[i]);
                tempProfile.targetSecond[i] = targetSecond_txt[0] == '\0' ? 0 : std::stol(targetSecond_txt);
                tempProfile.targetTemperature[i] =
                    targetTemperature_txt[0] == '\0' ? 0 : std::stol(targetTemperature_txt);
            }

            LV_APP_MUTEX_ENTER;
            memcpy(&(*pProfileLists)[*pSelectedProfile], &tempProfile, sizeof(Profile));
            LV_APP_MUTEX_EXIT;
#ifdef PICO_BOARD
            if (EEPROM.init(
                    EEPROM_I2CBUS, EEPROM_SDA, EEPROM_SCL,
                    EEPROM_BusSpeed)) // For some reason, we need to always init before doing anything with I2C BUS
                EEPROM.memWrite(0, *pProfileLists, sizeof(*pProfileLists));
            else
                printf("EEPROM Not detected!\n");
#endif
        },
        LV_EVENT_CLICKED, NULL);

    char buf[16];

    dataPointTA = createTextArea(box, 2, 100, false, LV_ALIGN_TOP_LEFT, 0, 50);
    sprintf(buf, "%d", tempProfile.dataPoint);
    lv_textarea_set_text(dataPointTA, buf);
    lv_obj_t *ta_label = lv_label_create(box);
    lvc_label_init(ta_label);
    lv_obj_align_to(ta_label, dataPointTA, LV_ALIGN_OUT_TOP_LEFT, 0, 0);
    lv_label_set_text_static(ta_label, "Data Count : ");
    lv_obj_add_event_cb(dataPointTA, ta_is_modified_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(dataPointTA, ta_event_cb, LV_EVENT_ALL, scr_cont); // Attach keyboard to text area
    lv_obj_add_event_cb( // Draw or redraw grid box when dataPointTA is sent LV_EVENT_READY (Keyboard enter)
        dataPointTA,
        [](lv_event_t *e) {
            static constexpr lv_coord_t row_height = 35;
            static constexpr lv_coord_t column_dsc[] = {30, LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
            static constexpr const char *grid_label_text[] = {"No.", "Target Second", "Target Temperature"};
            static lv_coord_t *row_dsc = NULL;

            lv_obj_t *scr_cont = (lv_obj_t *)lv_event_get_user_data(e);
            const char *textArea_txt = lv_textarea_get_text(dataPointTA);

            if (grid_box) // If grid_box is already drawn, delete the object first
                lv_obj_del(grid_box);
            grid_box = lv_obj_create(box);
            lv_obj_set_size(grid_box, lv_pct(100), LV_SIZE_CONTENT);
            lv_obj_align(grid_box, LV_ALIGN_TOP_MID, 0, 150);
            lv_obj_set_layout(grid_box, LV_LAYOUT_GRID);
            lv_obj_set_style_pad_top(grid_box, 0, 0);

            // If textarea is empty then set dataPoint to 0, else set it to numbers specified on textarea
            tempProfile.dataPoint = textArea_txt[0] != '\0' ? std::stol(textArea_txt) : 0;
            if (tempProfile.dataPoint > 20) // Clip dataPoint if it's bigger than 20
            {
                tempProfile.dataPoint = 20;
                lv_textarea_set_text(dataPointTA, "20");
            }

            if (row_dsc) // If row_dsc is already allocated then reallocate following memories
            {
                row_dsc = (lv_coord_t *)realloc(row_dsc, (tempProfile.dataPoint + 2) * sizeof(lv_coord_t));
                // Because targetSecondTA is allocated at the same time as row_dsc, the if statement can only check for
                // row_dsc
                targetSecondTA = (lv_obj_t **)realloc(targetSecondTA, sizeof(lv_coord_t *) * tempProfile.dataPoint);
                targetTemperatureTA =
                    (lv_obj_t **)realloc(targetTemperatureTA, sizeof(lv_coord_t *) * tempProfile.dataPoint);
            }
            else
            { // It seems that row_dsc is not yet allocated, let's allocate it first
                row_dsc = (lv_coord_t *)malloc((tempProfile.dataPoint + 2) * sizeof(lv_coord_t));
                targetSecondTA = (lv_obj_t **)malloc(sizeof(lv_coord_t *) * tempProfile.dataPoint);
                targetTemperatureTA = (lv_obj_t **)malloc(sizeof(lv_coord_t *) * tempProfile.dataPoint);
            }
            // Fill the allocated row_dsc with row_height, but set the last index to LV_GRID_TEMPLATE_LAST
            std::fill(row_dsc, &row_dsc[0] + tempProfile.dataPoint + 1, row_height);
            row_dsc[tempProfile.dataPoint + 1] = LV_GRID_TEMPLATE_LAST;

            // Set the grid description for grid_box
            lv_obj_set_grid_dsc_array(grid_box, column_dsc, row_dsc);

            for (int i = 0; i < 3; i++) // Create column descriptor for each column with label
            {
                lv_obj_t *grid_label = lv_label_create(grid_box);
                lvc_label_init(grid_label);
                lv_label_set_text_static(grid_label, grid_label_text[i]);
                lv_obj_set_grid_cell(grid_label, LV_GRID_ALIGN_CENTER, i, 1, LV_GRID_ALIGN_CENTER, 0, 1);
            }
            for (int i = 0; i < tempProfile.dataPoint; i++) // Create dataPoint numbers of row on the gridBox
            {
                char buf[16];
                // Row number label
                lv_obj_t *no_label = lv_label_create(grid_box);
                lvc_label_init(no_label);
                lv_label_set_text_fmt(no_label, "%d", i + 1);
                lv_obj_set_grid_cell(no_label, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, i + 1, 1);

                // Create text area for targetSeconds
                targetSecondTA[i] = createTextArea(grid_box, 3, 100, false, LV_ALIGN_CENTER, 0, 0);
                sprintf(buf, "%d", tempProfile.targetSecond[i]);
                lv_textarea_set_text(targetSecondTA[i], buf);
                lv_obj_set_grid_cell(targetSecondTA[i], LV_GRID_ALIGN_CENTER, 1, 1, LV_GRID_ALIGN_CENTER, i + 1, 1);
                lv_obj_add_event_cb(targetSecondTA[i], ta_event_cb, LV_EVENT_ALL,
                                    scr_cont); // Attach keyboard to textarea
                lv_obj_add_event_cb(targetSecondTA[i], ta_is_modified_cb, LV_EVENT_VALUE_CHANGED, NULL);
                no_label = lv_label_create(grid_box); // Create "s" label on the right side of textarea
                lv_obj_add_flag(no_label, LV_OBJ_FLAG_IGNORE_LAYOUT);
                lvc_label_init(no_label);
                lv_label_set_text_static(no_label, " s");
                lv_obj_align_to(no_label, targetSecondTA[i], LV_ALIGN_OUT_RIGHT_MID, 0, 0);

                targetTemperatureTA[i] = createTextArea(grid_box, 3, 100, false, LV_ALIGN_CENTER, 0, 0);
                sprintf(buf, "%d", tempProfile.targetTemperature[i]);
                lv_textarea_set_text(targetTemperatureTA[i], buf);
                lv_obj_set_grid_cell(targetTemperatureTA[i], LV_GRID_ALIGN_CENTER, 2, 1, LV_GRID_ALIGN_CENTER, i + 1,
                                     1);
                lv_obj_add_event_cb(targetTemperatureTA[i], ta_is_modified_cb, LV_EVENT_VALUE_CHANGED, NULL);
                lv_obj_add_event_cb(targetTemperatureTA[i], ta_event_cb, LV_EVENT_ALL, scr_cont);
                no_label = lv_label_create(grid_box);
                lv_obj_add_flag(no_label, LV_OBJ_FLAG_IGNORE_LAYOUT);
                lvc_label_init(no_label);
                lv_label_set_text_static(no_label, " °C");
                lv_obj_align_to(no_label, targetTemperatureTA[i], LV_ALIGN_OUT_RIGHT_MID, 0, 0);
                if (i == 0)
                {
                    lv_textarea_set_text(targetSecondTA[i], "0");
                    lv_textarea_set_text(targetTemperatureTA[i], "30");
                    lv_obj_add_state(targetSecondTA[i], LV_STATE_DISABLED);
                    lv_obj_add_state(targetTemperatureTA[i], LV_STATE_DISABLED);
                }
            }
            // Make sure the layout is updated
            lv_obj_update_layout(grid_box);
            lv_obj_update_layout(box);
        },
        LV_EVENT_READY, scr_cont);

    startTopHeaterAtTA = createTextArea(box, 2, 100, false, LV_ALIGN_TOP_LEFT, 0, 100);
    sprintf(buf, "%d", tempProfile.startTopHeaterAt);
    lv_textarea_set_text(startTopHeaterAtTA, buf);
    ta_label = lv_label_create(box);
    lvc_label_init(ta_label);
    lv_obj_align_to(ta_label, startTopHeaterAtTA, LV_ALIGN_OUT_TOP_LEFT, 0, 0);
    lv_label_set_text_static(ta_label, "Start Heater At Index : ");
    lv_obj_add_event_cb(startTopHeaterAtTA, ta_is_modified_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(startTopHeaterAtTA, ta_event_cb, LV_EVENT_ALL, scr_cont);

    lv_event_send(dataPointTA, LV_EVENT_READY, NULL);

    modified = false;
}

namespace AppVarSettings
{
lv_obj_t *header, *topHeater_cont, *bottomHeater_cont;
lv_coord_t elem_y_offset[] = {0, 70, 70};
} // namespace AppVarSettings
void app_settings(uint32_t delay)
{
    using namespace AppVarSettings;

    static auto createTextArea = [](lv_obj_t *parent, uint16_t maxLen, uint16_t textAreaLen, bool isFloat,
                                    lv_align_t align, lv_coord_t x_offs, lv_coord_t y_offs) -> lv_obj_t * {
        lv_obj_t *textArea = lv_textarea_create(parent);
        lv_obj_set_style_bg_color(textArea, lv_palette_darken(LV_PALETTE_GREY, 4), LV_PART_MAIN);
        lv_obj_set_style_border_color(textArea, bs_indigo_300, 0);
        lv_obj_set_style_border_width(textArea, 1, 0);
        lv_textarea_set_max_length(textArea, maxLen);
        lv_textarea_set_one_line(textArea, true);
        lv_obj_align(textArea, align, x_offs, y_offs);
        lv_obj_set_width(textArea, textAreaLen);
        lv_obj_set_style_pad_all(textArea, 5, LV_PART_MAIN);
        lv_textarea_set_accepted_chars(textArea, isFloat ? "0123456789." : "0123456789");
        return textArea;
    };

    static lv_coord_t ta_y_offs[] = {25, 63, 101};
    static lv_coord_t ta_x_offs[] = {0, 5, 0};
    static const char *msg[] = {"P", "I", "D"};

    scr_settings = lv_obj_create(NULL);
    lv_scr_load_anim(scr_settings, LV_SCR_LOAD_ANIM_NONE, 0, delay, true);
    init_keyboard(scr_settings);

    lv_obj_t *scr_cont = lv_obj_create(scr_settings);
    lv_obj_set_style_pad_all(scr_cont, 0, 0);
    lv_obj_set_style_border_width(scr_cont, 0, 0);
    lv_obj_set_style_radius(scr_cont, 0, 0);
    lv_obj_set_style_bg_color(scr_cont, lv_obj_get_style_bg_color(scr_settings, 0), 0);
    lv_obj_set_width(scr_cont, 480);
    lv_obj_set_height(scr_cont, 320);

    header = lv_obj_create(scr_cont);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_size(header, 480, 60);
    lv_obj_set_scrollbar_mode(header, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(header, 0, 0);
    app_anim_y(header, delay, 0, false);

    lv_obj_t *back = lv_btn_create(header);
    lv_obj_remove_style_all(back);
    lv_obj_set_size(back, 50, 50);
    lvc_btn_init(back, LV_SYMBOL_LEFT, LV_ALIGN_LEFT_MID, -15, 0, &lv_font_montserrat_24,
                 lv_obj_get_style_bg_color(header, 0));
    lv_obj_clear_flag(back, LV_OBJ_FLAG_CLICK_FOCUSABLE);
    lv_obj_add_event_cb(
        back,
        [](lv_event_t *e) {
            for (int i = 0; i < 3; i++)
            {
                float tval = std::stof(lv_textarea_get_text(lv_obj_get_child(topHeater_cont, 2 * (i + 1))));
                float bval = std::stof(lv_textarea_get_text(lv_obj_get_child(bottomHeater_cont, 2 * (i + 1))));
                LV_APP_MUTEX_ENTER;
                (*pTopHeaterPID)[i] = tval;
                (*pBottomHeaterPID)[i] = bval;
                LV_APP_MUTEX_EXIT;
            }
#ifdef PICO_BOARD

            if (EEPROM.init(
                    EEPROM_I2CBUS, EEPROM_SDA, EEPROM_SCL,
                    EEPROM_BusSpeed)) // For some reason, we need to always init before doing anything with I2C BUS
            {
                EEPROM.memWrite(sizeof(*pProfileLists), *pTopHeaterPID, sizeof(*pTopHeaterPID));
                EEPROM.memWrite(sizeof(*pProfileLists) + sizeof(*pTopHeaterPID), *pBottomHeaterPID,
                                sizeof(*pBottomHeaterPID));
            }
            else
                printf("EEPROM Not detected!\n");
#endif
            for (uint32_t i = 0; i < lv_obj_get_child_cnt(scr_settings); i++)
            {
                lv_obj_t *child = lv_obj_get_child(scr_settings, i);
                app_anim_y(child, 0, elem_y_offset[lv_obj_get_index(child)], false, true);
            }
            app_home(animTime);
        },
        LV_EVENT_CLICKED, NULL);

    lv_obj_t *logo = lv_img_create(header);
    lv_img_set_src(logo, &hotberry_logo);
    lv_obj_align_to(logo, back, LV_ALIGN_OUT_RIGHT_MID, -20, 0);
    lv_img_set_zoom(logo, 190);
    
    lv_obj_t *settings_label = lv_label_create(header);
    lvc_label_init(settings_label, &lv_font_montserrat_24, LV_ALIGN_RIGHT_MID, 0, 0, bs_white);
    lv_label_set_text_static(settings_label, "SETTINGS");

    for (int i = 0; i < 2; i++)
    {
        lv_obj_t *cont = lv_obj_create(scr_cont);
        if (i == 0)
            topHeater_cont = cont;
        else
            bottomHeater_cont = cont;
        lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF);
        lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_align(cont, LV_ALIGN_TOP_LEFT, i == 0 ? 10 : 245, elem_y_offset[lv_obj_get_index(cont)]);
        lv_obj_set_size(cont, 225, LV_SIZE_CONTENT);
        lv_obj_t *cont_label = lv_label_create(cont);
        lvc_label_init(cont_label, &lv_font_montserrat_20, LV_ALIGN_TOP_LEFT, -5, -10);
        lv_label_set_text_static(cont_label, i == 0 ? "Top Heater PID" : "Bottom Heater PID");
        for (int y = 0; y < 3; y++)
        {
            char ta_buf[10];
            lv_obj_t *pid_label = lv_label_create(cont);
            lvc_label_init(pid_label, &lv_font_montserrat_20, LV_ALIGN_TOP_LEFT, ta_x_offs[y], ta_y_offs[y]);
            lv_obj_t *pid_ta = createTextArea(cont, pidIsFloat ? 10 : 4, 100, pidIsFloat, LV_ALIGN_TOP_LEFT, 0, 0);
            LV_APP_MUTEX_ENTER;
            sprintf(ta_buf, "%f", i == 0 ? (*pTopHeaterPID)[y] : (*pBottomHeaterPID)[y]);
            LV_APP_MUTEX_EXIT;
            lv_textarea_set_text(pid_ta, ta_buf);
            lv_obj_align_to(pid_ta, pid_label, LV_ALIGN_OUT_RIGHT_MID, -10 - ta_x_offs[y], 0);
            lv_label_set_text(pid_label, msg[y]);
            lv_obj_add_event_cb(pid_ta, ta_event_cb, LV_EVENT_ALL, scr_cont);
        }
        app_anim_y(cont, delay, 0, false);
    }
}

void app_anim_y(lv_obj_t *obj, uint32_t delay, lv_coord_t offs, bool reverse, bool out)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_time(&a, animTime);
    lv_anim_set_delay(&a, delay);

    lv_coord_t obj_y = lv_obj_get_y(obj);

    // Adjust the direction of the animation depending on "out" value
    if (out)
    {
        lv_anim_set_values(&a, reverse ? obj_y - animTranslationY : obj_y,
                           reverse ? obj_y + animTranslationY : obj_y - animTranslationY);
        lv_obj_fade_out(obj, animTime - 50, delay);
    }
    else
    {
        lv_anim_set_values(&a, reverse ? obj_y + animTranslationY : obj_y - animTranslationY, obj_y + offs);

        lv_obj_fade_in(obj, animTime + 50, delay + animTime - 300);
    }
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_y);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_start(&a);
}

lv_obj_t *rollpick_create(WidgetParameterData *wpd, const char *headerTitle, const char *options,
                          const lv_font_t *headerFont, lv_coord_t width, lv_coord_t height)
{
    lv_obj_t *overlay = lvc_create_overlay();

    lv_obj_t *modal = lv_obj_create(overlay);
    lv_obj_set_size(modal, width, height); // Most fit number
    lv_obj_align(modal, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *modalTitle = lv_label_create(modal);
    lvc_label_init(modalTitle, headerFont, LV_ALIGN_TOP_LEFT, 0, -5);
    lv_label_set_text_static(modalTitle, headerTitle);

    lv_obj_t *modalRoller = lv_roller_create(modal);
    lv_roller_set_options(modalRoller, options, LV_ROLLER_MODE_NORMAL);
    lv_obj_set_width(modalRoller, lv_pct(100));
    lv_roller_set_visible_row_count(modalRoller, 3);
    lv_obj_set_style_bg_color(modalRoller, bs_indigo_700, LV_PART_SELECTED);
    lv_obj_align(modalRoller, LV_ALIGN_CENTER, 0, -10);

    lv_obj_t *modalButton = lv_btn_create(modal);
    lvc_btn_init(modalButton, "Choose", LV_ALIGN_BOTTOM_LEFT, 50, 0, &lv_font_montserrat_12);
    lv_obj_add_event_cb(
        modalButton,
        [](lv_event_t *event) {
            lv_obj_t *clickedBtn = lv_event_get_target(event);
            lv_obj_t *modal = lv_obj_get_parent(clickedBtn);
            lv_obj_t *overlay = lv_obj_get_parent(modal);
            lv_obj_t *modalRoller = lv_obj_get_child(modal, 1);
            WidgetParameterData *wpd = (WidgetParameterData *)lv_event_get_user_data(event);
            LV_APP_MUTEX_ENTER;
            *(uint16_t *)wpd->param = lv_roller_get_selected(modalRoller);
            LV_APP_MUTEX_EXIT;
            // Get selected roller value
            lv_event_send(wpd->issuer, LV_EVENT_REFRESH, wpd);
            // Exit from modal
            lv_obj_del(overlay);
        },
        LV_EVENT_CLICKED, wpd);

    modalButton = lv_btn_create(modal);
    lvc_btn_init(modalButton, "Cancel", LV_ALIGN_BOTTOM_RIGHT, -50, 0, &lv_font_montserrat_12);
    lv_obj_add_event_cb(
        modalButton,
        [](lv_event_t *event) {
            void *ovl = lv_event_get_user_data(event);
            lv_obj_del((lv_obj_t *)ovl);
        },
        LV_EVENT_CLICKED, overlay);
    return modalRoller;
}

lv_obj_t *modal_create_confirm(WidgetParameterData *modalConfirmData, const char *message, const char *headerText,
                               const lv_font_t *headerFont, const lv_font_t *messageFont, lv_color_t headerTextColor,
                               lv_color_t textColor, lv_color_t headerColor, const char *confirmButtonText,
                               const char *cancelButtonText, lv_coord_t xSize, lv_coord_t ySize)
{
    lv_obj_t *modal = modal_create_alert(message, headerText, headerFont, messageFont, headerTextColor, textColor,
                                         headerColor, cancelButtonText, xSize, ySize);
    lv_obj_t *okButton = lv_btn_create(modal);
    lvc_btn_init(okButton, confirmButtonText, LV_ALIGN_BOTTOM_RIGHT, -120, -15);
    lv_obj_add_event_cb(
        okButton,
        [](lv_event_t *e) {
            WidgetParameterData *modalConfirmData = (WidgetParameterData *)lv_event_get_user_data(e);
            lv_obj_t *btn = lv_event_get_target(e);
            lv_event_send(modalConfirmData->issuer, LV_EVENT_REFRESH, modalConfirmData);
            lv_obj_del(lv_obj_get_parent(lv_obj_get_parent(btn)));
        },
        LV_EVENT_CLICKED, modalConfirmData);
    return modal;
}

lv_obj_t *modal_create_alert(const char *message, const char *headerText, const lv_font_t *headerFont,
                             const lv_font_t *messageFont, lv_color_t headerTextColor, lv_color_t textColor,
                             lv_color_t headerColor, const char *buttonText, lv_coord_t xSize, lv_coord_t ySize)
{
    lv_obj_t *overlay = lvc_create_overlay();

    lv_obj_t *modal = lv_obj_create(overlay);
    lv_obj_center(modal);
    lv_obj_set_size(modal, xSize, ySize);
    lv_obj_set_style_pad_all(modal, 0, 0);

    lv_obj_t *modalHeader = lv_obj_create(modal);
    lv_obj_align(modalHeader, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_size(modalHeader, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_radius(overlay, 0, 0);
    lv_obj_set_style_bg_color(modalHeader, headerColor, 0);

    lv_obj_t *warningLabel = lv_label_create(modalHeader);
    lvc_label_init(warningLabel, &lv_font_montserrat_20, LV_ALIGN_TOP_LEFT, 0, 0, headerTextColor, LV_TEXT_ALIGN_LEFT,
                   LV_LABEL_LONG_WRAP, lv_pct(100));
    lv_label_set_text_static(warningLabel, headerText);

    lv_obj_t *error = lv_label_create(modal);
    lvc_label_init(error, &lv_font_montserrat_14, LV_ALIGN_CENTER, 0, 0, textColor, LV_TEXT_ALIGN_CENTER,
                   LV_LABEL_LONG_WRAP, lv_pct(100));
    lv_label_set_text_static(error, message);

    lv_obj_t *okButton = lv_btn_create(modal);
    lvc_btn_init(okButton, buttonText, LV_ALIGN_BOTTOM_RIGHT, -15, -15);
    lv_obj_add_event_cb(
        okButton, [](lv_event_t *e) { lv_obj_del((lv_obj_t *)lv_event_get_user_data(e)); }, LV_EVENT_CLICKED, overlay);
    return modal;
}

lv_obj_t *modal_create_input_number(WidgetParameterData *modalConfirmData, bool isFloat, uint8_t maxLen,
                                    const char *headerText, uint16_t textAreaLen, const lv_font_t *headerFont,
                                    lv_color_t headerTextColor, lv_color_t headerColor, const char *confirmButtonText,
                                    const char *cancelButtonText, lv_coord_t xSize, lv_coord_t ySize)
{
    static constexpr lv_obj_flag_t LV_TEXT_AREA_IS_FLOAT = LV_OBJ_FLAG_USER_1;
    lv_obj_t *modal = modal_create_alert("", headerText, headerFont, lv_font_default(), headerTextColor, bs_white,
                                         headerColor, cancelButtonText, xSize, ySize);
    lv_obj_del(lv_obj_get_child(modal, 1)); // Delete alert message and replace with textarea

    lv_obj_t *textArea = lv_textarea_create(modal);
    lv_obj_set_style_bg_color(textArea, lv_palette_darken(LV_PALETTE_GREY, 4), LV_PART_MAIN);
    lv_obj_set_style_border_color(textArea, bs_indigo_300, 0);
    lv_obj_set_style_border_width(textArea, 1, 0);
    lv_textarea_set_max_length(textArea, maxLen);
    lv_textarea_set_one_line(textArea, true);
    lv_obj_align(textArea, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_width(textArea, textAreaLen);
    lv_obj_set_style_pad_all(textArea, 5, LV_PART_MAIN);
    lv_textarea_set_accepted_chars(textArea, isFloat ? "0123456789." : "0123456789");
    // Attach keyboard to textArea, and pass overlay (parent of modal) pointer to be get it's height modified when
    // keyboard appear
    lv_obj_add_event_cb(textArea, ta_event_cb, LV_EVENT_ALL, lv_obj_get_parent(modal));
    // Force focus on textarea so keyboard appear on creation of modal
    lv_event_send(textArea, LV_EVENT_FOCUSED, NULL);
    // Force defocus textarea so keyboard disappear on deletion of modal
    lv_obj_add_event_cb(
        modal,
        [](lv_event_t *e) {
            lv_obj_t *ta = (lv_obj_t *)lv_event_get_user_data(e);
            lv_event_send(ta, LV_EVENT_DEFOCUSED, NULL);
        },
        LV_EVENT_DELETE, textArea);

    // Add custom flag for float number
    if (isFloat)
        lv_obj_add_flag(textArea, LV_TEXT_AREA_IS_FLOAT);

    lv_obj_t *okButton = lv_btn_create(modal);
    lvc_btn_init(okButton, confirmButtonText, LV_ALIGN_BOTTOM_RIGHT, -120, -15);
    lv_obj_add_event_cb(
        okButton,
        [](lv_event_t *e) {
            WidgetParameterData *modalConfirmData = (WidgetParameterData *)lv_event_get_user_data(e);
            lv_obj_t *btn = lv_event_get_target(e);
            lv_obj_t *textArea = lv_obj_get_child(lv_obj_get_parent(btn), 2);
            const char *textArea_txt = lv_textarea_get_text(textArea);

            /**
             * Convert textarea content to uint32_t or float and store to referenced
            WidgetParameterData depending on flag stored on textarea
             */
            if (lv_obj_has_flag(textArea, LV_TEXT_AREA_IS_FLOAT))
            {
                if (textArea_txt[0] != '.' && textArea_txt[0] != '\0') // Avoid core dump
                {
                    *(float *)modalConfirmData->param = std::stof(textArea_txt);
                    lv_event_send(modalConfirmData->issuer, LV_EVENT_REFRESH, modalConfirmData);
                }
            }
            else if (textArea_txt[0] != '\0') // Avoid core dump
            {
                *(uint32_t *)modalConfirmData->param = std::stol(textArea_txt);
                lv_event_send(modalConfirmData->issuer, LV_EVENT_REFRESH, modalConfirmData);
            }
            lv_obj_del(lv_obj_get_parent(lv_obj_get_parent(btn)));
        },
        LV_EVENT_CLICKED, modalConfirmData);
    return modal;
}

lv_obj_t *lvc_create_overlay()
{
    lv_obj_t *overlay = lv_obj_create(lv_scr_act());
    lv_obj_set_size(overlay, app_display_width, app_display_height);
    lv_obj_set_style_border_width(overlay, 0, 0);
    lv_obj_set_style_radius(overlay, 0, 0);
    lv_obj_set_style_bg_color(overlay, bs_dark, 0);
    lv_obj_set_style_bg_opa(overlay, 178, 0); // 70% opacity
    return overlay;
}

void lvc_label_init(lv_obj_t *label, const lv_font_t *font, lv_align_t align, lv_coord_t offsetX, lv_coord_t offsetY,
                    lv_color_t textColor, lv_text_align_t alignText, lv_label_long_mode_t longMode,
                    lv_coord_t textWidth)
{
    lv_obj_set_style_text_color(label, textColor, 0);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_align(label, alignText, 0);
    lv_obj_align(label, align, offsetX, offsetY);
    if (longMode != LV_LABEL_LONG_WRAP) // Set long mode if set value is not defaulted
        lv_label_set_long_mode(label, longMode);
    if (textWidth != 0) // Only set label width if passed textWidth value is not 0
        lv_obj_set_width(label, textWidth);
}

lv_obj_t *lvc_btn_init(lv_obj_t *btn, const char *labelText, lv_align_t align, lv_coord_t offsetX, lv_coord_t offsetY,
                       const lv_font_t *font, lv_color_t bgColor, lv_color_t textColor, lv_text_align_t alignText,
                       lv_label_long_mode_t longMode, lv_coord_t labelWidth, lv_coord_t btnSizeX, lv_coord_t btnSizeY)
{
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text_static(label, labelText);
    lv_obj_align(btn, align, offsetX, offsetY);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_bg_color(btn, bgColor, 0);
    lv_obj_set_style_text_color(label, textColor, 0);
    lv_obj_set_style_text_align(label, alignText, 0);
    lv_label_set_long_mode(label, longMode);
    if (labelWidth != 0)
        lv_obj_set_width(label, labelWidth);
    lv_obj_center(label); // Center the label
    if (labelWidth != 0)
        lv_obj_set_width(label, labelWidth);
    if (btnSizeX != 0)
        lv_obj_set_width(btn, btnSizeX);
    if (btnSizeY != 0)
        lv_obj_set_height(btn, btnSizeY);
    return label;
}