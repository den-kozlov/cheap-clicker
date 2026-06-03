#pragma once
#include <stdint.h>

typedef struct {
    uint8_t round;
    uint8_t winner_idx;
    uint8_t test_steps[4];   // varying step sizes
    uint8_t fixed_delay_ms;  // delay stays fixed at 10ms
    uint8_t range;           // current search half-range in step units
} CcSpeedTuneState;

CcSpeedTuneState* cc_speed_tune_state_alloc(void);
void cc_speed_tune_state_free(CcSpeedTuneState* s);
void cc_speed_tune_state_reset(CcSpeedTuneState* s);
void cc_speed_tune_state_advance(CcSpeedTuneState* s);
