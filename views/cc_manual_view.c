#include "cc_manual_view.h"
#include "../cheap_clicker_i.h"
#include <gui/elements.h>
#include <gui/canvas.h>
#include <string.h>
#include <stdio.h>

static const char* const KEY_LABELS[5] = {"Up", "Dn", "Lt", "Rt", "OK"};
static const uint8_t ROW_Y[5] = {8, 19, 30, 41, 52};

typedef struct {
    char labels[5][20];
    bool ble_connected;
    uint8_t holding_key; // CC_BUTTON_IDX_NONE = none, 0-4 = row to invert
} CcManualViewModel;

struct CcManualView {
    View* view;
    CcManualViewCallback cb;
    void* cb_ctx;
    uint8_t long_pressed; // bitmask: bit i set when InputKey i had InputTypeLong
};

static void cc_manual_view_draw(Canvas* canvas, void* _m) {
    CcManualViewModel* m = _m;
    canvas_set_font(canvas, FontSecondary);

    for(uint8_t i = 0; i < 5; i++) {
        uint8_t y = ROW_Y[i];
        bool held = (m->holding_key == i);

        if(held) {
            canvas_set_color(canvas, ColorBlack);
            canvas_draw_box(canvas, 0, y - 8, 128, 10);
            canvas_set_color(canvas, ColorWhite);
        }

        char buf[24];
        snprintf(buf, sizeof(buf), "%s: %s", KEY_LABELS[i], m->labels[i]);
        canvas_draw_str(canvas, 0, y, buf);

        if(held) {
            canvas_set_color(canvas, ColorBlack);
        }
    }

    // BT status: right-aligned, same colour as row 0 if it is inverted
    if(m->holding_key == 0) canvas_set_color(canvas, ColorWhite);
    canvas_draw_str_aligned(
        canvas, 127, ROW_Y[0], AlignRight, AlignBottom,
        m->ble_connected ? "BT:OK" : "BT:--");
    canvas_set_color(canvas, ColorBlack);

    // Persistent config hint at bottom
    canvas_draw_str(canvas, 0, 63, "Hold Bk: config");
}

static bool cc_manual_view_input(InputEvent* event, void* ctx) {
    CcManualView* v = ctx;

    if(event->key == InputKeyBack) {
        if(event->type == InputTypeLong) {
            if(v->cb) v->cb(v->cb_ctx, CcManualViewEventConfigure, InputKeyBack);
            return true;
        }
        return false; // short/press/release on Back → let dispatcher exit
    }

    uint8_t idx = (uint8_t)event->key;
    if(idx > 4) return false; // only Up(0) Down(1) Left(2) Right(3) Ok(4)

    if(event->type == InputTypeShort) {
        if(v->cb) v->cb(v->cb_ctx, CcManualViewEventFire, event->key);
        return true;
    }
    if(event->type == InputTypeLong) {
        v->long_pressed |= (uint8_t)(1u << idx);
        if(v->cb) v->cb(v->cb_ctx, CcManualViewEventLongBegin, event->key);
        return true;
    }
    if(event->type == InputTypeRelease) {
        if(v->long_pressed & (uint8_t)(1u << idx)) {
            v->long_pressed &= (uint8_t)~(1u << idx);
            if(v->cb) v->cb(v->cb_ctx, CcManualViewEventLongRelease, event->key);
            return true;
        }
        return false;
    }
    return false; // InputTypePress and InputTypeRepeat are ignored
}

CcManualView* cc_manual_view_alloc(void) {
    CcManualView* v = malloc(sizeof(CcManualView));
    furi_check(v != NULL);
    v->view = view_alloc();
    view_set_context(v->view, v);
    view_allocate_model(v->view, ViewModelTypeLocking, sizeof(CcManualViewModel));
    view_set_draw_callback(v->view, cc_manual_view_draw);
    view_set_input_callback(v->view, cc_manual_view_input);
    v->cb = NULL;
    v->cb_ctx = NULL;
    v->long_pressed = 0;
    return v;
}

void cc_manual_view_free(CcManualView* v) {
    furi_assert(v);
    view_free(v->view);
    free(v);
}

View* cc_manual_view_get_view(CcManualView* v) {
    furi_assert(v);
    return v->view;
}

void cc_manual_view_set_callback(CcManualView* v, CcManualViewCallback cb, void* ctx) {
    furi_assert(v);
    v->cb = cb;
    v->cb_ctx = ctx;
}

void cc_manual_view_update(
    CcManualView* v,
    const uint8_t* layout,
    const char button_names[][32],
    uint8_t button_count,
    bool ble_connected,
    uint8_t holding_key) {
    furi_assert(v);
    with_view_model(
        v->view,
        CcManualViewModel* m,
        {
            for(uint8_t i = 0; i < 5; i++) {
                uint8_t btn = layout[i];
                if(btn == CC_BUTTON_IDX_NONE || btn >= button_count) {
                    strlcpy(m->labels[i], "-", sizeof(m->labels[i]));
                } else {
                    strlcpy(m->labels[i], button_names[btn], sizeof(m->labels[i]));
                }
            }
            m->ble_connected = ble_connected;
            m->holding_key = holding_key;
        },
        true);
}
