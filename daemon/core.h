#ifndef NAN_CORE_H_
#define NAN_CORE_H_

#include <stdint.h>
#include <ev.h>

#include <state.h>
#include <wire.h>

#include "io.h"

struct ev_state
{
    struct ev_loop *loop;
    ev_timer send_discovery_beacon;
    ev_timer discovery_window;
    ev_timer discovery_window_end;
    ev_timer clean_peers;
    ev_io read_stdin;
    ev_io read_wlan;
    ev_io read_host;
};

struct daemon_state
{
    struct nan_state nan_state;
    struct io_state io_state;
    struct ev_state ev_state;

    uint64_t start_time_usec;

    const char *dump;
    char *last_cmd;
};

int nan_init(struct daemon_state *state, const char *wlan, const char *host, int channel, const char *dump);

void nan_free(struct daemon_state *state);

void read_wlan(struct ev_loop *loop, ev_io *handle, int revents);

void nan_schedule(struct ev_loop *loop, struct daemon_state *state);

#endif //NAN_CORE_H_