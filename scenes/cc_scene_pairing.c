#include "../cheap_clicker_i.h"
#include "../helpers/cc_profile.h"
#include "../helpers/cc_ble.h"

void cc_scene_pairing_on_enter(void* context) {
    CheapClickerApp* app = context;

    popup_reset(app->popup);
    popup_set_header(app->popup, "Pairing...", 64, 10, AlignCenter, AlignCenter);
    popup_set_text(
        app->popup,
        "Open Bluetooth on iPhone\nand connect to CheapClip\n\nBack = cancel",
        64, 38, AlignCenter, AlignCenter);

    view_dispatcher_switch_to_view(app->view_dispatcher, CheapClickerViewPopup);
}

bool cc_scene_pairing_on_event(void* context, SceneManagerEvent event) {
    CheapClickerApp* app = context;
    if(event.type != SceneManagerEventTypeCustom) return false;

    if(event.event == CheapClickerCustomEventBleConnected) {
        cc_profile_save(app, app->active_profile_idx);
        cc_profile_save_active(app);
        app->new_profile_pending = false;
        // Return to main screen — device is now active
        scene_manager_search_and_switch_to_previous_scene(
            app->scene_manager, CheapClickerSceneStart);
        return true;
    }
    return false;
}

void cc_scene_pairing_on_exit(void* context) {
    CheapClickerApp* app = context;
    if(app->new_profile_pending) {
        // Back pressed without pairing — discard the pending device
        cc_ble_stop(app);
        if(app->profile_count > 0) {
            app->profile_count--;
            memset(&app->profiles[app->profile_count], 0, sizeof(CcProfile));
        }
        if(app->active_profile_idx >= app->profile_count)
            app->active_profile_idx = app->profile_count > 0 ? app->profile_count - 1 : 0;
        app->new_profile_pending = false;
    }
}
