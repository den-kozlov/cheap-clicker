#pragma once
#include <stdint.h>

typedef struct {
    uint8_t round;
    uint8_t winner_idx;
    uint8_t test_delays[4];
    uint8_t test_step;
    uint8_t range_ms;
} CcSpeedTuneState;

CcSpeedTuneState* cc_speed_tune_state_alloc(void);
void cc_speed_tune_state_free(CcSpeedTuneState* s);
void cc_speed_tune_state_reset(CcSpeedTuneState* s);
void cc_speed_tune_state_advance(CcSpeedTuneState* s);
