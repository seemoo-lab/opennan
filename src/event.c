#include "event.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct nan_event_listeners_item
{
    // The type of the event
    enum nan_event_type event;
    // Trigger event only for specific service or all if NULL
    char *service_name;
    // Listener function
    nan_event_listener_t listener;
    // Additional data passed to listener
    void *additional_data;
};

void nan_event_state_init(struct nan_event_state *state)
{
    state->listeners = list_init();
}

void nan_add_event_listener(struct nan_event_state *state,
                            const enum nan_event_type event, const char *service_name,
                            const nan_event_listener_t listener, void *additional_data)
{
    struct nan_event_listeners_item *item = malloc(sizeof(struct nan_event_listeners_item));
    item->event = event;
    item->listener = listener;
    item->additional_data = additional_data;
    item->service_name = NULL;

    if (service_name != NULL)
    {
        item->service_name = malloc(strlen(service_name) + 1);
        strcpy(item->service_name, service_name);
    }

    list_add(state->listeners, (any_t)item);
}

void nan_remove_event_listener(struct nan_event_state *state, nan_event_listener_t listener)
{
    struct nan_event_listeners_item *item = NULL;
    LIST_REMOVE(state->listeners, item, item->listener == listener);
}

bool matches_event(const struct nan_event_listeners_item *item,
                   const enum nan_event_type event, const char *service_name)
{
    if (item->event != event)
        return false;

    if (item->service_name != NULL)
        return strstr(service_name, item->service_name) == service_name;

    return true;
}

void nan_dispatch_event(struct nan_event_state *state, enum nan_event_type event,
                        const char *service_name, void *data)
{
    struct nan_event_listeners_item *item = NULL;
    LIST_FILTER_FOR_EACH(state->listeners, item,
                         matches_event(item, event, service_name),
                         item->listener(event, data, item->additional_data));
}

char *nan_event_type_to_string(enum nan_event_type type)
{
    switch (type)
    {
    case EVENT_DISCOVERY_RESULT:
        return "DISCOVERY RESULT";
    case EVENT_RECEIVE:
        return "RECEIVE";
    case EVENT_REPLIED:
        return "REPLIED";
    case EVENT_PUBLISH_TERMINATED:
        return "PUBLISH TERMINATED";
    case EVENT_SUBSCRIBE_TERMINATED:
        return "SUBSCRIBE TERMINATED";
    default:
        return "UNKNOWN";
    }
}