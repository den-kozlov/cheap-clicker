#include "../cheap_clicker_i.h"
#include "../views/cc_monument_view.h"
#include "../helpers/cc_monument.h"
#include "../helpers/cc_ble.h"
#include <string.h>

static void cc_monument_view_cb(void* ctx, CcMonumentViewEvent event) {
    view_dispatcher_send_custom_event(
        ((CheapClickerApp*)ctx)->view_dispatcher, (uint32_t)event);
}

static void cc_monument_refresh(CheapClickerApp* app) {
    static char name_buf[CC_MAX_BUTTONS][CC_MAX_NAME_LEN];
    for(uint8_t i = 0; i < app->button_count; i++) {
        strlcpy(name_buf[i], app->buttons[i].name, CC_MAX_NAME_LEN);
    }
    cc_monument_view_update(
        app->monument_view,
        (const char (*)[32])name_buf,
        app->button_count,
        cc_ble_is_connected(app));
}

void cc_scene_monument_on_enter(void* context) {
    CheapClickerApp* app = context;
    cc_monument_view_set_callback(app->monument_view, cc_monument_view_cb, app);
    cc_monument_refresh(app);
    cc_monument_view_set_running(app->monument_view, cc_monument_is_running(app->monument));
    view_dispatcher_switch_to_view(app->view_dispatcher, CheapClickerViewMonument);
}

bool cc_scene_monument_on_event(void* context, SceneManagerEvent event) {
    CheapClickerApp* app = context;

    if(event.type == SceneManagerEventTypeBack) {
        if(cc_monument_is_running(app->monument)) {
            cc_monument_stop(app->monument);
            cc_monument_view_set_running(app->monument_view, false);
        }
        return false;
    }

    if(event.type != SceneManagerEventTypeCustom) return false;

    switch(event.event) {
    case CcMonumentViewEventStart: {
        uint8_t btn = cc_monument_view_get_selected_btn(app->monument_view);
        if(btn != CC_BUTTON_IDX_NONE && cc_ble_is_connected(app)) {
            cc_monument_start(app->monument, app, btn);
            cc_monument_view_set_running(app->monument_view, true);
        }
        return true;
    }
    case CcMonumentViewEventStop:
        cc_monument_stop(app->monument);
        cc_monument_view_set_running(app->monument_view, false);
        return true;

    case CheapClickerCustomEventBleConnected:
        cc_monument_refresh(app);
        return true;

    case CheapClickerCustomEventBleDisconnected:
        if(cc_monument_is_running(app->monument)) {
            cc_monument_stop(app->monument);
            cc_monument_view_set_running(app->monument_view, false);
        }
        cc_monument_refresh(app);
        return true;

    default:
        return false;
    }
}

void cc_scene_monument_on_exit(void* context) {
    CheapClickerApp* app = context;
    if(cc_monument_is_running(app->monument)) {
        cc_monument_stop(app->monument);
    }
}
