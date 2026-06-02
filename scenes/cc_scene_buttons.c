#include "../cheap_clicker_i.h"
#include "../helpers/cc_profile.h"

static void cc_buttons_cb(void* context, uint32_t index) {
    view_dispatcher_send_custom_event(((CheapClickerApp*)context)->view_dispatcher, index);
}

void cc_scene_buttons_on_enter(void* context) {
    CheapClickerApp* app = context;
    CcProfile* p = &app->profiles[app->active_profile_idx];
    submenu_reset(app->submenu);
    for(uint8_t i = 0; i < p->button_count; i++) {
        char label[64];
        snprintf(label, sizeof(label), "[%u] %s (%d,%d)", i, p->buttons[i].name,
                 p->buttons[i].x, p->buttons[i].y);
        submenu_add_item(app->submenu, label, i, cc_buttons_cb, app);
    }
    view_dispatcher_switch_to_view(app->view_dispatcher, CheapClickerViewSubmenu);
}

bool cc_scene_buttons_on_event(void* context, SceneManagerEvent event) {
    CheapClickerApp* app = context;
    if(event.type != SceneManagerEventTypeCustom) return false;
    app->edit_button_idx = (uint8_t)event.event;
    strlcpy(app->text_input_buf,
            app->profiles[app->active_profile_idx].buttons[app->edit_button_idx].name,
            CC_MAX_NAME_LEN);
    app->button_edit_mode = 0xFF;
    scene_manager_next_scene(app->scene_manager, CheapClickerSceneButtonEdit);
    return true;
}

void cc_scene_buttons_on_exit(void* context) {
    submenu_reset(((CheapClickerApp*)context)->submenu);
}
