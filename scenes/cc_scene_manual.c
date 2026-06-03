#include "../cheap_clicker_i.h"
#include "../views/cc_manual_view.h"
#include "../helpers/cc_ble.h"
#include "../helpers/cc_manual.h"
#include <string.h>

// Custom event encoding (packed into uint32_t for view_dispatcher):
//   0x000..0x0FF  — submenu selection index (0 = "None", 1..N = button index+1)
//   0x100..0x1FF  — Fire event  (low byte = InputKey)
//   0x200..0x2FF  — Reassign event (low byte = InputKey)
#define CC_MANUAL_EVT_FIRE(key)     (0x100u | (uint32_t)(key))
#define CC_MANUAL_EVT_REASSIGN(key) (0x200u | (uint32_t)(key))

static void cc_manual_view_cb(void* ctx, CcManualViewEvent event, InputKey key) {
    CheapClickerApp* app = ctx;
    uint32_t ev = (event == CcManualViewEventFire)
        ? CC_MANUAL_EVT_FIRE(key)
        : CC_MANUAL_EVT_REASSIGN(key);
    view_dispatcher_send_custom_event(app->view_dispatcher, ev);
}

static void cc_manual_submenu_cb(void* ctx, uint32_t index) {
    view_dispatcher_send_custom_event(((CheapClickerApp*)ctx)->view_dispatcher, index);
}

static void cc_manual_refresh(CheapClickerApp* app) {
    // CcButtonDef has name + type fields, so we can't pass app->buttons directly as a
    // contiguous char[][32] array. Copy names into a flat buffer first.
    static char name_buf[CC_MAX_BUTTONS][32];
    for(uint8_t i = 0; i < app->button_count; i++) {
        strlcpy(name_buf[i], app->buttons[i].name, 32);
    }
    cc_manual_view_update(
        app->manual_view,
        app->manual_layout,
        (const char (*)[32])name_buf,
        app->button_count,
        cc_ble_is_connected(app));
}

void cc_scene_manual_on_enter(void* context) {
    CheapClickerApp* app = context;
    app->manual_pending_key = CC_BUTTON_IDX_NONE;
    cc_ble_reset_cursor(app);
    cc_manual_view_set_callback(app->manual_view, cc_manual_view_cb, app);
    cc_manual_refresh(app);
    view_dispatcher_switch_to_view(app->view_dispatcher, CheapClickerViewManual);
}

bool cc_scene_manual_on_event(void* context, SceneManagerEvent event) {
    CheapClickerApp* app = context;

    // Back while submenu is shown: return to manual view without saving
    if(event.type == SceneManagerEventTypeBack) {
        if(app->manual_pending_key != CC_BUTTON_IDX_NONE) {
            app->manual_pending_key = CC_BUTTON_IDX_NONE;
            submenu_reset(app->submenu);
            cc_manual_refresh(app);
            view_dispatcher_switch_to_view(app->view_dispatcher, CheapClickerViewManual);
            return true;
        }
        return false;
    }

    if(event.type != SceneManagerEventTypeCustom) return false;

    uint32_t ev = event.event;

    if(ev >= 0x100u && ev < 0x200u) {
        // Fire: press the mapped button via BLE
        InputKey key = (InputKey)(ev & 0xFFu);
        uint8_t btn_idx = app->manual_layout[(uint8_t)key];
        if(btn_idx != CC_BUTTON_IDX_NONE &&
           btn_idx < app->button_count &&
           app->active_profile_idx != CC_PROFILE_IDX_NONE) {
            CcProfile* p = &app->profiles[app->active_profile_idx];
            if(app->buttons[btn_idx].type == CcButtonTypePress) {
                cc_ble_click_at(app, p->calib[btn_idx].x, p->calib[btn_idx].y);
            } else {
                cc_ble_press_button(
                    app,
                    p->trigger_x, p->trigger_y,
                    p->calib[btn_idx].x, p->calib[btn_idx].y,
                    100);
            }
        }
        return true;
    }

    if(ev >= 0x200u && ev < 0x300u) {
        // Reassign: show submenu of available buttons
        app->manual_pending_key = (uint8_t)(ev & 0xFFu);
        submenu_reset(app->submenu);
        submenu_add_item(app->submenu, "None", 0, cc_manual_submenu_cb, app);
        for(uint8_t i = 0; i < app->button_count; i++) {
            submenu_add_item(app->submenu, app->buttons[i].name, i + 1, cc_manual_submenu_cb, app);
        }
        uint8_t cur = app->manual_layout[app->manual_pending_key];
        uint32_t selected = (cur == CC_BUTTON_IDX_NONE) ? 0 : (uint32_t)cur + 1;
        submenu_set_selected_item(app->submenu, selected);
        view_dispatcher_switch_to_view(app->view_dispatcher, CheapClickerViewSubmenu);
        return true;
    }

    // Submenu selection (ev < 0x100, i.e. 0..button_count)
    if(app->manual_pending_key == CC_BUTTON_IDX_NONE) return false;
    if(ev == 0) {
        app->manual_layout[app->manual_pending_key] = CC_BUTTON_IDX_NONE;
    } else if(ev <= app->button_count) {
        app->manual_layout[app->manual_pending_key] = (uint8_t)(ev - 1);
    }
    app->manual_pending_key = CC_BUTTON_IDX_NONE;
    cc_manual_layout_save(app);
    submenu_reset(app->submenu);
    cc_manual_refresh(app);
    view_dispatcher_switch_to_view(app->view_dispatcher, CheapClickerViewManual);
    return true;
}

void cc_scene_manual_on_exit(void* context) {
    CheapClickerApp* app = context;
    submenu_reset(app->submenu);
    app->manual_pending_key = CC_BUTTON_IDX_NONE;
}
