#include "cc_manual_view.h"
#include "../cheap_clicker_i.h"
#include <gui/elements.h>
#include <gui/canvas.h>
#include <string.h>
#include <stdio.h>

// Left panel: D-pad schematic (0-62px wide)
// Right panel: key→name list (64-127px)
// Separator at x=63

#define BOX_W 12
#define BOX_H  9
#define BOX_R  2

// D-pad center at (37, 32)
#define DPAD_CX   37
#define DPAD_CY   32
#define DPAD_STEP 13  // distance from center to each direction button center

typedef struct {
    char labels[5][9]; // Up=0 Down=1 Right=2 Left=3 Ok=4; 8 chars + null; "-" if NONE
    bool ble_connected;
} CcManualViewModel;

struct CcManualView {
    View* view;
    CcManualViewCallback cb;
    void* cb_ctx;
};

static void cc_manual_draw_box(Canvas* canvas, int x, int y, const char* letter) {
    canvas_draw_rframe(canvas, x, y, BOX_W, BOX_H, BOX_R);
    canvas_draw_str_aligned(
        canvas, x + BOX_W / 2, y + BOX_H / 2, AlignCenter, AlignCenter, letter);
}

static void cc_manual_view_draw(Canvas* canvas, void* _m) {
    CcManualViewModel* m = _m;

    canvas_set_font(canvas, FontSecondary);

    // Left panel: controller schematic
    cc_manual_draw_box(canvas, DPAD_CX - BOX_W / 2, DPAD_CY - DPAD_STEP - BOX_H / 2, "U");
    cc_manual_draw_box(canvas, DPAD_CX - BOX_W / 2, DPAD_CY + DPAD_STEP - BOX_H / 2, "D");
    cc_manual_draw_box(canvas, DPAD_CX - DPAD_STEP - BOX_W / 2, DPAD_CY - BOX_H / 2, "L");
    cc_manual_draw_box(canvas, DPAD_CX + DPAD_STEP - BOX_W / 2, DPAD_CY - BOX_H / 2, "R");
    cc_manual_draw_box(canvas, DPAD_CX - BOX_W / 2, DPAD_CY - BOX_H / 2, "O");

    // Separator
    canvas_draw_line(canvas, 63, 0, 63, 63);

    // Right panel: key→button name list
    static const char* const key_letters[5] = {"U", "D", "R", "L", "O"};
    for(uint8_t i = 0; i < 5; i++) {
        char buf[12];
        snprintf(buf, sizeof(buf), "%s:%s", key_letters[i], m->labels[i]);
        canvas_draw_str(canvas, 65, 9 + i * 11, buf);
    }

    // BLE status at bottom-right
    canvas_draw_str(canvas, 65, 62, m->ble_connected ? "BT:OK" : "BT:--");
}

static bool cc_manual_view_input(InputEvent* event, void* ctx) {
    CcManualView* v = ctx;

    if(event->key == InputKeyBack) return false;

    uint8_t idx = (uint8_t)event->key;
    if(idx > 4) return false; // only Up(0) Down(1) Left(2) Right(3) Ok(4)

    if(event->type == InputTypeShort) {
        if(v->cb) v->cb(v->cb_ctx, CcManualViewEventFire, event->key);
        return true;
    }
    if(event->type == InputTypeLong) {
        if(v->cb) v->cb(v->cb_ctx, CcManualViewEventReassign, event->key);
        return true;
    }
    return false;
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
    bool ble_connected) {
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
        },
        true);
}
