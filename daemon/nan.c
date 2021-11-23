#include "core.h"

#include <unistd.h>
#include <stdlib.h>
#include <syslog.h>
#include <stdbool.h>
#include <ev.h>
#include <net/if.h>

#include <log.h>
#include <utils.h>
#include <service.h>

#define DEFAULT_NAN_DEVICE "nan0"
#define FAILED_DUMP "failed.pcap"

void print_usage(char *arg0)
{
	printf("Usage: %s [options] <interface>\n", arg0);
	printf("\n");
	printf("Arguments:\n");
	printf(" interface                The wireless interface to use for frame capturing\n");
	printf("                          and injection. Must support monitor mode.\n");
	printf("\n");
	printf("Options:\n");
	printf(" -v                       Increase log level\n");
	printf(" -d [file=failed.pcap]    Dump failed frames into a .pcap file\n");
	printf("\n");
	printf(" -n string                Change virtual interface name. Defaut is nan0\n");
	printf(" -c number                Set interface channel. Default is 6\n");
	printf(" -M                       Do not enable monitor mode on interface\n");
	printf(" -C                       Do not set channel on interface\n");
	printf(" -U                       Do not set interface up/down\n");
}

int main(int argc, char *argv[])
{
	log_set_level(LOG_INFO);

	bool dump = false;
	char *dump_file = FAILED_DUMP;

	char wlan[IFNAMSIZ] = "";
	char host[IFNAMSIZ] = DEFAULT_NAN_DEVICE;
	int channel = 6;

	struct daemon_state state;
	state.start_time_usec = clock_time_usec();

	int c;
	while ((c = getopt(argc, argv, "vd::n:c:hMCU")) != -1)
	{
		switch (c)
		{
		case 'h':
			print_usage(argv[0]);
			return 0;
		case 'v':
			log_increase_level();
			break;
		case 'd':
			dump = true;
			if (optarg)
				strcpy(dump_file, optarg);
			break;
		case 'n':
			strcpy(host, optarg);
			break;
		case 'c':
			channel = atoi(optarg);
			break;
		case 'M':
			state.io_state.no_monitor = true;
			break;
		case 'C':
			state.io_state.no_channel = true;
			break;
		case 'U':
			state.io_state.no_updown = true;
			break;
		case '?':
			switch (optopt)
			{
			case 'n':
			case 'c':
			case 's':
			case 'p':
				log_error("Option -%c requires an argument.", optopt);
				break;
			default:
				log_error("Unknown option `-%c'.", optopt);
			}
			return EXIT_FAILURE;
		default:
			abort();
		}
	}

	if (argc - optind != 1)
	{
		log_error("Incorrect number of arguments: %d", argc - optind);
		print_usage(argv[0]);
		return EXIT_FAILURE;
	}
	strcpy(wlan, argv[optind]);

	switch (channel)
	{
	case 6:
	case 44:
	case 149:
		break;
	default:
		log_error("Unsupported channel %d (use 6, 44, or 149)", channel);
		return EXIT_FAILURE;
	}

	if (nan_init(&state, wlan, host, channel, dump ? dump_file : 0) < 0)
	{
		log_error("could not initialize core");
		return EXIT_FAILURE;
	}

	printf("88b 88    db    88b 88\n"
		   "88Yb88   dPYb   88Yb88\n"
		   "88 Y88  dP__Yb  88 Y88\n"
		   "88  Y8 dP''''Yb 88  Y8\n");

	if (state.io_state.wlan_ifindex)
		log_info("WLAN device: %s (addr %s)", state.io_state.wlan_ifname, ether_addr_to_string(&state.io_state.if_ether_addr));
	if (state.io_state.host_ifindex)
		log_info("Host device: %s", state.io_state.host_ifname);
	log_info("Initial Cluster ID: %s", ether_addr_to_string(&state.nan_state.cluster.cluster_id));

	struct ev_loop *loop = EV_DEFAULT;
	nan_schedule(loop, &state);
	ev_run(loop, 0);

	nan_free(&state);

	return EXIT_SUCCESS;
}