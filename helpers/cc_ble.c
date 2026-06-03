#include "cc_ble.h"

#include <furi.h>
#include <furi_hal_usb_hid.h>
#include <bt/bt_service/bt.h>
#include <ble_profile/extra_profiles/hid_profile.h>
#include <gui/view_dispatcher.h>
#include <math.h>

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
    float step = 127.0f;
    for(int i = 0; i < 200; i++) {
        ble_profile_hid_mouse_move(app->ble_hid_profile, -(int16_t)step, 0);
        furi_delay_ms(2);
        step /= 1.05f;
    }
    step = 127.0f;
    for(int i = 0; i < 20; i++) {
        ble_profile_hid_mouse_move(app->ble_hid_profile, 20, 0);
        furi_delay_ms(5);
    }
    for(int i = 0; i < 100; i++) {
        ble_profile_hid_mouse_move(app->ble_hid_profile, 0, -(int16_t)step);
        furi_delay_ms(2);
        step /= 1.05f;
    }

    s_cur_x = 400;
    s_cur_y = 0;
}

// ---------------------------------------------------------------------------
// Two-phase move: bulk (calibrated speed) then fine (step=1, a≈1)
// ---------------------------------------------------------------------------

void cc_ble_move_to(CheapClickerApp* app, int16_t x, int16_t y) {
    furi_assert(app);
    if(!app->ble_hid_profile) return;

    const uint8_t BULK_STEP  = CC_ACCEL_CAL_STEP_1;
    const uint8_t BULK_DELAY = CC_ACCEL_CAL_DELAY_1;
    const uint8_t FINE_DELAY = 15;

    float a_bulk = app->accel[1];
    // effective real pixels per bulk step; switch to fine when remaining < this
    float eff_bulk = (float)BULK_STEP * a_bulk;

    int16_t dx = x - s_cur_x;
    int16_t dy = y - s_cur_y;

    // Bulk phase: diagonal, stop when remaining on each axis < eff_bulk
    while((dx < 0 ? -dx : dx) > (int16_t)eff_bulk || (dy < 0 ? -dy : dy) > (int16_t)eff_bulk) {
        int8_t sx = 0, sy = 0;
        if((dx < 0 ? -dx : dx) > (int16_t)eff_bulk)
            sx = dx > 0 ? (int8_t)BULK_STEP : -(int8_t)BULK_STEP;
        if((dy < 0 ? -dy : dy) > (int16_t)eff_bulk)
            sy = dy > 0 ? (int8_t)BULK_STEP : -(int8_t)BULK_STEP;
        if(sx == 0 && sy == 0) break;
        ble_profile_hid_mouse_move(app->ble_hid_profile, sx, sy);
        // Track real position (each step moves eff_bulk real pixels)
        if(sx != 0) s_cur_x += (int16_t)roundf((sx > 0 ? 1.0f : -1.0f) * eff_bulk);
        if(sy != 0) s_cur_y += (int16_t)roundf((sy > 0 ? 1.0f : -1.0f) * eff_bulk);
        dx = x - s_cur_x;
        dy = y - s_cur_y;
        furi_delay_ms(BULK_DELAY);
    }

    // Fine phase: step=1, a≈1 — pixel-perfect landing
    dx = x - s_cur_x;
    dy = y - s_cur_y;
    while(dx != 0 || dy != 0) {
        int8_t sx = dx > 0 ? 1 : (dx < 0 ? -1 : 0);
        int8_t sy = dy > 0 ? 1 : (dy < 0 ? -1 : 0);
        ble_profile_hid_mouse_move(app->ble_hid_profile, sx, sy);
        s_cur_x += sx;
        s_cur_y += sy;
        dx = x - s_cur_x;
        dy = y - s_cur_y;
        furi_delay_ms(FINE_DELAY);
    }
    furi_delay_ms(10);
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
    FuriHalBleProfileBase* profile = app->ble_hid_profile;

    cc_ble_move_to(app, trigger_x, trigger_y);
    ble_profile_hid_mouse_press(profile, HID_MOUSE_BTN_LEFT);
    furi_delay_ms(panel_delay_ms);
    cc_ble_move_to(app, btn_x, btn_y);
    furi_delay_ms(80);
    ble_profile_hid_mouse_release(profile, HID_MOUSE_BTN_LEFT);
    furi_delay_ms(50);
}

void cc_ble_click_at(CheapClickerApp* app, int16_t x, int16_t y) {
    furi_assert(app);

    if(!app->ble_hid_profile) return;
    FuriHalBleProfileBase* profile = app->ble_hid_profile;

    cc_ble_move_to(app, x, y);
    furi_delay_ms(5);
    ble_profile_hid_mouse_press(profile, HID_MOUSE_BTN_LEFT);
    furi_delay_ms(5);
    ble_profile_hid_mouse_release(profile, HID_MOUSE_BTN_LEFT);
    furi_delay_ms(5);
}

void cc_ble_move_by(CheapClickerApp* app, int8_t dx, int8_t dy) {
    furi_assert(app);
    if(!app->ble_hid_profile) return;
    ble_profile_hid_mouse_move(app->ble_hid_profile, dx, dy);
    furi_delay_ms(20);
    s_cur_x += dx;
    s_cur_y += dy;
    if(s_cur_x < 0) s_cur_x = 0;
    if(s_cur_y < 0) s_cur_y = 0;
}

void cc_ble_get_pos(CheapClickerApp* app, int16_t* x, int16_t* y) {
    UNUSED(app);
    *x = s_cur_x;
    *y = s_cur_y;
}

bool cc_ble_is_connected(CheapClickerApp* app) {
    UNUSED(app);
    return s_connected;
}

// ---------------------------------------------------------------------------
// Accel calibration: draw reference + test line, symmetric returns
// ---------------------------------------------------------------------------

// Draw n steps right at (step, delay_ms) with left button held, then return left at same speed
static void draw_and_return(
    CheapClickerApp* app,
    uint8_t step,
    uint8_t delay_ms,
    uint8_t n) {
    ble_profile_hid_mouse_press(app->ble_hid_profile, HID_MOUSE_BTN_LEFT);
    furi_delay_ms(30);
    for(uint8_t i = 0; i < n; i++) {
        ble_profile_hid_mouse_move(app->ble_hid_profile, (int8_t)step, 0);
        s_cur_x += step;
        furi_delay_ms(delay_ms);
    }
    ble_profile_hid_mouse_release(app->ble_hid_profile, HID_MOUSE_BTN_LEFT);
    furi_delay_ms(50);
    // Symmetric return: same speed, same step count — OS accel cancels out
    for(uint8_t i = 0; i < n; i++) {
        ble_profile_hid_mouse_move(app->ble_hid_profile, -(int8_t)step, 0);
        s_cur_x -= step;
        furi_delay_ms(delay_ms);
    }
    furi_delay_ms(30);
}

void cc_ble_draw_accel_lines(CheapClickerApp* app, uint8_t cal_point, uint8_t m_test) {
    furi_assert(app);
    if(!app->ble_hid_profile) return;

    static const uint8_t cal_step[]  = {CC_ACCEL_CAL_STEP_0,  CC_ACCEL_CAL_STEP_1};
    static const uint8_t cal_delay[] = {CC_ACCEL_CAL_DELAY_0, CC_ACCEL_CAL_DELAY_1};

    // Reference line at current position
    draw_and_return(app, CC_ACCEL_REF_STEP, CC_ACCEL_REF_DELAY, CC_ACCEL_N_REF);
    furi_delay_ms(100);

    // Move down to test line start using slow step=1 (a≈1, no error)
    int16_t y_test = s_cur_y + CC_ACCEL_Y_SPACING;
    while(s_cur_y != y_test) {
        int8_t sy = s_cur_y < y_test ? 1 : -1;
        ble_profile_hid_mouse_move(app->ble_hid_profile, 0, sy);
        s_cur_y += sy;
        furi_delay_ms(CC_ACCEL_REF_DELAY);
    }
    furi_delay_ms(50);

    // Test line
    draw_and_return(app, cal_step[cal_point], cal_delay[cal_point], m_test);
}
