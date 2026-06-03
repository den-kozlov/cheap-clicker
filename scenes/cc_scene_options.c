#include "../cheap_clicker_i.h"

typedef enum {
    CcOptionsItemButtons = 0,
} CcOptionsItem;

static void cc_options_cb(void* context, uint32_t index) {
    view_dispatcher_send_custom_event(((CheapClickerApp*)context)->view_dispatcher, index);
}

void cc_scene_options_on_enter(void* context) {
    CheapClickerApp* app = context;
    submenu_reset(app->submenu);
    submenu_add_item(app->submenu, "Buttons", CcOptionsItemButtons, cc_options_cb, app);
    submenu_set_selected_item(
        app->submenu,
        scene_manager_get_scene_state(app->scene_manager, CheapClickerSceneOptions));
    view_dispatcher_switch_to_view(app->view_dispatcher, CheapClickerViewSubmenu);
}

bool cc_scene_options_on_event(void* context, SceneManagerEvent event) {
    CheapClickerApp* app = context;
    if(event.type != SceneManagerEventTypeCustom) return false;
    scene_manager_set_scene_state(app->scene_manager, CheapClickerSceneOptions, event.event);
    if(event.event == CcOptionsItemButtons) {
        scene_manager_next_scene(app->scene_manager, CheapClickerSceneButtons);
    }
    return true;
}

void cc_scene_options_on_exit(void* context) {
    submenu_reset(((CheapClickerApp*)context)->submenu);
}
