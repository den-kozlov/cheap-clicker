#include "cc_monument_view.h"
#include "../cheap_clicker_i.h"
#include <furi.h>
#include <gui/elements.h>
#include <gui/canvas.h>
#include <string.h>

typedef struct {
    char    button_names[CC_MAX_BUTTONS][CC_MAX_NAME_LEN];
    uint8_t button_count;
    uint8_t selected_btn;   // CC_BUTTON_IDX_NONE = nothing selected (auto button)
    uint8_t heal_btn;       // CC_BUTTON_IDX_NONE = nothing selected
    uint8_t focused_row;    // 0=auto, 1=heal, 2=start
    bool    is_running;
    bool    connected;
} CcMonumentViewModel;

struct CcMonumentView {
    View*                  view;
    CcMonumentViewCallback cb;
    void*                  cb_ctx;
};

static void cc_monument_view_draw(Canvas* canvas, void* _m) {
    CcMonumentViewModel* m = _m;

    // Row 0: Auto button
    if(m->focused_row == 0) {
        canvas_set_color(canvas, ColorBlack);
        canvas_draw_box(canvas, 0, 1, 128, 14);
        canvas_set_color(canvas, ColorWhite);
    } else {
        canvas_set_color(canvas, ColorBlack);
    }
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 2, 12, "Auto:");
    canvas_set_font(canvas, FontSecondary);
    if(m->selected_btn == CC_BUTTON_IDX_NONE) {
        canvas_draw_str(canvas, 35, 12, "<select>");
    } else {
        canvas_draw_str(canvas, 35, 12, m->button_names[m->selected_btn]);
    }
    canvas_set_color(canvas, ColorBlack);

    // Row 1: Heal button
    if(m->focused_row == 1) {
        canvas_set_color(canvas, ColorBlack);
        canvas_draw_box(canvas, 0, 15, 128, 14);
        canvas_set_color(canvas, ColorWhite);
    } else {
        canvas_set_color(canvas, ColorBlack);
    }
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 2, 26, "Heal:");
    canvas_set_font(canvas, FontSecondary);
    if(m->heal_btn == CC_BUTTON_IDX_NONE) {
        canvas_draw_str(canvas, 35, 26, "<none>");
    } else {
        canvas_draw_str(canvas, 35, 26, m->button_names[m->heal_btn]);
    }
    canvas_set_color(canvas, ColorBlack);

    // Row 2: Start/Stop
    bool can_start = (m->selected_btn != CC_BUTTON_IDX_NONE) && m->connected && !m->is_running;

    if(m->is_running) {
        elements_button_center(canvas, "Stop");
        if(m->heal_btn != CC_BUTTON_IDX_NONE) {
            elements_button_right(canvas, "Heal");
        }
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

    uint8_t selected_btn = CC_BUTTON_IDX_NONE;
    uint8_t button_count = 0;
    uint8_t focused_row  = 0;
    bool    is_running   = false;
    bool    connected    = false;

    with_view_model(
        v->view,
        CcMonumentViewModel* m,
        {
            selected_btn = m->selected_btn;
            button_count = m->button_count;
            focused_row  = m->focused_row;
            is_running   = m->is_running;
            connected    = m->connected;
        },
        false);

    bool can_start = (selected_btn != CC_BUTTON_IDX_NONE) && connected && !is_running;

    switch(event->key) {
    case InputKeyUp:
        with_view_model(
            v->view,
            CcMonumentViewModel* m,
            { m->focused_row = (m->focused_row + 2) % 3; },
            true);
        return true;

    case InputKeyDown:
        with_view_model(
            v->view,
            CcMonumentViewModel* m,
            { m->focused_row = (m->focused_row + 1) % 3; },
            true);
        return true;

    case InputKeyOk:
        if(focused_row == 0) {
            if(button_count > 0 && v->cb) v->cb(v->cb_ctx, CcMonumentViewEventSelectAuto);
        } else if(focused_row == 1) {
            if(button_count > 0 && v->cb) v->cb(v->cb_ctx, CcMonumentViewEventSelectHeal);
        } else { // focused_row == 2
            if(is_running) {
                if(v->cb) v->cb(v->cb_ctx, CcMonumentViewEventStop);
            } else if(can_start) {
                if(v->cb) v->cb(v->cb_ctx, CcMonumentViewEventStart);
            }
        }
        return true;

    case InputKeyRight:
        if(is_running) {
            if(v->cb) v->cb(v->cb_ctx, CcMonumentViewEventHealOnce);
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
        {
            m->selected_btn = CC_BUTTON_IDX_NONE;
            m->heal_btn     = CC_BUTTON_IDX_NONE;
            m->focused_row  = 0;
        },
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
            if(m->heal_btn != CC_BUTTON_IDX_NONE && m->heal_btn >= count) {
                m->heal_btn = (count > 0) ? 0 : CC_BUTTON_IDX_NONE;
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

void cc_monument_view_set_auto_btn(CcMonumentView* v, uint8_t idx) {
    furi_assert(v);
    with_view_model(v->view, CcMonumentViewModel* m, { m->selected_btn = idx; }, true);
}

void cc_monument_view_set_heal_btn(CcMonumentView* v, uint8_t idx) {
    furi_assert(v);
    with_view_model(v->view, CcMonumentViewModel* m, { m->heal_btn = idx; }, true);
}

uint8_t cc_monument_view_get_heal_btn(CcMonumentView* v) {
    furi_assert(v);
    uint8_t h = CC_BUTTON_IDX_NONE;
    with_view_model(v->view, CcMonumentViewModel* m, { h = m->heal_btn; }, false);
    return h;
}
