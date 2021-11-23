#ifndef NAN_CLUSTER_H_
#define NAN_CLUSTER_H_

#include <stdint.h>
#include <netinet/ether.h>

struct nan_cluster_state
{
    struct ether_addr cluster_id;
    uint64_t cluster_grade;
};

struct ether_addr nan_cluster_id_new();

void nan_cluster_state_init(struct nan_cluster_state *state);

int nan_cluster_compare_grade(uint8_t master_preferenceA, uint64_t timestampA,
                              uint8_t master_preferenceB, uint64_t timestampB);

uint64_t nan_calculate_cluster_grade(uint8_t master_preference, uint64_t timestamp);

#endif // NAN_CLUSTER_H_