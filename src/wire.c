#include "wire.h"

#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>

#include "log.h"
struct buf
{
    const uint8_t *data;
    uint8_t *current;
    int start;
    int end;
    size_t size;
    bool owned;
    int error;
};

struct buf *buf_new_owned(size_t size)
{
    struct buf *buf = (struct buf *)malloc(sizeof(struct buf));
    buf->data = (const uint8_t *)malloc(size);
    buf->current = &(*(uint8_t *)buf->data);
    buf->size = size;
    buf->start = 0;
    buf->end = size;
    buf->owned = true;
    buf->error = 0;
    return buf;
}

struct buf *buf_new_copy(const uint8_t *data, size_t size)
{
    struct buf *buf = buf_new_owned(size);
    memcpy((void *)buf->data, data, size);

    return buf;
}

struct buf *buf_new_const(const uint8_t *data, size_t size)
{
    struct buf *buf = (struct buf *)malloc(sizeof(struct buf));
    buf->data = data;
    buf->current = &(*(uint8_t *)buf->data);
    buf->size = size;
    buf->start = 0;
    buf->end = size;
    buf->owned = false;
    buf->error = 0;
    return buf;
}

void buf_free(struct buf *buf)
{
    if (buf->owned)
        free((void *)buf->data);
    free(buf);
}

int buf_error(struct buf *buf)
{
    return buf->error;
}

uint8_t *buf_current(struct buf *buf)
{
    return buf->current;
}

const uint8_t *buf_data(struct buf *buf)
{
    return buf->data + buf->start;
}

const uint8_t *buf_orig_data(struct buf *buf)
{
    return buf->data + buf->start;
}

size_t buf_size(struct buf *buf)
{
    return buf->end - buf->start;
}

size_t buf_orig_size(struct buf *buf)
{
    return buf->size;
}

size_t buf_position(struct buf *buf)
{
    return abs(buf->data - buf->current);
}

int buf_rest(struct buf *buf)
{
    return buf->end - buf_position(buf);
}

int buf_advance(struct buf *buf, size_t length)
{
    if ((int)length > buf_rest(buf))
    {
        buf->error = -1;
        return -1;
    }
    buf->current += length;
    return length;
}

int buf_strip(struct buf *buf, size_t length)
{
    if (length > buf_position(buf))
    {
        buf->error = -1;
        return -1;
    }

    buf->start += length;
    return length;
}

int buf_take(struct buf *buf, size_t length)
{
    if (buf_rest(buf) < (int)length)
    {
        buf->error = -1;
        return -1;
    }

    buf->end -= length;
    return length;
}

void buf_resize(struct buf *buf, size_t size)
{
    if (buf->owned)
    {
        buf->data = (const uint8_t *)realloc((void *)buf->data, size);
    }
    buf->size = size;
}

#define write(type, to)                    \
    if (buf_rest(buf) < (int)sizeof(type)) \
    {                                      \
        buf->error = -1;                   \
        return 0;                          \
    }                                      \
    *(type *)buf->current = to(value);     \
    buf->current += sizeof(type);          \
    return sizeof(type);

int write_u8(struct buf *buf, uint8_t value)
{
    write(uint8_t, )
}

int write_le16(struct buf *buf, uint16_t value)
{
    write(uint16_t, htole16)
}

int write_be16(struct buf *buf, uint16_t value)
{
    write(uint16_t, htobe16)
}

int write_le32(struct buf *buf, uint32_t value)
{
    write(uint32_t, htole32)
}

int write_be32(struct buf *buf, uint32_t value)
{
    write(uint32_t, htobe32)
}

int write_le64(struct buf *buf, uint64_t value)
{
    write(uint64_t, htole64)
}

int write_be64(struct buf *buf, uint64_t value)
{
    write(uint64_t, htobe64)
}

int write_ether_addr(struct buf *buf, const struct ether_addr *addr)
{
    if (buf_rest(buf) < ETHER_ADDR_LEN)
    {
        buf->error = -1;
        return -1;
    }
    *(struct ether_addr *)buf->current = *addr;
    buf->current += ETHER_ADDR_LEN;
    return ETHER_ADDR_LEN;
}

int write_bytes(struct buf *buf, const uint8_t *bytes, size_t size)
{
    if (buf_rest(buf) < (int)size)
    {
        buf->error = -1;
        return -1;
    }
    memcpy(buf->current, bytes, size);
    buf->current += size;
    return size;
}

#define read(type, to)                      \
    if (buf_rest(buf) < (int)sizeof(type))  \
    {                                       \
        buf->error = -1;                    \
        return 0;                           \
    }                                       \
    if (value)                              \
        *value = to(*(type *)buf->current); \
    buf->current += sizeof(type);           \
    return sizeof(type);

int read_u8(struct buf *buf, uint8_t *value)
{
    read(uint8_t, )
}

int read_le16(struct buf *buf, uint16_t *value)
{
    read(uint16_t, le16toh)
}

int read_be16(struct buf *buf, uint16_t *value)
{
    read(uint16_t, be16toh)
}

int read_le32(struct buf *buf, uint32_t *value)
{
    read(uint32_t, le32toh)
}

int read_be32(struct buf *buf, uint32_t *value)
{
    read(uint32_t, be32toh)
}

int read_le64(struct buf *buf, uint64_t *value)
{
    read(uint64_t, le64toh)
}

int read_be64(struct buf *buf, uint64_t *value)
{
    read(uint64_t, be64toh)
}

int read_ether_addr(struct buf *buf, struct ether_addr *addr)
{
    if (ETHER_ADDR_LEN > buf_rest(buf))
    {
        buf->error = -1;
        return 0;
    }
    if (addr)
        *addr = *(struct ether_addr *)buf->current;

    return ETHER_ADDR_LEN;
}

int read_bytes(struct buf *buf, const uint8_t **bytes, size_t length)
{
    if (buf_rest(buf) < (int)length)
    {
        buf->error = -1;
        return 0;
    }
    if (bytes)
        *bytes = buf->current;
    buf->current += length;
    return length;
}

int read_bytes_copy(struct buf *buf, uint8_t *bytes, size_t length)
{
    if (buf_rest(buf) < (int)length)
    {
        buf->error = -1;
        return 0;
    }
    if (bytes)
        memcpy(bytes, buf->current, length);
    buf->current += length;
    return length;
}
