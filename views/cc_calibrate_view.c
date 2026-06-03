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
    CcCalibrateViewGetPosCallback get_pos_cb;
    void* get_pos_ctx;
    CcCalibrateViewSkipCallback skip_cb;
    void* skip_ctx;
    CcCalibrateViewTestCallback test_cb;
    void* test_ctx;
    CcCalibrateViewAbortCallback abort_cb;
    void* abort_ctx;
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
    canvas_draw_str(canvas, 0, 52, "OK=confirm  LongOK=test");
    canvas_draw_str(canvas, 0, 62, "Dpad=move  Back=skip");
}

static bool cc_calibrate_input_callback(InputEvent* event, void* context) {
    furi_assert(context);
    CcCalibrateView* v = context;

    if(event->key == InputKeyBack && event->type == InputTypeShort) {
        if(v->skip_cb) {
            v->skip_cb(v->skip_ctx);
        }
        return true;
    }

    if(event->key == InputKeyBack && event->type == InputTypeLong) {
        if(v->abort_cb) {
            v->abort_cb(v->abort_ctx);
        }
        return true;
    }

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

    if(event->key == InputKeyOk && event->type == InputTypeLong) {
        int16_t x = 0, y = 0;
        with_view_model(
            v->view,
            CcCalibrateModel * model,
            {
                x = model->x;
                y = model->y;
            },
            false);
        if(v->test_cb) {
            v->test_cb(v->test_ctx, x, y);
        }
        return true;
    }

    if(event->key == InputKeyUp || event->key == InputKeyDown || event->key == InputKeyLeft ||
       event->key == InputKeyRight) {
        if(event->type == InputTypeShort || event->type == InputTypeLong ||
           event->type == InputTypeRepeat) {
            int8_t step = 20;
                //(event->type == InputTypeLong || event->type == InputTypeRepeat) ? 20 : 5;
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

            if(v->get_pos_cb) {
                int16_t nx = 0, ny = 0;
                v->get_pos_cb(v->get_pos_ctx, &nx, &ny);
                with_view_model(
                    v->view,
                    CcCalibrateModel * model,
                    {
                        model->x = nx;
                        model->y = ny;
                    },
                    true);
            }
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
    v->get_pos_cb = NULL;
    v->get_pos_ctx = NULL;
    v->skip_cb = NULL;
    v->skip_ctx = NULL;
    v->test_cb = NULL;
    v->test_ctx = NULL;
    v->abort_cb = NULL;
    v->abort_ctx = NULL;
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

void cc_calibrate_view_set_get_pos_callback(
    CcCalibrateView* v,
    CcCalibrateViewGetPosCallback cb,
    void* context) {
    furi_assert(v);
    v->get_pos_cb = cb;
    v->get_pos_ctx = context;
}

void cc_calibrate_view_set_label(CcCalibrateView* v, const char* label) {
    furi_assert(v);
    with_view_model(
        v->view,
        CcCalibrateModel * model,
        { strlcpy(model->label, label, sizeof(model->label)); },
        true);
}

void cc_calibrate_view_set_skip_callback(
    CcCalibrateView* v,
    CcCalibrateViewSkipCallback cb,
    void* context) {
    furi_assert(v);
    v->skip_cb = cb;
    v->skip_ctx = context;
}

void cc_calibrate_view_set_test_callback(
    CcCalibrateView* v,
    CcCalibrateViewTestCallback cb,
    void* context) {
    furi_assert(v);
    v->test_cb = cb;
    v->test_ctx = context;
}

void cc_calibrate_view_set_abort_callback(
    CcCalibrateView* v,
    CcCalibrateViewAbortCallback cb,
    void* context) {
    furi_assert(v);
    v->abort_cb = cb;
    v->abort_ctx = context;
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
