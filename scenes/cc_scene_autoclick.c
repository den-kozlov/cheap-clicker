#include "../cheap_clicker_i.h"
#include "../views/cc_autoclick_view.h"
#include "../helpers/cc_autoclick.h"
#include "../helpers/cc_ble.h"
#include <string.h>

static void cc_autoclick_view_cb(void* ctx, CcAutoclickViewEvent event) {
    view_dispatcher_send_custom_event(
        ((CheapClickerApp*)ctx)->view_dispatcher, (uint32_t)event);
}

static void cc_autoclick_refresh(CheapClickerApp* app) {
    static char name_buf[CC_MAX_BUTTONS][CC_MAX_NAME_LEN];
    for(uint8_t i = 0; i < app->button_count; i++) {
        strlcpy(name_buf[i], app->buttons[i].name, CC_MAX_NAME_LEN);
    }
    cc_autoclick_view_update(
        app->autoclick_view,
        (const char (*)[32])name_buf,
        app->button_count,
        cc_ble_is_connected(app));
}

void cc_scene_autoclick_on_enter(void* context) {
    CheapClickerApp* app = context;
    cc_autoclick_view_set_callback(app->autoclick_view, cc_autoclick_view_cb, app);
    cc_autoclick_refresh(app);
    cc_autoclick_view_set_running(app->autoclick_view, cc_autoclick_is_running(app->autoclick));
    view_dispatcher_switch_to_view(app->view_dispatcher, CheapClickerViewAutoclick);
}

bool cc_scene_autoclick_on_event(void* context, SceneManagerEvent event) {
    CheapClickerApp* app = context;

    if(event.type == SceneManagerEventTypeBack) {
        if(cc_autoclick_is_running(app->autoclick)) {
            cc_autoclick_stop(app->autoclick);
            cc_autoclick_view_set_running(app->autoclick_view, false);
        }
        return false;
    }

    if(event.type != SceneManagerEventTypeCustom) return false;

    switch(event.event) {
    case CcAutoclickViewEventStart: {
        uint8_t btn = cc_autoclick_view_get_selected_btn(app->autoclick_view);
        if(btn != CC_BUTTON_IDX_NONE && cc_ble_is_connected(app)) {
            cc_autoclick_start(app->autoclick, app, btn);
            cc_autoclick_view_set_running(app->autoclick_view, true);
        }
        return true;
    }
    case CcAutoclickViewEventStop:
        cc_autoclick_stop(app->autoclick);
        cc_autoclick_view_set_running(app->autoclick_view, false);
        return true;

    case CheapClickerCustomEventBleConnected:
        cc_autoclick_refresh(app);
        return true;

    case CheapClickerCustomEventBleDisconnected:
        if(cc_autoclick_is_running(app->autoclick)) {
            cc_autoclick_stop(app->autoclick);
            cc_autoclick_view_set_running(app->autoclick_view, false);
        }
        cc_autoclick_refresh(app);
        return true;

    default:
        return false;
    }
}

void cc_scene_autoclick_on_exit(void* context) {
    CheapClickerApp* app = context;
    if(cc_autoclick_is_running(app->autoclick)) {
        cc_autoclick_stop(app->autoclick);
    }
}
