#include "cc_monument_view.h"
#include "../cheap_clicker_i.h"
#include <furi.h>
#include <gui/elements.h>
#include <gui/canvas.h>
#include <string.h>

typedef struct {
    char    button_names[CC_MAX_BUTTONS][CC_MAX_NAME_LEN];
    uint8_t button_count;
    uint8_t selected_btn;  // CC_BUTTON_IDX_NONE = nothing selected
    bool    is_running;
    bool    connected;
} CcMonumentViewModel;

struct CcMonumentView {
    View*                   view;
    CcMonumentViewCallback cb;
    void*                   cb_ctx;
};

static void cc_monument_view_draw(Canvas* canvas, void* _m) {
    CcMonumentViewModel* m = _m;

    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 2, 12, "Button:");

    if(m->button_count == 0) {
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str(canvas, 50, 12, "<no buttons>");
    } else if(m->selected_btn == CC_BUTTON_IDX_NONE) {
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str(canvas, 50, 12, "<select>");
    } else {
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str(canvas, 50, 12, m->button_names[m->selected_btn]);
    }

    bool can_start = (m->selected_btn != CC_BUTTON_IDX_NONE) && m->connected && !m->is_running;
    bool can_stop  = m->is_running;

    if(can_stop) {
        elements_button_center(canvas, "Stop");
    } else if(can_start) {
        elements_button_center(canvas, "Start");
    } else {
        canvas_set_color(canvas, ColorBlack);
        canvas_draw_rframe(canvas, 39, 52, 50, 11, 3);
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str_aligned(canvas, 64, 58, AlignCenter, AlignCenter, "Start");
    }
}

static bool cc_monument_view_input(InputEvent* event, void* ctx) {
    CcMonumentView* v = ctx;
    if(event->type != InputTypeShort) return false;

    uint8_t button_count = 0;
    uint8_t selected_btn = CC_BUTTON_IDX_NONE;
    bool    is_running   = false;
    bool    connected    = false;

    with_view_model(
        v->view,
        CcMonumentViewModel* m,
        {
            button_count = m->button_count;
            selected_btn = m->selected_btn;
            is_running   = m->is_running;
            connected    = m->connected;
        },
        false);

    switch(event->key) {
    case InputKeyUp:
        if(button_count == 0) return true;
        with_view_model(
            v->view,
            CcMonumentViewModel* m,
            {
                if(m->selected_btn == CC_BUTTON_IDX_NONE) {
                    m->selected_btn = m->button_count - 1;
                } else if(m->selected_btn > 0) {
                    m->selected_btn--;
                }
            },
            true);
        return true;

    case InputKeyDown:
        if(button_count == 0) return true;
        with_view_model(
            v->view,
            CcMonumentViewModel* m,
            {
                if(m->selected_btn == CC_BUTTON_IDX_NONE) {
                    m->selected_btn = 0;
                } else if(m->selected_btn < m->button_count - 1) {
                    m->selected_btn++;
                }
            },
            true);
        return true;

    case InputKeyOk:
        if(is_running) {
            if(v->cb) v->cb(v->cb_ctx, CcMonumentViewEventStop);
        } else if(selected_btn != CC_BUTTON_IDX_NONE && connected) {
            if(v->cb) v->cb(v->cb_ctx, CcMonumentViewEventStart);
        }
        return true;

    default:
        return false;
    }
}

CcMonumentView* cc_monument_view_alloc(void) {
    CcMonumentView* v = malloc(sizeof(CcMonumentView));
    furi_check(v);
    v->view = view_alloc();
    view_set_context(v->view, v);
    view_allocate_model(v->view, ViewModelTypeLocking, sizeof(CcMonumentViewModel));
    view_set_draw_callback(v->view, cc_monument_view_draw);
    view_set_input_callback(v->view, cc_monument_view_input);
    with_view_model(
        v->view,
        CcMonumentViewModel* m,
        { m->selected_btn = CC_BUTTON_IDX_NONE; },
        false);
    v->cb     = NULL;
    v->cb_ctx = NULL;
    return v;
}

void cc_monument_view_free(CcMonumentView* v) {
    furi_assert(v);
    view_free(v->view);
    free(v);
}

View* cc_monument_view_get_view(CcMonumentView* v) {
    furi_assert(v);
    return v->view;
}

void cc_monument_view_set_callback(CcMonumentView* v, CcMonumentViewCallback cb, void* ctx) {
    furi_assert(v);
    v->cb     = cb;
    v->cb_ctx = ctx;
}

void cc_monument_view_update(
    CcMonumentView* v,
    const char (*names)[32],
    uint8_t count,
    bool connected) {
    furi_assert(v);
    with_view_model(
        v->view,
        CcMonumentViewModel* m,
        {
            m->button_count = count;
            m->connected    = connected;
            for(uint8_t i = 0; i < count; i++) {
                strlcpy(m->button_names[i], names[i], CC_MAX_NAME_LEN);
            }
            if(m->selected_btn != CC_BUTTON_IDX_NONE && m->selected_btn >= count) {
                m->selected_btn = (count > 0) ? 0 : CC_BUTTON_IDX_NONE;
            }
        },
        true);
}

void cc_monument_view_set_running(CcMonumentView* v, bool running) {
    furi_assert(v);
    with_view_model(v->view, CcMonumentViewModel* m, { m->is_running = running; }, true);
}

uint8_t cc_monument_view_get_selected_btn(CcMonumentView* v) {
    furi_assert(v);
    uint8_t sel = CC_BUTTON_IDX_NONE;
    with_view_model(v->view, CcMonumentViewModel* m, { sel = m->selected_btn; }, false);
    return sel;
}
