#include "keyboard.h"
#include <stdio.h>
#include <unistd.h>

lv_obj_t *regularKeyboard;
lv_obj_t *numericKeyboard;
char kb_copy_buffer[128];
void kb_custom_event_cb(lv_event_t *e);
void kb_event_cb(lv_event_t *e);

void init_keyboard(lv_obj_t *parent)
{
    regularKeyboard = lv_keyboard_create(parent);
    lv_keyboard_set_map(regularKeyboard, LV_KEYBOARD_MODE_TEXT_LOWER, (const char **)regularKeyboard_map,
                        regularKeyboard_controlMap);
    lv_obj_remove_event_cb(regularKeyboard, lv_keyboard_def_event_cb);
    lv_obj_add_event_cb(regularKeyboard, kb_custom_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_keyboard_set_textarea(regularKeyboard, NULL);
    lv_obj_add_flag(regularKeyboard, LV_OBJ_FLAG_HIDDEN);

    numericKeyboard = lv_keyboard_create(parent);
    lv_keyboard_set_map(numericKeyboard, LV_KEYBOARD_MODE_TEXT_LOWER, (const char **)numericKeyboard_map,
                        numericKeyboard_controlMap);
    lv_keyboard_set_textarea(numericKeyboard, NULL);
    lv_obj_add_flag(numericKeyboard, LV_OBJ_FLAG_HIDDEN);

    lv_obj_add_flag(numericKeyboard, LV_OBJ_FLAG_FLOATING);
    lv_obj_align(numericKeyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
}

void kb_custom_event_cb(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_target(e);
    lv_keyboard_t *kb = (lv_keyboard_t *)obj;
    uint16_t btn_id = lv_btnmatrix_get_selected_btn(obj);
    if (btn_id == LV_BTNMATRIX_BTN_NONE)
        return;
    const char *txt = lv_btnmatrix_get_btn_text(obj, lv_btnmatrix_get_selected_btn(obj));
    if (kb->ta == NULL)
        return;
    if (strcmp(txt, "Copy") == 0)
        strcpy(kb_copy_buffer, lv_textarea_get_text(kb->ta));
    else if (strcmp(txt, "Paste") == 0)
        lv_textarea_add_text(kb->ta, kb_copy_buffer);
    else if (strcmp(txt, LV_SYMBOL_NEW_LINE) == 0)
    { // Pressing enter will send LV_EVENT_READY
        lv_res_t res = lv_event_send(obj, LV_EVENT_READY, NULL);
        if (res != LV_RES_OK)
            return;
    }
    else if (strcmp(txt, "abc") == 0)
    {
        kb->mode = LV_KEYBOARD_MODE_TEXT_LOWER;
        lv_btnmatrix_set_map(obj, (const char **)regularKeyboard_map);
        lv_btnmatrix_set_ctrl_map(obj, regularKeyboard_controlMap);
        return;
    }
    else
        lv_keyboard_def_event_cb(e);
}

void ta_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *ta = lv_event_get_target(e);
    lv_obj_t *kb = numericKeyboard;
    lv_obj_t *tv = (lv_obj_t *)lv_event_get_user_data(e);
    if (code == LV_EVENT_FOCUSED)
    {
        if (lv_indev_get_type(lv_indev_get_act()) != LV_INDEV_TYPE_KEYPAD)
        {
            lv_keyboard_set_textarea(kb, ta);
            lv_obj_set_style_max_height(kb, LV_HOR_RES * 2 / 3, 0);
            lv_obj_update_layout(tv); /*Be sure the sizes are recalculated*/
            lv_obj_set_height(tv, LV_VER_RES - lv_obj_get_height(kb));
            lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);
            lv_obj_scroll_to_view_recursive(ta, LV_ANIM_OFF);
        }
    }
    else if (code == LV_EVENT_DEFOCUSED)
    {
        lv_keyboard_set_textarea(kb, NULL);
        lv_obj_set_height(tv, LV_VER_RES);
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
        lv_indev_reset(NULL, ta);
    }
    else if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL)
    {
        lv_obj_set_height(tv, LV_VER_RES);
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_state(ta, LV_STATE_FOCUSED);
        lv_indev_reset(NULL, ta); /*To forget the last clicked object to make it focusable again*/
    }
}