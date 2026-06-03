#pragma once
#include "../cheap_clicker_i.h"

typedef struct CcMonument CcMonument;

CcMonument* cc_monument_alloc(void);
void         cc_monument_free(CcMonument* ac);
void         cc_monument_start(CcMonument* ac, CheapClickerApp* app, uint8_t button_idx);
void         cc_monument_stop(CcMonument* ac);
bool         cc_monument_is_running(CcMonument* ac);
