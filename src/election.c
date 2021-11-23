#include "election.h"
#include "timer.h"
#include "utils.h"
#include "log.h"

uint64_t nan_calculate_master_rank(const uint8_t master_preference,
                                   const uint8_t random_factor,
                                   const struct ether_addr *interface_address)
{
    uint64_t master_rank = master_preference;
    master_rank <<= 8;
    master_rank += random_factor;
    master_rank <<= 8;
    master_rank += interface_address->ether_addr_octet[5];
    master_rank <<= 8;
    master_rank += interface_address->ether_addr_octet[4];
    master_rank <<= 8;
    master_rank += interface_address->ether_addr_octet[3];
    master_rank <<= 8;
    master_rank += interface_address->ether_addr_octet[2];
    master_rank <<= 8;
    master_rank += interface_address->ether_addr_octet[1];
    master_rank <<= 8;
    master_rank += interface_address->ether_addr_octet[0];

    return master_rank;
}

bool nan_anchor_master_rank_equal(const struct nan_election_state *state, const uint64_t anchor_master_rank)
{
    return (state->anchor_master_rank & 0x0000ffffffffffff) == (anchor_master_rank & 0x0000ffffffffffff);
}

void nan_election_state_init(struct nan_election_state *state, const struct ether_addr *interface_address)
{
    state->master_state = MASTER;
    state->master_preference = 254;
    state->random_factor = 254;
    state->master_rank = nan_calculate_master_rank(state->master_preference,
                                                   state->random_factor, interface_address);

    state->anchor_master_rank = state->master_rank;
    state->anchor_master_beacon_transmission_time = 0;
    state->hop_count = 0;
    state->last_anchor_master_rank = 0;
    state->last_anchor_master_beacon_transmission_time = 0;

    state->ambtt_updated_in_dw = false;
    state->count_dws_without_ambtt_update = 0;
    state->received_better_hop_count_in_dw = false;
    state->count_dws_without_better_hop_count = 0;
}

void nan_save_current_anchor_master(struct nan_election_state *state)
{
    state->last_anchor_master_rank = state->anchor_master_rank;
    state->last_anchor_master_beacon_transmission_time = state->anchor_master_beacon_transmission_time;
}

void nan_set_anchor_master_other(struct nan_election_state *state, struct nan_election_data *data)
{
    nan_save_current_anchor_master(state);
    state->anchor_master_rank = data->anchor_master_rank;
    state->anchor_master_beacon_transmission_time = data->anchor_master_beacon_transmission_time;
    state->hop_count = data->hop_count + 1;
    state->anchor_master_address = *data->peer_address;
    state->master_state = NON_MASTER_SYNC;

    state->ambtt_updated_in_dw = true;
}

void nan_set_anchor_master_self(struct nan_election_state *state, const bool is_new_cluster)
{
    // TODO becomming master after DW ends
    state->master_state = MASTER;

    if (is_new_cluster)
    {
        state->last_anchor_master_rank = state->master_rank;
        state->last_anchor_master_beacon_transmission_time = 0;
    }
    else
    {
        nan_save_current_anchor_master(state);
    }

    state->anchor_master_rank = state->master_rank;
    state->anchor_master_beacon_transmission_time = 0;
    state->hop_count = 0;
}

enum nan_election_result nan_election_as_master(struct nan_election_state *state, struct nan_election_data *data)
{
    if (state->master_rank >= data->anchor_master_rank)
    {
        log_trace("nan_election_master disregarded: owm >= amr");
        return DISREGARDED;
    }
    if (nan_anchor_master_rank_equal(state, data->anchor_master_rank))
    {
        log_debug("nan_election_master disregarded: special equal amr");
        return DISREGARDED;
    }

    log_debug("nan_election_master master other");
    nan_set_anchor_master_other(state, data);
    return MASTER_OTHER;
}

enum nan_election_result nan_election_as_non_master(struct nan_election_state *state, struct nan_election_data *data)
{
    if (state->anchor_master_rank < data->anchor_master_rank)
    {
        if (state->last_anchor_master_rank == data->anchor_master_rank &&
            state->last_anchor_master_beacon_transmission_time >= data->anchor_master_beacon_transmission_time)
        {
            log_debug("nan_election_non_master disregarded: stored < amr && amr == last && stored >= ambtt");
            return DISREGARDED;
        }

        log_debug("nan_election_non_master no change: stored < amr");
        nan_set_anchor_master_other(state, data);
        return NO_CHANGE;
    }

    if (state->anchor_master_rank > data->anchor_master_rank)
    {
        if (!nan_anchor_master_rank_equal(state, data->anchor_master_rank))
        {
            log_debug("nan_election_non_master disregarded: stored > amr && !equal");
            return DISREGARDED;
        }

        if (state->master_rank > data->anchor_master_rank)
        {
            log_debug("nan_election_non_master master self: stored > amr && owm > amr");
            nan_set_anchor_master_self(state, false);
            return MASTER_SELF;
        }

        log_debug("nan_election_non_master master other: stored > amr && owm <= amr");
        nan_set_anchor_master_other(state, data);
        return MASTER_OTHER;
    }

    // state->anchor_master_rank == anchor_master_rank

    if (state->anchor_master_beacon_transmission_time < data->anchor_master_beacon_transmission_time)
    {
        state->anchor_master_beacon_transmission_time = data->anchor_master_beacon_transmission_time;
        state->ambtt_updated_in_dw = true;
    }
    if (state->hop_count > (data->hop_count + 1))
        state->hop_count = data->hop_count;

    log_debug("nan_election_non_master no change: stored == amr");
    return NO_CHANGE;
}

enum nan_election_result nan_election(struct nan_election_state *state,
                                      struct nan_election_data *data,
                                      const uint64_t synced_time_usec)
{
    if (data->hop_count == 0)
    {
        data->anchor_master_beacon_transmission_time = synced_time_usec;
    }

    uint64_t synced_time_tu = USEC_TO_TU(synced_time_usec);
    if (state->anchor_master_rank == data->anchor_master_rank &&
        data->anchor_master_beacon_transmission_time <= synced_time_tu * 16 * 512)
    {
        log_trace("nan_election: discard");
        return DISREGARDED;
    }

    if (data->hop_count < state->hop_count)
        state->received_better_hop_count_in_dw = true;

    if (state->master_state == MASTER)
        return nan_election_as_master(state, data);
    else
        return nan_election_as_non_master(state, data);
}

bool nan_update_master_rank(struct nan_election_state *state, const struct ether_addr *interface_address)
{
    const uint64_t new_master_rank = nan_calculate_master_rank(state->master_preference, state->random_factor, interface_address);

    if (new_master_rank == state->master_rank)
        return false;

    state->master_rank = new_master_rank;
    if (state->master_state == MASTER)
    {
        state->last_anchor_master_rank = state->anchor_master_rank;
        state->anchor_master_rank = state->master_rank;
    }
    else if (state->master_rank > state->anchor_master_rank)
    {
        log_debug("nan_update_master_rank: master self");
        nan_set_anchor_master_self(state, false);
    }

    log_debug("New master rank %lu", new_master_rank);
    return true;
}

bool nan_update_master_preference(struct nan_election_state *state, const uint64_t now_usec)
{
    const unsigned int elapsed_dws = nan_time_difference_dw(state->last_master_preference_update_usec, now_usec);

    if (elapsed_dws > NAN_MASTER_PREFERENCE_UPDATE_MIN_DW)
    {
        state->master_preference = NAN_MASTER_PREFERENCE;
        state->last_master_preference_update_usec = now_usec;
        return true;
    }

    return false;
}

bool nan_update_random_factor(struct nan_election_state *state, const uint64_t now_usec)
{
    const unsigned int elapsed_dws = nan_time_difference_dw(state->last_random_factor_update_usec, now_usec);

    if (elapsed_dws > NAN_RANDOM_FACOTR_UPDATE_MIN_DW)
    {
        state->random_factor = get_rand_num(0, 255);
        state->last_random_factor_update_usec = now_usec;
        return true;
    }

    return false;
}

void nan_check_anchor_master_expiration(struct nan_election_state *state)
{
    if (state->master_state == MASTER)
    {
        state->count_dws_without_ambtt_update = 0;
        state->count_dws_without_better_hop_count = 0;
        return;
    }

    if (state->received_better_hop_count_in_dw)
        state->count_dws_without_better_hop_count = 0;
    else
        state->count_dws_without_better_hop_count++;

    if (state->ambtt_updated_in_dw)
        state->count_dws_without_ambtt_update = 0;
    else
        state->count_dws_without_ambtt_update++;

    // Reset flags
    state->received_better_hop_count_in_dw = false;
    state->ambtt_updated_in_dw = false;

    // Check counts
    if (state->count_dws_without_ambtt_update >= 3)
    {
        log_debug("Received no ambtt update for 3 dws, set master self");
        // No ambtt update in 3 DWs
        nan_set_anchor_master_self(state, false);
    }
    else if (state->count_dws_without_better_hop_count >= 3)
    {
        log_debug("Received no better hop count for 3 dws but ambtt update");
        // No better hop count in 3 DWs, but at least on ambtt update
        state->hop_count = 255;
    }
}

char *nan_master_state_to_string(enum nan_master_state master_state)
{
    switch (master_state)
    {
    case MASTER:
        return "MASTER";
    case NON_MASTER_SYNC:
        return "NON MASTER SYNC";
    case NON_MASTER_NON_SYNC:
        return "NON MASTER NON SYNC";
    default:
        return "unknown master state";
    }
}