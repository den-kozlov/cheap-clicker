#pragma once
#include <gui/view.h>
typedef struct CcRunView CcRunView;
CcRunView* cc_run_view_alloc(void);
void cc_run_view_free(CcRunView* v);
View* cc_run_view_get_view(CcRunView* v);
