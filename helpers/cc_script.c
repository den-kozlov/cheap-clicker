#include "cc_script.h"
#include "cc_ble.h"
#include <storage/storage.h>
#include <string.h>
#include <stdlib.h>

#define TAG "CcScript"

#define CC_FILE_BUF_LEN 16
#define CC_EVT_RUN    (1u << 0)
#define CC_EVT_STOP   (1u << 1)
#define CC_EVT_PAUSE  (1u << 2)
#define CC_EVT_RESUME (1u << 3)

typedef struct {
    uint64_t file_offset; // position at start of loop body
    uint32_t remaining;   // UINT32_MAX = infinite
} CcLoopFrame;

struct CcScript {
    CheapClickerApp* app;
    FuriThread* thread;
    FuriMutex* mutex;

    Storage* storage;
    File* file;
    FuriString* file_path;
    uint8_t buf[CC_FILE_BUF_LEN + 1];
    uint8_t buf_start;
    uint8_t buf_len;
    bool file_end;

    CcScriptStatus status; // protected by mutex

    CcLoopFrame loop_stack[2];
    uint8_t loop_depth;

    FuriString* line;
    size_t line_num;
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

static uint64_t cc_file_tell(CcScript* s) {
    return storage_file_tell(s->file) - (s->buf_len - s->buf_start);
}

static void cc_file_seek(CcScript* s, uint64_t offset) {
    furi_assert(offset <= UINT32_MAX);  // scripts fit well within 4 GB
    storage_file_seek(s->file, (uint32_t)offset, true);
    s->buf_start = 0;
    s->buf_len = 0;
    s->file_end = false;
}

// ---------------------------------------------------------------------------
// Command execution
// ---------------------------------------------------------------------------

static uint32_t cc_execute_line(CcScript* s) {
    const char* line = furi_string_get_cstr(s->line);

    // Trim leading whitespace
    while(*line == ' ' || *line == '\t') line++;

    // Skip blank lines and comments
    if(line[0] == '\0' || line[0] == '#') return 0;

    // PRESS <name>
    if(strncmp(line, "PRESS ", 6) == 0) {
        const char* btn_name = line + 6;

        // Look up active profile
        CheapClickerApp* app = s->app;
        if(app->active_profile_idx == CC_PROFILE_IDX_NONE ||
           app->active_profile_idx >= app->profile_count) {
            furi_mutex_acquire(s->mutex, FuriWaitForever);
            snprintf(
                s->status.error_msg,
                sizeof(s->status.error_msg),
                "No active profile");
            furi_mutex_release(s->mutex);
            return UINT32_MAX;
        }

        CcProfile* profile = &app->profiles[app->active_profile_idx];

        // Find button by name
        int found = -1;
        for(uint8_t i = 0; i < profile->button_count; i++) {
            if(strcmp(profile->buttons[i].name, btn_name) == 0) {
                found = i;
                break;
            }
        }

        if(found < 0) {
            furi_mutex_acquire(s->mutex, FuriWaitForever);
            snprintf(
                s->status.error_msg,
                sizeof(s->status.error_msg),
                "Button not found: %.40s",
                btn_name);
            furi_mutex_release(s->mutex);
            return UINT32_MAX;
        }

        CcButton* btn = &profile->buttons[found];

        furi_mutex_acquire(s->mutex, FuriWaitForever);
        snprintf(s->status.current_cmd, sizeof(s->status.current_cmd), "PRESS %s", btn_name);
        furi_mutex_release(s->mutex);

        cc_ble_press_button(
            app,
            profile->trigger_x,
            profile->trigger_y,
            btn->x,
            btn->y,
            200);

        return 0;
    }

    // DELAY <ms>
    if(strncmp(line, "DELAY ", 6) == 0) {
        char* end;
        long ms_val = strtol(line + 6, &end, 10);
        if(end == line + 6 || ms_val < 0) {
            furi_mutex_acquire(s->mutex, FuriWaitForever);
            snprintf(s->status.error_msg, sizeof(s->status.error_msg), "Bad DELAY value");
            furi_mutex_release(s->mutex);
            return UINT32_MAX;
        }
        uint32_t delay_ms = (uint32_t)ms_val;

        furi_mutex_acquire(s->mutex, FuriWaitForever);
        snprintf(s->status.current_cmd, sizeof(s->status.current_cmd), "DELAY %lu", (unsigned long)delay_ms);
        furi_mutex_release(s->mutex);

        return delay_ms;
    }

    // LOOP <count>
    if(strncmp(line, "LOOP ", 5) == 0) {
        if(s->loop_depth >= 2) {
            furi_mutex_acquire(s->mutex, FuriWaitForever);
            snprintf(
                s->status.error_msg,
                sizeof(s->status.error_msg),
                "LOOP nesting exceeds max depth 2");
            furi_mutex_release(s->mutex);
            return UINT32_MAX;
        }

        char* end;
        long loop_count = strtol(line + 5, &end, 10);
        if(end == line + 5 || loop_count < 0) {
            furi_mutex_acquire(s->mutex, FuriWaitForever);
            snprintf(s->status.error_msg, sizeof(s->status.error_msg), "Bad LOOP count");
            furi_mutex_release(s->mutex);
            return UINT32_MAX;
        }

        CcLoopFrame* frame = &s->loop_stack[s->loop_depth];
        frame->file_offset = cc_file_tell(s);
        // count=0 means infinite; UINT32_MAX is the sentinel for "never decrement"
        frame->remaining = (loop_count == 0) ? UINT32_MAX : (uint32_t)(loop_count - 1);
        s->loop_depth++;

        furi_mutex_acquire(s->mutex, FuriWaitForever);
        snprintf(s->status.current_cmd, sizeof(s->status.current_cmd), "LOOP %ld", loop_count);
        furi_mutex_release(s->mutex);

        return 0;
    }

    // END
    if(strcmp(line, "END") == 0) {
        if(s->loop_depth == 0) {
            furi_mutex_acquire(s->mutex, FuriWaitForever);
            snprintf(
                s->status.error_msg, sizeof(s->status.error_msg), "END without LOOP");
            furi_mutex_release(s->mutex);
            return UINT32_MAX;
        }

        CcLoopFrame* frame = &s->loop_stack[s->loop_depth - 1];

        if(frame->remaining == UINT32_MAX || frame->remaining > 0) {
            // More iterations remain
            if(frame->remaining != UINT32_MAX) {
                frame->remaining--;
            }
            cc_file_seek(s, frame->file_offset);
        } else {
            // This iteration was the last one — pop frame
            s->loop_depth--;
        }

        return 0;
    }

    // Unknown command
    furi_mutex_acquire(s->mutex, FuriWaitForever);
    snprintf(s->status.error_msg, sizeof(s->status.error_msg), "Unknown: %.40s", line);
    furi_mutex_release(s->mutex);
    return UINT32_MAX;
}

// ---------------------------------------------------------------------------
// Worker thread
// ---------------------------------------------------------------------------

static int32_t cc_script_worker(void* context) {
    CcScript* s = context;

    // Wait for RUN event
    uint32_t start_flags =
        furi_thread_flags_wait(CC_EVT_RUN | CC_EVT_STOP, FuriFlagWaitAny, FuriWaitForever);
    furi_thread_flags_clear(CC_EVT_RUN | CC_EVT_STOP);
    if(start_flags & CC_EVT_STOP) return 0;

    furi_mutex_acquire(s->mutex, FuriWaitForever);
    s->status.state = CcScriptStateRunning;
    s->status.line_cur = 0;
    furi_mutex_release(s->mutex);

    while(true) {
        uint32_t flags = furi_thread_flags_get();

        if(flags & CC_EVT_STOP) {
            furi_thread_flags_clear(CC_EVT_STOP);
            break;
        }

        if(flags & CC_EVT_PAUSE) {
            furi_thread_flags_clear(CC_EVT_PAUSE);

            furi_mutex_acquire(s->mutex, FuriWaitForever);
            s->status.state = CcScriptStatePaused;
            furi_mutex_release(s->mutex);

            view_dispatcher_send_custom_event(
                s->app->view_dispatcher, CheapClickerCustomEventScriptUpdate);

            // Wait for RESUME or STOP
            uint32_t woke = furi_thread_flags_wait(
                CC_EVT_RESUME | CC_EVT_STOP, FuriFlagWaitAny, FuriWaitForever);
            furi_thread_flags_clear(CC_EVT_RESUME | CC_EVT_STOP);

            if(woke & CC_EVT_STOP) break;

            furi_mutex_acquire(s->mutex, FuriWaitForever);
            s->status.state = CcScriptStateRunning;
            furi_mutex_release(s->mutex);
        }

        if(!cc_read_line(s)) {
            // EOF
            furi_mutex_acquire(s->mutex, FuriWaitForever);
            s->status.state = CcScriptStateDone;
            furi_mutex_release(s->mutex);
            view_dispatcher_send_custom_event(
                s->app->view_dispatcher, CheapClickerCustomEventScriptDone);
            break;
        }

        furi_mutex_acquire(s->mutex, FuriWaitForever);
        s->status.line_cur++;
        furi_mutex_release(s->mutex);

        uint32_t delay = cc_execute_line(s);

        if(delay == UINT32_MAX) {
            furi_mutex_acquire(s->mutex, FuriWaitForever);
            s->status.state = CcScriptStateError;
            furi_mutex_release(s->mutex);
            view_dispatcher_send_custom_event(
                s->app->view_dispatcher, CheapClickerCustomEventScriptError);
            break;
        }

        view_dispatcher_send_custom_event(
            s->app->view_dispatcher, CheapClickerCustomEventScriptUpdate);

        if(delay > 0) furi_delay_ms(delay);
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

    // Stop the thread (wakes from any wait state)
    cc_script_stop(script);
    furi_thread_join(script->thread);
    furi_thread_free(script->thread);

    // Close file if open
    if(storage_file_is_open(script->file)) {
        storage_file_close(script->file);
    }
    storage_file_free(script->file);
    furi_record_close(RECORD_STORAGE);

    furi_mutex_free(script->mutex);
    furi_string_free(script->file_path);
    furi_string_free(script->line);

    free(script);
}

bool cc_script_open(CcScript* script, const char* path) {
    furi_check(script);
    furi_check(path);

    // Close existing file if open
    if(storage_file_is_open(script->file)) {
        storage_file_close(script->file);
    }

    furi_string_set(script->file_path, path);

    bool ok = storage_file_open(
        script->file, path, FSAM_READ, FSOM_OPEN_EXISTING);

    // Reset buffer and state
    script->buf_start = 0;
    script->buf_len = 0;
    script->file_end = false;
    script->loop_depth = 0;
    script->line_num = 0;

    furi_mutex_acquire(script->mutex, FuriWaitForever);
    script->status.state = CcScriptStateIdle;
    script->status.line_cur = 0;
    script->status.current_cmd[0] = '\0';
    script->status.error_msg[0] = '\0';
    furi_mutex_release(script->mutex);

    return ok;
}

void cc_script_run(CcScript* script) {
    furi_check(script);
    furi_thread_flags_set(furi_thread_get_id(script->thread), CC_EVT_RUN);
}

void cc_script_stop(CcScript* script) {
    furi_check(script);
    // Send both STOP and RESUME so the thread wakes from a paused wait
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
