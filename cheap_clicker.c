#include "cheap_clicker_i.h"
#include "views/cc_calibrate_view.h"
#include "views/cc_run_view.h"
#include "views/cc_start_view.h"
#include "helpers/cc_profile.h"
#include "helpers/cc_ble.h"
#include "helpers/cc_script.h"
#include "helpers/cc_manual.h"
#include "helpers/cc_autoclick.h"
#include "views/cc_manual_view.h"
#include "views/cc_autoclick_view.h"

static bool cc_signal_handler(uint32_t signal, void* arg, void* context) {
    UNUSED(arg);
    CheapClickerApp* app = context;
    if(signal == FuriSignalExit) {
        view_dispatcher_stop(app->view_dispatcher);
        return true;
    }
    return false;
}

static bool cc_custom_event_callback(void* context, uint32_t event) {
    furi_assert(context);
    CheapClickerApp* app = context;
    return scene_manager_handle_custom_event(app->scene_manager, event);
}

static bool cc_back_event_callback(void* context) {
    furi_assert(context);
    CheapClickerApp* app = context;
    return scene_manager_handle_back_event(app->scene_manager);
}

CheapClickerApp* cheap_clicker_alloc(void) {
    CheapClickerApp* app = malloc(sizeof(CheapClickerApp));
    furi_check(app != NULL);
    memset(app, 0, sizeof(CheapClickerApp));

    app->gui = furi_record_open(RECORD_GUI);
    app->bt = furi_record_open(RECORD_BT);
    app->notifications = furi_record_open(RECORD_NOTIFICATION);
    app->ble_hid_profile = NULL;

    app->view_dispatcher = view_dispatcher_alloc();
    view_dispatcher_set_event_callback_context(app->view_dispatcher, app);
    view_dispatcher_set_custom_event_callback(app->view_dispatcher, cc_custom_event_callback);
    view_dispatcher_set_navigation_event_callback(app->view_dispatcher, cc_back_event_callback);
    view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);

    app->scene_manager = scene_manager_alloc(&cc_scene_handlers, app);

    app->submenu = submenu_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, CheapClickerViewSubmenu, submenu_get_view(app->submenu));

    app->text_input = text_input_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, CheapClickerViewTextInput, text_input_get_view(app->text_input));

    app->var_item_list = variable_item_list_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher,
        CheapClickerViewVariableList,
        variable_item_list_get_view(app->var_item_list));

    app->popup = popup_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, CheapClickerViewPopup, popup_get_view(app->popup));

    app->calibrate_view = cc_calibrate_view_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher,
        CheapClickerViewCalibrate,
        cc_calibrate_view_get_view(app->calibrate_view));

    app->run_view = cc_run_view_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher,
        CheapClickerViewRun,
        cc_run_view_get_view(app->run_view));

    app->start_view = cc_start_view_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher,
        CheapClickerViewStart,
        cc_start_view_get_view(app->start_view));

    app->manual_view = cc_manual_view_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher,
        CheapClickerViewManual,
        cc_manual_view_get_view(app->manual_view));

    app->autoclick_view = cc_autoclick_view_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher,
        CheapClickerViewAutoclick,
        cc_autoclick_view_get_view(app->autoclick_view));

    app->autoclick = cc_autoclick_alloc();

    app->script_path = furi_string_alloc();
    app->profile_count = 0;
    app->active_profile_idx = CC_PROFILE_IDX_NONE;
    app->manual_pending_key = CC_BUTTON_IDX_NONE;
    app->script = NULL;

    app->script = cc_script_alloc(app);

    return app;
}

void cheap_clicker_free(CheapClickerApp* app) {
    furi_assert(app);

    if(app->script) {
        cc_script_free(app->script);
        app->script = NULL;
    }

    view_dispatcher_remove_view(app->view_dispatcher, CheapClickerViewSubmenu);
    submenu_free(app->submenu);
    view_dispatcher_remove_view(app->view_dispatcher, CheapClickerViewTextInput);
    text_input_free(app->text_input);
    view_dispatcher_remove_view(app->view_dispatcher, CheapClickerViewVariableList);
    variable_item_list_free(app->var_item_list);
    view_dispatcher_remove_view(app->view_dispatcher, CheapClickerViewPopup);
    popup_free(app->popup);
    view_dispatcher_remove_view(app->view_dispatcher, CheapClickerViewCalibrate);
    cc_calibrate_view_free(app->calibrate_view);
    view_dispatcher_remove_view(app->view_dispatcher, CheapClickerViewRun);
    cc_run_view_free(app->run_view);
    view_dispatcher_remove_view(app->view_dispatcher, CheapClickerViewStart);
    cc_start_view_free(app->start_view);
    view_dispatcher_remove_view(app->view_dispatcher, CheapClickerViewManual);
    cc_manual_view_free(app->manual_view);

    if(app->autoclick) {
        cc_autoclick_free(app->autoclick);
        app->autoclick = NULL;
    }
    view_dispatcher_remove_view(app->view_dispatcher, CheapClickerViewAutoclick);
    cc_autoclick_view_free(app->autoclick_view);

    scene_manager_free(app->scene_manager);
    view_dispatcher_free(app->view_dispatcher);

    furi_string_free(app->script_path);

    furi_record_close(RECORD_GUI);
    furi_record_close(RECORD_BT);
    furi_record_close(RECORD_NOTIFICATION);

    free(app);
}

int32_t cheap_clicker_app(void* p) {
    UNUSED(p);
    CheapClickerApp* app = cheap_clicker_alloc();

    cc_buttons_load(app);
    cc_manual_layout_load(app);
    cc_profile_load_all(app);
    cc_profile_load_active(app);
    if(app->profile_count == 0) {
        app->active_profile_idx = CC_PROFILE_IDX_NONE;
    } else if(app->active_profile_idx >= app->profile_count) {
        app->active_profile_idx = 0;
    }

    if(app->profile_count > 0) {
        cc_ble_start(app);
    }

    furi_thread_set_signal_callback(furi_thread_get_current(), cc_signal_handler, app);

    scene_manager_next_scene(app->scene_manager, CheapClickerSceneStart);
    view_dispatcher_run(app->view_dispatcher);

    if(app->ble_hid_profile != NULL) {
        cc_ble_stop(app);
    }
    if(app->new_profile_pending && app->profile_count > 0) {
        app->profile_count--;
        app->new_profile_pending = false;
    }
    cc_profile_save_active(app);

    furi_thread_set_signal_callback(furi_thread_get_current(), NULL, NULL);
    cheap_clicker_free(app);
    return 0;
}
