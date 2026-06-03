#pragma once
#include "../cheap_clicker_i.h"

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
void cc_ble_get_pos(CheapClickerApp* app, int16_t* x, int16_t* y);
bool cc_ble_is_connected(CheapClickerApp* app);
