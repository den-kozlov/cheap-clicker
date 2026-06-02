#pragma once
#include <gui/view.h>
typedef struct CcCalibrateView CcCalibrateView;
CcCalibrateView* cc_calibrate_view_alloc(void);
void cc_calibrate_view_free(CcCalibrateView* v);
View* cc_calibrate_view_get_view(CcCalibrateView* v);
