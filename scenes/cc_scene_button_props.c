#include "../cheap_clicker_i.h"
#include "../helpers/cc_profile.h"

static const char* const s_type_names[] = {"Press", "DragAndRelease"};

typedef enum {
    CcBtnPropsName = 0,
    CcBtnPropsType,
    CcBtnPropsSave,
    CcBtnPropsDelete,
} CcBtnPropsIdx;

static void cc_btn_props_type_cb(VariableItem* item) {
    CheapClickerApp* app = variable_item_get_context(item);
    uint8_t idx = variable_item_get_current_value_index(item);
    app->edit_button_staging.type = (CcButtonType)idx;
    variable_item_set_current_value_text(item, s_type_names[idx]);
}

static void cc_btn_props_enter_cb(void* context, uint32_t index) {
    view_dispatcher_send_custom_event(((CheapClickerApp*)context)->view_dispatcher, index);
}

static void cc_btn_props_popup_cb(void* context) {
    view_dispatcher_send_custom_event(
        ((CheapClickerApp*)context)->view_dispatcher, CheapClickerCustomEventPopupDismissed);
}

static void cc_btn_props_show_error(CheapClickerApp* app, const char* msg) {
    popup_reset(app->popup);
    popup_set_header(app->popup, "Error", 64, 10, AlignCenter, AlignTop);
    popup_set_text(app->popup, msg, 64, 32, AlignCenter, AlignTop);
    popup_set_timeout(app->popup, 2000);
    popup_enable_timeout(app->popup);
    popup_set_callback(app->popup, cc_btn_props_popup_cb);
    popup_set_context(app->popup, app);
    view_dispatcher_switch_to_view(app->view_dispatcher, CheapClickerViewPopup);
}

static void cc_btn_props_rebuild(CheapClickerApp* app) {
    variable_item_list_reset(app->var_item_list);

    // Name
    VariableItem* name_item =
        variable_item_list_add(app->var_item_list, "Name", 1, NULL, NULL);
    variable_item_set_current_value_text(name_item, app->edit_button_staging.name);

    // Type
    VariableItem* type_item = variable_item_list_add(
        app->var_item_list,
        "Type",
        2,
        cc_btn_props_type_cb,
        app);
    uint8_t type_idx = (uint8_t)app->edit_button_staging.type;
    variable_item_set_current_value_index(type_item, type_idx);
    variable_item_set_current_value_text(type_item, s_type_names[type_idx]);

    // Save
    variable_item_list_add(app->var_item_list, "Save", 0, NULL, NULL);

    // Delete (only for existing buttons)
    if(!app->is_new_button) {
        variable_item_list_add(app->var_item_list, "Delete", 0, NULL, NULL);
    }

    variable_item_list_set_enter_callback(app->var_item_list, cc_btn_props_enter_cb, app);
}

void cc_scene_button_props_on_enter(void* context) {
    CheapClickerApp* app = context;
    cc_btn_props_rebuild(app);
    view_dispatcher_switch_to_view(app->view_dispatcher, CheapClickerViewVariableList);
}

bool cc_scene_button_props_on_event(void* context, SceneManagerEvent event) {
    CheapClickerApp* app = context;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == CheapClickerCustomEventPopupDismissed) {
            view_dispatcher_switch_to_view(app->view_dispatcher, CheapClickerViewVariableList);
            return true;
        }

        switch(event.event) {
        case CcBtnPropsName:
            strlcpy(app->text_input_buf, app->edit_button_staging.name, CC_MAX_NAME_LEN);
            app->button_edit_mode = 0xFF;
            scene_manager_next_scene(app->scene_manager, CheapClickerSceneButtonEdit);
            return true;

        case CcBtnPropsType:
            // Handled inline by the change callback; nothing extra needed
            return true;

        case CcBtnPropsSave: {
            const char* new_name = app->edit_button_staging.name;
            if(new_name[0] == '\0') {
                cc_btn_props_show_error(app, "Name is empty");
                return true;
            }
            // Uniqueness check
            for(uint8_t i = 0; i < app->button_count; i++) {
                if(!app->is_new_button && i == app->edit_button_idx) continue;
                if(strcmp(app->buttons[i].name, new_name) == 0) {
                    cc_btn_props_show_error(app, "Name already used");
                    return true;
                }
            }
            // Commit
            app->buttons[app->edit_button_idx] = app->edit_button_staging;
            if(app->is_new_button) app->button_count++;
            cc_buttons_save(app);
            scene_manager_previous_scene(app->scene_manager);
            return true;
        }

        case CcBtnPropsDelete:
            if(!app->is_new_button) {
                cc_button_delete(app, app->edit_button_idx);
            }
            scene_manager_previous_scene(app->scene_manager);
            return true;

        default:
            break;
        }
    }

    return false;
}

void cc_scene_button_props_on_exit(void* context) {
    variable_item_list_reset(((CheapClickerApp*)context)->var_item_list);
}
