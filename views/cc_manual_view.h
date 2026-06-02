#pragma once

#include <gui/view.h>
#include <input/input.h>

typedef struct CcManualView CcManualView;

typedef enum {
    CcManualViewEventFire,     // short press; key passed as argument to callback
    CcManualViewEventReassign, // long press;  key passed as argument to callback
} CcManualViewEvent;

typedef void (*CcManualViewCallback)(void* ctx, CcManualViewEvent event, InputKey key);

CcManualView* cc_manual_view_alloc(void);
void cc_manual_view_free(CcManualView* v);
View* cc_manual_view_get_view(CcManualView* v);
void cc_manual_view_set_callback(CcManualView* v, CcManualViewCallback cb, void* ctx);

// Update the displayed button names for all 5 key slots.
// layout[i] is a button index (0-based into buttons[]) or CC_BUTTON_IDX_NONE.
void cc_manual_view_update(
    CcManualView* v,
    const uint8_t* layout,
    const char button_names[][32],
    uint8_t button_count,
    bool ble_connected);
