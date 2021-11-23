#include "cluster.h"
#include "frame.h"
#include "utils.h"
#include "timer.h"

struct ether_addr nan_cluster_id_new()
{
    struct ether_addr cluster_id = NAN_CLUSTER_ID_BASE;

    cluster_id.ether_addr_octet[4] = get_rand_num(0, 255);
    cluster_id.ether_addr_octet[5] = get_rand_num(0, 255);
    return cluster_id;
}

void nan_cluster_state_init(struct nan_cluster_state *state)
{
    state->cluster_id = nan_cluster_id_new();
}

int nan_cluster_compare_grade(uint8_t master_preferenceA, uint64_t timestampA,
                              uint8_t master_preferenceB, uint64_t timestampB)
{
    if (master_preferenceA == master_preferenceB)
        return (timestampA & 0x7ffff) < (timestampB & 0x7ffff);
        
    return master_preferenceA < master_preferenceB;
}

uint64_t nan_calculate_cluster_grade(uint8_t master_preference, uint64_t timestamp)
{
    uint64_t cluster_grade = master_preference << 6;
    cluster_grade += timestamp & 0x7ffff;
    return cluster_grade;
}
