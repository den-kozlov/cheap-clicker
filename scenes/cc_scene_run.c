#include "../cheap_clicker_i.h"
#include "../helpers/cc_ble.h"
#include "../helpers/cc_script.h"
#include "../views/cc_run_view.h"

static const char* cc_basename(const char* path) {
    const char* last = path;
    for(const char* p = path; *p; p++)
        if(*p == '/') last = p + 1;
    return last;
}

static void cc_run_btn_cb(void* context, CcRunViewEvent event) {
    CheapClickerApp* app = context;
    if(event == CcRunViewEventStop) {
        cc_script_stop(app->script);
        scene_manager_previous_scene(app->scene_manager);
    } else if(event == CcRunViewEventPause) {
        cc_script_pause(app->script);
    } else if(event == CcRunViewEventResume) {
        cc_script_resume(app->script);
    }
}

void cc_scene_run_on_enter(void* context) {
    CheapClickerApp* app = context;
    cc_run_view_set_button_callback(app->run_view, cc_run_btn_cb, app);

    const char* path = furi_string_get_cstr(app->script_path);
    if(!cc_script_open(app->script, path)) {
        scene_manager_previous_scene(app->scene_manager);
        return;
    }

    CcScriptStatus status;
    cc_script_get_status(app->script, &status);
    cc_run_view_update(app->run_view, &status, cc_basename(path));

    view_dispatcher_switch_to_view(app->view_dispatcher, CheapClickerViewRun);
    cc_script_run(app->script);
}

bool cc_scene_run_on_event(void* context, SceneManagerEvent event) {
    CheapClickerApp* app = context;
    if(event.type != SceneManagerEventTypeCustom) return false;
    if(event.event == CheapClickerCustomEventScriptUpdate ||
       event.event == CheapClickerCustomEventScriptDone ||
       event.event == CheapClickerCustomEventScriptError) {
        CcScriptStatus status;
        cc_script_get_status(app->script, &status);
        cc_run_view_update(app->run_view, &status,
                           cc_basename(furi_string_get_cstr(app->script_path)));
        return true;
    }
    return false;
}

void cc_scene_run_on_exit(void* context) {
    cc_script_stop(((CheapClickerApp*)context)->script);
}
