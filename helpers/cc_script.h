#pragma once
#include "../cheap_clicker_i.h"

typedef enum {
    CcScriptStateIdle,
    CcScriptStateCompiling,
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

typedef enum {
    CcInstrPress,
    CcInstrDelay,
    CcInstrLoopStart,
    CcInstrLoopEnd,
} CcInstrType;

typedef struct {
    CcInstrType type;
    union {
        uint8_t  button_idx;  // Press: resolved index into app->buttons[]
        uint32_t delay_ms;    // Delay
        uint32_t loop_count;  // LoopStart: 0 = infinite
        // LoopEnd: no payload (runtime stack holds body_pc)
    };
} CcInstr;

#define CC_SCRIPT_MAX_INSTRS 512

typedef struct CcScript CcScript;

CcScript* cc_script_alloc(CheapClickerApp* app);
void cc_script_free(CcScript* script);
void cc_script_open(CcScript* script, const char* path);
void cc_script_run(CcScript* script);
void cc_script_stop(CcScript* script);
void cc_script_pause(CcScript* script);
void cc_script_resume(CcScript* script);
void cc_script_get_status(CcScript* script, CcScriptStatus* out);
