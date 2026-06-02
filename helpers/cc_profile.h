#pragma once
#include "../cheap_clicker_i.h"

void cc_profile_load_all(CheapClickerApp* app);
void cc_profile_save(CheapClickerApp* app, uint8_t idx);
uint8_t cc_profile_add(CheapClickerApp* app, const char* name, const char* ble_name);
void cc_profile_delete(CheapClickerApp* app, uint8_t idx);
void cc_profile_save_active(CheapClickerApp* app);
void cc_profile_load_active(CheapClickerApp* app);
