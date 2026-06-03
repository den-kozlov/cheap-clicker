#include "../cheap_clicker_i.h"
#include "../helpers/cc_ble.h"
#include "../helpers/cc_profile.h"
#include <gui/modules/dialog_ex.h>
#include <gui/modules/popup.h>
#include <gui/modules/submenu.h>
#include <furi.h>

#define SPEED_TUNE_REF_DELAY 60
#define SPEED_TUNE_REF_STEP  5

typedef enum {
    CcSpeedTunePhaseInstructions,
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
    s->test_step = 20;
    s->range_ms = 27;
    s->test_delays[0] = 3;
    s->test_delays[1] = 8;
    s->test_delays[2] = 15;
    s->test_delays[3] = 30;
}

void cc_speed_tune_state_advance(CcSpeedTuneState* s) {
    furi_assert(s);
    uint8_t w = s->test_delays[s->winner_idx];
    uint8_t r = s->range_ms / 4;
    if(r < 1) r = 1;
    s->test_delays[0] = (w > r * 2) ? (w - r * 2) : 1;
    s->test_delays[1] = (w > r) ? (w - r) : 1;
    s->test_delays[2] = w + r;
    s->test_delays[3] = w + r * 2;
    s->range_ms = r * 2;
    s->round++;
    s->winner_idx = 0;
}

// ---------------------------------------------------------------------------
// Forward declarations
// ---------------------------------------------------------------------------

static void show_instructions(CheapClickerApp* app);
static void show_drawing(CheapClickerApp* app);
static void show_selection(CheapClickerApp* app);
static void show_continue(CheapClickerApp* app);

// ---------------------------------------------------------------------------
// Dialog callback
// ---------------------------------------------------------------------------

static void cc_speed_tune_dialog_cb(DialogExResult result, void* context) {
    CheapClickerApp* app = context;
    CcSpeedTuneState* s = app->speed_tune;
    CcSpeedTunePhase phase = (CcSpeedTunePhase)scene_manager_get_scene_state(
        app->scene_manager, CheapClickerSceneSpeedTune);

    if(phase == CcSpeedTunePhaseInstructions) {
        if(result == DialogExResultRight) {
            scene_manager_set_scene_state(
                app->scene_manager, CheapClickerSceneSpeedTune, CcSpeedTunePhaseDrawing);
            show_drawing(app);
        } else {
            scene_manager_previous_scene(app->scene_manager);
        }
    } else if(phase == CcSpeedTunePhaseContinue) {
        if(result == DialogExResultRight) {
            cc_speed_tune_state_advance(s);
            scene_manager_set_scene_state(
                app->scene_manager, CheapClickerSceneSpeedTune, CcSpeedTunePhaseInstructions);
            show_instructions(app);
        } else {
            app->move_step = s->test_step;
            app->move_delay_ms = s->test_delays[s->winner_idx];
            cc_profile_save_active(app);
            scene_manager_previous_scene(app->scene_manager);
        }
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

static void show_instructions(CheapClickerApp* app) {
    dialog_ex_reset(app->dialog_ex);
    dialog_ex_set_header(app->dialog_ex, "Speed Tune", 64, 4, AlignCenter, AlignTop);
    dialog_ex_set_text(
        app->dialog_ex,
        "Open drawing app on phone.\nPlace cursor top-left.\nPress OK to draw test lines.",
        64,
        32,
        AlignCenter,
        AlignCenter);
    dialog_ex_set_left_button_text(app->dialog_ex, "Back");
    dialog_ex_set_right_button_text(app->dialog_ex, "OK");
    dialog_ex_set_result_callback(app->dialog_ex, cc_speed_tune_dialog_cb);
    dialog_ex_set_context(app->dialog_ex, app);
    view_dispatcher_switch_to_view(app->view_dispatcher, CheapClickerViewDialogEx);
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
    cc_ble_draw_tune_lines(app, s->test_step, SPEED_TUNE_REF_DELAY, s->test_delays);

    scene_manager_set_scene_state(
        app->scene_manager, CheapClickerSceneSpeedTune, CcSpeedTunePhaseSelection);
    show_selection(app);
}

static void show_selection(CheapClickerApp* app) {
    CcSpeedTuneState* s = app->speed_tune;
    submenu_reset(app->submenu);
    submenu_set_header(app->submenu, "Which line matches Ref?");

    char label[32];
    snprintf(label, sizeof(label), "A: delay=%ums", (unsigned)s->test_delays[0]);
    submenu_add_item(app->submenu, label, 0, cc_speed_tune_submenu_cb, app);
    snprintf(label, sizeof(label), "B: delay=%ums", (unsigned)s->test_delays[1]);
    submenu_add_item(app->submenu, label, 1, cc_speed_tune_submenu_cb, app);
    snprintf(label, sizeof(label), "C: delay=%ums", (unsigned)s->test_delays[2]);
    submenu_add_item(app->submenu, label, 2, cc_speed_tune_submenu_cb, app);
    snprintf(label, sizeof(label), "D: delay=%ums", (unsigned)s->test_delays[3]);
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
        "Round %u done.\nWinner: %c  delay=%ums",
        (unsigned)s->round,
        'A' + s->winner_idx,
        (unsigned)s->test_delays[s->winner_idx]);

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
        app->scene_manager, CheapClickerSceneSpeedTune, CcSpeedTunePhaseInstructions);
    show_instructions(app);
}

bool cc_scene_speed_tune_on_event(void* context, SceneManagerEvent event) {
    CheapClickerApp* app = context;
    CcSpeedTuneState* s = app->speed_tune;
    CcSpeedTunePhase phase = (CcSpeedTunePhase)scene_manager_get_scene_state(
        app->scene_manager, CheapClickerSceneSpeedTune);

    if(event.type == SceneManagerEventTypeBack) {
        if(phase == CcSpeedTunePhaseInstructions) {
            scene_manager_previous_scene(app->scene_manager);
            return true;
        }
        if(phase == CcSpeedTunePhaseContinue) {
            app->move_step = s->test_step;
            app->move_delay_ms = s->test_delays[s->winner_idx];
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
    submenu_reset(app->submenu);
    dialog_ex_reset(app->dialog_ex);
    popup_reset(app->popup);
}
