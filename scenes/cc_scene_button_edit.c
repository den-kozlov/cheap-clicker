#include "../cheap_clicker_i.h"
#include "../helpers/cc_profile.h"

static void cc_button_edit_done(void* context) {
    view_dispatcher_send_custom_event(((CheapClickerApp*)context)->view_dispatcher, 0);
}

void cc_scene_button_edit_on_enter(void* context) {
    CheapClickerApp* app = context;
    text_input_reset(app->text_input);
    text_input_set_header_text(app->text_input, "Button name:");
    text_input_set_result_callback(
        app->text_input, cc_button_edit_done, app,
        app->text_input_buf, CC_MAX_NAME_LEN, false);
    view_dispatcher_switch_to_view(app->view_dispatcher, CheapClickerViewTextInput);
}

bool cc_scene_button_edit_on_event(void* context, SceneManagerEvent event) {
    CheapClickerApp* app = context;
    if(event.type != SceneManagerEventTypeCustom || event.event != 0) return false;
    CcProfile* p = &app->profiles[app->active_profile_idx];
    uint32_t prev = scene_manager_get_scene_state(app->scene_manager, CheapClickerSceneProfileEdit);
    if(prev == 0xFF) {
        strlcpy(p->buttons[app->edit_button_idx].name, app->text_input_buf, CC_MAX_NAME_LEN);
    } else if(prev == 0) {
        // CcProfEditName = 0: rename profile name
        strlcpy(p->name, app->text_input_buf, CC_MAX_NAME_LEN);
    } else if(prev == 1) {
        // CcProfEditBleName = 1: rename BLE name
        strlcpy(p->ble_name, app->text_input_buf, CC_MAX_BLE_NAME_LEN);
    }
    cc_profile_save(app, app->active_profile_idx);
    scene_manager_previous_scene(app->scene_manager);
    return true;
}

void cc_scene_button_edit_on_exit(void* context) {
    UNUSED(context);
}
