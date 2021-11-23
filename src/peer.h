#ifndef CODE_PEER_H
#define CODE_PEER_H

#include <netinet/ether.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdbool.h>

#include "list.h"
#include "moving_average.h"

#define HOST_NAME_LENGTH_MAX 64
#define PEER_DEFAULT_TIMEOUT_USEC TU_TO_USEC(512) * 10
#define PEER_DEFAULT_CLEAN_INTERVAL_USEC TU_TO_USEC(512) * 2
#define PEER_RSSI_BUFFER_SIZE 32

#ifndef RSSI_CLOSE
#define RSSI_CLOSE -60
#endif

#ifndef RSSI_MIDDLE
#define RSSI_MIDDLE -75
#endif

struct nan_peer
{
    struct ether_addr cluster_id;
    struct ether_addr addr;
    struct in6_addr ipv6_addr;

    uint64_t last_update;
    uint64_t last_timestamp;

    uint8_t master_preference;
    uint8_t last_master_preference;
    uint8_t random_factor;

    uint64_t anchor_master_rank;
    uint64_t last_anchor_master_rank;
    uint32_t ambtt;
    uint8_t hop_count;
    bool master_candidate;

    signed char rssi_average;
    moving_average_t rssi_average_state;

    bool availability_all_slots;
    list_t availability_entries;
};

struct nan_peer_availability_entry
{
    // The duration of the availability slot in TU
    int duration_tu;
    // The repeatition period of the avialability slot in TU
    int period_tu;
    // The start offset of the availability slot after DW0 in TU
    int start_offset_tu;
    //
    uint8_t time_bitmap[7];
};

enum peer_status
{
    PEER_ADD = 2,
    PEER_UPDATE = 1,    /* Peer updated */
    PEER_OK = 0,        /* New peer added */
    PEER_MISSING = -1,  /* Peer does not exist */
    PEER_INTERNAL = -2, /* Internal error */
};

typedef void (*nan_peer_callback)(struct nan_peer *peer, void *arg);

struct nan_peer_state
{
    list_t peers;
    uint64_t timeout_usec;
    uint64_t clean_interval_usec;

    /* Allows to hook adding of peers */
    nan_peer_callback peer_add_callback;
    void *peer_add_callback_data;

    /* Allows to hook removing of peers */
    nan_peer_callback peer_remove_callback;
    void *peer_remove_callback_data;
};

/**
 * Init peer state
 */
void nan_peer_state_init(struct nan_peer_state *state);

/**
 * Set callbacks to hook adding and removing of peers.
 * 
 * @param state - The current peers state
 * @param peer_add_callback - Called when a new peer is added
 * @param peer_add_data - Additional data for the add callback 
 * @param peer_remove_callback - Called when a peer is removed
 * @param peer_remove_data - Additional data for the remove callback
 */
void nan_peer_set_callbacks(struct nan_peer_state *state,
                            nan_peer_callback peer_add_callback, void *peer_add_data,
                            nan_peer_callback peer_remove_callback, void *peer_remove_data);

/**
 * Get a peer from the storage matching the give address
 */
enum peer_status nan_peer_get(struct nan_peer_state *state, const struct ether_addr *addr, struct nan_peer **peer);

/** 
 * Adds a peer to the storage if it is not already included
 */
enum peer_status nan_peer_add(struct nan_peer_state *state, const struct ether_addr *addr,
                              const struct ether_addr *cluster_id, uint64_t now);

/**
 * Remove the given peer from the list of known peers
 * 
 * @param state - The current peer state
 * @param peer - The peer to remove
 */
void nan_peer_remove(struct nan_peer_state *state, struct nan_peer *peer);

/**
 * Update the peer's master indication information.
 * 
 * @param peer - The peer to update
 * @param master_preference - Received master preference
 * @param random_factor - Received random factor
 */
void nan_peer_set_master_indication(struct nan_peer *peer,
                                    const uint8_t master_preference,
                                    const uint8_t random_factor);

/**
 * Update the peer's anchor master information.
 * 
 * @param peer - The peer to update
 * @param anchor_master_rank - The received anchor master rank
 * @param ambtt - The received anchor master beacon transmission time
 * @param hop_count - The received an hop count to the anchor master
 */
void nan_peer_set_anchor_master_information(struct nan_peer *peer, const uint64_t anchor_master_rank,
                                            const uint32_t ambtt, const uint8_t hop_count);

/**
 * Update the peer with information from the received beacon
 * 
 * @param peer - The peer to update
 * @param rssi - Received RSSI value
 * @param timestamp - Received timestamp
 */
void nan_peer_set_beacon_information(struct nan_peer *peer, const signed char rssi,
                                     const uint64_t timestamp);

/**
 * Removes all peers that were last updated before the timeout.
 * Invokes the remove callback if present in the state.
 * 
 * @param state - The current peers state, including the used timeout value
 * @param now_usec - The current time in microseconds
 */
void nan_peers_clean(struct nan_peer_state *state, uint64_t now_usec);

#endif //CODE_PEER_H