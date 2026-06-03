#pragma once

#include <gui/view.h>
#include "../helpers/cc_script.h"

typedef struct CcRunView CcRunView;

typedef enum {
    CcRunViewEventStart,
    CcRunViewEventStop,
    CcRunViewEventPause,
    CcRunViewEventResume,
} CcRunViewEvent;

typedef void (*CcRunViewButtonCallback)(void* context, CcRunViewEvent event);

CcRunView* cc_run_view_alloc(void);
void cc_run_view_free(CcRunView* v);
View* cc_run_view_get_view(CcRunView* v);

void cc_run_view_set_button_callback(CcRunView* v, CcRunViewButtonCallback cb, void* context);

// Update the displayed status snapshot (call from scene on ScriptUpdate events)
void cc_run_view_update(CcRunView* v, const CcScriptStatus* status, const char* script_name);
