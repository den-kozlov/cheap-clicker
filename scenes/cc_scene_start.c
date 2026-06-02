#include "../cheap_clicker_i.h"

typedef enum { CcStartRun = 0, CcStartProfiles, CcStartScripts } CcStartIdx;

static void cc_start_cb(void* context, uint32_t index) {
    view_dispatcher_send_custom_event(((CheapClickerApp*)context)->view_dispatcher, index);
}

void cc_scene_start_on_enter(void* context) {
    CheapClickerApp* app = context;
    submenu_reset(app->submenu);
    char run_label[48] = "Run Script";
    if(app->profile_count > 0)
        snprintf(run_label, sizeof(run_label), "Run [%s]", app->profiles[app->active_profile_idx].name);
    submenu_add_item(app->submenu, run_label, CcStartRun, cc_start_cb, app);
    submenu_add_item(app->submenu, "Profiles", CcStartProfiles, cc_start_cb, app);
    submenu_add_item(app->submenu, "Scripts", CcStartScripts, cc_start_cb, app);
    submenu_set_selected_item(app->submenu,
        scene_manager_get_scene_state(app->scene_manager, CheapClickerSceneStart));
    view_dispatcher_switch_to_view(app->view_dispatcher, CheapClickerViewSubmenu);
}

bool cc_scene_start_on_event(void* context, SceneManagerEvent event) {
    CheapClickerApp* app = context;
    if(event.type != SceneManagerEventTypeCustom) return false;
    scene_manager_set_scene_state(app->scene_manager, CheapClickerSceneStart, event.event);
    if(event.event == CcStartProfiles) {
        scene_manager_next_scene(app->scene_manager, CheapClickerSceneProfiles);
    } else if(event.event == CcStartRun || event.event == CcStartScripts) {
        if(app->profile_count == 0) {
            // No profiles — go to Profiles scene instead to add one
            scene_manager_next_scene(app->scene_manager, CheapClickerSceneProfiles);
        } else {
            scene_manager_next_scene(app->scene_manager, CheapClickerSceneScripts);
        }
    }
    return true;
}

void cc_scene_start_on_exit(void* context) {
    submenu_reset(((CheapClickerApp*)context)->submenu);
}
