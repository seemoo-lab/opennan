#ifndef NAN_ELECTION_H_
#define NAN_ELECTION_H_

#include <stdint.h>
#include <stdbool.h>
#include <netinet/ether.h>

#include "timer.h"

#define NAN_MASTER_PREFERENCE_UPDATE_MIN_DW 240
#define NAN_RANDOM_FACOTR_UPDATE_MIN_DW 120
#define NAN_RANDOM_FACOTR_UPDATE_MAX_DW 240

#ifndef NAN_MASTER_PREFERENCE
#define NAN_MASTER_PREFERENCE 254;
#endif

enum nan_master_state
{
    NON_MASTER_NON_SYNC,
    NON_MASTER_SYNC,
    MASTER,
};

enum nan_election_result
{
    MASTER_SELF = 2,
    MASTER_OTHER = 1,
    NO_CHANGE = 0,
    DISREGARDED = -1,
};

struct nan_election_state
{
    enum nan_master_state master_state;
    uint64_t master_rank;

    uint8_t master_preference;
    uint64_t last_master_preference_update_usec;

    uint8_t random_factor;
    uint64_t last_random_factor_update_usec;

    struct ether_addr anchor_master_address;
    uint64_t anchor_master_rank;
    uint32_t anchor_master_beacon_transmission_time;
    uint8_t hop_count;

    uint64_t last_anchor_master_rank;
    uint32_t last_anchor_master_beacon_transmission_time;

    bool ambtt_updated_in_dw;
    bool received_better_hop_count_in_dw;
    uint8_t count_dws_without_ambtt_update;
    uint8_t count_dws_without_better_hop_count;
};

struct nan_election_data
{
    const struct ether_addr *peer_address;
    uint64_t anchor_master_rank;
    uint32_t anchor_master_beacon_transmission_time;
    uint8_t hop_count;
};

/**
 * Initiate the election state with default values.
 * 
 * @param state - The election state to init
 * @param interface_address - The current address of the used ethernet interface
 */
void nan_election_state_init(struct nan_election_state *state, const struct ether_addr *interface_address);

/**
 * Calculate the master rank.
 * 
 * @param master_preference
 * @param random_factor
 * @param interface_address
 * @return The calculated master rank
 */
uint64_t nan_calculate_master_rank(const uint8_t master_preference,
                                   const uint8_t random_factor,
                                   const struct ether_addr *interface_address);

/**
 * Set this device as anchor master.
 * 
 * @param state - The current election state
 * @param is_new_cluster - Whether the device became anchor master due to initialization of a new cluster
 */
void nan_set_anchor_master_self(struct nan_election_state *state, const bool is_new_cluster);

/**
 * Set another peer as anchor master
 * 
 * @param state - The current election state
 * @param data - Required data for the election
 */
void nan_set_anchor_master_other(struct nan_election_state *state, struct nan_election_data *data);

/**
 * Compare the lower 6 octets of the current anchor master rank with the received value.
 * 
 * @param state - The current election state
 * @param anchor_master_rank - The received value
 * @returns Whether the lower 6 octets of both values are equally
 */
bool nan_anchor_master_rank_equal(const struct nan_election_state *state, const uint64_t anchor_master_rank);

/**
 * Conduct the election of an anchor master using the values of a received sync beacon.
 * 
 * @param state - The current election state
 * @param data - Required data for the election
 * @param synced_time_usec - The synced time in microseconds
 * @returns The election's result
 */
enum nan_election_result nan_election(struct nan_election_state *state,
                                      struct nan_election_data *data,
                                      const uint64_t synced_time_usec);

/**
 * Update the master rank after any of its components has changed:
 *  - `state->master_preference`
 *  - `state->random_factor`
 *  - `interface_address`
 * 
 * @param state - The current election state
 * @param interface_address - The current address of the used ethernet interface
 * @returns Whether the master rank was updated
 */
bool nan_update_master_rank(struct nan_election_state *state, const struct ether_addr *interface_address);

/**
 * Updates master preference if allowed.
 * 
 * @param state - The current election state
 * @param now_usec - The current time in microseconds
 * @returns Whether the master preference was updated 
 */
bool nan_update_master_preference(struct nan_election_state *state, const uint64_t now_usec);

/**
 * Updates random factor if allowed.
 * 
 * @param state - The current election state
 * @param now_usec - The current time in microseconds
 * @returns Whether the random factor was updated
 */
bool nan_update_random_factor(struct nan_election_state *state, const uint64_t now_usec);

/**
 * Checks for the expiration of the currenct anchor master and updates accordingly.
 * HAS TO BE called at the end of a DW.
 * 
 * @param state - The current election state
 */
void nan_check_anchor_master_expiration(struct nan_election_state *state);

/**
 * Get the master state value as string.
 * 
 * @param master_state
 * @returns The master state as string
 */
char *nan_master_state_to_string(enum nan_master_state master_state);

#endif // NAN_ELECTION_H_