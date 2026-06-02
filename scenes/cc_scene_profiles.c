#include "../cheap_clicker_i.h"
#include "../helpers/cc_profile.h"
#include "../helpers/cc_ble.h"

static void cc_profiles_cb(void* context, uint32_t index) {
    view_dispatcher_send_custom_event(((CheapClickerApp*)context)->view_dispatcher, index);
}

void cc_scene_profiles_on_enter(void* context) {
    CheapClickerApp* app = context;
    submenu_reset(app->submenu);
    for(uint8_t i = 0; i < app->profile_count; i++)
        submenu_add_item(app->submenu, app->profiles[i].name, i, cc_profiles_cb, app);
    if(app->profile_count < CC_MAX_PROFILES)
        submenu_add_item(app->submenu, "Connect New", CC_MAX_PROFILES, cc_profiles_cb, app);
    submenu_set_selected_item(
        app->submenu,
        scene_manager_get_scene_state(app->scene_manager, CheapClickerSceneProfiles));
    view_dispatcher_switch_to_view(app->view_dispatcher, CheapClickerViewSubmenu);
}

bool cc_scene_profiles_on_event(void* context, SceneManagerEvent event) {
    CheapClickerApp* app = context;
    if(event.type != SceneManagerEventTypeCustom) return false;
    scene_manager_set_scene_state(app->scene_manager, CheapClickerSceneProfiles, event.event);
    if(event.event == CC_MAX_PROFILES) {
        // save=false: keep in memory only until BLE pairs (see cc_scene_pairing)
        uint8_t idx = cc_profile_add(app, "iPhone", "CheapClip", false);
        if(idx != CC_PROFILE_IDX_NONE) {
            app->active_profile_idx = idx;
            app->new_profile_pending = true;
            if(app->ble_hid_profile) cc_ble_stop(app);
            cc_ble_start(app);
            scene_manager_next_scene(app->scene_manager, CheapClickerScenePairing);
        }
    } else {
        // Select existing device — make active and return to start screen
        app->active_profile_idx = (uint8_t)event.event;
        cc_profile_save_active(app);
        if(app->ble_hid_profile) cc_ble_stop(app);
        cc_ble_start(app);
        scene_manager_previous_scene(app->scene_manager);
    }
    return true;
}

void cc_scene_profiles_on_exit(void* context) {
    submenu_reset(((CheapClickerApp*)context)->submenu);
}
