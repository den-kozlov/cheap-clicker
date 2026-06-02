#include "cc_autoclick.h"
#include "cc_ble.h"
#include <furi.h>
#include <stdlib.h>
#include <string.h>

struct CcAutoclick {
    FuriThread*      thread;
    CheapClickerApp* app;
    uint8_t          button_idx;
    volatile bool    running;
};

static int32_t cc_autoclick_worker(void* context) {
    CcAutoclick* ac = context;
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
    }
    return 0;
}

CcAutoclick* cc_autoclick_alloc(void) {
    CcAutoclick* ac = malloc(sizeof(CcAutoclick));
    furi_check(ac);
    memset(ac, 0, sizeof(CcAutoclick));
    return ac;
}

void cc_autoclick_free(CcAutoclick* ac) {
    furi_assert(ac);
    cc_autoclick_stop(ac);
    free(ac);
}

void cc_autoclick_start(CcAutoclick* ac, CheapClickerApp* app, uint8_t button_idx) {
    furi_assert(ac);
    furi_assert(!ac->running);
    ac->app = app;
    ac->button_idx = button_idx;
    ac->running = true;
    ac->thread = furi_thread_alloc_ex("CcAutoclick", 1024, cc_autoclick_worker, ac);
    furi_thread_start(ac->thread);
}

void cc_autoclick_stop(CcAutoclick* ac) {
    furi_assert(ac);
    if(!ac->running) return;
    ac->running = false;
    if(ac->thread) {
        furi_thread_join(ac->thread);
        furi_thread_free(ac->thread);
        ac->thread = NULL;
    }
}

bool cc_autoclick_is_running(CcAutoclick* ac) {
    furi_assert(ac);
    return ac->running;
}
