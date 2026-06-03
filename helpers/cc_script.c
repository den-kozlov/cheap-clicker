#include "cc_script.h"
#include "cc_ble.h"
#include <storage/storage.h>
#include <string.h>
#include <stdlib.h>

#define TAG "CcScript"

#define CC_FILE_BUF_LEN 16
#define CC_EVT_COMPILE (1u << 0)
#define CC_EVT_RUN     (1u << 1)
#define CC_EVT_STOP    (1u << 2)
#define CC_EVT_PAUSE   (1u << 3)
#define CC_EVT_RESUME  (1u << 4)

typedef struct {
    uint16_t body_pc;    // index of first instruction inside the loop body
    uint32_t remaining;  // iterations left; UINT32_MAX = infinite
} CcRunLoopFrame;

struct CcScript {
    CheapClickerApp* app;
    FuriThread* thread;
    FuriMutex* mutex;
    volatile bool kill; // set only by cc_script_free to permanently stop the thread

    // File streaming — used only during compile pass
    Storage* storage;
    File* file;
    FuriString* file_path;
    uint8_t buf[CC_FILE_BUF_LEN + 1];
    uint8_t buf_start;
    uint8_t buf_len;
    bool file_end;
    FuriString* line;
    size_t line_num;

    // Compiled program
    CcInstr* instrs;
    uint16_t instr_count;

    // Execution state
    uint16_t pc;
    CcRunLoopFrame loop_stack[2];
    uint8_t loop_depth;

    CcScriptStatus status; // protected by mutex
};

// ---------------------------------------------------------------------------
// File streaming helpers
// ---------------------------------------------------------------------------

static bool cc_read_line(CcScript* s) {
    furi_string_reset(s->line);
    while(true) {
        if(s->buf_start >= s->buf_len) {
            if(s->file_end) return furi_string_size(s->line) > 0;
            s->buf_len = (uint8_t)storage_file_read(s->file, s->buf, CC_FILE_BUF_LEN);
            s->buf_start = 0;
            if(s->buf_len == 0) {
                s->file_end = true;
                return furi_string_size(s->line) > 0;
            }
        }
        uint8_t c = s->buf[s->buf_start++];
        if(c == '\n') {
            s->line_num++;
            return true;
        }
        if(c != '\r') furi_string_push_back(s->line, (char)c);
    }
}

// ---------------------------------------------------------------------------
// Compile pass (runs on worker thread)
// ---------------------------------------------------------------------------

static bool cc_script_do_compile(CcScript* s) {
    if(!storage_file_open(s->file, furi_string_get_cstr(s->file_path),
                          FSAM_READ, FSOM_OPEN_EXISTING)) {
        snprintf(s->status.error_msg, sizeof(s->status.error_msg), "Cannot open file");
        return false;
    }

    s->buf_start = 0;
    s->buf_len = 0;
    s->file_end = false;
    s->line_num = 0;

    CcInstr* tmp = malloc(CC_SCRIPT_MAX_INSTRS * sizeof(CcInstr));
    if(!tmp) {
        storage_file_close(s->file);
        snprintf(s->status.error_msg, sizeof(s->status.error_msg), "Out of memory");
        return false;
    }
    uint16_t count = 0;
    uint8_t pending_depth = 0;
    bool ok = true;

    while(cc_read_line(s)) {
        const char* line = furi_string_get_cstr(s->line);
        while(*line == ' ' || *line == '\t') line++;
        if(line[0] == '\0' || line[0] == '#') continue;

        if(count >= CC_SCRIPT_MAX_INSTRS) {
            snprintf(s->status.error_msg, sizeof(s->status.error_msg),
                "Script too long (max %d instructions)", CC_SCRIPT_MAX_INSTRS);
            ok = false;
            break;
        }

        if(strncmp(line, "PRESS ", 6) == 0) {
            const char* btn_name = line + 6;
            int found = -1;
            for(uint8_t i = 0; i < s->app->button_count; i++) {
                if(strcasecmp(s->app->buttons[i].name, btn_name) == 0) {
                    found = i;
                    break;
                }
            }
            if(found < 0) {
                snprintf(s->status.error_msg, sizeof(s->status.error_msg),
                    "Ln %zu: btn not found: %.20s", s->line_num, btn_name);
                ok = false;
                break;
            }
            tmp[count++] = (CcInstr){.type = CcInstrPress, .button_idx = (uint8_t)found};

        } else if(strncmp(line, "DELAY ", 6) == 0) {
            char* end;
            long ms = strtol(line + 6, &end, 10);
            if(end == line + 6 || ms < 0) {
                snprintf(s->status.error_msg, sizeof(s->status.error_msg),
                    "Line %zu: bad DELAY value", s->line_num);
                ok = false;
                break;
            }
            tmp[count++] = (CcInstr){.type = CcInstrDelay, .delay_ms = (uint32_t)ms};

        } else if(strncmp(line, "LOOP ", 5) == 0) {
            if(pending_depth >= 2) {
                snprintf(s->status.error_msg, sizeof(s->status.error_msg),
                    "Line %zu: LOOP nesting > 2", s->line_num);
                ok = false;
                break;
            }
            char* end;
            long n = strtol(line + 5, &end, 10);
            if(end == line + 5 || n < 0) {
                snprintf(s->status.error_msg, sizeof(s->status.error_msg),
                    "Line %zu: bad LOOP count", s->line_num);
                ok = false;
                break;
            }
            pending_depth++;
            tmp[count++] = (CcInstr){.type = CcInstrLoopStart, .loop_count = (uint32_t)n};

        } else if(strcmp(line, "END") == 0) {
            if(pending_depth == 0) {
                snprintf(s->status.error_msg, sizeof(s->status.error_msg),
                    "Line %zu: END without LOOP", s->line_num);
                ok = false;
                break;
            }
            pending_depth--;
            tmp[count++] = (CcInstr){.type = CcInstrLoopEnd};

        } else {
            snprintf(s->status.error_msg, sizeof(s->status.error_msg),
                "Ln %zu: unknown: %.20s", s->line_num, line);
            ok = false;
            break;
        }
    }

    storage_file_close(s->file);

    if(ok && pending_depth > 0) {
        snprintf(s->status.error_msg, sizeof(s->status.error_msg), "Unclosed LOOP");
        ok = false;
    }

    if(ok) {
        s->instrs = tmp;
        s->instr_count = count;
    } else {
        free(tmp);
    }

    return ok;
}

// ---------------------------------------------------------------------------
// Instruction executor
// ---------------------------------------------------------------------------

static uint32_t cc_execute_instr(CcScript* s) {
    CcInstr* instr = &s->instrs[s->pc];

    switch(instr->type) {
    case CcInstrPress: {
        CheapClickerApp* app = s->app;
        if(app->active_profile_idx == CC_PROFILE_IDX_NONE ||
           app->active_profile_idx >= app->profile_count) {
            furi_mutex_acquire(s->mutex, FuriWaitForever);
            snprintf(s->status.error_msg, sizeof(s->status.error_msg), "No active profile");
            furi_mutex_release(s->mutex);
            return UINT32_MAX;
        }
        CcProfile* profile = &app->profiles[app->active_profile_idx];
        uint8_t idx = instr->button_idx;
        CcButtonDef* btn = &app->buttons[idx];
        CcButtonCalib* calib = &profile->calib[idx];

        furi_mutex_acquire(s->mutex, FuriWaitForever);
        snprintf(s->status.current_cmd, sizeof(s->status.current_cmd), "PRESS %s", btn->name);
        furi_mutex_release(s->mutex);

        if(btn->type == CcButtonTypePress) {
            cc_ble_click_at(app, calib->x, calib->y);
        } else {
            cc_ble_press_button(
                app, profile->trigger_x, profile->trigger_y, calib->x, calib->y, 200);
        }
        s->pc++;
        return 0;
    }

    case CcInstrDelay:
        furi_mutex_acquire(s->mutex, FuriWaitForever);
        snprintf(s->status.current_cmd, sizeof(s->status.current_cmd),
            "DELAY %lu", (unsigned long)instr->delay_ms);
        furi_mutex_release(s->mutex);
        s->pc++;
        return instr->delay_ms;

    case CcInstrLoopStart:
        s->loop_stack[s->loop_depth].body_pc = s->pc + 1;
        s->loop_stack[s->loop_depth].remaining =
            (instr->loop_count == 0) ? UINT32_MAX : instr->loop_count;
        s->loop_depth++;
        s->pc++;
        return 0;

    case CcInstrLoopEnd: {
        CcRunLoopFrame* frame = &s->loop_stack[s->loop_depth - 1];
        if(frame->remaining == UINT32_MAX) {
            s->pc = frame->body_pc;
        } else if(frame->remaining > 1) {
            frame->remaining--;
            s->pc = frame->body_pc;
        } else {
            s->loop_depth--;
            s->pc++;
        }
        return 0;
    }
    }

    return 0; // unreachable
}

// ---------------------------------------------------------------------------
// Worker thread
// ---------------------------------------------------------------------------

static int32_t cc_script_worker(void* context) {
    CcScript* s = context;

    while(!s->kill) {
        // Clear stale interrupt flags before waiting for the next operation
        furi_thread_flags_clear(CC_EVT_STOP | CC_EVT_PAUSE | CC_EVT_RESUME);

        uint32_t flags = furi_thread_flags_wait(
            CC_EVT_COMPILE | CC_EVT_RUN | CC_EVT_STOP, FuriFlagWaitAny, FuriWaitForever);
        furi_thread_flags_clear(CC_EVT_COMPILE | CC_EVT_RUN | CC_EVT_STOP);

        if(s->kill) break;

        // ---- Compile ----
        if(flags & CC_EVT_COMPILE) {
            furi_delay_ms(50); // let the GUI thread render "Compiling..." before we start
            bool ok = cc_script_do_compile(s);
            if(s->kill) break;

            furi_mutex_acquire(s->mutex, FuriWaitForever);
            s->status.state = ok ? CcScriptStateIdle : CcScriptStateError;
            furi_mutex_release(s->mutex);
            view_dispatcher_send_custom_event(
                s->app->view_dispatcher, CheapClickerCustomEventScriptUpdate);
            continue;
        }

        // ---- Run ----
        if(!(flags & CC_EVT_RUN)) continue;

        furi_mutex_acquire(s->mutex, FuriWaitForever);
        s->status.state = CcScriptStateRunning;
        s->status.line_cur = 0;
        furi_mutex_release(s->mutex);

        bool error = false;

        while(s->pc < s->instr_count) {
            uint32_t f = furi_thread_flags_get();

            if(f & CC_EVT_STOP) {
                furi_thread_flags_clear(CC_EVT_STOP);
                goto next_run;
            }

            if(f & CC_EVT_PAUSE) {
                furi_thread_flags_clear(CC_EVT_PAUSE);

                furi_mutex_acquire(s->mutex, FuriWaitForever);
                s->status.state = CcScriptStatePaused;
                furi_mutex_release(s->mutex);
                view_dispatcher_send_custom_event(
                    s->app->view_dispatcher, CheapClickerCustomEventScriptUpdate);

                uint32_t woke = furi_thread_flags_wait(
                    CC_EVT_RESUME | CC_EVT_STOP, FuriFlagWaitAny, FuriWaitForever);
                furi_thread_flags_clear(CC_EVT_RESUME | CC_EVT_STOP);
                if(woke & CC_EVT_STOP) goto next_run;

                furi_mutex_acquire(s->mutex, FuriWaitForever);
                s->status.state = CcScriptStateRunning;
                furi_mutex_release(s->mutex);
            }

            furi_mutex_acquire(s->mutex, FuriWaitForever);
            s->status.line_cur = s->pc;
            furi_mutex_release(s->mutex);

            uint32_t delay = cc_execute_instr(s);

            if(delay == UINT32_MAX) {
                furi_mutex_acquire(s->mutex, FuriWaitForever);
                s->status.state = CcScriptStateError;
                furi_mutex_release(s->mutex);
                view_dispatcher_send_custom_event(
                    s->app->view_dispatcher, CheapClickerCustomEventScriptError);
                error = true;
                break;
            }

            view_dispatcher_send_custom_event(
                s->app->view_dispatcher, CheapClickerCustomEventScriptUpdate);

            if(delay > 0) furi_delay_ms(delay);
        }

        if(!error && s->pc >= s->instr_count) {
            furi_mutex_acquire(s->mutex, FuriWaitForever);
            s->status.state = CcScriptStateDone;
            furi_mutex_release(s->mutex);
            view_dispatcher_send_custom_event(
                s->app->view_dispatcher, CheapClickerCustomEventScriptDone);
        }

    next_run:;
    }

    return 0;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

CcScript* cc_script_alloc(CheapClickerApp* app) {
    CcScript* s = malloc(sizeof(CcScript));
    furi_check(s);
    memset(s, 0, sizeof(CcScript));

    s->app = app;
    s->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    furi_check(s->mutex);

    s->file_path = furi_string_alloc();
    s->line = furi_string_alloc();

    s->storage = furi_record_open(RECORD_STORAGE);
    s->file = storage_file_alloc(s->storage);

    s->status.state = CcScriptStateIdle;

    s->thread = furi_thread_alloc_ex("CcScript", 2048, cc_script_worker, s);
    furi_thread_start(s->thread);

    return s;
}

void cc_script_free(CcScript* script) {
    furi_check(script);

    script->kill = true;
    // Wake the thread from any waiting state
    furi_thread_flags_set(
        furi_thread_get_id(script->thread), CC_EVT_STOP | CC_EVT_RESUME);
    furi_thread_join(script->thread);
    furi_thread_free(script->thread);

    if(storage_file_is_open(script->file)) storage_file_close(script->file);
    storage_file_free(script->file);
    furi_record_close(RECORD_STORAGE);

    furi_mutex_free(script->mutex);
    furi_string_free(script->file_path);
    furi_string_free(script->line);

    free(script->instrs);
    free(script);
}

void cc_script_open(CcScript* script, const char* path) {
    furi_check(script);
    furi_check(path);

    // Free any previously compiled program
    free(script->instrs);
    script->instrs = NULL;
    script->instr_count = 0;
    script->pc = 0;
    script->loop_depth = 0;

    furi_string_set(script->file_path, path);

    furi_mutex_acquire(script->mutex, FuriWaitForever);
    script->status.state = CcScriptStateCompiling;
    script->status.line_cur = 0;
    script->status.current_cmd[0] = '\0';
    script->status.error_msg[0] = '\0';
    furi_mutex_release(script->mutex);

    // Kick off async compile on the worker thread
    furi_thread_flags_set(furi_thread_get_id(script->thread), CC_EVT_COMPILE);
}

void cc_script_run(CcScript* script) {
    furi_check(script);
    furi_thread_flags_set(furi_thread_get_id(script->thread), CC_EVT_RUN);
}

void cc_script_stop(CcScript* script) {
    furi_check(script);
    furi_thread_flags_set(
        furi_thread_get_id(script->thread), CC_EVT_STOP | CC_EVT_RESUME);
}

void cc_script_pause(CcScript* script) {
    furi_check(script);
    furi_thread_flags_set(furi_thread_get_id(script->thread), CC_EVT_PAUSE);
}

void cc_script_resume(CcScript* script) {
    furi_check(script);
    furi_thread_flags_set(furi_thread_get_id(script->thread), CC_EVT_RESUME);
}

void cc_script_get_status(CcScript* script, CcScriptStatus* out) {
    furi_check(script);
    furi_check(out);
    furi_mutex_acquire(script->mutex, FuriWaitForever);
    *out = script->status;
    furi_mutex_release(script->mutex);
}
