#include "../cheap_clicker_i.h"
#include "../views/cc_manual_view.h"
#include "../helpers/cc_ble.h"
#include "../helpers/cc_manual.h"
#include <string.h>

// Custom event encoding (packed into uint32_t for view_dispatcher):
//   0x000..0x0FF  — submenu button selection (0=None, 1..N=button idx+1)
//   0x100..0x1FF  — Fire (short press), low byte = InputKey
//   0x300..0x3FF  — LongBegin, low byte = InputKey
//   0x400..0x4FF  — LongRelease, low byte = InputKey
//   0x500         — Configure (long Back)
//   0x600..0x604  — Key selected from phase-1 config submenu, low byte = InputKey 0-4
#define CC_MANUAL_EVT_FIRE(key)         (0x100u | (uint32_t)(key))
#define CC_MANUAL_EVT_LONG_BEGIN(key)   (0x300u | (uint32_t)(key))
#define CC_MANUAL_EVT_LONG_RELEASE(key) (0x400u | (uint32_t)(key))
#define CC_MANUAL_EVT_CONFIGURE         (0x500u)
#define CC_MANUAL_EVT_KEY_SELECT(key)   (0x600u | (uint32_t)(key))

static void cc_manual_view_cb(void* ctx, CcManualViewEvent event, InputKey key) {
    CheapClickerApp* app = ctx;
    uint32_t ev;
    switch(event) {
    case CcManualViewEventFire:
        ev = CC_MANUAL_EVT_FIRE(key);
        break;
    case CcManualViewEventLongBegin:
        ev = CC_MANUAL_EVT_LONG_BEGIN(key);
        break;
    case CcManualViewEventLongRelease:
        ev = CC_MANUAL_EVT_LONG_RELEASE(key);
        break;
    case CcManualViewEventConfigure:
        ev = CC_MANUAL_EVT_CONFIGURE;
        break;
    default:
        return;
    }
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
        cc_ble_is_connected(app),
        app->manual_holding_key);
}

void cc_scene_manual_on_enter(void* context) {
    CheapClickerApp* app = context;
    app->manual_pending_key = CC_BUTTON_IDX_NONE;
    app->manual_holding_key = CC_BUTTON_IDX_NONE;
    cc_ble_reset_cursor(app);
    cc_manual_view_set_callback(app->manual_view, cc_manual_view_cb, app);
    cc_manual_refresh(app);
    view_dispatcher_switch_to_view(app->view_dispatcher, CheapClickerViewManual);
}

bool cc_scene_manual_on_event(void* context, SceneManagerEvent event) {
    CheapClickerApp* app = context;

    // Back while any submenu phase is active: return to manual view without saving
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

    // Fire: short press — immediate action
    if(ev >= 0x100u && ev < 0x200u) {
        InputKey key = (InputKey)(ev & 0xFFu);
        uint8_t btn_idx = app->manual_layout[(uint8_t)key];
        if(btn_idx != CC_BUTTON_IDX_NONE && btn_idx < app->button_count &&
           app->active_profile_idx != CC_PROFILE_IDX_NONE) {
            CcProfile* p = &app->profiles[app->active_profile_idx];
            if(app->buttons[btn_idx].type == CcButtonTypePress) {
                cc_ble_click_at(app, p->calib[btn_idx].x, p->calib[btn_idx].y);
            } else {
                cc_ble_press_button(
                    app, p->trigger_x, p->trigger_y,
                    p->calib[btn_idx].x, p->calib[btn_idx].y, 100);
            }
        }
        return true;
    }

    // LongBegin: position cursor; for DragAndRelease also hold mouse
    if(ev >= 0x300u && ev < 0x400u) {
        InputKey key = (InputKey)(ev & 0xFFu);
        uint8_t btn_idx = app->manual_layout[(uint8_t)key];
        if(btn_idx != CC_BUTTON_IDX_NONE && btn_idx < app->button_count &&
           app->active_profile_idx != CC_PROFILE_IDX_NONE) {
            app->manual_holding_key = (uint8_t)key;
            CcProfile* p = &app->profiles[app->active_profile_idx];
            if(app->buttons[btn_idx].type == CcButtonTypeDragAndRelease) {
                cc_ble_drag_begin(
                    app, p->trigger_x, p->trigger_y,
                    p->calib[btn_idx].x, p->calib[btn_idx].y, 100);
            } else {
                cc_ble_move_to(app, p->calib[btn_idx].x, p->calib[btn_idx].y);
            }
            cc_manual_refresh(app);
        }
        return true;
    }

    // LongRelease: complete the deferred action
    if(ev >= 0x400u && ev < 0x500u) {
        InputKey key = (InputKey)(ev & 0xFFu);
        if(app->manual_holding_key == (uint8_t)key) {
            uint8_t btn_idx = app->manual_layout[(uint8_t)key];
            if(btn_idx != CC_BUTTON_IDX_NONE && btn_idx < app->button_count) {
                if(app->buttons[btn_idx].type == CcButtonTypeDragAndRelease) {
                    cc_ble_mouse_release(app);
                } else {
                    cc_ble_click_now(app);
                }
            }
            app->manual_holding_key = CC_BUTTON_IDX_NONE;
            cc_manual_refresh(app);
        }
        return true;
    }

    // Configure: long Back — show phase-1 key-selection submenu
    if(ev == 0x500u) {
        static char key_labels[5][64];
        static const char* const key_names[5] = {"Up", "Down", "Left", "Right", "OK"};
        app->manual_pending_key = 0xFE; // sentinel: in key-selection phase
        submenu_reset(app->submenu);
        for(uint8_t i = 0; i < 5; i++) {
            uint8_t btn = app->manual_layout[i];
            if(btn == CC_BUTTON_IDX_NONE || btn >= app->button_count) {
                snprintf(key_labels[i], sizeof(key_labels[i]), "%s: -", key_names[i]);
            } else {
                snprintf(
                    key_labels[i], sizeof(key_labels[i]),
                    "%s: %s", key_names[i], app->buttons[btn].name);
            }
            submenu_add_item(
                app->submenu, key_labels[i], CC_MANUAL_EVT_KEY_SELECT(i),
                cc_manual_submenu_cb, app);
        }
        view_dispatcher_switch_to_view(app->view_dispatcher, CheapClickerViewSubmenu);
        return true;
    }

    // Key selected from phase-1 submenu (0x600..0x604) — show phase-2 button submenu
    if(ev >= 0x600u && ev <= 0x604u) {
        app->manual_pending_key = (uint8_t)(ev - 0x600u);
        submenu_reset(app->submenu);
        submenu_add_item(app->submenu, "None", 0, cc_manual_submenu_cb, app);
        for(uint8_t i = 0; i < app->button_count; i++) {
            submenu_add_item(
                app->submenu, app->buttons[i].name, i + 1, cc_manual_submenu_cb, app);
        }
        uint8_t cur = app->manual_layout[app->manual_pending_key];
        uint32_t selected = (cur == CC_BUTTON_IDX_NONE) ? 0 : (uint32_t)cur + 1;
        submenu_set_selected_item(app->submenu, selected);
        view_dispatcher_switch_to_view(app->view_dispatcher, CheapClickerViewSubmenu);
        return true;
    }

    // Button picked in phase-2 submenu (ev < 0x100)
    if(ev >= 0x100u) return false;
    if(app->manual_pending_key == CC_BUTTON_IDX_NONE || app->manual_pending_key == 0xFE)
        return false;
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
    if(app->manual_holding_key != CC_BUTTON_IDX_NONE) {
        cc_ble_mouse_release(app);
        app->manual_holding_key = CC_BUTTON_IDX_NONE;
    }
    submenu_reset(app->submenu);
    app->manual_pending_key = CC_BUTTON_IDX_NONE;
}
