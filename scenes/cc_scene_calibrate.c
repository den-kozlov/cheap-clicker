#include "../cheap_clicker_i.h"
#include "../helpers/cc_profile.h"
#include "../helpers/cc_ble.h"
#include "../views/cc_calibrate_view.h"
#include <ble_profile/extra_profiles/hid_profile.h>

// 0 = trigger step; 1..button_count = button steps
static uint8_t s_calibrate_step;
// true when a DragAndRelease button step has the trigger button held down
static bool s_trigger_held;
// single-mode: calibrate one button only
static bool s_single_mode;
static uint8_t s_single_step;

static void cc_cal_release_trigger(CheapClickerApp* app) {
    if(s_trigger_held && app->ble_hid_profile) {
        ble_profile_hid_mouse_release(app->ble_hid_profile, HID_MOUSE_BTN_LEFT);
        furi_delay_ms(50);
    }
    s_trigger_held = false;
}

static void advance_calibrate(CheapClickerApp* app) {
    CcProfile* p = &app->profiles[app->active_profile_idx];
    s_calibrate_step++;

    uint8_t end_step = s_single_mode ? s_single_step : app->button_count;
    if(s_calibrate_step > end_step) {
        cc_profile_save_calib(app, app->active_profile_idx);
        scene_manager_previous_scene(app->scene_manager);
        return;
    }

    uint8_t bi = s_calibrate_step - 1;
    char label[48];
    snprintf(
        label,
        sizeof(label),
        "[%u/%u] %s",
        s_calibrate_step,
        app->button_count,
        app->buttons[bi].name);
    cc_calibrate_view_set_label(app->calibrate_view, label);

    int16_t sx = p->calib[bi].x;
    int16_t sy = p->calib[bi].y;
    cc_calibrate_view_set_coords(app->calibrate_view, sx, sy);

    if(app->buttons[bi].type == CcButtonTypeDragAndRelease) {
        // Move to trigger and hold left button so the target becomes visible on the game screen
        cc_ble_reset_cursor(app);
        furi_delay_ms(50);
        cc_ble_move_to(app, p->trigger_x, p->trigger_y);
        if(app->ble_hid_profile) {
            ble_profile_hid_mouse_press(app->ble_hid_profile, HID_MOUSE_BTN_LEFT);
            furi_delay_ms(100);
        }
        s_trigger_held = true;
        cc_ble_move_to(app, sx, sy);
    } else {
        cc_ble_reset_cursor(app);
        cc_ble_move_to(app, sx, sy);
    }
}

static void cc_cal_skip_cb(void* context) {
    CheapClickerApp* app = context;
    if(s_calibrate_step == 0 || s_single_mode) {
        cc_cal_release_trigger(app);
        scene_manager_previous_scene(app->scene_manager);
        return;
    }
    cc_cal_release_trigger(app);
    advance_calibrate(app);
}

static void cc_cal_abort_cb(void* context) {
    CheapClickerApp* app = context;
    cc_cal_release_trigger(app);
    if(app->active_profile_idx != CC_PROFILE_IDX_NONE &&
       app->active_profile_idx < app->profile_count) {
        cc_profile_save_calib(app, app->active_profile_idx);
    }
    scene_manager_previous_scene(app->scene_manager);
}

static void cc_cal_test_cb(void* context, int16_t x, int16_t y) {
    CheapClickerApp* app = context;
    CcProfile* p = &app->profiles[app->active_profile_idx];

    if(s_calibrate_step == 0) {
        cc_ble_click_at(app, x, y);
        return;
    }

    uint8_t bi = s_calibrate_step - 1;
    cc_cal_release_trigger(app);

    if(app->buttons[bi].type == CcButtonTypeDragAndRelease) {
        cc_ble_press_button(app, p->trigger_x, p->trigger_y, x, y, 100);
        // Don't re-hold the trigger: dragging back to the button position would
        // look like a second DnR press to the game. Cursor is already at x,y.
        s_trigger_held = false;
    } else {
        cc_ble_click_at(app, x, y);
    }
}

static void cc_cal_confirm_cb(void* context, int16_t x, int16_t y) {
    CheapClickerApp* app = context;
    CcProfile* p = &app->profiles[app->active_profile_idx];

    if(s_calibrate_step == 0) {
        p->trigger_x = x;
        p->trigger_y = y;
    } else {
        uint8_t bi = s_calibrate_step - 1;
        if(bi < app->button_count) {
            p->calib[bi].x = x;
            p->calib[bi].y = y;
        }
    }

    // Release trigger before advancing (advance_calibrate may re-hold for next DnR button)
    cc_cal_release_trigger(app);
    advance_calibrate(app);
}

static void cc_cal_move_cb(void* context, int8_t dx, int8_t dy) {
    CheapClickerApp* app = context;
    cc_ble_move_by(app, dx, dy);
}

static void cc_cal_get_pos_cb(void* context, int16_t* x, int16_t* y) {
    CheapClickerApp* app = context;
    cc_ble_get_pos(app, x, y);
}

void cc_scene_calibrate_on_enter(void* context) {
    CheapClickerApp* app = context;

    if(app->profile_count == 0 || app->active_profile_idx == CC_PROFILE_IDX_NONE) {
        scene_manager_previous_scene(app->scene_manager);
        return;
    }

    s_single_mode = app->calibrate_single_mode;
    s_trigger_held = false;

    cc_calibrate_view_set_confirm_callback(app->calibrate_view, cc_cal_confirm_cb, app);
    cc_calibrate_view_set_move_callback(app->calibrate_view, cc_cal_move_cb, app);
    cc_calibrate_view_set_get_pos_callback(app->calibrate_view, cc_cal_get_pos_cb, app);
    cc_calibrate_view_set_skip_callback(app->calibrate_view, cc_cal_skip_cb, app);
    cc_calibrate_view_set_test_callback(app->calibrate_view, cc_cal_test_cb, app);
    cc_calibrate_view_set_abort_callback(app->calibrate_view, cc_cal_abort_cb, app);

    if(!app->ble_hid_profile) cc_ble_start(app);

    CcProfile* p = &app->profiles[app->active_profile_idx];

    if(s_single_mode) {
        uint8_t bi = app->calibrate_target_btn;
        s_single_step = bi + 1;
        s_calibrate_step = bi + 1;

        char label[48];
        snprintf(label, sizeof(label), "[1/1] %s", app->buttons[bi].name);
        cc_calibrate_view_set_label(app->calibrate_view, label);

        int16_t sx = p->calib[bi].x;
        int16_t sy = p->calib[bi].y;
        cc_calibrate_view_set_coords(app->calibrate_view, sx, sy);

        if(app->buttons[bi].type == CcButtonTypeDragAndRelease) {
            cc_ble_reset_cursor(app);
            furi_delay_ms(50);
            cc_ble_move_to(app, p->trigger_x, p->trigger_y);
            if(app->ble_hid_profile) {
                ble_profile_hid_mouse_press(app->ble_hid_profile, HID_MOUSE_BTN_LEFT);
                furi_delay_ms(100);
            }
            s_trigger_held = true;
            cc_ble_move_to(app, sx, sy);
        } else {
            cc_ble_reset_cursor(app);
            cc_ble_move_to(app, sx, sy);
        }
    } else {
        s_calibrate_step = 0;
        cc_calibrate_view_set_label(app->calibrate_view, "TRIGGER");
        cc_calibrate_view_set_coords(app->calibrate_view, p->trigger_x, p->trigger_y);
        cc_ble_reset_cursor(app);
        cc_ble_move_to(app, p->trigger_x, p->trigger_y);
    }

    view_dispatcher_switch_to_view(app->view_dispatcher, CheapClickerViewCalibrate);
}

bool cc_scene_calibrate_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void cc_scene_calibrate_on_exit(void* context) {
    CheapClickerApp* app = context;
    // Ensure trigger is released if user presses Back mid-calibration
    cc_cal_release_trigger(app);
    app->calibrate_single_mode = false;
    if(app->active_profile_idx != CC_PROFILE_IDX_NONE &&
       app->active_profile_idx < app->profile_count) {
        cc_profile_save_calib(app, app->active_profile_idx);
    }
}
