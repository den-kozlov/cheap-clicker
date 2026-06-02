#pragma once
#include "../cheap_clicker_i.h"

typedef enum {
    CcScriptStateIdle,
    CcScriptStateRunning,
    CcScriptStatePaused,
    CcScriptStateDone,
    CcScriptStateError,
} CcScriptState;

typedef struct {
    CcScriptState state;
    size_t line_cur;
    char current_cmd[64];
    char error_msg[64];
} CcScriptStatus;

typedef struct CcScript CcScript;

CcScript* cc_script_alloc(CheapClickerApp* app);
void cc_script_free(CcScript* script);
bool cc_script_open(CcScript* script, const char* path);
void cc_script_run(CcScript* script);
void cc_script_stop(CcScript* script);
void cc_script_pause(CcScript* script);
void cc_script_resume(CcScript* script);
void cc_script_get_status(CcScript* script, CcScriptStatus* out);
