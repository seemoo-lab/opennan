#include "sync.h"

#include <stdio.h>
#include <endian.h>

#include "utils.h"
#include "log.h"

union nan_master_rank
{
    uint64_t value;
    struct
    {
#if __BYTE_ORDER == __LITTLE_ENDIAN
        struct ether_addr address;
        uint8_t random_factor;
        uint8_t master_preference;
#else
        uint8_t master_preference;
        uint8_t random_factor;
        struct ether_addr address;
#endif
    } __attribute__((__packed__)) parts;
};

uint64_t nan_calculate_master_rank(const uint8_t master_preference,
                                   const uint8_t random_factor,
                                   const struct ether_addr *address)
{
    union nan_master_rank master_rank = {
        .parts = {
            .address = *address,
            .master_preference = master_preference,
            .random_factor = random_factor}};

    return master_rank.value;
}

struct ether_addr *nan_get_address_from_master_rank(const uint64_t *master_rank)
{
    return &((union nan_master_rank *)master_rank)->parts.address;
}

bool nan_is_master_rank_issuer(const struct ether_addr *addr, const uint64_t master_rank)
{
    return ether_addr_equal(addr, nan_get_address_from_master_rank(&master_rank));
}

bool nan_is_same_master_rank_issuer(const uint64_t master_rank1, const uint64_t master_rank2)
{
    return nan_is_master_rank_issuer(nan_get_address_from_master_rank(&master_rank1), master_rank2);
}

bool nan_is_anchor_master(const struct nan_sync_state *state, const struct ether_addr *addr)
{
    return nan_is_master_rank_issuer(addr, state->anchor_master_rank);
}

bool nan_is_anchor_master_self(const struct nan_sync_state *state)
{
    return nan_is_same_master_rank_issuer(state->master_rank,
                                          state->anchor_master_rank);
}

struct ether_addr *nan_get_anchor_master_address(const struct nan_sync_state *state)
{
    return nan_get_address_from_master_rank(&state->anchor_master_rank);
}

void nan_sync_state_init(struct nan_sync_state *state,
                         const struct ether_addr *interface_address)
{
    state->role = MASTER;
    state->master_rank = nan_calculate_master_rank(0, 0, interface_address);
    state->master_preference = 0;
    state->random_factor = 0;

    state->last_master_preference_update_usec = 0;
    state->last_random_factor_update_usec = 0;

    state->anchor_master_rank = state->master_rank;
    state->ambtt = 0;
    state->hop_count = 0;

    state->last_anchor_master_rank = state->anchor_master_rank;
    state->last_ambtt = 0;
    state->last_hop_count = 0;

    state->count_dws_without_ambtt_update = 0;
    state->count_dws_without_hop_count_update = 0;
}

void nan_check_master_candidate(const struct nan_sync_state *state, struct nan_peer *peer)
{
    peer->master_candidate = false;
    if (peer->anchor_master_rank != state->anchor_master_rank)
        return;

    if (peer->hop_count < state->hop_count)
        peer->master_candidate = true;

    if (peer->hop_count == state->hop_count && nan_get_peer_master_rank(peer) > state->master_rank)
        peer->master_candidate = true;
}

void nan_master_election(struct nan_sync_state *state, const list_t peers, const uint64_t now_usec)
{
    struct nan_peer *peer;
    int count_higher_mr = 0;
    int count_rssi_close = 0;
    int count_rssi_middle = 0;
    int count_rssi_close_higher_mr = 0;
    int count_rssi_close_master_candidate = 0;
    int count_rssi_middle_higher_mr = 0;
    int count_rssi_middle_master_candidate = 0;

    LIST_FOR_EACH(peers, peer, {
        uint64_t peer_master_rank = nan_get_peer_master_rank(peer);

        // Asume currently at the end of a DW
        // Check if last update was in the current DW plus 4 TU guard
        log_debug("%d", USEC_TO_TU(now_usec - peer->last_update));
        if (now_usec - peer->last_update > TU_TO_USEC(NAN_DW_LENGTH_TU + 4)) 
            continue;

        if (peer->rssi_average > RSSI_CLOSE)
        {
            count_rssi_close++;

            if (peer_master_rank > state->master_rank)
                count_rssi_close_higher_mr++;

            if (peer->master_candidate)
                count_rssi_close_master_candidate++;
        }

        if (peer->rssi_average > RSSI_MIDDLE)
        {
            count_rssi_middle++;

            if (peer_master_rank > state->master_rank)
                count_rssi_middle_higher_mr++;

            if (peer->master_candidate)
                count_rssi_middle_master_candidate++;
        }

        if (peer_master_rank > state->master_rank)
            count_higher_mr++;
    })

    if (state->role == MASTER)
    {
        // master -> sync
        if (count_rssi_close_higher_mr >= 1 || count_rssi_middle_higher_mr >= 3)
        {
            log_debug("master election: transition from master to sync");
            state->role = SYNC;
        }
    }
    else
    {
        // non-master -> master
        if (count_rssi_close == 0 && count_higher_mr > 0)
        {
            log_debug("master election: transition from non-master to master");
            state->role = MASTER;
        }
    }

    if (state->role == SYNC)
    {
        // sync -> non-sync
        if (count_rssi_close_master_candidate >= 1 ||
            count_rssi_middle_master_candidate >= 3)
        {
            log_debug("master election: transition from sync to non-sync");
            state->role = NON_SYNC;
        }
    }
    else
    {
        // non-sync -> sync
        if (count_rssi_close_master_candidate == 0 && count_rssi_middle_master_candidate < 3)
        {
            log_debug("master election: transition from non-sync to sync");
            state->role = SYNC;
        }
    }
}

void nan_save_current_anchor_master(struct nan_sync_state *state)
{
    state->last_anchor_master_rank = state->anchor_master_rank;
    state->last_ambtt = state->ambtt;
}

void nan_set_anchor_master_other(struct nan_sync_state *state, const struct nan_peer *peer)
{
    nan_save_current_anchor_master(state);
    state->anchor_master_rank = peer->anchor_master_rank;
    state->ambtt = peer->ambtt;
    state->hop_count = peer->hop_count + 1;
}

void nan_set_anchor_master_self(struct nan_sync_state *state)
{
    nan_save_current_anchor_master(state);
    state->anchor_master_rank = state->master_rank;
    state->ambtt = 0;
    state->hop_count = 0;
}

void nan_anchor_master_selection(struct nan_sync_state *state,
                                 const struct nan_peer *peer,
                                 const uint64_t synced_time_tu)
{
    if (state->anchor_master_rank == peer->anchor_master_rank &&
        peer->ambtt <= synced_time_tu * 16 * 512)
    {
        log_trace("anchor master selection: received outdated amr");
        return;
    }

    if (nan_is_anchor_master_self(state))
    {
        if (state->master_rank >= peer->anchor_master_rank)
        {
            log_debug("anchor master selection: own mr > received amr");
            return;
        }
        if (nan_is_same_master_rank_issuer(state->master_rank, peer->anchor_master_rank))
        {
            log_debug("anchor master selection: received own amr");
            return;
        }

        log_debug("anchor master selection: received better amr");
        nan_set_anchor_master_other(state, peer);
    }

    // !state->anchor_master_self

    if (state->anchor_master_rank < peer->anchor_master_rank)
    {
        if (state->last_anchor_master_rank == peer->anchor_master_rank &&
            state->last_ambtt >= peer->ambtt)
        {
            log_debug("anchor master selection: received outdated amr");
            return;
        }

        log_debug("anchor master selection: received better amr from current am");
        nan_set_anchor_master_other(state, peer);
        return;
    }

    if (state->anchor_master_rank > peer->anchor_master_rank)
    {
        if (!nan_is_same_master_rank_issuer(state->anchor_master_rank, peer->anchor_master_rank))
        {
            log_debug("anchor master selection: received lower amr from non-am peer");
            return;
        }

        if (state->master_rank > peer->anchor_master_rank)
        {
            log_debug("anchor master selection: received amr from current am lower than own mr");
            nan_set_anchor_master_self(state);
            return;
        }

        log_debug("anchor master selection: received lower amr from current am");
        nan_set_anchor_master_other(state, peer);
        return;
    }

    // state->anchor_master_rank == anchor_master_rank

    if (state->ambtt < peer->ambtt)
    {
        state->ambtt = peer->ambtt;
    }
    if (state->hop_count > (peer->hop_count + 1))
        state->hop_count = peer->hop_count;

    log_debug("anchor master selection: no change");
}

void nan_check_anchor_master_expiration(struct nan_sync_state *state)
{
    if (nan_is_anchor_master_self(state))
    {
        state->count_dws_without_ambtt_update = 0;
        state->count_dws_without_hop_count_update = 0;
        return;
    }

    if (state->hop_count == state->last_hop_count)
        state->count_dws_without_hop_count_update++;
    else
        state->count_dws_without_hop_count_update = 0;

    if (state->ambtt == state->last_ambtt)
        state->count_dws_without_ambtt_update++;
    else
        state->count_dws_without_ambtt_update = 0;

    // Check counts
    if (state->count_dws_without_ambtt_update >= 3)
    {
        log_debug("Received no ambtt update for 3 dws, set master self");
        nan_set_anchor_master_self(state);
    }
    else if (state->count_dws_without_hop_count_update >= 3)
    {
        log_debug("Received no better hop count for 3 dws but ambtt update");
        // No better hop count in 3 DWs, but at least on ambtt update
        state->hop_count = 255;
    }
}

bool nan_update_master_rank(struct nan_sync_state *state,
                            const struct ether_addr *interface_address)
{
    if (interface_address == NULL)
        interface_address = nan_get_address_from_master_rank(&state->master_rank);

    uint64_t new_master_rank = nan_calculate_master_rank(state->master_preference,
                                                         state->random_factor,
                                                         interface_address);

    if (new_master_rank == state->master_rank)
        return false;

    state->master_rank = new_master_rank;
    if (nan_is_anchor_master_self(state))
    {
        state->last_anchor_master_rank = state->anchor_master_rank;
        state->anchor_master_rank = state->master_rank;
    }
    else if (state->master_rank > state->anchor_master_rank)
    {
        log_debug("nan_update_master_rank: master self");
        nan_set_anchor_master_self(state);
    }

    log_debug("New master rank %lu", new_master_rank);
    return true;
}

bool nan_update_master_preference(struct nan_sync_state *state, const struct nan_peer *peer,
                                  const uint64_t now_usec)
{
    if (peer->master_preference == peer->last_master_preference)
        return false;

    const unsigned int elapsed_dws = nan_time_difference_dw(
        state->last_master_preference_update_usec, now_usec);

    bool updated = false;
    if (elapsed_dws > NAN_RANDOM_FACTOR_UPDATE_MIN_DW)
    {
        state->random_factor = get_rand_num(0, 255);
        state->last_random_factor_update_usec = now_usec;
        updated = true;
    }

    if (elapsed_dws > NAN_MASTER_PREFERENCE_UPDATE_MIN_DW)
    {
        state->master_preference = NAN_MASTER_PREFERENCE;
        state->last_master_preference_update_usec = now_usec;
        updated = true;
    }

    return updated && nan_update_master_rank(state, NULL);
}

char *nan_role_to_string(enum nan_role role)
{
    switch (role)
    {
    case MASTER:
        return "MASTER";
    case SYNC:
        return "NON MASTER SYNC";
    case NON_SYNC:
        return "NON MASTER NON SYNC";
    default:
        return "unknown role";
    }
}