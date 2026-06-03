#pragma once

#include <gui/view.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    CcMonumentViewEventStart,
    CcMonumentViewEventStop,
    CcMonumentViewEventSelectAuto,
    CcMonumentViewEventSelectHeal,
    CcMonumentViewEventHealOnce,
} CcMonumentViewEvent;

typedef void (*CcMonumentViewCallback)(void* ctx, CcMonumentViewEvent event);

typedef struct CcMonumentView CcMonumentView;

CcMonumentView* cc_monument_view_alloc(void);
void            cc_monument_view_free(CcMonumentView* v);
View*           cc_monument_view_get_view(CcMonumentView* v);
void            cc_monument_view_set_callback(CcMonumentView* v, CcMonumentViewCallback cb, void* ctx);
void            cc_monument_view_update(CcMonumentView* v, const char (*names)[32], uint8_t count, bool connected);
void            cc_monument_view_set_running(CcMonumentView* v, bool running);
uint8_t         cc_monument_view_get_selected_btn(CcMonumentView* v);
void            cc_monument_view_set_auto_btn(CcMonumentView* v, uint8_t idx);
void            cc_monument_view_set_heal_btn(CcMonumentView* v, uint8_t idx);
uint8_t         cc_monument_view_get_heal_btn(CcMonumentView* v);
