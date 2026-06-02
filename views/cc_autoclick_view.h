#pragma once

#include <gui/view.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    CcAutoclickViewEventStart,
    CcAutoclickViewEventStop,
} CcAutoclickViewEvent;

typedef void (*CcAutoclickViewCallback)(void* ctx, CcAutoclickViewEvent event);

typedef struct CcAutoclickView CcAutoclickView;

CcAutoclickView* cc_autoclick_view_alloc(void);
void             cc_autoclick_view_free(CcAutoclickView* v);
View*            cc_autoclick_view_get_view(CcAutoclickView* v);
void             cc_autoclick_view_set_callback(CcAutoclickView* v, CcAutoclickViewCallback cb, void* ctx);
void             cc_autoclick_view_update(
                     CcAutoclickView* v,
                     const char (*names)[32],
                     uint8_t count,
                     bool connected);
void             cc_autoclick_view_set_running(CcAutoclickView* v, bool running);
uint8_t          cc_autoclick_view_get_selected_btn(CcAutoclickView* v);
