#pragma once

#include <gui/view.h>

typedef struct CcStartView CcStartView;

typedef enum {
    CcStartViewEventDeviceSelect,
    CcStartViewEventDeviceEdit,
    CcStartViewEventScripts,
} CcStartViewEvent;

typedef void (*CcStartViewCallback)(void* context, CcStartViewEvent event);

CcStartView* cc_start_view_alloc(void);
void cc_start_view_free(CcStartView* v);
View* cc_start_view_get_view(CcStartView* v);
void cc_start_view_set_callback(CcStartView* v, CcStartViewCallback cb, void* ctx);

// device_name=NULL → no device (<Connect New>). show_scripts only applies when device is active.
void cc_start_view_update(CcStartView* v, const char* device_name, bool show_scripts);

void cc_start_view_set_cursor(CcStartView* v, uint8_t cursor);
uint8_t cc_start_view_get_cursor(CcStartView* v);
