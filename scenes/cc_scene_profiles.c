#include "../cheap_clicker_i.h"
#include "../helpers/cc_profile.h"

static void cc_profiles_cb(void* context, uint32_t index) {
    view_dispatcher_send_custom_event(((CheapClickerApp*)context)->view_dispatcher, index);
}

void cc_scene_profiles_on_enter(void* context) {
    CheapClickerApp* app = context;
    submenu_reset(app->submenu);
    for(uint8_t i = 0; i < app->profile_count; i++)
        submenu_add_item(app->submenu, app->profiles[i].name, i, cc_profiles_cb, app);
    if(app->profile_count < CC_MAX_PROFILES)
        submenu_add_item(app->submenu, "+ Add Profile", CC_MAX_PROFILES, cc_profiles_cb, app);
    view_dispatcher_switch_to_view(app->view_dispatcher, CheapClickerViewSubmenu);
}

bool cc_scene_profiles_on_event(void* context, SceneManagerEvent event) {
    CheapClickerApp* app = context;
    if(event.type != SceneManagerEventTypeCustom) return false;
    if(event.event == CC_MAX_PROFILES) {
        uint8_t idx = cc_profile_add(app, "New Profile", "CheapClip");
        if(idx != CC_PROFILE_IDX_NONE) {
            app->active_profile_idx = idx;
            scene_manager_next_scene(app->scene_manager, CheapClickerSceneProfileEdit);
        }
    } else {
        app->active_profile_idx = (uint8_t)event.event;
        scene_manager_next_scene(app->scene_manager, CheapClickerSceneProfileEdit);
    }
    return true;
}

void cc_scene_profiles_on_exit(void* context) {
    submenu_reset(((CheapClickerApp*)context)->submenu);
}
