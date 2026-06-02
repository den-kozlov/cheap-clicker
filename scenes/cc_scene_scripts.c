#include "../cheap_clicker_i.h"
#include <dialogs/dialogs.h>

void cc_scene_scripts_on_enter(void* context) {
    CheapClickerApp* app = context;
    DialogsApp* dialogs = furi_record_open(RECORD_DIALOGS);
    DialogsFileBrowserOptions opts;
    dialog_file_browser_set_basic_options(&opts, ".txt", NULL);
    opts.base_path = APP_DATA_PATH("scripts");
    opts.hide_ext = false;
    bool selected = dialog_file_browser_show(
        dialogs, app->script_path, app->script_path, &opts);
    furi_record_close(RECORD_DIALOGS);
    if(selected)
        scene_manager_next_scene(app->scene_manager, CheapClickerSceneRun);
    else
        scene_manager_previous_scene(app->scene_manager);
}

bool cc_scene_scripts_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void cc_scene_scripts_on_exit(void* context) {
    UNUSED(context);
}
