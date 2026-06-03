#include "cc_monument.h"
#include "cc_ble.h"
#include <furi.h>
#include <stdlib.h>
#include <string.h>

struct CcMonument {
    FuriThread*      thread;
    CheapClickerApp* app;
    uint8_t          button_idx;
    volatile bool    running;
    volatile uint8_t heal_request;
};

static int32_t cc_monument_worker(void* context) {
    CcMonument* ac = context;
    while(ac->running) {
        CheapClickerApp* app = ac->app;
        uint8_t idx = ac->button_idx;
        if(app->active_profile_idx == CC_PROFILE_IDX_NONE) break;
        CcProfile* p = &app->profiles[app->active_profile_idx];
        if(app->buttons[idx].type == CcButtonTypePress) {
            cc_ble_click_at(app, p->calib[idx].x, p->calib[idx].y);
        } else {
            cc_ble_press_button(
                app, p->trigger_x, p->trigger_y,
                p->calib[idx].x, p->calib[idx].y, 300);
        }
        uint8_t h = ac->heal_request;
        if(h != CC_BUTTON_IDX_NONE) {
            ac->heal_request = CC_BUTTON_IDX_NONE;
            if(app->buttons[h].type == CcButtonTypePress) {
                cc_ble_click_at(app, p->calib[h].x, p->calib[h].y);
            } else {
                cc_ble_press_button(app, p->trigger_x, p->trigger_y,
                                    p->calib[h].x, p->calib[h].y, 100);
            }
        }
    }
    return 0;
}

CcMonument* cc_monument_alloc(void) {
    CcMonument* ac = malloc(sizeof(CcMonument));
    furi_check(ac);
    memset(ac, 0, sizeof(CcMonument));
    ac->heal_request = CC_BUTTON_IDX_NONE;
    return ac;
}

void cc_monument_free(CcMonument* ac) {
    furi_assert(ac);
    cc_monument_stop(ac);
    free(ac);
}

void cc_monument_start(CcMonument* ac, CheapClickerApp* app, uint8_t button_idx) {
    furi_assert(ac);
    furi_assert(!ac->running);
    ac->app = app;
    ac->button_idx = button_idx;
    ac->heal_request = CC_BUTTON_IDX_NONE;
    ac->running = true;
    ac->thread = furi_thread_alloc_ex("CcMonument", 1024, cc_monument_worker, ac);
    furi_thread_start(ac->thread);
}

void cc_monument_stop(CcMonument* ac) {
    furi_assert(ac);
    if(!ac->running) return;
    ac->running = false;
    if(ac->thread) {
        furi_thread_join(ac->thread);
        furi_thread_free(ac->thread);
        ac->thread = NULL;
    }
}

bool cc_monument_is_running(CcMonument* ac) {
    furi_assert(ac);
    return ac->running;
}

void cc_monument_request_heal(CcMonument* ac, uint8_t heal_btn_idx) {
    furi_assert(ac);
    ac->heal_request = heal_btn_idx;
}
