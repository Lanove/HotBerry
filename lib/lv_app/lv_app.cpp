#include "lv_app.h"

static constexpr uint32_t animTime = 500;
static constexpr uint32_t animTranslationY = 150;

void (*home_btn_press_cb[])(uint32_t) = {app_auto, app_manual, app_profiles, app_settings};
static lv_obj_t *scr_home;
static lv_obj_t *scr_auto;
static lv_obj_t *scr_manual;
static lv_obj_t *scr_profiles;
static lv_obj_t *scr_settings;

namespace AppHomeVar
{
static const lv_color_t boxColors[] = {md_blue, md_teal, md_red, md_purple};
static const lv_color_t gradColors[] = {md_grad_blue, md_grad_teal, md_grad_red, md_grad_purple};
static const char *message[] = {"AUTO\nOPERATION", "MANUAL\nOPERATION", "PROFILES", "SETTINGS"};
static const void *iconSrc[] = {&robot_icon, &finger_icon, &documents_icon, &setting_icon};
static const lv_align_t align[] = {LV_ALIGN_TOP_LEFT, LV_ALIGN_TOP_RIGHT, LV_ALIGN_BOTTOM_LEFT, LV_ALIGN_BOTTOM_RIGHT};
static const lv_coord_t x_offs[] = {10, -10, 10, -10};
static const lv_coord_t y_offs[] = {5, 5, -5, -5};
static const bool reverseAnim[] = {false, false, true, true};
} // namespace AppHomeVar

void lv_app_entry()
{
    static constexpr uint32_t hotberry_fadein_dur = 000;
    static constexpr uint32_t hotberry_stay_dur = 000;
    static constexpr uint32_t hotberry_fadeout_dur = 000;

    lv_obj_t *scr = lv_obj_create(NULL);
    lv_scr_load(scr);
    lv_obj_t *hb_logo = lv_img_create(scr);
    lv_img_set_src(hb_logo, &hotberry_logo);
    lv_obj_align(hb_logo, LV_ALIGN_CENTER, 0, 0);

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

    app_home(hotberry_fadein_dur + hotberry_stay_dur + hotberry_fadeout_dur);
}

void app_home(uint32_t delay)
{
    using namespace AppHomeVar;
    scr_home = lv_obj_create(NULL);
    lv_scr_load_anim(scr_home, LV_SCR_LOAD_ANIM_NONE, 0, delay, true);
    lv_obj_set_scrollbar_mode(scr_home, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(scr_home, LV_OBJ_FLAG_SCROLLABLE);
    for (int i = 0; i < 4; i++)
    {
        lv_obj_t *box = lv_btn_create(scr_home);
        lv_obj_set_size(box, 225, 150);
        lv_obj_set_style_radius(box, 2, 0);
        lv_obj_set_style_bg_grad_dir(box, LV_GRAD_DIR_VER, 0);
        lv_obj_set_style_bg_grad_color(box, gradColors[i], 0);
        lv_obj_set_style_bg_color(box, boxColors[i], 0);
        lv_obj_align(box, align[i], x_offs[i], y_offs[i]);
        lv_obj_add_event_cb(
            box,
            [](lv_event_t *e) {
                lv_obj_t *box = lv_event_get_target(e);
                for (int i = 0; i < 4; i++)
                {
                    lv_obj_t *child = lv_obj_get_child(scr_home, i);
                    app_anim_y(child, 0, y_offs[i], reverseAnim[i], true);
                }
                (*home_btn_press_cb[lv_obj_get_index(box)])(animTime);
            },
            LV_EVENT_PRESSED, NULL);

        lv_obj_t *label = lv_label_create(box);
        lv_label_set_text_fmt(label, "%s", message[i]);
        lv_obj_align(label, LV_ALIGN_BOTTOM_LEFT, 0, 0);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_20, 0);

        lv_obj_t *icon = lv_img_create(box);
        lv_img_set_src(icon, iconSrc[i]);
        lv_obj_align(icon, LV_ALIGN_TOP_RIGHT, 10, -10);
        app_anim_y(box, delay, y_offs[i], reverseAnim[i]);
    }
}

namespace AppAutoVar
{
lv_coord_t elem_y_offset[] = {-7, 0, 44, 94, 134, 204, 274};
lv_obj_t *chart, *auto_label, *run_btn, *profile_btn, *back_btn, *heater[2];
} // namespace AppAutoVar
void app_auto(uint32_t delay)
{
    using namespace AppAutoVar;
    scr_auto = lv_obj_create(NULL);
    lv_scr_load_anim(scr_auto, LV_SCR_LOAD_ANIM_NONE, 0, delay, true);
    lv_obj_set_scrollbar_mode(scr_auto, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(scr_auto, LV_OBJ_FLAG_SCROLLABLE);

    chart = lv_chart_create(scr_auto);
    lv_obj_set_style_bg_color(chart, lv_obj_get_style_bg_color(scr_auto, 0), 0);
    lv_obj_set_style_line_color(chart, bs_gray_500, LV_PART_MAIN);
    lv_obj_set_size(chart, 340, 300);
    lv_obj_align(chart, LV_ALIGN_LEFT_MID, 30, elem_y_offset[lv_obj_get_index(chart)]);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_obj_set_style_border_side(chart, LV_BORDER_SIDE_LEFT | LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(chart, bs_gray_100, 0);
    lv_obj_set_style_radius(chart, 0, 0);
    lv_chart_set_axis_tick(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 0, 5, 1, true, 80);
    lv_chart_set_axis_tick(chart, LV_CHART_AXIS_PRIMARY_X, 0, 0, 10, 1, true, 50);
    lv_chart_set_div_line_count(chart, 0, 0);
    lv_obj_fade_in(chart, animTime, delay + animTime - 200);

    /*Add two data series*/
    lv_chart_series_t *ser1 = lv_chart_add_series(chart, bs_warning, LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_series_t *ser2 = lv_chart_add_series(chart, bs_red, LV_CHART_AXIS_PRIMARY_Y);

    /*Set the next points on 'ser1'*/
    lv_chart_set_next_value(chart, ser1, 10);
    lv_chart_set_next_value(chart, ser1, 10);
    lv_chart_set_next_value(chart, ser1, 10);
    lv_chart_set_next_value(chart, ser1, 10);
    lv_chart_set_next_value(chart, ser1, 10);
    lv_chart_set_next_value(chart, ser1, 10);
    lv_chart_set_next_value(chart, ser1, 10);
    lv_chart_set_next_value(chart, ser1, 30);
    lv_chart_set_next_value(chart, ser1, 70);
    lv_chart_set_next_value(chart, ser1, 90);

    /*Directly set points on 'ser2'*/
    ser2->y_points[0] = 90;
    ser2->y_points[1] = 70;
    ser2->y_points[2] = 65;
    ser2->y_points[3] = 65;
    ser2->y_points[4] = 65;
    ser2->y_points[5] = 65;
    ser2->y_points[6] = 65;
    ser2->y_points[7] = 65;
    ser2->y_points[8] = 65;
    ser2->y_points[9] = 65;

    lv_chart_refresh(chart); /*Required after direct set*/

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

    lv_obj_t *profile_btn = lv_btn_create(scr_auto);
    lv_obj_set_size(profile_btn, 100, 30);
    lv_obj_set_style_radius(profile_btn, 2, 0);
    lvc_btn_init(profile_btn, "PROFILE 0", LV_ALIGN_TOP_RIGHT, -3, elem_y_offset[lv_obj_get_index(profile_btn)],
                 &lv_font_montserrat_16);
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
        lv_label_set_text(heater_temp, "555°C");
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
            uint32_t child_cnt = lv_obj_get_child_cnt(scr_auto);
            for (int i = 0; i < child_cnt; i++)
            {
                lv_obj_t *child = lv_obj_get_child(scr_auto, i);
                if (child == chart)
                    lv_obj_fade_out(child, animTime, 0);
                else
                    app_anim_y(child, 0, elem_y_offset[i], false, true);
            }
            app_home(animTime);
        },
        LV_EVENT_PRESSED, NULL);
}

namespace AppManualVar
{
lv_coord_t elem_y_offset[] = {-7, 0, 44, 94, 184, 274};
lv_obj_t *chart, *auto_label, *run_btn,  *back_btn, *heater[2];
} // namespace AppAutoVar
void app_manual(uint32_t delay)
{
    using namespace AppManualVar;
    scr_manual = lv_obj_create(NULL);
    lv_scr_load_anim(scr_manual, LV_SCR_LOAD_ANIM_NONE, 0, delay, true);
    lv_obj_set_scrollbar_mode(scr_manual, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(scr_manual, LV_OBJ_FLAG_SCROLLABLE);

    chart = lv_chart_create(scr_manual);
    lv_obj_set_style_bg_color(chart, lv_obj_get_style_bg_color(scr_manual, 0), 0);
    lv_obj_set_style_line_color(chart, bs_gray_500, LV_PART_MAIN);
    lv_obj_set_size(chart, 340, 300);
    lv_obj_align(chart, LV_ALIGN_LEFT_MID, 30, elem_y_offset[lv_obj_get_index(chart)]);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_obj_set_style_border_side(chart, LV_BORDER_SIDE_LEFT | LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(chart, bs_gray_100, 0);
    lv_obj_set_style_radius(chart, 0, 0);
    lv_chart_set_axis_tick(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 0, 5, 1, true, 80);
    lv_chart_set_axis_tick(chart, LV_CHART_AXIS_PRIMARY_X, 0, 0, 10, 1, true, 50);
    lv_chart_set_div_line_count(chart, 0, 0);
    lv_obj_fade_in(chart, animTime, delay + animTime - 200);

    /*Add two data series*/
    lv_chart_series_t *ser1 = lv_chart_add_series(chart, bs_warning, LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_series_t *ser2 = lv_chart_add_series(chart, bs_red, LV_CHART_AXIS_PRIMARY_Y);

    /*Set the next points on 'ser1'*/
    lv_chart_set_next_value(chart, ser1, 10);
    lv_chart_set_next_value(chart, ser1, 10);
    lv_chart_set_next_value(chart, ser1, 10);
    lv_chart_set_next_value(chart, ser1, 10);
    lv_chart_set_next_value(chart, ser1, 10);
    lv_chart_set_next_value(chart, ser1, 10);
    lv_chart_set_next_value(chart, ser1, 10);
    lv_chart_set_next_value(chart, ser1, 30);
    lv_chart_set_next_value(chart, ser1, 70);
    lv_chart_set_next_value(chart, ser1, 90);

    /*Directly set points on 'ser2'*/
    ser2->y_points[0] = 90;
    ser2->y_points[1] = 70;
    ser2->y_points[2] = 65;
    ser2->y_points[3] = 65;
    ser2->y_points[4] = 65;
    ser2->y_points[5] = 65;
    ser2->y_points[6] = 65;
    ser2->y_points[7] = 65;
    ser2->y_points[8] = 65;
    ser2->y_points[9] = 65;

    lv_chart_refresh(chart); /*Required after direct set*/

    auto_label = lv_label_create(scr_manual);
    lv_obj_set_width(auto_label, 100);
    lvc_label_init(auto_label, &lv_font_montserrat_18, LV_ALIGN_TOP_RIGHT, -3,
                   elem_y_offset[lv_obj_get_index(auto_label)], bs_white);
    lv_label_set_text(auto_label, "Manual\nOperation");
    app_anim_y(auto_label, delay, elem_y_offset[lv_obj_get_index(auto_label)], false);

    run_btn = lv_btn_create(scr_manual);
    lv_obj_set_width(run_btn, 100);
    lv_obj_set_style_radius(run_btn, 2, 0);
    lvc_btn_init(run_btn, LV_SYMBOL_PLAY " START", LV_ALIGN_TOP_RIGHT, -3, elem_y_offset[lv_obj_get_index(run_btn)],
                 &lv_font_montserrat_20, md_red);
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
        lv_label_set_text(heater_temp, "555°C");
        lv_obj_align(heater_temp, LV_ALIGN_CENTER, 7, 0);
        lv_obj_t *heater_label = lv_label_create(heater[i]);
        lv_obj_set_style_text_font(heater_label, &lv_font_montserrat_12, 0);
        lv_obj_align(heater_label, LV_ALIGN_BOTTOM_MID, 0, 7);
        lv_label_set_text(heater_label, heater_label_msg[i]);
        lv_obj_t* sv_label = lv_label_create(heater[i]);
        lvc_label_init(sv_label,&lv_font_montserrat_12,LV_ALIGN_TOP_RIGHT,10,-7,bs_white);
        lv_label_set_text(sv_label,"SV : 555°C");
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
            uint32_t child_cnt = lv_obj_get_child_cnt(scr_manual);
            for (int i = 0; i < child_cnt; i++)
            {
                lv_obj_t *child = lv_obj_get_child(scr_manual, i);
                if (child == chart)
                    lv_obj_fade_out(child, animTime, 0);
                else
                    app_anim_y(child, 0, elem_y_offset[i], false, true);
            }
            app_home(animTime);
        },
        LV_EVENT_PRESSED, NULL);
}
void app_profiles(uint32_t delay)
{
    scr_profiles = lv_obj_create(NULL);
    lv_scr_load_anim(scr_profiles, LV_SCR_LOAD_ANIM_NONE, 0, delay, true);
    lv_obj_set_scrollbar_mode(scr_profiles, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(scr_profiles, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *header = lv_obj_create(scr_profiles);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_size(header, 480, 60);
    lv_obj_set_scrollbar_mode(header, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(header, 0, 0);
    app_anim_y(header, delay, 0, false);

    lv_obj_t *back = lv_btn_create(header);
    lv_obj_remove_style_all(back);
    lvc_btn_init(back, LV_SYMBOL_LEFT, LV_ALIGN_LEFT_MID, 0, 0, &lv_font_montserrat_24,
                 lv_obj_get_style_bg_color(header, 0));
    lv_obj_clear_flag(back, LV_OBJ_FLAG_CLICK_FOCUSABLE);
    lv_obj_add_event_cb(
        back,
        [](lv_event_t *e) {
            for (uint32_t i = 0; i < lv_obj_get_child_cnt(scr_profiles); i++)
            {
                lv_obj_t *child = lv_obj_get_child(scr_profiles, i);
                app_anim_y(child, 0, 0, false, true);
            }
            app_home(animTime);
        },
        LV_EVENT_PRESSED, NULL);

    lv_obj_t *logo = lv_img_create(header);
    lv_img_set_src(logo, &hotberry_logo);
    lv_obj_align_to(logo, back, LV_ALIGN_OUT_RIGHT_MID, -10, 0);
    lv_img_set_zoom(logo, 190);

    lv_obj_t *settings_label = lv_label_create(header);
    lvc_label_init(settings_label, &lv_font_montserrat_24, LV_ALIGN_RIGHT_MID, 0, 0, bs_white);
    lv_label_set_text_static(settings_label, "PROFILES");
}
void app_settings(uint32_t delay)
{
    scr_settings = lv_obj_create(NULL);
    lv_scr_load_anim(scr_settings, LV_SCR_LOAD_ANIM_NONE, 0, delay, true);
    lv_obj_set_scrollbar_mode(scr_settings, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(scr_settings, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *header = lv_obj_create(scr_settings);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_size(header, 480, 60);
    lv_obj_set_scrollbar_mode(header, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(header, 0, 0);
    app_anim_y(header, delay, 0, false);

    lv_obj_t *back = lv_btn_create(header);
    lv_obj_remove_style_all(back);
    lvc_btn_init(back, LV_SYMBOL_LEFT, LV_ALIGN_LEFT_MID, 0, 0, &lv_font_montserrat_24,
                 lv_obj_get_style_bg_color(header, 0));
    lv_obj_clear_flag(back, LV_OBJ_FLAG_CLICK_FOCUSABLE);
    lv_obj_add_event_cb(
        back,
        [](lv_event_t *e) {
            for (uint32_t i = 0; i < lv_obj_get_child_cnt(scr_settings); i++)
            {
                lv_obj_t *child = lv_obj_get_child(scr_settings, i);
                app_anim_y(child, 0, 0, false, true);
            }
            app_home(animTime);
        },
        LV_EVENT_PRESSED, NULL);

    lv_obj_t *logo = lv_img_create(header);
    lv_img_set_src(logo, &hotberry_logo);
    lv_obj_align_to(logo, back, LV_ALIGN_OUT_RIGHT_MID, -10, 0);
    lv_img_set_zoom(logo, 190);

    lv_obj_t *settings_label = lv_label_create(header);
    lvc_label_init(settings_label, &lv_font_montserrat_24, LV_ALIGN_RIGHT_MID, 0, 0, bs_white);
    lv_label_set_text_static(settings_label, "SETTINGS");
}

void app_anim_y(lv_obj_t *obj, uint32_t delay, lv_coord_t offs, bool reverse, bool out)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_time(&a, animTime);
    lv_anim_set_delay(&a, delay);

    lv_coord_t obj_y = lv_obj_get_y(obj);

    if (out)
        lv_anim_set_values(&a, reverse ? obj_y - animTranslationY : obj_y,
                           reverse ? obj_y + animTranslationY : obj_y - animTranslationY);
    else
        lv_anim_set_values(&a, reverse ? obj_y + animTranslationY : obj_y - animTranslationY, obj_y + offs);

    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_y);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_start(&a);

    if (out)
        lv_obj_fade_out(obj, animTime - 50, delay);
    else
        lv_obj_fade_in(obj, animTime + 50, delay + animTime - 300);
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