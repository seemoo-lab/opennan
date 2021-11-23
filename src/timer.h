#ifndef NAN_TIMER_H_
#define NAN_TIMER_H_

#include <stdint.h>
#include <stdbool.h>

#include "moving_average.h"

// The length of a discovery window in time units
#define NAN_DW_LENGTH_TU 16
// The interval between the start of two discovery windows in time units
#define NAN_DW_INTERVAL_TU 512
// Offset to reduce wrong dw calculation due to user space timing delay
#define NAN_OFFSET_DW_LENGTH_TU 2
// The length of the warmup timer in seconds
#define NAN_WARMUP_TIMER_SEC 120
// The length of the initial passive scan in microseconds
#define NAN_INITIAL_SCAN_TIMER_USEC 1000000

struct nan_timer_state
{
    uint64_t now_usec;
    // The devices base time synced with the current cluster
    uint64_t base_time_usec;

    // Time of the last send discovery beacon
    uint64_t last_discovery_beacon_usec;
    // Whether the warmup is done
    bool warmup_done;
    // Whether the initial scan is done
    bool initial_scan_done;

    int average_error;
    moving_average_t *average_error_state;
};

/**
 * Initialize the timer state.
 * 
 * @param state - The timer state to initialize
 * @param now_usec - The current time in miliseconds
 */
void nan_timer_state_init(struct nan_timer_state *state, const uint64_t now_usec,
                          moving_average_t *moving_average_state);

/**
 * Get the synchronized time value in microseconds.
 * 
 * @param state - The current timer state
 * @param now_usec - The current time in microseconds
 * @returns The synchronized time value in microseconds.
 */
uint64_t nan_timer_get_synced_time_usec(const struct nan_timer_state *state, const uint64_t now_usec);

/**
 * Get the synchronized time value in time units.
 * 
 * @param state - The current timer state
 * @param now_usec - The current time in microseconds
 * @returns The synchronized time value in time units.
 */
uint64_t nan_timer_get_synced_time_tu(const struct nan_timer_state *state, const uint64_t now_usec);

/**
 * Get the synchronized time value with error correction in microseconds.
 * 
 * @param state - The current timer state
 * @param now_usec - The current time in microseconds
 * @returns The synchronized time value in microseconds.
 */
uint64_t nan_timer_get_fixed_time_usec(const struct nan_timer_state *state, const uint64_t now_usec);

/**
 * Get the synchronized time value with error correction in time units.
 * 
 * @param state - The current timer state
 * @param now_usec - The current time in microseconds
 * @returns The synchronized time value in time units.
 */
uint64_t nan_timer_get_fixed_time_tu(const struct nan_timer_state *state, const uint64_t now_usec);

/**
 * Sync the timer with a received value.
 * 
 * @param state - The current timer state
 * @param now_usec - The current time in microseconds
 * @param timestamp - The received timestamp value
 */
void nan_timer_sync_time(struct nan_timer_state *state, const uint64_t now_usec, const uint64_t timestamp);

/**
 * Calculate the current time offset and update the error correction accordingly.
 * @param state - The current timer state
 * @param now_usec - The current time in microseconds
 * @param timestamp - The received timestamp value
 */
void nan_timer_sync_error(struct nan_timer_state* state, const uint64_t now_usec, const uint64_t timestamp);

/**
 * Get the floored time difference in time units.
 * 
 * @param old_time - The older time value
 * @param new_time - The newer time value
 * @returns The floored difference in time units
 */
unsigned int nan_time_difference_tu(const uint64_t old_time, const uint64_t new_time);

/**
 * Get the count of DWs between the given time values.
 * 
 * @param old_time - The older time value
 * @param new_time - The newer time value
 * @returns The count of DWs 
 */
unsigned int nan_time_difference_dw(const uint64_t old_time, const uint64_t new_time);

/**
 * Check if we are currently in a discovery window.
 * 
 * @param state - The current timer state
 * @param now_usec - The current time in microseconds
 * @returns Whether we are in a discovery window 
 */
bool nan_timer_in_dw(const struct nan_timer_state *state, uint64_t now_usec);

/**
 * Check if we are currently in the discovery window 0.
 * 
 * @param state - The current timer state
 * @param now_usec - The current time in microseconds
 * @returns Whether we are in a discovery window 
 */
bool nan_timer_in_dw0(const struct nan_timer_state *state, uint64_t now_usec);

/**
 * Check if the given time is within the current discovery window.
 * 
 * @param state - The current timer state
 * @param now_usec - The current time in microseconds
 * @param target_usec - The target time in microseconds
 * @returns Whether the target time is within the current discovery window
 */
bool nan_timer_in_current_dw(const struct nan_timer_state *state,
                             const uint64_t now_usec, const uint64_t target_usec);

/**
 * Get the offset of the current time to the start of the current dw
 * 
 * @param state - The current timer state
 * @param now_usec - The curren time in microseconds
 * @returns The difference between the start of the current dw and the current time
 */
int nan_timer_dw_start_offset_tu(const struct nan_timer_state *state, uint64_t now_usec);

/** 
 * Returns time to start of next discovery window in microseconds.
 *
 * @param state - The current timer state
 * @param now_usec - The current time in microseconds
 * @returns - microseconds
 */
uint64_t nan_timer_next_dw_usec(const struct nan_timer_state *state, uint64_t now_usec);

/**
 * Returns time to end of current discovery window in microseconds.
 * 
 * @param state - The current timer state
 * @param now_usec - The current time in microseconds
 * @returns - microseconds
 */
uint64_t nan_timer_dw_end_usec(const struct nan_timer_state *state, uint64_t now_usec);

/**
 * Checks whether the warmup timer is expired and sets the warmup_done flag in this case.
 * Will only once return true.
 * 
 * @param state - The current timer state
 * @param now_usec - The current time in microseconds
 * @returns 
 */
bool nan_timer_warmup_expired(struct nan_timer_state *state, uint64_t now_usec);

/**
 * Cancel the nan warmup timer.
 * 
 * @param state - The current timer state
 */
void nan_timer_warmup_cancel(struct nan_timer_state *state);

/**
 * Check if the initial scan is done.
 * 
 * @param state - The current timer state
 * @param now_usec - The current time in microseconds
 * @returns Whether the the initial scan is done
 */
bool nan_timer_initial_scan_done(const struct nan_timer_state *state, uint64_t now_usec);

/**
 * Cancel initial scan.
 * 
 * @param state - The current timer state
 */
void nan_timer_initial_scan_cancel(struct nan_timer_state *state);

/**
 * Return whether we are currently allowed to send a discovery beacon.
 * 
 * @param state - The current timer state
 * @param now_usec - The current time in microseconds
 * @returns
 */
bool nan_timer_can_send_discovery_beacon(const struct nan_timer_state *state, uint64_t now_usec);

/**
 * Get the time to the start of the next discovery beacon in microseconds.
 * 
 * @param state - The current timer state
 * @param now_usec - The current time in microseconds
 * @returns - The time to the next discovery beacon slot
 */
uint64_t nan_timer_next_discovery_beacon_usec(const struct nan_timer_state *state, uint64_t now_usec);

/**
 * Set the time for the last discovery beacon.
 * 
 * @param state - The current timer state
 * @param time_usec - The time in microseconds
 */
void nan_timer_set_last_discovery_beacon_usec(struct nan_timer_state *state, const uint64_t time_usec);

#endif // NAN_TIMER_H_