#pragma once
#include <stdint.h>
#include <stdbool.h>

#define CC_ACCEL_CAL_POINTS 2

typedef struct {
    uint8_t cal_point;   // 0..1 — current calibration point index
    uint8_t m_low;       // binary search lower bound
    uint8_t m_high;      // binary search upper bound
    uint8_t m_cur;       // current test step count (midpoint)
    float   result[2];   // measured accel multipliers per cal point
    bool    done[2];     // which cal points are finished
} CcAccelTuneState;

CcAccelTuneState* cc_accel_tune_state_alloc(void);
void cc_accel_tune_state_free(CcAccelTuneState* s);
void cc_accel_tune_state_reset(CcAccelTuneState* s);
