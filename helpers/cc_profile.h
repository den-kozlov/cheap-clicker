#pragma once
#include "../cheap_clicker_i.h"

// Global button list (buttons.fds)
void cc_buttons_load(CheapClickerApp* app);
void cc_buttons_save(CheapClickerApp* app);
void cc_button_delete(CheapClickerApp* app, uint8_t btn_idx);

// Profile management
void cc_profile_load_all(CheapClickerApp* app);
void cc_profile_save(CheapClickerApp* app, uint8_t idx);
// save=true  → writes to SD immediately (existing profile flow)
// save=false → memory only; caller must call cc_profile_save() later (new-profile pairing flow)
uint8_t cc_profile_add(CheapClickerApp* app, const char* name, const char* ble_name, bool save);
void cc_profile_delete(CheapClickerApp* app, uint8_t idx);
void cc_profile_save_calib(CheapClickerApp* app, uint8_t idx);
void cc_profile_save_active(CheapClickerApp* app);
void cc_profile_load_active(CheapClickerApp* app);

// Monument Attack global settings (monument.fds — independent of profiles)
void cc_monument_settings_save(CheapClickerApp* app);
void cc_monument_settings_load(CheapClickerApp* app);
