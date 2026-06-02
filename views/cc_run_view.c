#include "cc_run_view.h"
#include <furi.h>

struct CcRunView { View* view; };

CcRunView* cc_run_view_alloc(void) {
    CcRunView* v = malloc(sizeof(CcRunView));
    furi_check(v != NULL);
    v->view = view_alloc();
    return v;
}
void cc_run_view_free(CcRunView* v) { view_free(v->view); free(v); }
View* cc_run_view_get_view(CcRunView* v) { return v->view; }
