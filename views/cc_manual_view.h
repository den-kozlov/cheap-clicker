#pragma once

#include <gui/view.h>
#include <input/input.h>

typedef struct CcManualView CcManualView;

typedef enum {
    CcManualViewEventFire,        // short press  → fire immediately
    CcManualViewEventLongBegin,   // long threshold crossed, key still held
    CcManualViewEventLongRelease, // released after a long press
    CcManualViewEventConfigure,   // long press Back → open key config
} CcManualViewEvent;

typedef void (*CcManualViewCallback)(void* ctx, CcManualViewEvent event, InputKey key);

CcManualView* cc_manual_view_alloc(void);
void cc_manual_view_free(CcManualView* v);
View* cc_manual_view_get_view(CcManualView* v);
void cc_manual_view_set_callback(CcManualView* v, CcManualViewCallback cb, void* ctx);

// Update displayed labels, BLE status, and holding-row highlight.
// layout[i] is a button index or CC_BUTTON_IDX_NONE.
// holding_key: 0-4 = that row draws inverted; CC_BUTTON_IDX_NONE = none.
void cc_manual_view_update(
    CcManualView* v,
    const uint8_t* layout,
    const char button_names[][32],
    uint8_t button_count,
    bool ble_connected,
    uint8_t holding_key);
