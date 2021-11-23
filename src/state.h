#ifndef NAN_STATE_H_
#define NAN_STATE_H_

#include <stdint.h>
#include <netinet/ether.h>

#include "attributes.h"
#include "peer.h"
#include "timer.h"
#include "cluster.h"
#include "ieee80211.h"
#include "channel.h"
#include "event.h"
#include "service.h"
#include "circular_buffer.h"
#include "sync.h"

struct nan_state
{
    // The hostname of the device
    char hostname[HOST_NAME_LENGTH_MAX + 1];
    // The ethernet address of the device
    struct ether_addr self_address;
    // The current ethernet address of the interface
    struct ether_addr interface_address;
    // Buffer for outgoing frames
    circular_buf_t buffer;

    // Information about used channels
    struct nan_channel_state channel;
    // Currently known peers
    struct nan_peer_state peers;
    // Timer used for clock syncronization
    struct nan_timer_state timer;
    // Information about the current cluster
    struct nan_cluster_state cluster;
    // Information about the current anchor master election
    struct nan_sync_state sync;
    // Attached event listeners
    struct nan_event_state events;
    // Service engine state
    struct nan_service_state services;
    // Needed information for IEEE 802.11 frames
    struct ieee80211_state ieee80211;
};

/** 
 * Initializes the state according to the specs.
 * 
 * @param state - The state to initialize
 * @param hostname - The hostname of the device
 * @param addr - The ethernet address of the device
 * @param now_usec - The current time in microseconds
 */
void init_nan_state(struct nan_state *state, const char *hostname,
                    struct ether_addr *addr, int channel, uint64_t now_usec);

#endif //NAN_STATE_H_
