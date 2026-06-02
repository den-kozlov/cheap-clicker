#include "cc_start_view.h"
#include <furi.h>
#include <gui/elements.h>
#include <gui/canvas.h>
#include <input/input.h>
#include <string.h>

#define DEVICE_CHIP_H 14
#define ITEM_H        13
#define ITEMS_Y0      (DEVICE_CHIP_H + 1)

typedef struct {
    char device_name[32];
    bool has_device;
    bool show_scripts;
    uint8_t cursor;
    uint8_t item_count;
} CcStartViewModel;

struct CcStartView {
    View* view;
    CcStartViewCallback cb;
    void* cb_ctx;
};

static void cc_start_view_draw(Canvas* canvas, void* _m) {
    CcStartViewModel* m = _m;

    // Device chip — inverted when cursor is on it
    if(m->cursor == 0) {
        canvas_set_color(canvas, ColorBlack);
        canvas_draw_box(canvas, 0, 0, 128, DEVICE_CHIP_H);
        canvas_set_color(canvas, ColorWhite);
    } else {
        canvas_set_color(canvas, ColorBlack);
    }
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str_aligned(
        canvas,
        64,
        DEVICE_CHIP_H / 2,
        AlignCenter,
        AlignCenter,
        m->has_device ? m->device_name : "<Connect New>");
    canvas_set_color(canvas, ColorBlack);

    // Separator
    canvas_draw_line(canvas, 0, DEVICE_CHIP_H, 127, DEVICE_CHIP_H);

    uint8_t row = 0;

    // Scripts item (only when has_device)
    if(m->show_scripts) {
        row++;
        uint8_t iy = ITEMS_Y0 + (row - 1) * ITEM_H;
        if(m->cursor == row) {
            canvas_set_color(canvas, ColorBlack);
            canvas_draw_box(canvas, 0, iy, 128, ITEM_H);
            canvas_set_color(canvas, ColorWhite);
        }
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str_aligned(canvas, 4, iy + ITEM_H - 2, AlignLeft, AlignBottom, "Scripts");
        canvas_set_color(canvas, ColorBlack);
    }

    // Manual item (only when has_device)
    if(m->show_scripts) {
        row++;
        uint8_t iy = ITEMS_Y0 + (row - 1) * ITEM_H;
        if(m->cursor == row) {
            canvas_set_color(canvas, ColorBlack);
            canvas_draw_box(canvas, 0, iy, 128, ITEM_H);
            canvas_set_color(canvas, ColorWhite);
        }
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str_aligned(canvas, 4, iy + ITEM_H - 2, AlignLeft, AlignBottom, "Manual");
        canvas_set_color(canvas, ColorBlack);
    }

    // Options item (always visible, last row)
    row++;
    {
        uint8_t iy = ITEMS_Y0 + (row - 1) * ITEM_H;
        if(m->cursor == row) {
            canvas_set_color(canvas, ColorBlack);
            canvas_draw_box(canvas, 0, iy, 128, ITEM_H);
            canvas_set_color(canvas, ColorWhite);
        }
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str_aligned(canvas, 4, iy + ITEM_H - 2, AlignLeft, AlignBottom, "Options");
        canvas_set_color(canvas, ColorBlack);
    }

    // Right-button hint when on device chip with active device
    if(m->cursor == 0 && m->has_device) {
        elements_button_right(canvas, "Edit");
    }
}

static bool cc_start_view_input(InputEvent* event, void* ctx) {
    CcStartView* v = ctx;
    if(event->type != InputTypeShort) return false;

    bool has_device = false;
    uint8_t cursor = 0;
    uint8_t item_count = 0;
    with_view_model(
        v->view,
        CcStartViewModel* m,
        {
            has_device = m->has_device;
            cursor = m->cursor;
            item_count = m->item_count;
        },
        false);

    switch(event->key) {
    case InputKeyUp:
        if(cursor > 0) {
            with_view_model(v->view, CcStartViewModel* m, { m->cursor--; }, true);
        }
        return true;
    case InputKeyDown:
        if(cursor < item_count - 1) {
            with_view_model(v->view, CcStartViewModel* m, { m->cursor++; }, true);
        }
        return true;
    case InputKeyOk:
        if(cursor == 0) {
            if(v->cb) v->cb(v->cb_ctx, CcStartViewEventDeviceSelect);
        } else if(cursor == 1 && has_device) {
            if(v->cb) v->cb(v->cb_ctx, CcStartViewEventScripts);
        } else if(cursor == 2 && has_device) {
            if(v->cb) v->cb(v->cb_ctx, CcStartViewEventManual);
        } else {
            // Options — last item, cursor == item_count - 1
            if(v->cb) v->cb(v->cb_ctx, CcStartViewEventOptions);
        }
        return true;
    case InputKeyRight:
        if(cursor == 0 && has_device) {
            if(v->cb) v->cb(v->cb_ctx, CcStartViewEventDeviceEdit);
            return true;
        }
        return false;
    default:
        return false;
    }
}

CcStartView* cc_start_view_alloc(void) {
    CcStartView* v = malloc(sizeof(CcStartView));
    furi_check(v);
    v->view = view_alloc();
    view_set_context(v->view, v);
    view_allocate_model(v->view, ViewModelTypeLocking, sizeof(CcStartViewModel));
    view_set_draw_callback(v->view, cc_start_view_draw);
    view_set_input_callback(v->view, cc_start_view_input);
    v->cb = NULL;
    v->cb_ctx = NULL;
    return v;
}

void cc_start_view_free(CcStartView* v) {
    furi_assert(v);
    view_free(v->view);
    free(v);
}

View* cc_start_view_get_view(CcStartView* v) {
    furi_assert(v);
    return v->view;
}

void cc_start_view_set_callback(CcStartView* v, CcStartViewCallback cb, void* ctx) {
    furi_assert(v);
    v->cb = cb;
    v->cb_ctx = ctx;
}

void cc_start_view_update(CcStartView* v, const char* device_name, bool show_scripts) {
    furi_assert(v);
    with_view_model(
        v->view,
        CcStartViewModel* m,
        {
            if(device_name) {
                strlcpy(m->device_name, device_name, sizeof(m->device_name));
                m->has_device = true;
            } else {
                m->device_name[0] = '\0';
                m->has_device = false;
            }
            m->show_scripts = show_scripts;
            m->item_count = 1 + (show_scripts ? 2 : 0) + 1; // device + optional scripts+manual + options
            if(m->cursor >= m->item_count) m->cursor = 0;
        },
        true);
}

void cc_start_view_set_cursor(CcStartView* v, uint8_t cursor) {
    furi_assert(v);
    with_view_model(
        v->view,
        CcStartViewModel* m,
        { if(cursor < m->item_count) m->cursor = cursor; },
        true);
}

uint8_t cc_start_view_get_cursor(CcStartView* v) {
    furi_assert(v);
    uint8_t cur = 0;
    with_view_model(v->view, CcStartViewModel* m, { cur = m->cursor; }, false);
    return cur;
}
