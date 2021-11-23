#ifndef NAN_UTILS_H_
#define NAN_UTILS_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <netinet/ether.h>
#include <netinet/in.h>

#include "ieee80211.h"

#define USEC_TO_TU(usec) (usec) / 1024
#define TU_TO_USEC(tu) (tu) * 1024

#define USEC_TO_SEC(usec) (usec) / 1000000
#define SEC_TO_USEC(sec) (sec) * 1000000

#define min(a, b) a < b ? a : b

void print_frame(const uint8_t *frame, const size_t length, FILE *fp);

/**
 * Converts the given ether address to a string.
 * In comparison to `ether_ntoa`, prints each octet with a fixed length of two digits.
 * 
 * @param addr
 * @return The address as formatted string
 */
char *ether_addr_to_string(const struct ether_addr *addr);

/**
 * Converts the given IPv6 address to a string.
 * 
 * @param ipv6_addr
 * @return The address as formatted string
 */
char *ipv6_addr_to_string(const struct in6_addr *ipv6_addr);

/**
 * Compare two ether addresses for equality.
 * 
 * @param addrA
 * @param addrB
 * @returns Whether given ether addresses are equal
 */
bool ether_addr_equal(const struct ether_addr *addrA, const struct ether_addr *addrB);

/**
 * Compare two ether addresses.
 * 
 * @param addrA
 * @param addrB
 * @returns -1 if addrA < addrB, 0 if addrA == addrB, 1 if addrA > addrB
 */
int compare_ether_addr(const struct ether_addr *addrA, const struct ether_addr *addrB);

/** 
 * Convert the given ether address to an IPv6 address using the algorithm from rfc4291.
 * 
 * @param ether_addr - The source ether address
 * @param ipv6_addr - The target IPv6 address
 */
void ether_addr_to_ipv6_addr(const struct ether_addr *ether_addr, struct in6_addr *ipv6_addr);

/**
 * Compare two OUI for equality.
 * 
 * @param oui1
 * @param oui2
 * @returns Whether given OUIs are equal
 */
bool oui_equal(const struct oui oui1, const struct oui oui2);

/**
 * Get the current time in microseconds.
 * 
 * @returns Current time in microseconds
 */
uint64_t clock_time_usec();

/**
 * Get a random number between a defined interval.
 * 
 * @param min - Lower bound 
 * @param max - Upper bound
 * @returns A random number between min and max
 */
uint8_t get_rand_num(uint8_t min, uint8_t max);

uint8_t increase_non_zero_id(uint8_t *last_id);

#endif // NAN_UTILS_H_