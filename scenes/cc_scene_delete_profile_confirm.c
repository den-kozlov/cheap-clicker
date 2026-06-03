#include "../cheap_clicker_i.h"
#include "../helpers/cc_profile.h"
#include "../helpers/cc_ble.h"

static void cc_delete_profile_confirm_cb(DialogExResult result, void* context) {
    CheapClickerApp* app = context;
    if(result == DialogExResultRight) {
        view_dispatcher_send_custom_event(
            app->view_dispatcher, CheapClickerCustomEventProfileSelected);
    } else {
        view_dispatcher_send_custom_event(
            app->view_dispatcher, CheapClickerCustomEventPopupDismissed);
    }
}

void cc_scene_delete_profile_confirm_on_enter(void* context) {
    CheapClickerApp* app = context;
    DialogEx* d = app->dialog_ex;
    dialog_ex_reset(d);
    dialog_ex_set_header(d, "Delete Profile?", 64, 10, AlignCenter, AlignCenter);
    dialog_ex_set_text(
        d, app->profiles[app->active_profile_idx].name, 64, 30, AlignCenter, AlignCenter);
    dialog_ex_set_left_button_text(d, "Cancel");
    dialog_ex_set_right_button_text(d, "Delete");
    dialog_ex_set_result_callback(d, cc_delete_profile_confirm_cb);
    dialog_ex_set_context(d, app);
    view_dispatcher_switch_to_view(app->view_dispatcher, CheapClickerViewDialogEx);
}

bool cc_scene_delete_profile_confirm_on_event(void* context, SceneManagerEvent event) {
    CheapClickerApp* app = context;
    if(event.type != SceneManagerEventTypeCustom) return false;
    if(event.event == CheapClickerCustomEventProfileSelected) {
        if(app->ble_hid_profile) cc_ble_stop(app);
        cc_profile_delete(app, app->active_profile_idx);
        scene_manager_search_and_switch_to_previous_scene(
            app->scene_manager, CheapClickerSceneStart);
        return true;
    }
    if(event.event == CheapClickerCustomEventPopupDismissed) {
        scene_manager_previous_scene(app->scene_manager);
        return true;
    }
    return false;
}

void cc_scene_delete_profile_confirm_on_exit(void* context) {
    dialog_ex_reset(((CheapClickerApp*)context)->dialog_ex);
}
