#ifndef NAN_TX_H_
#define NAN_TX_H_

#include <stdint.h>
#include <net/ethernet.h>

#include "state.h"
#include "frame.h"
#include "service.h"
#include "data_path.h"

/**
 * Check if we are allowed to send a discovery beacon now.
 * 
 * @param state - The current state
 * @param now_usec - The current time in microseconds
 * @returns Whether we are allowed to send a discovery beacon
 */
bool nan_can_send_discovery_beacon(const struct nan_state *state, uint64_t now_usec);

/**
 * Add a master indication attribute to the given buffer.
 * 
 * @param buf - The buffer to write to
 * @param state - The current state
 * @returns The data of the written attribute in bytes
 */
int nan_add_master_indication_attribute(struct buf *buf, const struct nan_state *state);

/**
 * Add a cluster attribute to the given buffer.
 * 
 * @param buf - The buffer to write to
 * @param state - The current state
 * @returns The data of the written attribute in bytes
 */
int nan_add_cluster_attribute(struct buf *buf, const struct nan_state *state);

int nan_add_service_id_list_attribute(struct buf *buf, const struct nan_state *state);

int nan_add_service_descriptor_attribute(struct buf *buf, const struct nan_service *service,
                                         const enum nan_service_control_type control_type,
                                         const uint8_t requestor_instance_id,
                                         const char *service_specific_info, const size_t service_specific_info_length);

int nan_add_service_descriptor_extension_attribute(struct buf *buf, const struct nan_service *service,
                                                   const char *service_specific_info, const size_t service_specific_info_length);

int nan_add_device_capability_attribute(struct buf *buf);

int nan_add_availability_attribute(struct buf *buf);

int nan_add_data_path_attribute(struct buf *buf, const struct nan_data_path *data_path,
                                const struct ether_addr *initiator_address,
                                const enum nan_data_path_attribute_status status,
                                const enum nan_data_path_attribute_type type,
                                const uint8_t publish_id, const struct ether_addr *responder_address,
                                const char *ndp_specific_info, const size_t ndp_specific_info_length);

/**
 * Add a beacon header to the given buffer.
 * Use the `data_length` parameter to updated the length field later on.
 * 
 * @param buf - The buffer to write to
 * @param state - The current state
 * @param type - The type of beacon to add
 * @param data_length - Will be filled with a pointer to the header's length field
 * @param now_usec - Current time in microseconds
 * @returns The length of the written header in bytes
 */
void nan_add_beacon_header(struct buf *buf, struct nan_state *state, const enum nan_beacon_type type,
                           uint8_t **data_length, const uint64_t now_usec);

void nan_add_service_discovery_header(struct buf *buf, struct nan_state *state, const struct ether_addr *destination);

/**
 * Build a complete beacon frame.
 * 
 * @param buf - The buffer to write to
 * @oaram state - The current state
 * @param type - The type of beacon to build
 * @param now_usec - The current time in microseconds
 * @returns The length of the written frame in bytes
 */
void nan_build_beacon_frame(struct buf *buf, struct nan_state *state,
                            const enum nan_beacon_type type, const uint64_t now_usec);

void nan_build_service_discovery_frame(struct buf *buf, struct nan_state *state,
                                       const struct ether_addr *destination, const list_t announced_services);

/**
 * With this Method a service/application may request the NAN Discovery Engine to transmit 
 * a follow-up message with a given content to a given NAN Device and targeted to a given 
 * instance of the publish function or the subscribe function in the given NAN Device.
 * 
 * @param state - The current state
 * @param destination - Ethernet address of the destination device
 * @param instance_id - A valid `publish_id` or `subscribe_id`
 * @param request_instance_id - The `publish_id` or `subscribe_id` of the destination device
 * @param service_specific_info - Sequence of values which are to be transmitted in the frame body
 * @param service_specific_info_length - The length of the specific info
 * @returns Negative number on error, zero on success
 */
int nan_transmit(struct nan_state *state, const struct ether_addr *destination,
                 const uint8_t instance_id, const uint8_t requestor_instance_id,
                 const char *service_specific_info, const size_t service_specific_info_length);

#endif // NAN_TX_H_
