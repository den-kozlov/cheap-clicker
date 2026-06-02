#pragma once
#include "../cheap_clicker_i.h"

typedef struct CcAutoclick CcAutoclick;

CcAutoclick* cc_autoclick_alloc(void);
void         cc_autoclick_free(CcAutoclick* ac);
void         cc_autoclick_start(CcAutoclick* ac, CheapClickerApp* app, uint8_t button_idx);
void         cc_autoclick_stop(CcAutoclick* ac);
bool         cc_autoclick_is_running(CcAutoclick* ac);
