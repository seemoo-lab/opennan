#ifndef CRC32_H_
#define CRC32_H_

#include <stdint.h>

/**
 * @brief Calculates a crc32 checksum for the given buffer
 *
 * @param buf: the buffer for which the checksum should be calculated
 * @param size: the size of the given buffer
 *
 * @return the checksum of the given buffer
 * */
uint32_t crc32(const void *buf, unsigned long size);

#endif /* CRC32_H_ */
