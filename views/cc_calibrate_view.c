#include "cc_calibrate_view.h"
#include <furi.h>
#include <gui/elements.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    char label[48];
    int16_t x;
    int16_t y;
} CcCalibrateModel;

struct CcCalibrateView {
    View* view;
    CcCalibrateViewConfirmCallback confirm_cb;
    void* confirm_ctx;
    CcCalibrateViewMoveCallback move_cb;
    void* move_ctx;
};

static void cc_calibrate_draw_callback(Canvas* canvas, void* context) {
    furi_assert(context);
    CcCalibrateModel* model = context;

    char coords_buf[32];
    snprintf(coords_buf, sizeof(coords_buf), "X: %d  Y: %d", (int)model->x, (int)model->y);

    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 0, 12, "Calibrate:");

    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 0, 24, model->label);
    canvas_draw_str(canvas, 0, 38, coords_buf);
    canvas_draw_str(canvas, 0, 52, "OK=confirm  Back=skip");
    canvas_draw_str(canvas, 0, 62, "Dpad=move cursor");
}

static bool cc_calibrate_input_callback(InputEvent* event, void* context) {
    furi_assert(context);
    CcCalibrateView* v = context;

    if(event->key == InputKeyOk && event->type == InputTypeShort) {
        int16_t x = 0, y = 0;
        with_view_model(
            v->view,
            CcCalibrateModel * model,
            {
                x = model->x;
                y = model->y;
            },
            false);
        if(v->confirm_cb) {
            v->confirm_cb(v->confirm_ctx, x, y);
        }
        return true;
    }

    if(event->key == InputKeyUp || event->key == InputKeyDown || event->key == InputKeyLeft ||
       event->key == InputKeyRight) {
        if(event->type == InputTypeShort || event->type == InputTypeLong ||
           event->type == InputTypeRepeat) {
            int8_t step =
                (event->type == InputTypeLong || event->type == InputTypeRepeat) ? 20 : 5;
            int8_t dx = 0, dy = 0;

            if(event->key == InputKeyUp) {
                dy = -step;
            } else if(event->key == InputKeyDown) {
                dy = step;
            } else if(event->key == InputKeyLeft) {
                dx = -step;
            } else if(event->key == InputKeyRight) {
                dx = step;
            }

            if(v->move_cb) {
                v->move_cb(v->move_ctx, dx, dy);
            }

            with_view_model(
                v->view,
                CcCalibrateModel * model,
                {
                    model->x += dx;
                    model->y += dy;
                },
                true);
            return true;
        }
    }

    return false;
}

CcCalibrateView* cc_calibrate_view_alloc(void) {
    CcCalibrateView* v = malloc(sizeof(CcCalibrateView));
    furi_check(v != NULL);
    v->view = view_alloc();
    view_set_context(v->view, v);
    view_allocate_model(v->view, ViewModelTypeLocking, sizeof(CcCalibrateModel));
    view_set_draw_callback(v->view, cc_calibrate_draw_callback);
    view_set_input_callback(v->view, cc_calibrate_input_callback);
    v->confirm_cb = NULL;
    v->confirm_ctx = NULL;
    v->move_cb = NULL;
    v->move_ctx = NULL;
    return v;
}

void cc_calibrate_view_free(CcCalibrateView* v) {
    furi_assert(v);
    view_free(v->view);
    free(v);
}

View* cc_calibrate_view_get_view(CcCalibrateView* v) {
    furi_assert(v);
    return v->view;
}

void cc_calibrate_view_set_confirm_callback(
    CcCalibrateView* v,
    CcCalibrateViewConfirmCallback cb,
    void* context) {
    furi_assert(v);
    v->confirm_cb = cb;
    v->confirm_ctx = context;
}

void cc_calibrate_view_set_move_callback(
    CcCalibrateView* v,
    CcCalibrateViewMoveCallback cb,
    void* context) {
    furi_assert(v);
    v->move_cb = cb;
    v->move_ctx = context;
}

void cc_calibrate_view_set_label(CcCalibrateView* v, const char* label) {
    furi_assert(v);
    with_view_model(
        v->view,
        CcCalibrateModel * model,
        { strlcpy(model->label, label, sizeof(model->label)); },
        true);
}

void cc_calibrate_view_set_coords(CcCalibrateView* v, int16_t x, int16_t y) {
    furi_assert(v);
    with_view_model(
        v->view,
        CcCalibrateModel * model,
        {
            model->x = x;
            model->y = y;
        },
        true);
}
