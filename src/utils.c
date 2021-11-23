#include "utils.h"

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void print_frame(const uint8_t *frame, const size_t length, FILE *fp)
{
    if (fp == NULL)
        fp = stdout;

    for (unsigned int i = 0 & ~15; i < length; i++)
    {
        if ((i & 15) == 0)
            fprintf(fp, "%04x ", i);
        fprintf(fp, "%02x%c", (unsigned char)frame[i], ((i + 1) & 15) ? ' ' : '\n');
    }
    fprintf(fp, "\n");
}

char *ether_addr_to_string(const struct ether_addr *addr)
{
    static char current = 0;
    static char buf1[18];
    // Extra buffer to support 2 ether addresses in one printf
    static char buf2[18];
    if (current == 0)
    {
        sprintf(buf1, "%.2x:%.2x:%.2x:%.2x:%.2x:%.2x",
                addr->ether_addr_octet[0], addr->ether_addr_octet[1],
                addr->ether_addr_octet[2], addr->ether_addr_octet[3],
                addr->ether_addr_octet[4], addr->ether_addr_octet[5]);
        current = 1;
        return buf1;
    }
    else
    {
        sprintf(buf2, "%.2x:%.2x:%.2x:%.2x:%.2x:%.2x",
                addr->ether_addr_octet[0], addr->ether_addr_octet[1],
                addr->ether_addr_octet[2], addr->ether_addr_octet[3],
                addr->ether_addr_octet[4], addr->ether_addr_octet[5]);
        current = 0;
        return buf2;
    }
}

char *ipv6_addr_to_string(const struct in6_addr *ipv6_addr)
{
    static char buf[41];
    sprintf(buf, "%.4x:%.4x:%.4x:%.4x:%.4x:%.4x:%.4x:%.4x:",
            ipv6_addr->s6_addr16[0], ipv6_addr->s6_addr16[1],
            ipv6_addr->s6_addr16[2], ipv6_addr->s6_addr16[3],
            ipv6_addr->s6_addr16[4], ipv6_addr->s6_addr16[5],
            ipv6_addr->s6_addr16[6], ipv6_addr->s6_addr16[7]);

    return buf;
}

void ether_addr_to_ipv6_addr(const struct ether_addr *ether_addr, struct in6_addr *ipv6_addr)
{
    memset(ipv6_addr, 0, sizeof(struct in6_addr));
    ipv6_addr->s6_addr[0] = 0xfe;
    ipv6_addr->s6_addr[1] = 0x80;
    ipv6_addr->s6_addr[8] = ether_addr->ether_addr_octet[0] ^ 0x02;
    ipv6_addr->s6_addr[9] = ether_addr->ether_addr_octet[1];
    ipv6_addr->s6_addr[10] = ether_addr->ether_addr_octet[2];
    ipv6_addr->s6_addr[11] = 0xff;
    ipv6_addr->s6_addr[12] = 0xfe;
    ipv6_addr->s6_addr[13] = ether_addr->ether_addr_octet[3];
    ipv6_addr->s6_addr[14] = ether_addr->ether_addr_octet[4];
    ipv6_addr->s6_addr[15] = ether_addr->ether_addr_octet[5];
}

bool ether_addr_equal(const struct ether_addr *addrA, const struct ether_addr *addrB)
{
    for (unsigned int i = 0; i < sizeof(struct ether_addr); i++)
    {
        if (addrA->ether_addr_octet[i] != addrB->ether_addr_octet[i])
            return false;
    }
    return true;
}

int compare_ether_addr(const struct ether_addr *addrA, const struct ether_addr *addrB)
{
    for (unsigned int i = 0; i < sizeof(struct ether_addr); i++)
    {
        if (addrA->ether_addr_octet[i] != addrB->ether_addr_octet[i])
            return addrA->ether_addr_octet[i] - addrB->ether_addr_octet[i];
    }
    return 0;
};

bool oui_equal(const struct oui oui1, const struct oui oui2)
{
    for (unsigned int i = 0; i < sizeof(struct oui); i++)
    {
        if (oui1.byte[i] != oui2.byte[i])
        {
            return false;
        }
    }
    return true;
}

uint64_t clock_time_usec()
{
    int result;
    struct timespec now;
    uint64_t now_us = 0;

    result = clock_gettime(CLOCK_MONOTONIC, &now);
    if (!result)
    {
        now_us = now.tv_sec * 1000000;
        now_us += now.tv_nsec / 1000;
    }
    return now_us;
}

uint8_t get_rand_num(uint8_t min, uint8_t max)
{
    uint16_t range = max - min + 1;

    uint32_t chunk_size = RAND_MAX / range;
    uint32_t end_of_last_chunk = chunk_size * range;

    uint32_t r = rand();
    while (r >= end_of_last_chunk)
    {
        r = rand();
    }

    return min + r / chunk_size;
}

uint8_t increase_non_zero_id(uint8_t *last_id)
{
    (*last_id)++;
    if (*last_id == 0)
        (*last_id)++;

    return *last_id;
}