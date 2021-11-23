#ifndef NAN_SERVICE_H_
#define NAN_SERVICE_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <netinet/ether.h>

#include "event.h"
#include "list.h"
#include "circular_buffer.h"
#include "attributes.h"

enum nan_service_type
{
    PUBLISHED,
    SUBSCRIBED
};

enum nan_publish_type
{
    PUBLISH_UNSOLICITED,
    PUBLISH_SOLICITED,
    PUBLISH_BOTH,
};

enum nan_subscribe_type
{
    SUBSCRIBE_PASSIVE,
    SUBSCRIBE_ACTIVE
};

struct nan_service
{
    char *service_name;
    enum nan_service_type type;
    uint8_t instance_id;
    struct nan_service_id service_id;
    void *service_specific_info;
    size_t service_specific_info_length;
    uint8_t service_update_indicator;
    int time_to_live;
    union
    {
        struct
        {
            enum nan_publish_type type;
            /** 
             * If publish type is solicited only and received matching subscribe,
             * announce in next service discovery  
             */
            bool do_publish;
        } publish;
        struct
        {
            enum nan_subscribe_type type;
            /** 
             * If subscribe type is active, stop announcement once published match was received 
             */
            bool is_subscribed;
        } subscribe;
    } parameters;
};

struct nan_service_state
{
    list_t published_services;
    list_t subscribed_services;
    uint8_t last_instance_id;
};

/**
 * Initiate service state.
 * 
 * @param state - The service state to init
 */
void nan_service_state_init(struct nan_service_state *state);

/**
 * Find a matching service by its service id
 * 
 * @param state - The current service state
 * @param service_id - The service id to identify the service
 * @param type - The type of service. Either published, subscribed or -1 for both
 * @return A matching service
 */
struct nan_service *nan_get_service_by_service_id(const struct nan_service_state *state,
                                                  const struct nan_service_id *service_id,
                                                  const int type);

/**
 * Find a matching service by its instance id
 * 
 * @param state - The current service state
 * @param instance_id - The instance id to identify the service
 * @param type - The type of service. Either published, subscribed or -1 for both
 * @return A matching service
 */
struct nan_service *nan_get_service_by_instance_id(const struct nan_service_state *state,
                                                   const uint8_t instance_id,
                                                   const int type);

/**
 * Find a matching service by its name 
 * 
 * @param state - The current service state
 * @param service_name - The service name to identify the service
 * @param type - The type of service. Either published, subscribed or -1 for both
 * @return A matching service
 */
struct nan_service *nan_get_service_by_name(const struct nan_service_state *state,
                                            const char *service_name,
                                            const int type);

/**
 * Convert the given service id into a readable string
 * 
 * @param service_id - The service id to convert
 * @returns The service id as string
 */
char *nan_service_id_to_string(const struct nan_service_id *service_id);

/**
 * With this method a service/application makes a service discoverable 
 * with given parameters for other NAN Devices by publishing it.
 * 
 * @param state - The current service state
 * @param service_name - UTF-8 name string which identifies the service/application
 * @param type - The publish type of the service
 * @param time_to_live - Number of times a unsolicited publish frame is transmitted or -1 for no limit
 * @param service_specific_info - Sequence of values that are conveyed in the publish message
 * @param service_specific_info_length - The length of the specific info
 * @returns Zero on error, otherwise non-zero `publish_id` that uniquely identifies the instance of the publish function on this device
 */
uint8_t nan_publish(struct nan_service_state *state,
                    const char *service_name,
                    enum nan_publish_type type,
                    int time_to_live,
                    const void *service_specific_info,
                    const size_t service_specific_info_length);

/**
 * With this Method, a service/application requests the NAN Discovery Engine to indicate that the service specific
 * information corresponding to the instance of the publish function has changed. The updated service specific information
 * may be conveyed using publish messages and/or FSD messages.
 * 
 * @param state - The current service state
 * @param publish_id - The ID originally returned by an instance of the publish function
 * @param service_specific_info - Sequence of values that are conveyed in the publish message
 * @param service_specific_info_length - The length of the specific info
 * @returns Negative number on error, zero on success
 */
int nan_update_publish(struct nan_service_state *state, const uint8_t publish_id,
                       const void *service_specific_info, const size_t service_specific_info_length);

/**
 * With this method a service/application requests cancellation of an instance of the publish function.
 * 
 * @param state - The current service state
 * @param publish_id - The ID originally returned by an instance of the publish function
 * @returns Negative number on error, zero on success
 */
int nan_cancel_publish(struct nan_service_state *state, uint8_t publish_id);

/**
 * With this method a service/application requests that the NAN Discovery Engine 
 * to search for a service based on parameters given from other NAN Devices.
 * 
 * @param state - The current service state
 * @param service_name - UTF-8 name string which identifies the service/application
 * @param type - The subscribe type of the service
 * @param time_to_live - Number of times a unsolicited publish frame is transmitted or -1 for no limit
 * @param service_specific_info - Sequence of values which further specify the published service beyond the service name
 * @param service_specific_info_length - The length of the specific info
 * @returns Zero on error, otherwise non-zero `subscribe_id` that uniquely identifies the instance of the subscribe function on this device
 */
uint8_t nan_subscribe(struct nan_service_state *state,
                      const char *service_name,
                      enum nan_subscribe_type type,
                      int time_to_live,
                      const void *service_specific_info,
                      const size_t service_specific_info_length);

/**
 * With this method a service/application requests cancellation of an instance of the subscribe function.
 * 
 * @param state - The current service state
 * @param subscribe_id - The ID originally returned by an instance of the subscribe function
 * @returns Negative number on error, zero on success
 */
int nan_cancel_subscribe(struct nan_service_state *state, const uint8_t subscribe_id);

/**
 * Whether to actively send service information for published and subscribed services.
 * - A published service will be announced if its publish type is set to unsolicited or both
 * - A subscribed service will be announced if its subscribe type is set to active
 * 
 * @param service - The service to check
 * @returns Whether the service should be announced to other devices
 */
bool nan_should_announce_service(struct nan_service *service);

/**
 * Get all services that should be announced.
 * 
 * @param state - The current service state
 * @param announced_services - The list of announced services that will be filled
 */
void nan_get_services_to_announce(const struct nan_service_state *state, list_t announced_services);

/**
 * Update the services after the transmission of a service discovery frame.
 * 
 * @param announced_services - The list of announced services to update
 */
void nan_update_announced_services(list_t announced_services);

void nan_handle_received_service_discovery(const struct nan_service_state *state,
                                           struct nan_event_state *event_state,
                                           const struct ether_addr *self_address,
                                           const struct ether_addr *source_address,
                                           const struct ether_addr *destination_address,
                                           const struct nan_service_descriptor_attribute *service_descriptor);

/**
 * Convert the given publish type into a descriptive string
 * 
 * @param type - The publish type to convert
 * @return The publish type as string
 */
char *nan_publish_type_to_string(enum nan_publish_type type);

/**
 * Convert the given subscribe type into a descriptive string
 * 
 * @param type - The subscribe type to convert
 * @return The subscribe type as string
 */
char *nan_subscribe_type_to_string(enum nan_subscribe_type type);

#endif // NAN_SERVICE_H_