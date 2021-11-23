#ifndef NAN_IO_H_
#define NAN_IO_H_

#include <stdint.h>
#include <stdbool.h>
#include <pcap/pcap.h>
#include <net/if.h>

#ifdef __APPLE__
#include <net/ethernet.h>
#else
#include <netinet/ether.h>
#endif

struct io_state
{
    pcap_t *wlan_handle;
    char wlan_ifname[IFNAMSIZ]; /* name of WLAN iface */
    int wlan_ifindex;           /* index of WLAN iface */
    int wlan_fd;
    struct ether_addr if_ether_addr; /* MAC address of WLAN and host iface */
    char host_ifname[IFNAMSIZ];      /* name of host iface */
    int host_ifindex;                /* index of host iface */
    int host_fd;
    char *dumpfile;
    bool no_monitor;
    bool no_channel;
    bool no_updown;
};

int io_state_init(struct io_state *state, const char *wlan, const char *host, const int channel,
                  const struct ether_addr *bssid_filter);

void io_state_free(struct io_state *state);

int wlan_send(const struct io_state *state, const uint8_t *buffer, int length);

int host_send(const struct io_state *state, const uint8_t *buffer, int length);

int host_receive(const struct io_state *state, uint8_t *buffer, int *length);

#endif //NAN_IO_H_
