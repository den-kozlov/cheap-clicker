#include "cc_run_view.h"
#include <furi.h>
#include <gui/elements.h>
#include <gui/canvas.h>
#include <input/input.h>
#include <string.h>
#include <stdio.h>

typedef struct {
    char script_name[40];
    char current_cmd[64];
    char error_msg[64];
    CcScriptState state;
    size_t line_cur;
} CcRunViewModel;

struct CcRunView {
    View* view;
    CcRunViewButtonCallback cb;
    void* cb_ctx;
};

static const char* cc_run_view_state_str(CcScriptState state) {
    switch(state) {
    case CcScriptStateRunning:
        return "Running";
    case CcScriptStatePaused:
        return "Paused";
    case CcScriptStateDone:
        return "Done";
    case CcScriptStateError:
        return "Error";
    case CcScriptStateIdle:
    default:
        return "Idle";
    }
}

static void cc_run_view_draw_callback(Canvas* canvas, void* _model) {
    CcRunViewModel* m = _model;

    // Row 1 (y=12): script name centered, FontPrimary
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str_aligned(canvas, 64, 12, AlignCenter, AlignBottom, m->script_name);

    // Row 2 (y=24): state left + "Line: N" right, FontSecondary
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str_aligned(
        canvas, 0, 24, AlignLeft, AlignBottom, cc_run_view_state_str(m->state));

    char line_buf[24];
    snprintf(line_buf, sizeof(line_buf), "Line: %zu", m->line_cur);
    canvas_draw_str_aligned(canvas, 128, 24, AlignRight, AlignBottom, line_buf);

    // Row 3 (y=36): current_cmd or error_msg if Error state, FontSecondary
    canvas_set_font(canvas, FontSecondary);
    const char* row3 =
        (m->state == CcScriptStateError) ? m->error_msg : m->current_cmd;
    canvas_draw_str_aligned(canvas, 0, 36, AlignLeft, AlignBottom, row3);

    // Bottom buttons
    if(m->state == CcScriptStateRunning) {
        elements_button_left(canvas, "Stop");
        elements_button_right(canvas, "Pause");
    } else if(m->state == CcScriptStatePaused) {
        elements_button_left(canvas, "Stop");
        elements_button_right(canvas, "Resume");
    } else {
        // Done / Error / Idle
        elements_button_center(canvas, "Back");
    }
}

static bool cc_run_view_input_callback(InputEvent* event, void* context) {
    furi_assert(context);
    CcRunView* v = context;

    if(event->type != InputTypeShort) {
        return false;
    }

    // Snapshot the state from model
    CcScriptState state = CcScriptStateIdle;
    with_view_model(
        v->view, CcRunViewModel * m, { state = m->state; }, false);

    if(state == CcScriptStateRunning) {
        if(event->key == InputKeyLeft) {
            if(v->cb) v->cb(v->cb_ctx, CcRunViewEventStop);
            return true;
        } else if(event->key == InputKeyRight) {
            if(v->cb) v->cb(v->cb_ctx, CcRunViewEventPause);
            return true;
        }
    } else if(state == CcScriptStatePaused) {
        if(event->key == InputKeyLeft) {
            if(v->cb) v->cb(v->cb_ctx, CcRunViewEventStop);
            return true;
        } else if(event->key == InputKeyRight) {
            if(v->cb) v->cb(v->cb_ctx, CcRunViewEventResume);
            return true;
        }
    } else if(state == CcScriptStateDone || state == CcScriptStateError) {
        // Let scene's back handler take over
        return false;
    }

    return false;
}

CcRunView* cc_run_view_alloc(void) {
    CcRunView* v = malloc(sizeof(CcRunView));
    furi_check(v != NULL);
    v->view = view_alloc();
    view_set_context(v->view, v);
    view_allocate_model(v->view, ViewModelTypeLocking, sizeof(CcRunViewModel));
    view_set_draw_callback(v->view, cc_run_view_draw_callback);
    view_set_input_callback(v->view, cc_run_view_input_callback);
    v->cb = NULL;
    v->cb_ctx = NULL;
    return v;
}

void cc_run_view_free(CcRunView* v) {
    furi_assert(v);
    view_free(v->view);
    free(v);
}

View* cc_run_view_get_view(CcRunView* v) {
    furi_assert(v);
    return v->view;
}

void cc_run_view_set_button_callback(CcRunView* v, CcRunViewButtonCallback cb, void* context) {
    furi_assert(v);
    v->cb = cb;
    v->cb_ctx = context;
}

void cc_run_view_update(CcRunView* v, const CcScriptStatus* status, const char* script_name) {
    with_view_model(
        v->view,
        CcRunViewModel * m,
        {
            strlcpy(m->script_name, script_name, sizeof(m->script_name));
            strlcpy(m->current_cmd, status->current_cmd, sizeof(m->current_cmd));
            strlcpy(m->error_msg, status->error_msg, sizeof(m->error_msg));
            m->state = status->state;
            m->line_cur = status->line_cur;
        },
        true);
}
