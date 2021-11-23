#include "state.h"

#include <string.h>

void init_nan_state(struct nan_state *state, const char *hostname,
                    struct ether_addr *addr, int channel, uint64_t now_usec)
{
    strncpy(state->hostname, hostname, HOST_NAME_LENGTH_MAX);
    state->self_address = *addr;
    state->interface_address = *addr;

    state->buffer = circular_buf_init(16);

    nan_channel_state_init(&state->channel, channel);
    nan_cluster_state_init(&state->cluster);
    nan_sync_state_init(&state->sync, addr);
    nan_peer_state_init(&state->peers);
    nan_timer_state_init(&state->timer, now_usec);
    nan_event_state_init(&state->events);
    nan_service_state_init(&state->services);
    ieee80211_init_state(&state->ieee80211);
}
