#ifndef NAN_WIRE_H_
#define NAN_WIRE_H_

#include <stdint.h>
#include <net/ethernet.h>

#define BUF_MAX_LENGTH 65535

#ifdef __APPLE__

#include <machine/endian.h>
#include <libkern/OSByteOrder.h>

#define htobe16(x) OSSwapHostToBigInt16(x)
#define htole16(x) OSSwapHostToLittleInt16(x)
#define be16toh(x) OSSwapBigToHostInt16(x)
#define le16toh(x) OSSwapLittleToHostInt16(x)

#define htobe32(x) OSSwapHostToBigInt32(x)
#define htole32(x) OSSwapHostToLittleInt32(x)
#define be32toh(x) OSSwapBigToHostInt32(x)
#define le32toh(x) OSSwapLittleToHostInt32(x)

#define htobe64(x) OSSwapHostToBigInt64(x)
#define htole64(x) OSSwapHostToLittleInt64(x)
#define be64toh(x) OSSwapBigToHostInt64(x)
#define le64toh(x) OSSwapLittleToHostInt64(x)

#else

#include <endian.h>

#endif /* __APPLE__ */

/** 
 * Opaque buffer used for ethernet frames.
 */
struct buf;

/** 
 * Allocates a new buffer of given size.
 *
 * @param size Size of the buffer in bytes
 * @returns Pointer to the new buffer instance
 */
struct buf *buf_new_owned(size_t size);

/**
 * Allocates a new buffer and copies the given data into it.
 * 
 * @param data Pointer to the data to copy into the new buffer
 * @param size Size of the data 
 * @return Pointer to the new buffer instance
 */
struct buf *buf_new_copy(const uint8_t *data, size_t size);

/** 
 * Creates a new buffer on an already existing bytes array.
 *
 * @param data Immutable reference to buffer array
 * @param size Size of the buffer in bytes
 * @return Pointer to the new buffer instance
 */
struct buf *buf_new_const(const uint8_t *data, size_t size);

/** 
 * Deallocates a buffer instance and its data if not initiated using buf_new_const.
 *
 * @param buf Buffer instance to free
 */
void buf_free(struct buf *buf);

/**
 * Get the current error of the buffer
 * 
 * @param buf Buffer instance
 * @returns -1 if there was a read/write that exceeded the buffers capacity
 */
int buf_error(struct buf *buf);

/**
 * Get the current working data of the buffer
 * 
 * @param buf Buffer instance
 * @returns Pointer to the start of the buffer's data
 */
const uint8_t *buf_data(struct buf *buf);

/**
 * Get the original buffer data
 * 
 * @param buf Buffer instance
 * @returns Pointer to the start of the buffer's data
 */
const uint8_t *buf_orig_data(struct buf *buf);

/**
 * Get the pointer at the current working position in the buffer
 * 
 * @param buf The buffer instance
 * @returns Pointer to the current working position in the buffer
 */
uint8_t *buf_current(struct buf *buf);

/**
 * Get the size of the buffer's current working data.
 * 
 * @param buf The buffer instance
 * @returns The size of the buffer's working data
 */
size_t buf_size(struct buf *buf);

/**
 * Get the original size of the buffer's data
 * 
 * @param buf The buffer instance
 * @returns The size of the buffer
 */
size_t buf_orig_size(struct buf *buf);

/**
 * Get the index of current working position in the buffer
 * 
 * @param buf The buffer instance
 * @returns The index of the current working position
 */
size_t buf_position(struct buf *buf);

/**
 * Get the remaining size of the buffer after the current working position
 * 
 * @param buf The buffer instance
 * @returns The remaining size of the buffer
 */
int buf_rest(struct buf *buf);

/** 
 * Remove a porition from the front of the buffer's working data.
 *
 * @param buf The buffer instance
 * @param length The length of the porition to remove
 * @return 0 if successful or -1 if the length exceeds the buffers capacity
 */
int buf_advance(struct buf *buf, size_t length);

/** 
 * Remove a porition from the end of the buffer' working data.
 * 
 * @param buf The buffer instance
 * @param length The length of the porition to remove
 * @return 0 if successful or -1 if the length exceeds the buffers capacity
 */
int buf_take(struct buf *buf, size_t length);

/**
 * Resize the buffer. Reallocate buffer data if buffer is owned
 * 
 * @param buf The buffer instance
 * @param size The new size
 */
void buf_resize(struct buf *buf, size_t size);


/**
 * Write an unsigned byte to a buffer. 
 * Sets the error flag if buffer capacity is exceeded.
 * 
 * @param buf The buffer to write to
 * @param value The value to write
 * @returns The size of the written data or -1 on error
 */
int write_u8(struct buf *buf, uint8_t value);

/**
 * Write 2 unsigned bytes to a buffer in little endian. 
 * Sets the error flag if buffer capacity is exceeded.
 * 
 * @param buf The buffer to write to
 * @param value The value to write
 * @returns The size of the written data or -1 on error
 */
int write_le16(struct buf *buf, uint16_t value);

/**
 * Write 2 unsigned bytes to a buffer in bug endian. 
 * Sets the error flag if buffer capacity is exceeded.
 * 
 * @param buf The buffer to write to
 * @param value The value to write
 * @returns The size of the written data or -1 on error
 */
int write_be16(struct buf *buf, uint16_t value);

/**
 * Write 4 unsigned bytes to a buffer in little endian. 
 * Sets the error flag if buffer capacity is exceeded.
 * 
 * @param buf The buffer to write to
 * @param value The value to write
 * @returns The size of the written data or -1 on error
 */
int write_le32(struct buf *buf, uint32_t value);

/**
 * Write 4 unsigned bytes to a buffer in big endian. 
 * Sets the error flag if buffer capacity is exceeded.
 * 
 * @param buf The buffer to write to
 * @param value The value to write
 * @returns The size of the written data or -1 on error
 */
int write_be32(struct buf *buf, uint32_t value);

/**
 * Write 8 unsigned bytes to a buffer in little endian. 
 * Sets the error flag if buffer capacity is exceeded.
 * 
 * @param buf The buffer to write to
 * @param value The value to write
 * @returns The size of the written data or -1 on error
 */
int write_le64(struct buf *buf, uint64_t value);

/**
 * Write 8 unsigned bytes to a buffer in big endian. 
 * Sets the error flag if buffer capacity is exceeded.
 * 
 * @param buf The buffer to write to
 * @param value The value to write
 * @returns The size of the written data or -1 on error
 */
int write_be64(struct buf *buf, uint64_t value);

/**
 * Write an ether address bytes to a buffer in little endian. 
 * Sets the error flag if buffer capacity is exceeded.
 * 
 * @param buf The buffer to write to
 * @param addr The ether address to write
 * @returns The size of the written data or -1 on error
 */
int write_ether_addr(struct buf *buf, const struct ether_addr *addr);

/**
 * Write an arbitrary amount of unsigned bytes to a buffer. 
 * Sets the error flag if buffer capacity is exceeded.
 * 
 * @param buf The buffer to write to
 * @param bytes The data to write
 * @param size The size of the data
 * @returns The size of the written data or -1 on error
 */
int write_bytes(struct buf *buf, const uint8_t *bytes, size_t size);

/**
 * Read an unsigned bytes from the buffer.
 * Sets the error flag if buffer capacity is exceeded.
 *
 * @param buf The buffer to read from
 * @param value Pointer to read the data into
 * @returns The size of the read data or -1 on error
 */
int read_u8(struct buf *buf, uint8_t *value);

/**
 * Read 2 unsigned bytes from the buffer in litte endian.
 * Sets the error flag if buffer capacity is exceeded.
 *
 * @param buf The buffer to read from
 * @param value Pointer to read the data into
 * @returns The size of the read data or -1 on error
 */
int read_le16(struct buf *buf, uint16_t *value);

/**
 * Read 2 unsigned bytes from the buffer in big endian.
 * Sets the error flag if buffer capacity is exceeded.
 *
 * @param buf The buffer to read from
 * @param value Pointer to read the data into
 * @returns The size of the read data or -1 on error
 */
int read_be16(struct buf *buf, uint16_t *value);

/**
 * Read 4 unsigned bytes from the buffer in litte endian.
 * Sets the error flag if buffer capacity is exceeded.
 *
 * @param buf The buffer to read from
 * @param value Pointer to read the data into
 * @returns The size of the read data or -1 on error
 */
int read_le32(struct buf *buf, uint32_t *value);

/**
 * Read 4 unsigned bytes from the buffer in big endian.
 * Sets the error flag if buffer capacity is exceeded.
 *
 * @param buf The buffer to read from
 * @param value Pointer to read the data into
 * @returns The size of the read data or -1 on error
 */
int read_be32(struct buf *buf, uint32_t *value);

/**
 * Read 8 unsigned bytes from the buffer in litte endian.
 * Sets the error flag if buffer capacity is exceeded.
 *
 * @param buf The buffer to read from
 * @param value Pointer to read the data into
 * @returns The size of the read data or -1 on error
 */
int read_le64(struct buf *buf, uint64_t *value);

/**
 * Read 8 unsigned bytes from the buffer in big endian.
 * Sets the error flag if buffer capacity is exceeded.
 *
 * @param buf The buffer to read from
 * @param value Pointer to read the data into
 * @returns The size of the read data or -1 on error
 */
int read_be64(struct buf *buf, uint64_t *value);

/**
 * Read an ether address from the buffer.
 * Sets the error flag if buffer capacity is exceeded.
 *
 * @param buf The buffer to read from
 * @param addr Pointer to read the ether address into
 * @returns The size of the read data or -1 on error
 */
int read_ether_addr(struct buf *buf, struct ether_addr *addr);

/**
 * Read an arbitrary number of bytes from the buffer.
 * Sets the error flag if buffer capacity is exceeded.
 *
 * @param buf The buffer to read from
 * @param value Pointer to read the data into
 * @param size The size of the data to read
 * @returns The size of the read data or -1 on error
 */
int read_bytes(struct buf *buf, const uint8_t **bytes, size_t size);

/**
 * Copy an arbitrary number of bytes from the buffer.
 * Sets the error flag if buffer capacity is exceeded.
 *
 * @param buf The buffer to read from
 * @param value Pointer to copy the data into
 * @param size The size of the data to copy
 * @returns The size of the copied data or -1 on error
 */
int read_bytes_copy(struct buf *buf, uint8_t *bytes, size_t size);

#endif //NAN_WIRE_H_
