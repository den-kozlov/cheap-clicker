#include "../cheap_clicker_i.h"
#include "../helpers/cc_profile.h"
#include "../helpers/cc_ble.h"
#include "../views/cc_calibrate_view.h"
#include <ble_profile/extra_profiles/hid_profile.h>

static uint8_t s_calibrate_step;

static void advance_calibrate(CheapClickerApp* app) {
    CcProfile* p = &app->profiles[app->active_profile_idx];
    s_calibrate_step++;
    if(s_calibrate_step > p->button_count) {
        cc_profile_save(app, app->active_profile_idx);
        scene_manager_previous_scene(app->scene_manager);
        return;
    }
    char label[48];
    snprintf(label, sizeof(label), "[%u/%u] %s",
             s_calibrate_step, p->button_count,
             p->buttons[s_calibrate_step - 1].name);
    cc_calibrate_view_set_label(app->calibrate_view, label);
    cc_calibrate_view_set_coords(app->calibrate_view, 0, 0);
    cc_ble_reset_cursor(app);
}

static void cc_cal_confirm_cb(void* context, int16_t x, int16_t y) {
    CheapClickerApp* app = context;
    CcProfile* p = &app->profiles[app->active_profile_idx];
    if(s_calibrate_step == 0) {
        p->trigger_x = x;
        p->trigger_y = y;
    } else {
        uint8_t bi = s_calibrate_step - 1;
        if(bi < p->button_count) {
            p->buttons[bi].x = x;
            p->buttons[bi].y = y;
        }
    }
    advance_calibrate(app);
}

static void cc_cal_move_cb(void* context, int8_t dx, int8_t dy) {
    CheapClickerApp* app = context;
    if(app->ble_hid_profile)
        ble_profile_hid_mouse_move(app->ble_hid_profile, dx, dy);
}

void cc_scene_calibrate_on_enter(void* context) {
    CheapClickerApp* app = context;
    s_calibrate_step = 0;
    cc_calibrate_view_set_confirm_callback(app->calibrate_view, cc_cal_confirm_cb, app);
    cc_calibrate_view_set_move_callback(app->calibrate_view, cc_cal_move_cb, app);
    cc_calibrate_view_set_label(app->calibrate_view, "TRIGGER");
    cc_calibrate_view_set_coords(app->calibrate_view, 0, 0);
    if(!app->ble_hid_profile) cc_ble_start(app);
    cc_ble_reset_cursor(app);
    view_dispatcher_switch_to_view(app->view_dispatcher, CheapClickerViewCalibrate);
}

bool cc_scene_calibrate_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void cc_scene_calibrate_on_exit(void* context) {
    UNUSED(context);
}
