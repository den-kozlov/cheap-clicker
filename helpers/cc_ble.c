#include "cc_ble.h"

#include <furi.h>
#include <furi_hal_usb_hid.h>
#include <bt/bt_service/bt.h>
#include <ble_profile/extra_profiles/hid_profile.h>
#include <gui/view_dispatcher.h>

// Module-level singleton state
static int16_t s_cur_x = 0;
static int16_t s_cur_y = 0;
static volatile bool s_connected = false;

// Internal BLE status callback
static void cc_ble_status_cb(BtStatus status, void* context) {
    CheapClickerApp* app = context;
    s_connected = (status == BtStatusConnected);
    uint32_t event = s_connected ? CheapClickerCustomEventBleConnected :
                                   CheapClickerCustomEventBleDisconnected;
    view_dispatcher_send_custom_event(app->view_dispatcher, event);
}

void cc_ble_start(CheapClickerApp* app) {
    furi_assert(app);
    furi_assert(app->active_profile_idx < app->profile_count);

    CcProfile* profile = &app->profiles[app->active_profile_idx];

    bt_disconnect(app->bt);
    furi_delay_ms(200);

    bt_keys_storage_set_storage_path(app->bt, profile->keys_path);

    // device_name_prefix must be < 8 chars — truncate ble_name to 7 chars
    char name_prefix[8];
    snprintf(name_prefix, sizeof(name_prefix), "%.7s", profile->ble_name);

    BleProfileHidParams params = {
        .device_name_prefix = name_prefix,
        .mac_xor = app->active_profile_idx,
    };

    app->ble_hid_profile = bt_profile_start(app->bt, ble_profile_hid, &params);
    if(!app->ble_hid_profile) {
        FURI_LOG_E("CcBle", "bt_profile_start failed");
        return;
    }

    furi_hal_bt_start_advertising();
    bt_set_status_changed_callback(app->bt, cc_ble_status_cb, app);

    s_cur_x = 0;
    s_cur_y = 0;
    s_connected = false;
}

void cc_ble_stop(CheapClickerApp* app) {
    furi_assert(app);

    bt_set_status_changed_callback(app->bt, NULL, NULL);

    if(app->ble_hid_profile != NULL) {
        bt_disconnect(app->bt);
        furi_delay_ms(200);
        bt_keys_storage_set_default_path(app->bt);
        bt_profile_restore_default(app->bt);
        app->ble_hid_profile = NULL;
    }

    s_connected = false;
}

void cc_ble_switch_profile(CheapClickerApp* app, uint8_t idx) {
    furi_assert(app);

    cc_ble_stop(app);
    furi_delay_ms(200);
    app->active_profile_idx = idx;
    cc_ble_start(app);
}

void cc_ble_reset_cursor(CheapClickerApp* app) {
    furi_assert(app);

    if(!app->ble_hid_profile) return;

    for(int i = 0; i < 32; i++) {
        ble_profile_hid_mouse_move(app->ble_hid_profile, -127, -127);
        furi_delay_ms(2);
    }

    s_cur_x = 0;
    s_cur_y = 0;
}

void cc_ble_move_to(CheapClickerApp* app, int16_t x, int16_t y) {
    furi_assert(app);

    if(!app->ble_hid_profile) return;

    int16_t dx = x - s_cur_x;
    int16_t dy = y - s_cur_y;

    while(dx != 0 || dy != 0) {
        int8_t step_x = dx > 127 ? 127 : dx < -127 ? -127 : (int8_t)dx;
        int8_t step_y = dy > 127 ? 127 : dy < -127 ? -127 : (int8_t)dy;
        ble_profile_hid_mouse_move(app->ble_hid_profile, step_x, step_y);
        s_cur_x += step_x;
        s_cur_y += step_y;
        dx = x - s_cur_x;
        dy = y - s_cur_y;
        furi_delay_ms(2);
    }
}

void cc_ble_press_button(
    CheapClickerApp* app,
    int16_t trigger_x,
    int16_t trigger_y,
    int16_t btn_x,
    int16_t btn_y,
    uint32_t panel_delay_ms) {
    furi_assert(app);

    if(!app->ble_hid_profile) return;
    FuriHalBleProfileBase* profile = app->ble_hid_profile;  // snapshot

    cc_ble_reset_cursor(app);
    cc_ble_move_to(app, trigger_x, trigger_y);
    ble_profile_hid_mouse_press(profile, HID_MOUSE_BTN_LEFT);
    furi_delay_ms(panel_delay_ms);
    cc_ble_move_to(app, btn_x, btn_y);
    furi_delay_ms(80);
    ble_profile_hid_mouse_release(profile, HID_MOUSE_BTN_LEFT);
    furi_delay_ms(50);
}

bool cc_ble_is_connected(CheapClickerApp* app) {
    UNUSED(app);
    return s_connected;
}
