#include "timer.h"

#include <stdlib.h>

#include "frame.h"
#include "utils.h"
#include "log.h"

void nan_timer_state_init(struct nan_timer_state *state, const uint64_t now_usec)
{
    state->now_usec = now_usec;
    state->base_time_usec = now_usec;
    state->last_discovery_beacon_usec = 0;
    state->warmup_done = false;
    state->initial_scan_done = true;

    moving_average_init(state->average_error_state, state->average_error, int, 32);
}

void nan_timer_set_now_usec(struct nan_timer_state *state, uint64_t now_usec)
{
    state->now_usec = now_usec;
}

uint64_t nan_timer_get_synced_time_usec(const struct nan_timer_state *state, const uint64_t now_usec)
{
    return now_usec - state->base_time_usec;
}

uint64_t nan_timer_get_synced_time_tu(const struct nan_timer_state *state, const uint64_t now_usec)
{
    return USEC_TO_TU(nan_timer_get_synced_time_usec(state, now_usec));
}

uint64_t nan_timer_get_fixed_time_usec(const struct nan_timer_state *state, const uint64_t now_usec)
{
    return now_usec - state->base_time_usec - state->average_error;
}

uint64_t nan_timer_get_fixed_time_tu(const struct nan_timer_state *state, const uint64_t now_usec)
{
    return USEC_TO_TU(nan_timer_get_fixed_time_usec(state, now_usec));
}

void nan_timer_sync_time(struct nan_timer_state *state, const uint64_t now_usec, const uint64_t timestamp)
{
    long diff_usec = (long)nan_timer_get_synced_time_usec(state, now_usec) - timestamp;
    long diff_tu = USEC_TO_TU(diff_usec);
    if (abs(diff_tu) > 3)
        log_debug("High timer diff %ld usec (%ld tu)", diff_usec, diff_tu);

    state->base_time_usec += diff_usec;
}

void nan_timer_sync_error(struct nan_timer_state *state, const uint64_t now_usec, const uint64_t timestamp)
{
    int error_usec = (int)nan_timer_get_synced_time_usec(state, now_usec) - timestamp;

    // Skip too large differences
    if (abs(error_usec) > TU_TO_USEC(NAN_DW_INTERVAL_TU)) {
        log_debug("Error too large: %u", error_usec);
        return;
    }

    moving_average_add(state->average_error_state, state->average_error, int, error_usec);
}

unsigned int nan_time_difference_tu(const uint64_t old_time, const uint64_t new_time)
{
    return USEC_TO_TU(new_time) - USEC_TO_TU(old_time);
}

unsigned int nan_time_difference_dw(const uint64_t old_time, const uint64_t new_time)
{
    return nan_time_difference_tu(old_time, new_time) / NAN_DW_INTERVAL_TU;
}

bool nan_timer_in_dw(const struct nan_timer_state *state, const uint64_t now_usec)
{
    const uint64_t synced_time_tu = nan_timer_get_fixed_time_tu(state, now_usec);
    const uint64_t interval_time_tu = synced_time_tu % (uint64_t)NAN_DW_INTERVAL_TU;
    return interval_time_tu <= NAN_DW_LENGTH_TU;
}

bool nan_timer_in_dw0(const struct nan_timer_state *state, const uint64_t now_usec)
{
    if (!nan_timer_in_dw(state, now_usec))
        return false;

    const uint64_t synced_time_tu = nan_timer_get_fixed_time_tu(state, now_usec);
    const uint32_t interval_count = synced_time_tu / NAN_DW_INTERVAL_TU;
    const uint64_t dw_start_time = interval_count * NAN_DW_INTERVAL_TU;
    return (dw_start_time & 0x7fffff) == 0;
}

bool nan_timer_in_current_dw(const struct nan_timer_state *state,
                             const uint64_t now_usec, const uint64_t target_usec)
{
    return nan_timer_in_dw(state, target_usec) &&
           nan_time_difference_tu(target_usec, now_usec) < NAN_DW_INTERVAL_TU - NAN_DW_LENGTH_TU;
}

int nan_timer_dw_start_offset_tu(const struct nan_timer_state *state, uint64_t now_usec)
{
    const uint64_t synced_time_tu = nan_timer_get_fixed_time_tu(state, now_usec);
    const uint64_t interval_time_tu = synced_time_tu % NAN_DW_INTERVAL_TU;

    if (interval_time_tu > NAN_DW_INTERVAL_TU / 2)
        return (int)interval_time_tu - NAN_DW_INTERVAL_TU;
    else
        return interval_time_tu;
}

uint64_t nan_timer_next_dw_usec(const struct nan_timer_state *state, const uint64_t now_usec)
{
    const uint64_t synced_time_tu = nan_timer_get_fixed_time_tu(state, now_usec);
    const uint64_t interval_tu = (uint64_t)NAN_DW_INTERVAL_TU;
    const uint64_t next_dw_tu = interval_tu - (synced_time_tu % interval_tu);
    return TU_TO_USEC(next_dw_tu);
}

uint64_t nan_timer_dw_end_usec(const struct nan_timer_state *state, uint64_t now_usec)
{
    if (nan_timer_in_dw(state, now_usec))
        return nan_timer_next_dw_usec(state, now_usec - TU_TO_USEC(NAN_DW_LENGTH_TU));

    return nan_timer_next_dw_usec(state, now_usec) + TU_TO_USEC(NAN_DW_LENGTH_TU);
}

bool nan_timer_warmup_expired(struct nan_timer_state *state, const uint64_t now_usec)
{
    if (!state->warmup_done && now_usec - state->base_time_usec >= SEC_TO_USEC(NAN_WARMUP_TIMER_SEC))
    {
        state->warmup_done = true;
        return true;
    }
    return false;
}

void nan_timer_warmup_cancel(struct nan_timer_state *state)
{
    state->warmup_done = true;
}

bool nan_timer_initial_scan_done(const struct nan_timer_state *state, uint64_t now_usec)
{
    return state->initial_scan_done || now_usec - state->base_time_usec > NAN_INITIAL_SCAN_TIMER_USEC;
}

void nan_timer_initial_scan_cancel(struct nan_timer_state *state)
{
    log_debug("Initial scan cancelled");
    state->initial_scan_done = true;
}

bool nan_timer_can_send_discovery_beacon(const struct nan_timer_state *state, const uint64_t now_usec)
{
    return now_usec - state->last_discovery_beacon_usec >= TU_TO_USEC(NAN_DISCOVERY_BEACON_INTERVAL_TU);
}

uint64_t nan_timer_next_discovery_beacon_usec(const struct nan_timer_state *state, const uint64_t now_usec)
{
    if (nan_timer_can_send_discovery_beacon(state, now_usec))
        return 0;

    return now_usec - state->last_discovery_beacon_usec + TU_TO_USEC(NAN_DISCOVERY_BEACON_INTERVAL_TU);
}

void nan_timer_set_last_discovery_beacon_usec(struct nan_timer_state *state, const uint64_t time_usec)
{
    state->last_discovery_beacon_usec = time_usec;
}