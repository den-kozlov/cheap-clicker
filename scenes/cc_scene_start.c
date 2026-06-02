#include "../cheap_clicker_i.h"

static void cc_start_event_cb(void* context, CcStartViewEvent event) {
    view_dispatcher_send_custom_event(
        ((CheapClickerApp*)context)->view_dispatcher, (uint32_t)event);
}

void cc_scene_start_on_enter(void* context) {
    CheapClickerApp* app = context;

    bool has_device = app->active_profile_idx != CC_PROFILE_IDX_NONE;
    const char* name = has_device ? app->profiles[app->active_profile_idx].name : NULL;

    cc_start_view_set_callback(app->start_view, cc_start_event_cb, app);
    cc_start_view_update(app->start_view, name, has_device);

    uint32_t saved = scene_manager_get_scene_state(app->scene_manager, CheapClickerSceneStart);
    cc_start_view_set_cursor(app->start_view, (uint8_t)saved);

    view_dispatcher_switch_to_view(app->view_dispatcher, CheapClickerViewStart);
}

bool cc_scene_start_on_event(void* context, SceneManagerEvent event) {
    CheapClickerApp* app = context;
    if(event.type != SceneManagerEventTypeCustom) return false;

    scene_manager_set_scene_state(
        app->scene_manager,
        CheapClickerSceneStart,
        cc_start_view_get_cursor(app->start_view));

    switch((CcStartViewEvent)event.event) {
    case CcStartViewEventDeviceSelect:
        scene_manager_next_scene(app->scene_manager, CheapClickerSceneProfiles);
        break;
    case CcStartViewEventDeviceEdit:
        if(app->active_profile_idx != CC_PROFILE_IDX_NONE)
            scene_manager_next_scene(app->scene_manager, CheapClickerSceneProfileEdit);
        break;
    case CcStartViewEventScripts:
        scene_manager_next_scene(app->scene_manager, CheapClickerSceneScripts);
        break;
    case CcStartViewEventManual:
        scene_manager_next_scene(app->scene_manager, CheapClickerSceneManual);
        break;
    case CcStartViewEventOptions:
        scene_manager_next_scene(app->scene_manager, CheapClickerSceneOptions);
        break;
    default:
        return false;
    }
    return true;
}

void cc_scene_start_on_exit(void* context) {
    CheapClickerApp* app = context;
    scene_manager_set_scene_state(
        app->scene_manager,
        CheapClickerSceneStart,
        cc_start_view_get_cursor(app->start_view));
}
