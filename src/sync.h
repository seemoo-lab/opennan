#ifndef NAN_SYNC_H_
#define NAN_SYNC_H_

#include <stdint.h>
#include <stdbool.h>
#include <netinet/ether.h>

#include "timer.h"
#include "list.h"
#include "peer.h"

#define NAN_MASTER_PREFERENCE_UPDATE_MIN_DW 240
#define NAN_RANDOM_FACTOR_UPDATE_MIN_DW 120
#define NAN_RANDOM_FACTOR_UPDATE_MAX_DW 240

#ifndef NAN_MASTER_PREFERENCE
#define NAN_MASTER_PREFERENCE 200;
#endif

enum nan_role
{
    NON_SYNC,
    SYNC,
    MASTER,
};

struct nan_sync_state
{
    enum nan_role role;
    uint64_t master_rank;
    uint8_t master_preference;
    uint8_t random_factor;

    uint64_t last_master_preference_update_usec;
    uint64_t last_random_factor_update_usec;

    uint64_t anchor_master_rank;
    uint32_t ambtt;
    uint8_t hop_count;

    uint64_t last_anchor_master_rank;
    uint32_t last_ambtt;
    uint8_t last_hop_count;

    uint8_t count_dws_without_ambtt_update;
    uint8_t count_dws_without_hop_count_update;
};

/**
 * Initiate the sync state with default values.
 * 
 * @param state - The sync state to init
 * @param interface_address - The current address of the used ethernet interface
 */
void nan_sync_state_init(struct nan_sync_state *state,
                         const struct ether_addr *interface_address);

/**
 * Calculate the master rank.
 * 
 * @param master_preference - The device's master preference
 * @param random_factor - The device's random factor
 * @param address - The device's ethernet address
 * @return The calculated master rank
 */
uint64_t nan_calculate_master_rank(const uint8_t master_preference, const uint8_t random_factor,
                                   const struct ether_addr *address);

#define nan_get_peer_master_rank(peer) \
    nan_calculate_master_rank(peer->master_preference, peer->random_factor, &peer->addr)

/**
 * Update the master rank after the devices interface address has changed
 * 
 * @param state - The current sync state
 * @param interface_address - The current address of the used ethernet interface
 * @returns Whether the master rank was updated
 */
bool nan_update_master_rank(struct nan_sync_state *state,
                            const struct ether_addr *interface_address);

/**
 * Updates master preference and random factor if allowed.
 * 
 * @param state - The current sync state
 * @param peer - The peer that has transmitted the beacon
 * @param now_usec - The current time in microseconds
 * @returns Whether the master preference was updated 
 */
bool nan_update_master_preference(struct nan_sync_state *state, const struct nan_peer *peer,
                                  const uint64_t now_usec);

/**
 * Check whether the given peer is a candidate to become a Master.
 * Will set the corresponding flag of the peer.
 * 
 * @param state - The current sync state
 * @param peer - The peer to check and update
 */
void nan_check_master_candidate(const struct nan_sync_state *state, struct nan_peer *peer);

/**
 * Perform master election
 * 
 * @param state - The current sync state
 * @param peers - The list of known peers
 * @param now_usec - The current time in microseconds
 */
void nan_master_election(struct nan_sync_state *state, const list_t peers, const uint64_t now_usec);

/**
 * Get the address of the peer that has issued the master rank
 * 
 * @param master_rank
 * @returns The issuer's ethernet address
 */
struct ether_addr *nan_get_address_from_master_rank(const uint64_t *master_rank);

/**
 * Check whether the device behind given address is the current anchor master.
 * 
 * @param state - The current sync state
 * @param address - The address to check
 * @returns Whether the given address is currently the anchor master
 */
bool nan_is_anchor_master(const struct nan_sync_state *state, const struct ether_addr* addr);

/**
 * Check whether we are currently the anchor master.
 * 
 * @param state - The current sync state
 * @returns Whether we are currently the anchor master
 */
bool nan_is_anchor_master_self(const struct nan_sync_state *state);

/**
 * Check if the given address was the issuer of the given master rank.
 * 
 * @param address - The ethernet address
 * @param master_rank - The master rank
 * @returns Whether the address has issued the master rank
 */
bool nan_is_master_rank_issuer(const struct ether_addr *addr, const uint64_t master_rank);

/**
 * Check whether the given master ranks were issued by the same peer.
 * 
 * @param master_rank1 
 * @param master_rank2
 * @returns Whether the given master ranks were issued by the same peer
 */
bool nan_is_same_master_rank_issuer(const uint64_t master_rank1, const uint64_t master_rank2);


/**
 * Get the current anchor master address
 * 
 * @param state - The current sync state
 * @returns The current anchor master's address
 */
struct ether_addr *nan_get_anchor_master_address(const struct nan_sync_state *state);

/**
 * Conduct the anchor master selection using the values of a received sync beacon.
 * 
 * @param state - The current sync state
 * @param peer - The peer that has transmitted the sync beacon
 * @param synced_time_tu - The synced time in TU
 */
void nan_anchor_master_selection(struct nan_sync_state *state, const struct nan_peer *peer,
                                 const uint64_t synced_time_tu);

/**
 * Checks for the expiration of the currenct anchor master and updates accordingly.
 * HAS TO BE called at the end of a DW.
 * 
 * @param state - The current sync state
 */
void nan_check_anchor_master_expiration(struct nan_sync_state *state);

/**
 * Get the role value as string.
 * 
 * @param role
 * @returns The role as string
 */
char *nan_role_to_string(enum nan_role role);

#endif // NAN_SYNC_H