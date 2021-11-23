#include "data_path.h"

void nan_data_path_state_init(struct nan_data_path_state *state)
{
    state->data_paths = list_init();
    state->last_data_path_id = 0;
}

uint8_t nan_data_request(struct nan_data_path_state *state,
                         const uint8_t publish_id, const struct ether_addr *destination_address,
                         const char *service_specific_info, const size_t service_specific_info_length)
{
    
}