#include "../cheap_clicker_i.h"
#include "../helpers/cc_profile.h"

#define CC_BUTTONS_NEW_IDX CC_MAX_BUTTONS

static void cc_buttons_cb(void* context, uint32_t index) {
    view_dispatcher_send_custom_event(((CheapClickerApp*)context)->view_dispatcher, index);
}

void cc_scene_buttons_on_enter(void* context) {
    CheapClickerApp* app = context;
    submenu_reset(app->submenu);
    for(uint8_t i = 0; i < app->button_count; i++) {
        const char* type_tag =
            (app->buttons[i].type == CcButtonTypePress) ? "[P]" : "[D]";
        char label[48];
        snprintf(label, sizeof(label), "%s %s", type_tag, app->buttons[i].name);
        submenu_add_item(app->submenu, label, i, cc_buttons_cb, app);
    }
    if(app->button_count < CC_MAX_BUTTONS) {
        submenu_add_item(app->submenu, "<New>", CC_BUTTONS_NEW_IDX, cc_buttons_cb, app);
    }
    view_dispatcher_switch_to_view(app->view_dispatcher, CheapClickerViewSubmenu);
}

bool cc_scene_buttons_on_event(void* context, SceneManagerEvent event) {
    CheapClickerApp* app = context;
    if(event.type != SceneManagerEventTypeCustom) return false;

    if(event.event == CC_BUTTONS_NEW_IDX) {
        app->edit_button_idx = app->button_count;
        app->is_new_button = true;
        memset(&app->edit_button_staging, 0, sizeof(CcButtonDef));
        app->edit_button_staging.type = CcButtonTypeDragAndRelease;
    } else {
        app->edit_button_idx = (uint8_t)event.event;
        app->is_new_button = false;
        app->edit_button_staging = app->buttons[app->edit_button_idx];
    }

    scene_manager_next_scene(app->scene_manager, CheapClickerSceneButtonProps);
    return true;
}

void cc_scene_buttons_on_exit(void* context) {
    submenu_reset(((CheapClickerApp*)context)->submenu);
}
