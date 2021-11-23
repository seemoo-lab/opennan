#ifndef NAN_DATA_PATH_H_
#define NAN_DATA_PATH_H_

#include <stdint.h>
#include <net/ethernet.h>

#include "list.h"

struct nan_data_path_state
{
    list_t data_paths;
    uint8_t last_data_path_id;
};

struct nan_data_path
{
    const uint8_t data_path_id;
    const uint8_t service_instance_id;
    const struct ether_addr *destination_address;
};

void nan_data_path_state_init(struct nan_data_path_state *state);

uint8_t nan_data_request(struct nan_data_path_state *state,
                         const uint8_t publish_id, const struct ether_addr *destination_address,
                         const char *service_specific_info, const size_t service_specific_info_length);

#endif // NAN_DATA_PATH_H_