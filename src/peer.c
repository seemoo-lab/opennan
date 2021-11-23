#include "peer.h"

#include <stdlib.h>

#include "utils.h"
#include "log.h"

void nan_peer_state_init(struct nan_peer_state *state)
{
    state->peers = list_init();
    state->timeout_usec = PEER_DEFAULT_TIMEOUT_USEC;
    state->clean_interval_usec = PEER_DEFAULT_CLEAN_INTERVAL_USEC;

    state->peer_add_callback = NULL;
    state->peer_add_callback_data = NULL;
    state->peer_remove_callback = NULL;
    state->peer_remove_callback_data = NULL;
}

void nan_peer_set_callbacks(struct nan_peer_state *state,
                            nan_peer_callback peer_add_callback, void *peer_add_data,
                            nan_peer_callback peer_remove_callback, void *peer_remove_data)
{
    state->peer_add_callback = peer_add_callback;
    state->peer_add_callback_data = peer_add_data;
    state->peer_remove_callback = peer_remove_callback;
    state->peer_remove_callback_data = peer_remove_data;
}

struct nan_peer *nan_peer_new(const struct ether_addr *addr, const struct ether_addr *cluster_id)
{
    struct nan_peer *peer = malloc(sizeof(struct nan_peer));
    peer->addr = *addr;
    peer->cluster_id = *cluster_id;
    ether_addr_to_ipv6_addr(addr, &peer->ipv6_addr);

    peer->last_update = 0;
    peer->last_timestamp = 0;

    peer->master_preference = 0;
    peer->last_master_preference = 0;
    peer->random_factor = 0;

    peer->anchor_master_rank = 0;
    peer->ambtt = 0;
    peer->hop_count = 0;
    peer->master_candidate = false;

    moving_average_init(peer->rssi_average_state, peer->rssi_average,
                        signed char, PEER_RSSI_BUFFER_SIZE);

    return peer;
}

enum peer_status nan_peer_get(struct nan_peer_state *state, const struct ether_addr *addr, struct nan_peer **peer)
{
    LIST_FIND(state->peers, *peer, ether_addr_equal(&((*peer)->addr), addr))
    if (*peer)
        return PEER_OK;
    return PEER_MISSING;
}

enum peer_status nan_peer_add(struct nan_peer_state *state, const struct ether_addr *addr,
                              const struct ether_addr *cluster_id, uint64_t now_usec)
{
    struct nan_peer *peer = NULL;
    int status = nan_peer_get(state, addr, &peer);
    if (status == PEER_MISSING)
    {
        peer = nan_peer_new(addr, cluster_id);
        if (state->peer_add_callback != NULL)
            state->peer_add_callback(peer, state->peer_add_callback_data);

        log_debug("Added peer %s from cluster %s",
                  ether_addr_to_string(addr),
                  ether_addr_to_string(cluster_id));
    }

    peer->last_update = now_usec;

    if (status == PEER_MISSING)
    {
        list_add(state->peers, (any_t)peer);
        return PEER_ADD;
    }

    if (!ether_addr_equal(&peer->cluster_id, cluster_id))
    {
        log_debug("Updated cluster id of peer %s to %s",
                  ether_addr_to_string(&peer->addr), ether_addr_to_string(&peer->cluster_id));
        peer->cluster_id = *cluster_id;
    }

    return PEER_UPDATE;
}

void nan_peer_set_master_indication(struct nan_peer *peer,
                                    const uint8_t master_preference,
                                    const uint8_t random_factor)
{
    peer->last_master_preference = peer->master_preference;
    peer->master_preference = master_preference;
    peer->random_factor = random_factor;
}

void nan_peer_set_anchor_master_information(struct nan_peer *peer, const uint64_t anchor_master_rank,
                                            const uint32_t ambtt, const uint8_t hop_count)
{
    peer->last_anchor_master_rank = peer->anchor_master_rank;

    peer->anchor_master_rank = anchor_master_rank;
    peer->hop_count = hop_count;

    if (hop_count == 0)
        peer->ambtt = peer->last_timestamp;
    else
        peer->ambtt = ambtt;
}

void nan_peer_set_beacon_information(struct nan_peer *peer, const signed char rssi,
                                     const uint64_t timestamp)
{
    moving_average_add(peer->rssi_average_state, peer->rssi_average, signed char, rssi);
    peer->last_timestamp = timestamp;
}

void nan_peer_remove(struct nan_peer_state *state, struct nan_peer *peer)
{
    list_remove(state->peers, (any_t)peer);
    state->peer_remove_callback(peer, state->peer_remove_callback_data);
    free(peer);
}

void nan_peers_clean(struct nan_peer_state *state, uint64_t now_usec)
{
    struct nan_peer *peer;
    do
    {
        LIST_FIND(state->peers, peer, peer->last_update < now_usec - state->timeout_usec)
        if (peer)
            nan_peer_remove(state, peer);
    } while (peer);
}
