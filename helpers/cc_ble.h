#pragma once
#include "../cheap_clicker_i.h"

// Calibration speed points and reference line parameters
#define CC_ACCEL_CAL_STEP_0  10   // step size for medium cal point
#define CC_ACCEL_CAL_DELAY_0 2    // delay (ms) for medium cal point  → v=5 px/ms
#define CC_ACCEL_CAL_STEP_1  10   // step size for fast cal point
#define CC_ACCEL_CAL_DELAY_1 1    // delay (ms) for fast cal point   → v=10 px/ms
#define CC_ACCEL_N_REF       180  // reference line step count
#define CC_ACCEL_REF_STEP    5    // ref line step size (step=5 confirmed visible)
#define CC_ACCEL_REF_DELAY   15   // ms per reference step (slow enough: a≈1)
#define CC_ACCEL_Y_SPACING   20
// vertical gap between lines (px)
#define CC_ACCEL_Y_STEP      5    // step size for vertical repositioning moves
#define CC_ACCEL_Y_DELAY     20   // ms per vertical step

void cc_ble_start(CheapClickerApp* app);
void cc_ble_stop(CheapClickerApp* app);
void cc_ble_switch_profile(CheapClickerApp* app, uint8_t idx);
void cc_ble_move_to(CheapClickerApp* app, int16_t x, int16_t y);
void cc_ble_reset_cursor(CheapClickerApp* app);
void cc_ble_press_button(
    CheapClickerApp* app,
    int16_t trigger_x,
    int16_t trigger_y,
    int16_t btn_x,
    int16_t btn_y,
    uint32_t panel_delay_ms);
void cc_ble_click_at(CheapClickerApp* app, int16_t x, int16_t y);
void cc_ble_move_by(CheapClickerApp* app, int8_t dx, int8_t dy);
// draw_ref=true: draw reference line first (once per calibration session)
void cc_ble_draw_accel_lines(CheapClickerApp* app, uint8_t cal_point, uint8_t m_test, bool draw_ref);
void cc_ble_get_pos(CheapClickerApp* app, int16_t* x, int16_t* y);
bool cc_ble_is_connected(CheapClickerApp* app);
// Draw a single horizontal line of target_px real pixels at the given speed, then return.
// n = target_px / (step * a(v)); button is held during forward stroke only.
void cc_ble_draw_line(CheapClickerApp* app, uint8_t step, uint8_t delay_ms, uint16_t target_px);
// Draw 5 lines at different speeds, each corrected to the same target length via accel model.
void cc_ble_draw_verify_lines(CheapClickerApp* app);
