#ifndef NAN_EVENT_H_
#define NAN_EVENT_H_

#include <netinet/ether.h>

#include "list.h"

struct nan_event_discovery_result
{
    /**
     * Identifier that was originally returned by the instance of the subscribe function
     */
    uint8_t subscribe_id;
    /**
     * Sequence of values that were decoded from a frame received from the publisher
     */
    char *service_specific_info;
    /**
     * The length of the service specific info
     */
    size_t service_specific_info_length;
    /**
     * Version of the service specific information corresponding to the publish instance, 
     * which may be conveyed by publish messages and/or FSD messages
     */
    uint8_t service_update_indicator;
    /**
     * Identifier for the instance of the published service on a remote NAN Device
     */
    uint8_t publish_id;
    /**
     * NAN interface address of the publisher
     */
    const struct ether_addr *address;
};

struct nan_event_replied
{
    /**
     * The Identifier originally returned by the Publish function instance
     */
    uint8_t publish_id;
    /**
     * NAN interface address of the subscriber that triggered the transmission of the publish message
     */
    struct ether_addr *address;
    /**
     * `subscribe_id` obtained from the Subscribe message
     */
    uint8_t subscribe_id;
    /**
     * Sequence of values which were decoded from a frame received from the subscriber
     */
    char *service_specific_info;
    /**
     * The length of the service specific info
     */
    size_t service_specific_info_length;
};

enum nan_publish_subscribe_termination_reason
{
    TIMEOUT = 0,
    USER_REQUEST = 1,
    FAILURE = 2
};

struct nan_event_publish_terminated
{
    /**
     * The Identifier originally returned by the publish function instance
     */
    uint8_t publish_id;
    /**
     * Timeout, user request or Failure
     */
    enum nan_publish_subscribe_termination_reason reason;
};

struct nan_event_subscribe_terminated
{
    /**
     * The Identifier originally returned by the suscribe function instance
     */
    uint8_t subscribe_id;
    /**
     * Timeout, user request or Failure
     */
    enum nan_publish_subscribe_termination_reason reason;
};

struct nan_event_receive
{
    /**
     * The original `publish_id` or `subscribe_id` returned by publish or the subscribe function instance respectively
     */
    uint8_t instance_id;
    /**
     * Identifier of the Publish function or the Subscribe function in the NAN Device from which 
     * this follow-up message was received. It may not be present for ranging report
     */
    uint8_t peer_instance_id;
    /**
     * Sequence of values that were decoded from the received frame
     */
    char *service_specific_info;
    /**
     * The length of the service specific info
     */
    size_t service_specific_info_length;
    /**
     * NAN interface address of the NAN Device from which the frame was received
     */
    const struct ether_addr *address;
};

enum nan_event_type
{
    EVENT_DISCOVERY_RESULT,
    EVENT_REPLIED,
    EVENT_PUBLISH_TERMINATED,
    EVENT_SUBSCRIBE_TERMINATED,
    EVENT_RECEIVE
};

typedef void (*nan_event_listener_t)(enum nan_event_type event, void *event_data, void *additional_data);

struct nan_event_state
{
    list_t listeners;
};

void nan_event_state_init(struct nan_event_state *state);

/**
 * 
 * @param state - The current event state
 */

void nan_add_event_listener(struct nan_event_state *state,
                            const enum nan_event_type event, const char *service_name,
                            const nan_event_listener_t listener, void *additional_data);

void nan_remove_event_listener(struct nan_event_state *state, nan_event_listener_t listener);

void nan_dispatch_event(struct nan_event_state *state, enum nan_event_type event,
                        const char *service_name, void *data);

char *nan_event_type_to_string(enum nan_event_type type);

#endif // NAN_EVENT_H_