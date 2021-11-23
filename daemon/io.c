#include "io.h"

#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <net/if.h>
#include <sys/ioctl.h>

#ifndef __APPLE__
#include <linux/if_tun.h>
#else
#include <sys/sys_domain.h>
#include <sys/kern_control.h>
#include <netinet/in.h>
#include <netinet/in_var.h>
#endif /* __APPLE__ */

#include <log.h>
#include <wire.h>

#include "netutils.h"

static int open_nonblocking_device(const char *dev, pcap_t **pcap_handle, const struct ether_addr *bssid_filter)
{
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *handle = pcap_create(dev, errbuf);
    if (handle == NULL)
    {
        log_error("pcap: unable to open device %s (%s)", dev, errbuf);
        return -1;
    }

    pcap_set_snaplen(handle, 65535);
    pcap_set_promisc(handle, 1);
    pcap_set_timeout(handle, 1);
#ifdef __APPLE__
    /* On Linux, we activate monitor mode via nl80211 */
    pcap_set_rfmon(handle, 1);
#endif /* __APPLE__ */

    if (pcap_activate(handle) < 0)
    {
        log_error("pcap: unable to activate device %s (%s)", dev, pcap_geterr(handle));
        pcap_close(handle);
        return -1;
    }

    if (pcap_setnonblock(handle, 1, errbuf) < 0)
    {
        log_error("pcap: cannot set to non-blocking mode (%s)", errbuf);
        pcap_close(handle);
        return -1;
    }

    /* FIXME direction does not seem to have an effect (we get our own frames every time we poll) */
    if (pcap_setdirection(handle, PCAP_D_IN) < 0)
    {
        log_warn("pcap: unable to monitor only incoming traffic on device %s (%s)", dev, pcap_geterr(handle));
    }

    if (pcap_datalink(handle) != DLT_IEEE802_11_RADIO)
    {
        log_error("pcap: device %s does not support radiotap headers", dev);
        pcap_close(handle);
        return -1;
    }

    if (bssid_filter)
    {
        struct bpf_program filter;
        char filter_str[128];
        snprintf(filter_str, sizeof(filter_str), "wlan addr3 %s", ether_ntoa(bssid_filter));
        if (pcap_compile(handle, &filter, filter_str, 1, PCAP_NETMASK_UNKNOWN) < 0)
        {
            log_error("pcap: could not create filter (%s)", pcap_geterr(handle));
            return -1;
        }

        if (pcap_setfilter(handle, &filter) < 0)
        {
            log_error("pcap: could not set filter (%s)", pcap_geterr(handle));
            return -1;
        }
    }

    int fd = pcap_get_selectable_fd(handle);
    if (fd < 0)
    {
        log_error("pcap: unable to get fd");
        return -1;
    }

    *pcap_handle = handle;
    return fd;
}

static int open_tun(char *dev, const struct ether_addr *self)
{
#ifndef __APPLE__
    static int one = 1;
    struct ifreq ifr;
    int fd, err, s;

    if ((fd = open("/dev/net/tun", O_RDWR)) < 0)
    {
        log_error("tun: unable to open tun device %d", fd);
        return fd;
    }

    memset(&ifr, 0, sizeof(ifr));

    /* Flags: IFF_TUN   - TUN device (no Ethernet headers)
	 *        IFF_TAP   - TAP device
	 *        IFF_NO_PI - Do not provide packet information
	 */
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
    if (*dev)
        strncpy(ifr.ifr_name, dev, IFNAMSIZ);

    if ((err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0)
    {
        close(fd);
        return err;
    }
    strcpy(dev, ifr.ifr_name);

    /* Set non-blocking mode */
    if ((err = ioctl(fd, FIONBIO, &one)) < 0)
    {
        close(fd);
        return err;
    }

    // Create a socket for ioctl
    s = socket(AF_INET6, SOCK_DGRAM, 0);

    // Set HW address
    ifr.ifr_hwaddr.sa_family = 1; /* ARPHRD_ETHER */
    memcpy(ifr.ifr_hwaddr.sa_data, self, 6);
    if ((err = ioctl(s, SIOCSIFHWADDR, &ifr)) < 0)
    {
        log_error("tun: unable to set HW address");
        close(fd);
        return err;
    }

    // Get current flags and set them
    ioctl(s, SIOCGIFFLAGS, &ifr);
    ifr.ifr_flags |= IFF_UP | IFF_RUNNING;
    if ((err = ioctl(s, SIOCSIFFLAGS, &ifr)) < 0)
    {
        log_error("tun: unable to set up");
        close(fd);
        return err;
    }

    /* Set reduced MTU */
    ifr.ifr_mtu = 1450; /* TODO arbitary limit to fit all headers */
    if ((err = ioctl(s, SIOCSIFMTU, (void *)&ifr)) < 0)
    {
        log_error("tun: unable to set MTU");
        close(fd);
        return err;
    }

    close(s);

    return fd;
#else
    for (int i = 0; i < 16; ++i)
    {
        char tuntap[IFNAMSIZ];
        sprintf(tuntap, "/dev/tap%d", i);

        int fd = open(tuntap, O_RDWR);
        if (fd > 0)
        {
            struct ifreq ifr;
            struct in6_aliasreq ifr6;
            int err, s;

            if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0)
            {
                log_error("fcntl error on %s", tuntap);
                return -errno;
            }

            sprintf(dev, "tap%d", i);

            // Create a socket for ioctl
            s = socket(AF_INET6, SOCK_DGRAM, 0);

            // Set HW address
            strlcpy(ifr.ifr_name, dev, sizeof(ifr.ifr_name));
            ifr.ifr_addr.sa_len = sizeof(struct ether_addr);
            ifr.ifr_addr.sa_family = AF_LINK;
            memcpy(ifr.ifr_addr.sa_data, self, sizeof(struct ether_addr));
            if ((err = ioctl(s, SIOCSIFLLADDR, (caddr_t)&ifr)) < 0)
            {
                log_error("tun: unable to set HW address %s", ether_ntoa(self));
                close(fd);
                return err;
            }

            /* Set reduced MTU */
            ifr.ifr_mtu = 1450; /* TODO arbitary limit to fit all headers */
            if ((err = ioctl(s, SIOCSIFMTU, (caddr_t)&ifr)) < 0)
            {
                log_error("tun: unable to set MTU");
                close(fd);
                return err;
            }

            /* Set IPv6 address */
            memset(&ifr6, 0, sizeof(ifr6));
            strlcpy(ifr6.ifra_name, dev, sizeof(ifr6.ifra_name));

            ifr6.ifra_addr.sin6_len = sizeof(ifr6.ifra_addr);
            ifr6.ifra_addr.sin6_family = AF_INET6;
            rfc4291_addr(self, &ifr6.ifra_addr.sin6_addr);

            ifr6.ifra_prefixmask.sin6_len = sizeof(ifr6.ifra_prefixmask);
            ifr6.ifra_prefixmask.sin6_family = AF_INET6;
            memset((void *)&ifr6.ifra_prefixmask.sin6_addr, 0x00, sizeof(ifr6.ifra_prefixmask));
            for (int i = 0; i < 8; i++) /* prefix length: 64 */
                ifr6.ifra_prefixmask.sin6_addr.s6_addr[i] = 0xff;

            if (ioctl(s, SIOCAIFADDR_IN6, (caddr_t)&ifr6) < 0)
            {
                log_error("tun: unable to set IPv6 address, %s", strerror(errno));
                close(fd);
                return -errno;
            }

            close(s);

            return fd;
        }
    }
    log_error("tun: cannot open available device");
    return -1;
#endif /* __APPLE__ */
}

int io_state_init_wlan(struct io_state *state, const char *wlan, const int channel,
                       const struct ether_addr *bssid_filter)
{
    strcpy(state->wlan_ifname, wlan);

    state->wlan_ifindex = if_nametoindex(state->wlan_ifname);
    if (!state->wlan_ifindex)
    {
        log_error("No such interface exists %s", state->wlan_ifname);
        return -ENOENT;
    }

    if (!state->no_updown)
    {
        if (link_down(state->wlan_ifindex) < 0)
        {
            log_error("Could set link down: %s", state->wlan_ifname);
            return -1;
        }
    }

    if (!state->no_monitor)
    {
        if (set_monitor_mode(state->wlan_ifindex) < 0)
        {
            log_error("Could not put device in monitor mode: %s", state->wlan_ifname);
            return -1;
        }
    }

    if (!state->no_updown)
    {
        if (link_up(state->wlan_ifindex) < 0)
        {
            log_error("Could set link up: %", state->wlan_ifname);
            return -1;
        }
    }

    if (!state->no_channel)
    {
        if (set_channel(state->wlan_ifindex, channel))
        {
            log_error("Could not set channel of %s", state->wlan_ifname);
            return -1;
        }
    }

    state->wlan_fd = open_nonblocking_device(state->wlan_ifname, &state->wlan_handle, bssid_filter);
    if (state->wlan_fd < 0)
    {
        log_error("Could not open device %s: %d", state->wlan_ifname, state->wlan_fd);
        return -1;
    }

    if (link_ether_addr_get(state->wlan_ifname, &state->if_ether_addr) < 0)
    {
        log_error("Could not get LLC address from %s", state->wlan_ifname);
        return -1;
    }

    return 0;
}

static int io_state_init_host(struct io_state *state, const char *host)
{
    if (strlen(host) > 0)
    {
        strcpy(state->host_ifname, host);
        /* Host interface needs to have same ether_addr, to make active (!) monitor mode work */
        state->host_fd = open_tun(state->host_ifname, &state->if_ether_addr);

        int err;
        if ((err = state->host_fd) < 0)
        {
            log_error("Could not open device: %s", state->host_ifname);
            return err;
        }
        state->host_ifindex = if_nametoindex(state->host_ifname);
        if (!state->host_ifindex)
        {
            log_error("No such interface exists %s", state->host_ifname);
            return -ENOENT;
        }
    }
    else
    {
        log_debug("No host device given, start without host device");
    }

    return 0;
}

int io_state_init(struct io_state *state, const char *wlan, const char *host, const int channel,
                  const struct ether_addr *bssid_filter)
{
    int err;

    if ((err = io_state_init_wlan(state, wlan, channel, bssid_filter)))
        return err;

    if ((err = io_state_init_host(state, host)))
        return err;

    return 0;
}

void io_state_free(struct io_state *state)
{
    close(state->host_fd);
    pcap_close(state->wlan_handle);
}

int wlan_send(const struct io_state *state, const uint8_t *buffer, int length)
{
    if (!state || !state->wlan_handle)
        return -EINVAL;

    int result = pcap_inject(state->wlan_handle, buffer, length);
    if (result < 0)
        log_error("unable to inject packet (%s)", pcap_geterr(state->wlan_handle));
    else
        log_trace("injected %d bytes", result);

    return result;
}

int host_send(const struct io_state *state, const uint8_t *buffer, int length)
{
    if (!state || !state->host_fd)
        return -EINVAL;

    if (write(state->host_fd, buffer, length) < 0)
        return -errno;

    return 0;
}

int host_receive(const struct io_state *state, uint8_t *buffer, int *length)
{
    if (!state || !state->host_fd)
        return -EINVAL;

    ssize_t read_length = read(state->host_fd, buffer, *length);
    if (read_length < 0)
    {
        if (errno != EWOULDBLOCK)
            log_error("tun: error reading from device");
        return -errno;
    }

    *length = read_length;
    return 0;
}