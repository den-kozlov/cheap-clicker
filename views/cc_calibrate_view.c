#include "cc_calibrate_view.h"
#include <furi.h>

struct CcCalibrateView { View* view; };

CcCalibrateView* cc_calibrate_view_alloc(void) {
    CcCalibrateView* v = malloc(sizeof(CcCalibrateView));
    furi_check(v != NULL);
    v->view = view_alloc();
    return v;
}
void cc_calibrate_view_free(CcCalibrateView* v) { view_free(v->view); free(v); }
View* cc_calibrate_view_get_view(CcCalibrateView* v) { return v->view; }
