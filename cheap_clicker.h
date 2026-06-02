#pragma once

#include <furi.h>

typedef struct CheapClickerApp CheapClickerApp;

CheapClickerApp* cheap_clicker_alloc(void);
void cheap_clicker_free(CheapClickerApp* app);
int32_t cheap_clicker_app(void* p);
