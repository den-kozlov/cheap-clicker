#include "../cheap_clicker_i.h"
#include "../helpers/cc_ble.h"
#include "../helpers/cc_profile.h"
#include "../views/cc_calibrate_view.h"
#include <gui/modules/dialog_ex.h>
#include <gui/modules/popup.h>
#include <furi.h>

// Cal-point step sizes — must match cc_ble.h constants
static const uint8_t CAL_STEP[CC_ACCEL_CAL_POINTS]  = {CC_ACCEL_CAL_STEP_0,  CC_ACCEL_CAL_STEP_1};

typedef enum {
    AccelPhasePosition,
    AccelPhaseSearch,
    AccelPhaseDone,
} AccelPhase;

// ---------------------------------------------------------------------------
// State alloc / free / reset
// ---------------------------------------------------------------------------

CcAccelTuneState* cc_accel_tune_state_alloc(void) {
    CcAccelTuneState* s = malloc(sizeof(CcAccelTuneState));
    furi_check(s);
    cc_accel_tune_state_reset(s);
    return s;
}

void cc_accel_tune_state_free(CcAccelTuneState* s) {
    furi_assert(s);
    free(s);
}

void cc_accel_tune_state_reset(CcAccelTuneState* s) {
    furi_assert(s);
    s->cal_point  = 0;
    s->m_low      = 1;
    s->m_high     = (CC_ACCEL_N_REF * CC_ACCEL_REF_STEP) / CAL_STEP[0];
    s->m_cur      = (s->m_low + s->m_high) / 2;
    s->result[0]  = 1.0f;
    s->result[1]  = 1.0f;
    s->done[0]    = false;
    s->done[1]    = false;
}

// ---------------------------------------------------------------------------
// Forward declarations
// ---------------------------------------------------------------------------

static void show_position(CheapClickerApp* app);
static void show_drawing(CheapClickerApp* app);
static void show_done(CheapClickerApp* app);

// ---------------------------------------------------------------------------
// CalibrateView callbacks (position phase)
// ---------------------------------------------------------------------------

static void accel_move_cb(void* context, int8_t dx, int8_t dy) {
    cc_ble_move_by(context, dx, dy);
}

static void accel_get_pos_cb(void* context, int16_t* x, int16_t* y) {
    cc_ble_get_pos(context, x, y);
}

static void accel_confirm_cb(void* context, int16_t x, int16_t y) {
    UNUSED(x);
    UNUSED(y);
    CheapClickerApp* app = context;
    scene_manager_set_scene_state(
        app->scene_manager, CheapClickerSceneSpeedTune, AccelPhaseSearch);
    show_drawing(app);
}

static void accel_skip_cb(void* context) {
    scene_manager_previous_scene(((CheapClickerApp*)context)->scene_manager);
}

// ---------------------------------------------------------------------------
// Advance to next cal point or finish
// ---------------------------------------------------------------------------

static void advance_cal_point(CheapClickerApp* app) {
    CcAccelTuneState* s = app->accel_tune;

    // Record measured accel: ref_real ≈ N_REF × REF_STEP; test_real = m_cur × CAL_STEP × a
    float a = (float)(CC_ACCEL_N_REF * CC_ACCEL_REF_STEP) /
              ((float)s->m_cur * (float)CAL_STEP[s->cal_point]);
    s->result[s->cal_point] = a;
    s->done[s->cal_point]   = true;

    s->cal_point++;
    if(s->cal_point >= CC_ACCEL_CAL_POINTS) {
        // Apply and save
        app->accel[0] = s->result[0];
        app->accel[1] = s->result[1];
        cc_profile_save_active(app);
        scene_manager_set_scene_state(
            app->scene_manager, CheapClickerSceneSpeedTune, AccelPhaseDone);
        show_done(app);
    } else {
        // Next cal point: reset binary search bounds
        s->m_low  = 1;
        s->m_high = (CC_ACCEL_N_REF * CC_ACCEL_REF_STEP) / CAL_STEP[s->cal_point];
        s->m_cur  = (s->m_low + s->m_high) / 2;
        scene_manager_set_scene_state(
            app->scene_manager, CheapClickerSceneSpeedTune, AccelPhasePosition);
        show_position(app);
    }
}

// ---------------------------------------------------------------------------
// Dialog callback — compare phase
// ---------------------------------------------------------------------------

static void compare_dialog_cb(DialogExResult result, void* context) {
    CheapClickerApp* app   = context;
    CcAccelTuneState* s    = app->accel_tune;

    if(result == DialogExResultCenter) {
        // OK — lines match exactly
        advance_cal_point(app);
        return;
    }

    if(result == DialogExResultLeft) {
        // Test line is shorter → need more steps → raise lower bound
        s->m_low = s->m_cur + 1;
    } else {
        // Test line is longer → need fewer steps → lower upper bound
        if(s->m_cur > 1) s->m_high = s->m_cur - 1;
        else             s->m_high = 1;
    }

    if(s->m_low >= s->m_high) {
        // Converged
        s->m_cur = s->m_low < s->m_high ? s->m_low : s->m_high;
        advance_cal_point(app);
    } else {
        s->m_cur = (s->m_low + s->m_high) / 2;
        show_drawing(app);
    }
}

// ---------------------------------------------------------------------------
// Done dialog callback
// ---------------------------------------------------------------------------

static void done_dialog_cb(DialogExResult result, void* context) {
    UNUSED(result);
    scene_manager_previous_scene(((CheapClickerApp*)context)->scene_manager);
}

// ---------------------------------------------------------------------------
// Phase display helpers
// ---------------------------------------------------------------------------

static void show_position(CheapClickerApp* app) {
    CcAccelTuneState* s = app->accel_tune;
    int16_t x = 0, y = 0;
    cc_ble_get_pos(app, &x, &y);

    static char label[64];
    snprintf(
        label,
        sizeof(label),
        "Point %u/%u: step=%u\nDpad=move  OK=draw",
        (unsigned)(s->cal_point + 1),
        (unsigned)CC_ACCEL_CAL_POINTS,
        (unsigned)CAL_STEP[s->cal_point]);

    cc_calibrate_view_set_label(app->calibrate_view, label);
    cc_calibrate_view_set_coords(app->calibrate_view, x, y);
    cc_calibrate_view_set_move_callback(app->calibrate_view, accel_move_cb, app);
    cc_calibrate_view_set_get_pos_callback(app->calibrate_view, accel_get_pos_cb, app);
    cc_calibrate_view_set_confirm_callback(app->calibrate_view, accel_confirm_cb, app);
    cc_calibrate_view_set_skip_callback(app->calibrate_view, accel_skip_cb, app);
    view_dispatcher_switch_to_view(app->view_dispatcher, CheapClickerViewCalibrate);
}

static void show_drawing(CheapClickerApp* app) {
    CcAccelTuneState* s = app->accel_tune;

    popup_reset(app->popup);
    popup_set_header(app->popup, "Accel Tune", 64, 4, AlignCenter, AlignTop);
    static char text[48];
    snprintf(
        text,
        sizeof(text),
        "Drawing lines...\nPoint %u, M=%u",
        (unsigned)(s->cal_point + 1),
        (unsigned)s->m_cur);
    popup_set_text(app->popup, text, 64, 32, AlignCenter, AlignCenter);
    view_dispatcher_switch_to_view(app->view_dispatcher, CheapClickerViewPopup);

    // Blocking draw: reference line + test line
    cc_ble_draw_accel_lines(app, s->cal_point, s->m_cur);

    // Show comparison dialog
    static char body[64];
    snprintf(
        body,
        sizeof(body),
        "Is bottom line shorter\nor longer than top?\n(OK = same length)");

    dialog_ex_reset(app->dialog_ex);
    dialog_ex_set_header(app->dialog_ex, "Compare lines", 64, 2, AlignCenter, AlignTop);
    dialog_ex_set_text(app->dialog_ex, body, 64, 32, AlignCenter, AlignCenter);
    dialog_ex_set_left_button_text(app->dialog_ex, "< Short");
    dialog_ex_set_right_button_text(app->dialog_ex, "Long >");
    dialog_ex_set_result_callback(app->dialog_ex, compare_dialog_cb);
    dialog_ex_set_context(app->dialog_ex, app);
    view_dispatcher_switch_to_view(app->view_dispatcher, CheapClickerViewDialogEx);
}

static void show_done(CheapClickerApp* app) {
    CcAccelTuneState* s = app->accel_tune;

    static char body[48];
    snprintf(
        body,
        sizeof(body),
        "a1=%.2f (step=%u)\na2=%.2f (step=%u)\nSaved!",
        (double)s->result[0],
        (unsigned)CAL_STEP[0],
        (double)s->result[1],
        (unsigned)CAL_STEP[1]);

    dialog_ex_reset(app->dialog_ex);
    dialog_ex_set_header(app->dialog_ex, "Accel Calibrated", 64, 2, AlignCenter, AlignTop);
    dialog_ex_set_text(app->dialog_ex, body, 64, 32, AlignCenter, AlignCenter);
    dialog_ex_set_right_button_text(app->dialog_ex, "OK");
    dialog_ex_set_result_callback(app->dialog_ex, done_dialog_cb);
    dialog_ex_set_context(app->dialog_ex, app);
    view_dispatcher_switch_to_view(app->view_dispatcher, CheapClickerViewDialogEx);
}

// ---------------------------------------------------------------------------
// Scene handlers
// ---------------------------------------------------------------------------

void cc_scene_speed_tune_on_enter(void* context) {
    CheapClickerApp* app = context;
    cc_accel_tune_state_reset(app->accel_tune);
    scene_manager_set_scene_state(
        app->scene_manager, CheapClickerSceneSpeedTune, AccelPhasePosition);
    show_position(app);
}

bool cc_scene_speed_tune_on_event(void* context, SceneManagerEvent event) {
    CheapClickerApp* app = context;
    AccelPhase phase = (AccelPhase)scene_manager_get_scene_state(
        app->scene_manager, CheapClickerSceneSpeedTune);

    if(event.type == SceneManagerEventTypeBack) {
        if(phase == AccelPhasePosition || phase == AccelPhaseDone) {
            scene_manager_previous_scene(app->scene_manager);
            return true;
        }
        // During search: back goes to position phase for this point
        if(phase == AccelPhaseSearch) {
            scene_manager_set_scene_state(
                app->scene_manager, CheapClickerSceneSpeedTune, AccelPhasePosition);
            show_position(app);
            return true;
        }
    }
    return false;
}

void cc_scene_speed_tune_on_exit(void* context) {
    CheapClickerApp* app = context;
    cc_calibrate_view_set_confirm_callback(app->calibrate_view, NULL, NULL);
    cc_calibrate_view_set_move_callback(app->calibrate_view, NULL, NULL);
    cc_calibrate_view_set_get_pos_callback(app->calibrate_view, NULL, NULL);
    cc_calibrate_view_set_skip_callback(app->calibrate_view, NULL, NULL);
    dialog_ex_reset(app->dialog_ex);
    popup_reset(app->popup);
}
