#include "../cheap_clicker_i.h"
#include "../helpers/cc_profile.h"
#include "../helpers/cc_ble.h"

typedef enum {
    CcProfEditName = 0,
    CcProfEditBleName,
    CcProfEditButtons,
    CcProfEditCalibrate,
    CcProfEditSetActive,
    CcProfEditDelete,
} CcProfEditIdx;

static void cc_prof_edit_cb(void* context, uint32_t index) {
    view_dispatcher_send_custom_event(((CheapClickerApp*)context)->view_dispatcher, index);
}

void cc_scene_profile_edit_on_enter(void* context) {
    CheapClickerApp* app = context;
    CcProfile* p = &app->profiles[app->active_profile_idx];
    submenu_reset(app->submenu);
    char buf[48];
    snprintf(buf, sizeof(buf), "Name: %s", p->name);
    submenu_add_item(app->submenu, buf, CcProfEditName, cc_prof_edit_cb, app);
    snprintf(buf, sizeof(buf), "BLE: %s", p->ble_name);
    submenu_add_item(app->submenu, buf, CcProfEditBleName, cc_prof_edit_cb, app);
    submenu_add_item(app->submenu, "Buttons", CcProfEditButtons, cc_prof_edit_cb, app);
    submenu_add_item(app->submenu, "Calibrate", CcProfEditCalibrate, cc_prof_edit_cb, app);
    submenu_add_item(app->submenu, "Set as Active", CcProfEditSetActive, cc_prof_edit_cb, app);
    submenu_add_item(app->submenu, "Delete Profile", CcProfEditDelete, cc_prof_edit_cb, app);
    submenu_set_selected_item(app->submenu,
        scene_manager_get_scene_state(app->scene_manager, CheapClickerSceneProfileEdit));
    view_dispatcher_switch_to_view(app->view_dispatcher, CheapClickerViewSubmenu);
}

bool cc_scene_profile_edit_on_event(void* context, SceneManagerEvent event) {
    CheapClickerApp* app = context;
    if(event.type != SceneManagerEventTypeCustom) return false;
    CcProfile* p = &app->profiles[app->active_profile_idx];
    scene_manager_set_scene_state(app->scene_manager, CheapClickerSceneProfileEdit, event.event);
    switch((CcProfEditIdx)event.event) {
    case CcProfEditName:
        strlcpy(app->text_input_buf, p->name, CC_MAX_NAME_LEN);
        scene_manager_next_scene(app->scene_manager, CheapClickerSceneButtonEdit);
        break;
    case CcProfEditBleName:
        strlcpy(app->text_input_buf, p->ble_name, CC_MAX_BLE_NAME_LEN);
        scene_manager_next_scene(app->scene_manager, CheapClickerSceneButtonEdit);
        break;
    case CcProfEditButtons:
        scene_manager_next_scene(app->scene_manager, CheapClickerSceneButtons);
        break;
    case CcProfEditCalibrate:
        scene_manager_next_scene(app->scene_manager, CheapClickerSceneCalibrate);
        break;
    case CcProfEditSetActive:
        cc_profile_save_active(app);
        if(app->ble_hid_profile) cc_ble_stop(app);
        cc_ble_start(app);
        break;
    case CcProfEditDelete:
        cc_profile_delete(app, app->active_profile_idx);
        scene_manager_previous_scene(app->scene_manager);
        break;
    }
    return true;
}

void cc_scene_profile_edit_on_exit(void* context) {
    submenu_reset(((CheapClickerApp*)context)->submenu);
}
