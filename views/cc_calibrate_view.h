#pragma once

#include <gui/view.h>
#include <input/input.h>

typedef struct CcCalibrateView CcCalibrateView;

typedef void (*CcCalibrateViewConfirmCallback)(void* context, int16_t x, int16_t y);
typedef void (*CcCalibrateViewMoveCallback)(void* context, int8_t dx, int8_t dy);
typedef void (*CcCalibrateViewGetPosCallback)(void* context, int16_t* x, int16_t* y);
typedef void (*CcCalibrateViewSkipCallback)(void* context);
typedef void (*CcCalibrateViewAbortCallback)(void* context);
typedef void (*CcCalibrateViewTestCallback)(void* context, int16_t x, int16_t y);

CcCalibrateView* cc_calibrate_view_alloc(void);
void cc_calibrate_view_free(CcCalibrateView* v);
View* cc_calibrate_view_get_view(CcCalibrateView* v);

void cc_calibrate_view_set_confirm_callback(
    CcCalibrateView* v,
    CcCalibrateViewConfirmCallback cb,
    void* context);
void cc_calibrate_view_set_move_callback(
    CcCalibrateView* v,
    CcCalibrateViewMoveCallback cb,
    void* context);
void cc_calibrate_view_set_get_pos_callback(
    CcCalibrateView* v,
    CcCalibrateViewGetPosCallback cb,
    void* context);
void cc_calibrate_view_set_skip_callback(
    CcCalibrateView* v,
    CcCalibrateViewSkipCallback cb,
    void* context);
void cc_calibrate_view_set_abort_callback(
    CcCalibrateView* v,
    CcCalibrateViewAbortCallback cb,
    void* context);
void cc_calibrate_view_set_test_callback(
    CcCalibrateView* v,
    CcCalibrateViewTestCallback cb,
    void* context);

// Set the label shown for the current calibration point (e.g. "TRIGGER" or "[3/17] attack")
void cc_calibrate_view_set_label(CcCalibrateView* v, const char* label);

// Update displayed coordinates (call this every time the cursor moves)
void cc_calibrate_view_set_coords(CcCalibrateView* v, int16_t x, int16_t y);
