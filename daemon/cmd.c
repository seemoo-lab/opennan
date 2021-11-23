#include "cmd.h"

#include <string.h>
#include <stdbool.h>
#include <inttypes.h>

#include <log.h>
#include <peer.h>
#include <utils.h>
#include <sync.h>
#include <list.h>
#include <service.h>
#include <tx.h>
#include <timer.h>

void nan_cmd_print_help()
{
    log_info("Available commands");
    log_info("--------------------------------------------------------------------");
    log_info(" * help                                Prints this message");
    log_info("");
    log_info("Info");
    log_info(" * device                                  Prints current device state");
    log_info(" * sync                                    Prints current sync state");
    log_info(" * peers                                   Prints list of added peers");
    log_info(" * services [pub, sub]                     Prints list of PUBlished and/or SUBscribed services");
    log_info("");
    log_info("Action");
    log_info(" * publish %%service_name%%                Publish a service with the given name");
    log_info(" * subscribe %%service_name%%              Subscribe for a service with the given name");
    log_info(" * set mp %%number%%                       Set the master preference");
    log_info(" * set rf %%number%%                       Set the random factor");
    log_info(" * set disc %%boolean%%                    Enable or disable transmission of discovery beacons");
    log_info("");
    log_info("Peer Action");
    log_info(" * peer %%addr%% set timer %%tu%%          Shift the timer value of a peer");
    log_info(" * peer %%addr%% set counter %%number%%    Set transmission counter");
    log_info(" * peer %%addr%% rm                        Remove peer");
    log_info("");
    log_info("Misc");
    log_info(" * v+                                      Increase log verbosity");
    log_info(" * v-                                      Decrease log verbosity");
    log_info("--------------------------------------------------------------------");
    log_info("Submit empty line to redo last command (not supported for actions)");
    log_info("");
}

void nan_cmd_print_device_info(const struct nan_state *state)
{
    log_info("Device Info");
    log_info("---------------------------------------------");
    log_info("Interface address        %s", ether_addr_to_string(&state->self_address));
    log_info("Cluster ID               %s", ether_addr_to_string(&state->cluster.cluster_id));
    log_info("");
}

void nan_cmd_print_sync_info(const struct nan_state *state)
{
    uint64_t now_usec = clock_time_usec();

    uint64_t synced_time_usec = nan_timer_get_synced_time_usec(&state->timer, now_usec);
    uint64_t synced_time_tu = nan_timer_get_synced_time_tu(&state->timer, now_usec);
    uint64_t next_dw_usec = nan_timer_next_dw_usec(&state->timer, now_usec);

    const struct ether_addr *anchor_master_address = nan_get_anchor_master_address(&state->sync);

    log_info("Sync");
    log_info("---------------------------------------------");
    log_info("Current Time (usec)      %lu", now_usec);
    log_info("Synced Time (usec)       %lu", synced_time_usec);
    log_info("Synced Time (tu)         %lu", synced_time_tu);
    log_info("Next DW (usec)           %lu", next_dw_usec);
    log_info("Next DW (tu)             %lu", USEC_TO_TU(next_dw_usec));
    log_info("");
    log_info("Role                     %s", nan_role_to_string(state->sync.role));
    log_info("Master Rank              %lu", state->sync.master_rank);
    log_info("Master Preference        %u", state->sync.master_preference);
    log_info("Random Factor            %u", state->sync.random_factor);
    log_info("");
    log_info("Anchor Master Address    %s", ether_addr_to_string(anchor_master_address));
    log_info("Anchor Master Rank       %lu", state->sync.anchor_master_rank);
    log_info("AMBTT                    %u", state->sync.ambtt);
    log_info("Hop Count                %u", state->sync.hop_count);
    log_info("");
    log_info("Last Anchor Master Rank  %lu", state->sync.last_anchor_master_rank);
    log_info("Last AMBTT               %u", state->sync.last_ambtt);
    log_info("");
}

void nan_cmd_print_peers_info(const struct nan_state *state)
{
    log_info("Peers");
    log_info("---------------------------------------------");

    if (list_len(state->peers.peers) == 0)
    {
        log_info("No peer(s) added.");
        log_info("");
        return;
    }

    uint64_t now_usec = clock_time_usec();

    struct nan_peer *peer = NULL;
    LIST_FOR_EACH(state->peers.peers, peer, {
        unsigned int last_update_tu = USEC_TO_TU((int)(now_usec - peer->last_update));
        unsigned int last_update_dw = last_update_tu / NAN_DW_INTERVAL_TU;

        log_info("Peer Address             %s", ether_addr_to_string(&peer->addr));
        log_info("Peer IPv6 Address        %s", ipv6_addr_to_string(&peer->ipv6_addr));
        log_info("Peer Cluster ID          %s", ether_addr_to_string(&peer->cluster_id));
        log_info("RSSI                     %hhi", peer->rssi_average);
        log_info("Last Update              %u tu (%u dw)", last_update_tu, last_update_dw);
        log_info("Is master candidate?     %s", peer->master_candidate ? "TRUE" : "FALSE");
        log_info("");
        log_info("Master Rank              %lu", nan_get_peer_master_rank(peer));
        log_info("Master Preference        %u", peer->master_preference);
        log_info("Random Factor            %u", peer->random_factor);
        log_info("");
        log_info("Anchor Master Rank       %lu", peer->anchor_master_rank);
        log_info("AMBTT                    %u", peer->ambtt);
        log_info("Hop count to AM          %u", peer->hop_count);
        log_info("");
        log_info("Total shift              %d tu", peer->total_timer_shift_tu);
        log_info("");
        log_info("");
    });
}

void nan_cmd_print_services_info(const struct nan_state *state, char *args)
{
    bool print_published_services = true;
    bool print_subscribed_services = true;

    if (args != NULL && strcmp(args, "pub") == 0)
        print_subscribed_services = false;
    if (args != NULL && strcmp(args, "sub") == 0)
        print_published_services = false;

    if (print_subscribed_services)
    {
        log_info("Suscribed Services");
        log_info("---------------------------------------------");
        if (list_len(state->services.subscribed_services) == 0)
        {
            log_info("No service(s) subscribed.");
        }
        else
        {
            struct nan_service *service = NULL;
            LIST_FOR_EACH(state->services.subscribed_services, service, {
                log_info("Service Name             %s", service->service_name);
                log_info("Service ID               %s", ether_addr_to_string((const struct ether_addr *)&service->service_id));
                log_info("Subscribe ID             %u", service->instance_id);
                log_info("Type                     %s", nan_subscribe_type_to_string(service->parameters.subscribe.type));
                log_info("Time to live             %d", service->time_to_live);
                log_info("Is subscribed?           %s", service->parameters.subscribe.is_subscribed ? "true" : "false");
                if (service->service_specific_info && service->service_specific_info_length > 0)
                    log_info("Service Info             %.*s", service->service_specific_info_length, service->service_specific_info);
            });
        }

        log_info("");
    }

    if (print_published_services)
    {
        log_info("Published Services");
        log_info("---------------------------------------------");
        if (list_len(state->services.published_services) == 0)
        {
            log_info("No service(s) published.");
        }
        else
        {
            struct nan_service *service = NULL;
            LIST_FOR_EACH(state->services.published_services, service, {
                log_info("Service Name             %s", service->service_name);
                log_info("Service ID               %s", ether_addr_to_string((const struct ether_addr *)&service->service_id));
                log_info("Publish ID               %u", service->instance_id);
                log_info("Type                     %s", nan_publish_type_to_string(service->parameters.publish.type));
                log_info("Time to live             %d", service->time_to_live);
                if (service->service_specific_info && service->service_specific_info_length > 0)
                    log_info("Service Info             %.*s", service->service_specific_info_length, service->service_specific_info);
            });
        }

        log_info("");
    }
}

static void handle_event_receive(enum nan_event_type event, void *event_data, void *additional_data)
{
    (void)event;
    struct nan_event_receive *data = event_data;
    struct nan_state *state = additional_data;

    struct nan_service *service = nan_get_service_by_instance_id(&state->services, data->instance_id, -1);

    if (service == NULL)
    {
        log_warn("Received receive event for unknown service: %u", data->instance_id);
        return;
    }

    log_debug("Received response from %s for %s: %.*s",
              ether_addr_to_string(data->address), service->service_name,
              data->service_specific_info_length, data->service_specific_info);

    if (service->type == PUBLISHED)
        nan_cancel_publish(&state->services, service->instance_id);
    else
        nan_cancel_subscribe(&state->services, service->instance_id);
    nan_remove_event_listener(&state->events, handle_event_receive);
}

void nan_cmd_publish_service(struct nan_state *state, char *args)
{
    char *service_name = strtok(args, " ");
    char *service_info = strtok(NULL, " ");
    service_info = service_info ? service_info : "";

    if (nan_get_service_by_name(&state->services, service_name, -1) != NULL)
    {
        log_error("Service with name %s already registered", args);
        return;
    }

    uint8_t publish_id = nan_publish(&state->services, service_name,
                                     PUBLISH_BOTH, -1,
                                     service_info, strlen(service_info));
    nan_add_event_listener(&state->events, EVENT_RECEIVE, service_name,
                           handle_event_receive, state);

    log_info("Published service '%s' with data '%s' (%u)", service_name, service_info, publish_id);
}

static void handle_event_discovery_result(enum nan_event_type event, void *event_data, void *additional_data)
{
    (void)event;
    struct nan_event_discovery_result *data = event_data;
    struct nan_state *state = additional_data;

    struct nan_service *service = nan_get_service_by_instance_id(&state->services, data->subscribe_id, SUBSCRIBED);

    if (service == NULL)
    {
        log_warn("Received discovery result event for unknown service: %u", data->subscribe_id);
        return;
    }

    log_debug("Received discovery result from %s for %s",
              ether_addr_to_string(data->address), service->service_name);

    if (nan_transmit(state, data->address, data->subscribe_id, data->publish_id, "Hello world!", 12) < 0)
        log_warn("Could not transmit data to service %s", service->service_name);

    nan_remove_event_listener(&state->events, handle_event_discovery_result);
    nan_add_event_listener(&state->events, EVENT_RECEIVE, service->service_name,
                           handle_event_receive, state);
}

void nan_cmd_subscribe_service(struct nan_state *state, char *args)
{
    char *service_name = strtok(args, " ");

    if (nan_get_service_by_name(&state->services, args, -1) != NULL)
    {
        log_error("Service with name %s already registered", args);
        return;
    }

    uint8_t subscribe_id = nan_subscribe(&state->services, service_name,
                                         SUBSCRIBE_PASSIVE, -1, NULL, 0);
    nan_add_event_listener(&state->events, EVENT_DISCOVERY_RESULT, service_name,
                           handle_event_discovery_result, state);

    log_info("Subscribed for service '%s' (%u)", service_name, subscribe_id);
}

bool validate_number(char *value)
{
    const int parsed = atoi(value);
    if (parsed == 0 && strcmp(value, "") != 0 && strcmp(value, "0") != 0)
    {
        log_warn("Not a number: '%s'", value);
        return false;
    }
    return true;
}

bool validate_number_range(char *value, int min, int max)
{
    if (!validate_number(value))
        return false;

    const int parsed = atoi(value);
    if (parsed < min || parsed > max)
    {
        log_warn("Expected value between %d and %d but got %d", min, max, parsed);
        return false;
    }
    return true;
}

void nan_cmd_set_value(struct nan_state *state, char *args)
{
    char *target = strtok(args, " ");
    char *value = strtok(NULL, " ");

    if (!target || !value)
    {
        log_warn("Usage: set %%target%% %%value%%");
        return;
    }

    if (strcmp(target, "rf") == 0)
    {
        if (!validate_number_range(value, 0, 255))
            return;
        const int random_factor = atoi(value);

        state->sync.random_factor = random_factor;
        nan_update_master_rank(&state->sync, &state->interface_address);
    }
    else if (strcmp(target, "mp") == 0)
    {
        if (!validate_number_range(value, 0, 255))
            return;
        const int master_preference = atoi(value);

        state->sync.master_preference = master_preference;
        nan_update_master_rank(&state->sync, &state->interface_address);
    }
    else if (strcmp(target, "desync") == 0)
    {
        bool enable = (bool)atoi(value);

        if (enable)
        {
            if (list_len(state->peers.peers) < 2)
            {
                log_error("Cannot enable desync with less than 2 known peers");
                return;
            }

            struct nan_peer *peer;
            LIST_FIND(state->peers.peers, peer,
                      !nan_is_same_master_rank_issuer(state->sync.anchor_master_rank,
                                                     peer->anchor_master_rank));
            if (peer)
            {
                log_error("Cannot enable desync: Peer %s does not acknowledge us as anchor master",
                          ether_addr_to_string(&peer->addr));
                return;
            }
        }

        struct nan_peer *peer;
        LIST_FOR_EACH(state->peers.peers, peer, {
            peer->timer.base_time_usec = state->timer.base_time_usec;
            peer->old_timer.base_time_usec = state->timer.base_time_usec;
        })

        state->desync = enable;
        log_info("%s desync", enable ? "Enabled" : "Disabled");
    }
    else
    {
        log_warn("Unknown target for 'set' command: %s", target);
        return;
    }

    log_info("Set %s to %s", target, value);
}

struct nan_peer *get_peer(struct nan_state *state, const char *peer_address_arg)
{
    struct nan_peer *peer;
    LIST_FIND(state->peers.peers, peer, strstr(ether_addr_to_string(&peer->addr), peer_address_arg));
    if (peer == NULL)
        log_warn("Could not find peer by address matching: %s", peer_address_arg);

    return peer;
}

void nan_cmd_peer_set_value(struct nan_state *state, struct nan_peer *peer, char *args)
{
    char *field = strtok(args, " ");
    char *value = strtok(NULL, " ");

    if (strcmp(field, "timer") == 0)
    {
        if (!validate_number(value))
            return;

        const int offset_tu = atoi(value);

        peer->old_timer.base_time_usec = peer->timer.base_time_usec;
        peer->timer.base_time_usec += TU_TO_USEC(offset_tu);

        peer->old_timer_send_count = 0;
        peer->total_timer_shift_tu += offset_tu;
        log_info("Shifted timer of peer %s for %d tu", ether_addr_to_string(&peer->addr), offset_tu);
    }
    else
    {
        log_warn("Unknown target for 'set_peer' command: %s", field);
        return;
    }
}

void nan_cmd_peer(struct nan_state *state, char *args)
{
    char *peer_address_arg = strtok(args, " ");
    char *cmd = strtok(NULL, " ");
    char *cmd_args = strtok(NULL, "");

    if (!peer_address_arg || !cmd)
    {
        log_warn("Invalid arguments");
        return;
    }

    struct nan_peer *peer = get_peer(state, args);
    if (peer == NULL)
        return;

    if (strcmp(cmd, "set") == 0)
    {
        if (!cmd_args)
        {
            log_warn("Invalid arguments");
            return;
        }

        nan_cmd_peer_set_value(state, peer, cmd_args);
    }
    else if (strcmp(cmd, "rm") == 0)
    {
        char *peer_addr_string = ether_addr_to_string(&peer->addr);
        nan_peer_remove(&state->peers, peer);
        log_info("Removed peer %s", peer_addr_string);
    }
    else if (strcmp(cmd, "ping") == 0)
    {
        char *message = cmd_args ? cmd_args : "#0000ff";

        nan_publish(&state->services, "servicename", PUBLISH_UNSOLICITED, -1, message, strlen(message));
        nan_add_event_listener(&state->events, EVENT_RECEIVE,
                               "servicename", handle_event_receive, state);
        log_info("Ping peer %s", ether_addr_to_string(&peer->addr));
    }
    else if (strcmp(cmd, "forward") == 0)
    {
        bool enable = (bool)atoi(cmd_args);

        peer->forward = enable;

        log_info("%s forward for peer %s",
                 enable ? "Enabled" : "Disabled",
                 ether_addr_to_string(&peer->addr));
    }
    else if (strcmp(cmd, "modify") == 0)
    {
        bool enable = (bool)atoi(cmd_args);

        peer->modify = enable;

        log_info("%s modify for peer %s",
                 enable ? "Enabled" : "Disabled",
                 ether_addr_to_string(&peer->addr));
    }
    else
    {
        log_warn("Unknown peer command: %s", cmd);
    }
}

static void free_last_cmd(char **last_cmd)
{
    if (*last_cmd)
    {
        free(*last_cmd);
        *last_cmd = NULL;
    }
}

void nan_handle_cmd(struct nan_state *state, char *input, char **last_cmd)
{
    if (strlen(input) == 0)
    {
        if (*last_cmd == NULL)
            return;
        input = *last_cmd;
    }
    bool store_last_cmd = true;
    char *cmd = strtok(input, " ");
    char *args = strtok(NULL, "");

    if (strcmp(cmd, "help") == 0)
        nan_cmd_print_help();
    else if (strcmp(cmd, "device") == 0)
        nan_cmd_print_device_info(state);
    else if (strcmp(cmd, "sync") == 0)
        nan_cmd_print_sync_info(state);
    else if (strcmp(cmd, "peers") == 0)
        nan_cmd_print_peers_info(state);
    else if (strcmp(cmd, "services") == 0)
        nan_cmd_print_services_info(state, args);
    else
    {
        store_last_cmd = false;
        if (strcmp(cmd, "v+") == 0)
            log_increase_level();
        else if (strcmp(cmd, "v-") == 0)
            log_decrease_level();
        else if (strcmp(cmd, "publish") == 0)
            nan_cmd_publish_service(state, args);
        else if (strcmp(cmd, "subscribe") == 0)
            nan_cmd_subscribe_service(state, args);
        else if (strcmp(cmd, "set") == 0)
            nan_cmd_set_value(state, args);
        else if (strcmp(cmd, "peer") == 0)
            nan_cmd_peer(state, args);
        else if (strlen(input) > 0)
        {
            log_warn("Unknown command: %s", input);
            free_last_cmd(last_cmd);
            return;
        }
    }

    if (!store_last_cmd)
        free_last_cmd(last_cmd);
    else if (input != *last_cmd)
    {
        free_last_cmd(last_cmd);
        size_t len = strlen(input) + 1;
        *last_cmd = malloc(len);
        memcpy(*last_cmd, input, len);
    }
}
