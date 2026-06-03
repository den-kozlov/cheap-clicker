#include "../cheap_clicker_i.h"
#include "../helpers/cc_ble.h"
#include "../helpers/cc_profile.h"
#include "../views/cc_calibrate_view.h"
#include <gui/modules/dialog_ex.h>
#include <gui/modules/popup.h>
#include <gui/modules/submenu.h>
#include <furi.h>

#define SPEED_TUNE_REF_DELAY 60
#define SPEED_TUNE_REF_STEP  5

typedef enum {
    CcSpeedTunePhasePosition,
    CcSpeedTunePhaseDrawing,
    CcSpeedTunePhaseSelection,
    CcSpeedTunePhaseContinue,
} CcSpeedTunePhase;

// ---------------------------------------------------------------------------
// State helpers
// ---------------------------------------------------------------------------

CcSpeedTuneState* cc_speed_tune_state_alloc(void) {
    CcSpeedTuneState* s = malloc(sizeof(CcSpeedTuneState));
    furi_check(s);
    cc_speed_tune_state_reset(s);
    return s;
}

void cc_speed_tune_state_free(CcSpeedTuneState* s) {
    furi_assert(s);
    free(s);
}

void cc_speed_tune_state_reset(CcSpeedTuneState* s) {
    furi_assert(s);
    s->round = 1;
    s->winner_idx = 0;
    s->fixed_delay_ms = 10;
    s->range = 9;
    // Round 1: geometric spread covering orders of magnitude
    s->test_steps[0] = 20;
    s->test_steps[1] = 10;
    s->test_steps[2] = 5;
    s->test_steps[3] = 2;
}

void cc_speed_tune_state_advance(CcSpeedTuneState* s) {
    furi_assert(s);
    uint8_t w = s->test_steps[s->winner_idx];
    uint8_t r = s->range / 2;
    if(r < 1) r = 1;
    s->test_steps[0] = (w > r) ? (w - r) : 1;
    s->test_steps[1] = (w > r / 2) ? (w - r / 2) : 1;
    s->test_steps[2] = w + r / 2;
    s->test_steps[3] = w + r;
    s->range = r;
    s->round++;
    s->winner_idx = 0;
}

// ---------------------------------------------------------------------------
// Forward declarations
// ---------------------------------------------------------------------------

static void show_position(CheapClickerApp* app);
static void show_drawing(CheapClickerApp* app);
static void show_selection(CheapClickerApp* app);
static void show_continue(CheapClickerApp* app);

// ---------------------------------------------------------------------------
// CalibrateView callbacks (position phase)
// ---------------------------------------------------------------------------

static void cc_speed_tune_move_cb(void* context, int8_t dx, int8_t dy) {
    cc_ble_move_by(context, dx, dy);
}

static void cc_speed_tune_get_pos_cb(void* context, int16_t* x, int16_t* y) {
    cc_ble_get_pos(context, x, y);
}

static void cc_speed_tune_confirm_cb(void* context, int16_t x, int16_t y) {
    UNUSED(x);
    UNUSED(y);
    CheapClickerApp* app = context;
    scene_manager_set_scene_state(
        app->scene_manager, CheapClickerSceneSpeedTune, CcSpeedTunePhaseDrawing);
    show_drawing(app);
}

static void cc_speed_tune_skip_cb(void* context) {
    scene_manager_previous_scene(((CheapClickerApp*)context)->scene_manager);
}

// ---------------------------------------------------------------------------
// Dialog callback (continue phase)
// ---------------------------------------------------------------------------

static void cc_speed_tune_dialog_cb(DialogExResult result, void* context) {
    CheapClickerApp* app = context;
    CcSpeedTuneState* s = app->speed_tune;

    if(result == DialogExResultRight) {
        cc_speed_tune_state_advance(s);
        scene_manager_set_scene_state(
            app->scene_manager, CheapClickerSceneSpeedTune, CcSpeedTunePhasePosition);
        show_position(app);
    } else {
        app->move_step = s->test_steps[s->winner_idx];
        app->move_delay_ms = s->fixed_delay_ms;
        cc_profile_save_active(app);
        scene_manager_previous_scene(app->scene_manager);
    }
}

// ---------------------------------------------------------------------------
// Submenu callback
// ---------------------------------------------------------------------------

static void cc_speed_tune_submenu_cb(void* context, uint32_t index) {
    view_dispatcher_send_custom_event(
        ((CheapClickerApp*)context)->view_dispatcher, index);
}

// ---------------------------------------------------------------------------
// Phase display helpers
// ---------------------------------------------------------------------------

static void show_position(CheapClickerApp* app) {
    int16_t x = 0, y = 0;
    cc_ble_get_pos(app, &x, &y);

    cc_calibrate_view_set_label(app->calibrate_view, "Open drawing app.\nDpad=move  OK=start draw");
    cc_calibrate_view_set_coords(app->calibrate_view, x, y);
    cc_calibrate_view_set_move_callback(app->calibrate_view, cc_speed_tune_move_cb, app);
    cc_calibrate_view_set_get_pos_callback(app->calibrate_view, cc_speed_tune_get_pos_cb, app);
    cc_calibrate_view_set_confirm_callback(app->calibrate_view, cc_speed_tune_confirm_cb, app);
    cc_calibrate_view_set_skip_callback(app->calibrate_view, cc_speed_tune_skip_cb, app);
    view_dispatcher_switch_to_view(app->view_dispatcher, CheapClickerViewCalibrate);
}

static void show_drawing(CheapClickerApp* app) {
    popup_reset(app->popup);
    popup_set_header(app->popup, "Speed Tune", 64, 4, AlignCenter, AlignTop);
    popup_set_text(
        app->popup,
        "Drawing lines...\nDon't touch the phone.",
        64,
        32,
        AlignCenter,
        AlignCenter);
    view_dispatcher_switch_to_view(app->view_dispatcher, CheapClickerViewPopup);

    CcSpeedTuneState* s = app->speed_tune;
    cc_ble_draw_tune_lines(app, s->fixed_delay_ms, SPEED_TUNE_REF_STEP, s->test_steps);

    scene_manager_set_scene_state(
        app->scene_manager, CheapClickerSceneSpeedTune, CcSpeedTunePhaseSelection);
    show_selection(app);
}

static void show_selection(CheapClickerApp* app) {
    CcSpeedTuneState* s = app->speed_tune;
    submenu_reset(app->submenu);
    submenu_set_header(app->submenu, "Which line matches Ref?");

    char label[32];
    snprintf(label, sizeof(label), "A: step=%u", (unsigned)s->test_steps[0]);
    submenu_add_item(app->submenu, label, 0, cc_speed_tune_submenu_cb, app);
    snprintf(label, sizeof(label), "B: step=%u", (unsigned)s->test_steps[1]);
    submenu_add_item(app->submenu, label, 1, cc_speed_tune_submenu_cb, app);
    snprintf(label, sizeof(label), "C: step=%u", (unsigned)s->test_steps[2]);
    submenu_add_item(app->submenu, label, 2, cc_speed_tune_submenu_cb, app);
    snprintf(label, sizeof(label), "D: step=%u", (unsigned)s->test_steps[3]);
    submenu_add_item(app->submenu, label, 3, cc_speed_tune_submenu_cb, app);

    submenu_set_selected_item(app->submenu, s->winner_idx);
    view_dispatcher_switch_to_view(app->view_dispatcher, CheapClickerViewSubmenu);
}

static void show_continue(CheapClickerApp* app) {
    CcSpeedTuneState* s = app->speed_tune;
    static char body[64];
    snprintf(
        body,
        sizeof(body),
        "Round %u done.\nWinner: %c  step=%u",
        (unsigned)s->round,
        'A' + s->winner_idx,
        (unsigned)s->test_steps[s->winner_idx]);

    dialog_ex_reset(app->dialog_ex);
    dialog_ex_set_header(app->dialog_ex, "Speed Tune", 64, 4, AlignCenter, AlignTop);
    dialog_ex_set_text(app->dialog_ex, body, 64, 32, AlignCenter, AlignCenter);
    dialog_ex_set_left_button_text(app->dialog_ex, "Save");
    dialog_ex_set_right_button_text(app->dialog_ex, "More");
    dialog_ex_set_result_callback(app->dialog_ex, cc_speed_tune_dialog_cb);
    dialog_ex_set_context(app->dialog_ex, app);
    view_dispatcher_switch_to_view(app->view_dispatcher, CheapClickerViewDialogEx);
}

// ---------------------------------------------------------------------------
// Scene handlers
// ---------------------------------------------------------------------------

void cc_scene_speed_tune_on_enter(void* context) {
    CheapClickerApp* app = context;
    cc_speed_tune_state_reset(app->speed_tune);
    scene_manager_set_scene_state(
        app->scene_manager, CheapClickerSceneSpeedTune, CcSpeedTunePhasePosition);
    show_position(app);
}

bool cc_scene_speed_tune_on_event(void* context, SceneManagerEvent event) {
    CheapClickerApp* app = context;
    CcSpeedTuneState* s = app->speed_tune;
    CcSpeedTunePhase phase = (CcSpeedTunePhase)scene_manager_get_scene_state(
        app->scene_manager, CheapClickerSceneSpeedTune);

    if(event.type == SceneManagerEventTypeBack) {
        if(phase == CcSpeedTunePhasePosition) {
            scene_manager_previous_scene(app->scene_manager);
            return true;
        }
        if(phase == CcSpeedTunePhaseContinue) {
            app->move_step = s->test_steps[s->winner_idx];
            app->move_delay_ms = s->fixed_delay_ms;
            cc_profile_save_active(app);
            scene_manager_previous_scene(app->scene_manager);
            return true;
        }
        return false;
    }

    if(event.type == SceneManagerEventTypeCustom) {
        if(phase == CcSpeedTunePhaseSelection) {
            s->winner_idx = (uint8_t)event.event;
            scene_manager_set_scene_state(
                app->scene_manager, CheapClickerSceneSpeedTune, CcSpeedTunePhaseContinue);
            show_continue(app);
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
    submenu_reset(app->submenu);
    dialog_ex_reset(app->dialog_ex);
    popup_reset(app->popup);
}
