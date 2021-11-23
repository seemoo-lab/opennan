#include "service.h"

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>

#include "utils.h"
#include "sha256.h"
#include "log.h"

void nan_service_state_init(struct nan_service_state *state)
{
    state->published_services = list_init();
    state->subscribed_services = list_init();
    state->last_instance_id = 0;
}

struct nan_service *nan_get_service_by_service_id(const struct nan_service_state *state,
                                                  const struct nan_service_id *service_id,
                                                  const int type)
{
    struct nan_service *service = NULL;
    if (type == SUBSCRIBED || type == -1)
    {
        LIST_FIND(state->subscribed_services, service, memcmp(service_id, &service->service_id, NAN_SERVICE_ID_LENGTH) == 0);
    }

    if (type == PUBLISHED || (type == -1 && service == NULL))
    {
        LIST_FIND(state->published_services, service, memcmp(service_id, &service->service_id, NAN_SERVICE_ID_LENGTH) == 0);
    }

    return service;
}

struct nan_service *nan_get_service_by_instance_id(const struct nan_service_state *state,
                                                   const uint8_t instance_id,
                                                   const int type)
{
    struct nan_service *service = NULL;
    if (type == SUBSCRIBED || type == -1)
    {
        LIST_FIND(state->subscribed_services, service, service->instance_id == instance_id);
    }

    if (type == PUBLISHED || (type == -1 && service == NULL))
    {
        LIST_FIND(state->published_services, service, service->instance_id == instance_id);
    }

    return service;
}

struct nan_service *nan_get_service_by_name(const struct nan_service_state *state,
                                            const char *service_name,
                                            const int type)
{
    struct nan_service *service = NULL;
    if (type == SUBSCRIBED || type == -1)
    {
        LIST_FIND(state->subscribed_services, service,
                  strcmp(service_name, service->service_name) == 0);
    }

    if (type == PUBLISHED || (type == -1 && service == NULL))
    {
        LIST_FIND(state->published_services, service,
                  strcmp(service_name, service->service_name) == 0);
    }

    return service;
}

/**
 * Create the service id for the given service name.
 * 
 * @param service_name - The name of the service
 * @param service_id - Pointer to write the service id to
 */
void nan_service_id_create(const char *service_name, struct nan_service_id *service_id)
{
    size_t service_name_length = strlen(service_name);
    char *lower_case_name = malloc(service_name_length + 1);
    for (unsigned int i = 0; i < service_name_length; i++)
        lower_case_name[i] = tolower(service_name[i]);

    uint8_t hash[128];
    sha256(lower_case_name, service_name_length, hash);

    for (unsigned int i = 0; i < NAN_SERVICE_ID_LENGTH; i++)
        service_id->byte[i] = hash[i];

    free(lower_case_name);
}

char *nan_service_id_to_string(const struct nan_service_id *service_id)
{
    static char buf[18];
    sprintf(buf, "%.2x:%.2x:%.2x:%.2x:%.2x:%.2x",
            service_id->byte[0], service_id->byte[1],
            service_id->byte[2], service_id->byte[3],
            service_id->byte[4], service_id->byte[5]);

    return buf;
}

struct nan_service *nan_service_new(
    struct nan_service_state *state,
    const char *service_name,
    const int time_to_live,
    const void *service_specific_info,
    const size_t service_specific_info_length)
{
    struct nan_service *service = malloc(sizeof(struct nan_service));
    service->time_to_live = time_to_live;
    service->instance_id = increase_non_zero_id(&state->last_instance_id);

    service->service_name = malloc(strlen(service_name) + 1);
    strcpy(service->service_name, service_name);
    nan_service_id_create(service_name, &service->service_id);

    service->service_specific_info = NULL;
    service->service_specific_info_length = 0;
    if (service_specific_info && service_specific_info_length > 0)
    {
        service->service_specific_info = malloc(service_specific_info_length);
        service->service_specific_info_length = service_specific_info_length;
        memcpy(service->service_specific_info, service_specific_info, service_specific_info_length);
        service->service_specific_info_length = service_specific_info_length;
    }

    return service;
}

uint8_t nan_publish(struct nan_service_state *state,
                    const char *service_name,
                    enum nan_publish_type type,
                    int time_to_live,
                    const void *service_specific_info,
                    const size_t service_specific_info_length)
{
    struct nan_service *service = nan_service_new(state, service_name, time_to_live,
                                                  service_specific_info,
                                                  service_specific_info_length);
    service->type = PUBLISHED;
    service->parameters.publish.type = type;
    service->parameters.publish.do_publish = false;

    list_add(state->published_services, (any_t)service);
    return service->instance_id;
}

int nan_update_publish(struct nan_service_state *state, const uint8_t publish_id,
                       const void *service_specific_info, const size_t service_specific_info_length)
{
    struct nan_service *service = NULL;
    LIST_FIND(state->published_services, service, service->instance_id == publish_id);

    if (service)
    {
        service->service_specific_info = realloc(service->service_specific_info, service_specific_info_length);
        memcpy(service->service_specific_info, service_specific_info, service_specific_info_length);
        return 0;
    }

    return -1;
}

int nan_cancel_publish(struct nan_service_state *state, uint8_t publish_id)
{
    struct nan_service *service = NULL;
    LIST_REMOVE(state->published_services, service, service->instance_id == publish_id);

    if (service)
    {
        free(service);
        return 0;
    }
    return -1;
}

uint8_t nan_subscribe(struct nan_service_state *state,
                      const char *service_name,
                      enum nan_subscribe_type type,
                      int time_to_live,
                      const void *service_specific_info,
                      const size_t service_specific_info_length)
{
    struct nan_service *service = nan_service_new(state, service_name, time_to_live,
                                                  service_specific_info,
                                                  service_specific_info_length);

    service->type = SUBSCRIBED;
    service->parameters.subscribe.type = type;
    service->parameters.subscribe.is_subscribed = false;

    list_add(state->subscribed_services, (any_t)service);
    return service->instance_id;
}

int nan_cancel_subscribe(struct nan_service_state *state, const uint8_t subscribe_id)
{
    struct nan_service *service = NULL;
    LIST_REMOVE(state->subscribed_services, service, service->instance_id == subscribe_id);

    if (service)
    {
        free(service);
        return 0;
    }

    return -1;
}

bool nan_should_announce_service(struct nan_service *service)
{
    if (service->type == PUBLISHED)
    {
        if (service->time_to_live == 0)
            return false;

        if (service->parameters.publish.type == PUBLISH_SOLICITED &&
            !service->parameters.publish.do_publish)
            return false;

        return true;
    }

    if (service->type == SUBSCRIBED)
    {
        if (service->parameters.subscribe.type == SUBSCRIBE_PASSIVE)
            return false;

        if (service->parameters.subscribe.is_subscribed)
            return false;

        if (service->time_to_live == 0)
            return false;

        return true;
    }

    return false;
}

void nan_get_services_to_announce(const struct nan_service_state *state, list_t announced_services)
{
    struct nan_service *service;
    LIST_FILTER_FOR_EACH(state->published_services, service, nan_should_announce_service(service), {
        list_add(announced_services, (any_t)service);
    });
    LIST_FILTER_FOR_EACH(state->subscribed_services, service, nan_should_announce_service(service), {
        list_add(announced_services, (any_t)service);
    });
}

void nan_update_announced_services(list_t announced_services)
{
    struct nan_service *service;
    LIST_FOR_EACH(announced_services, service, {
        if (service->time_to_live > 0)
            service->time_to_live--;

        if (service->type == PUBLISHED)
            service->parameters.publish.do_publish = false;
    });
}

void nan_handle_received_service_discovery(const struct nan_service_state *state,
                                           struct nan_event_state *event_state,
                                           const struct ether_addr *self_address,
                                           const struct ether_addr *source_address,
                                           const struct ether_addr *destination_address,
                                           const struct nan_service_descriptor_attribute *service_descriptor)
{
    if (service_descriptor->control.service_control_type == CONTROL_TYPE_PUBLISH)
    {
        struct nan_service *service = nan_get_service_by_service_id(state, &service_descriptor->service_id, SUBSCRIBED);

        if (service == NULL)
        {
            log_trace("Received publish service discovery frame for unknown service: %s",
                      nan_service_id_to_string(&service_descriptor->service_id));
            return;
        }

        struct nan_event_discovery_result event_data;
        event_data.address = source_address;
        event_data.publish_id = service_descriptor->instance_id;
        event_data.subscribe_id = service->instance_id;
        event_data.service_update_indicator = service->service_update_indicator;
        event_data.service_specific_info = service_descriptor->service_info;
        event_data.service_specific_info_length = service_descriptor->service_info_length;

        nan_dispatch_event(event_state, EVENT_DISCOVERY_RESULT, service->service_name, &event_data);
    }

    else if (service_descriptor->control.service_control_type == CONTROL_TYPE_SUBSCRIBE)
    {
        struct nan_service *service = nan_get_service_by_service_id(state, &service_descriptor->service_id, PUBLISHED);

        if (service == NULL)
        {
            log_trace("Received subscribe service discovery frame for unknown service: %s",
                      nan_service_id_to_string(&service_descriptor->service_id));
            return;
        }

        service->parameters.publish.do_publish = true;
    }

    else if (service_descriptor->control.service_control_type == CONTROL_TYPE_FOLLOW_UP)
    {
        // Handle only follow ups to our self
        if (memcmp(self_address, destination_address, ETHER_ADDR_LEN) != 0)
            return;

        struct nan_service *service = nan_get_service_by_service_id(state, &service_descriptor->service_id, -1);
        if (service == NULL)
        {

            log_error("Received follow up service discovery frame for unknown service: %s",
                      nan_service_id_to_string(&service_descriptor->service_id));
            return;
        }

        struct nan_event_receive event_data;
        event_data.address = source_address;
        event_data.instance_id = service->instance_id;
        event_data.peer_instance_id = service_descriptor->instance_id;
        event_data.service_specific_info = service_descriptor->service_info;
        event_data.service_specific_info_length = service_descriptor->service_info_length;

        nan_dispatch_event(event_state, EVENT_RECEIVE, service->service_name, &event_data);
    }

    else
    {
        log_warn("Received service discovery frame of unknown type '%d' for service: %s",
                 service_descriptor->control.service_control_type, nan_service_id_to_string(&service_descriptor->service_id));
    }
}

void nan_updated_replied_service(struct nan_service *service)
{
    service->parameters.subscribe.is_subscribed = true;
}

char *nan_publish_type_to_string(enum nan_publish_type type)
{
    switch (type)
    {
    case PUBLISH_BOTH:
        return "BOTH";
    case PUBLISH_SOLICITED:
        return "SOLICITED";
    case PUBLISH_UNSOLICITED:
        return "UNSOLICITED";
    default:
        return "UNKNOWN";
    }
}
char *nan_subscribe_type_to_string(enum nan_subscribe_type type)
{
    switch (type)
    {
    case SUBSCRIBE_ACTIVE:
        return "ACTIVE";
    case SUBSCRIBE_PASSIVE:
        return "PASSIVE";
    default:
        return "UNKNOWN";
    }
}