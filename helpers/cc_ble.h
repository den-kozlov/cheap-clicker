#pragma once
#include "../cheap_clicker_i.h"

// Calibration speed points and reference line parameters
#define CC_ACCEL_CAL_STEP_0  5    // step size for slow cal point
#define CC_ACCEL_CAL_DELAY_0 10   // delay (ms) for slow cal point
#define CC_ACCEL_CAL_STEP_1  20   // step size for fast cal point
#define CC_ACCEL_CAL_DELAY_1 10   // delay (ms) for fast cal point
#define CC_ACCEL_N_REF       80   // reference line length in steps (step=1)
#define CC_ACCEL_REF_STEP    1
#define CC_ACCEL_REF_DELAY   20   // ms per reference step
#define CC_ACCEL_Y_SPACING   60   // vertical gap between ref and test lines (px)

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
void cc_ble_draw_accel_lines(CheapClickerApp* app, uint8_t cal_point, uint8_t m_test);
void cc_ble_get_pos(CheapClickerApp* app, int16_t* x, int16_t* y);
bool cc_ble_is_connected(CheapClickerApp* app);
