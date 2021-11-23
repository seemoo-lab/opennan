#ifndef CIRCULAR_BUFFER_H_
#define CIRCULAR_BUFFER_H_

#include <stdbool.h>
#include <stddef.h>

/**
 * Opaque buffer structure and type
 */
typedef struct circular_buf *circular_buf_t;

/**
 * Allow any type of data in the buffer
 */
typedef void *any_t;

/**
 * Instantiate a new circular buffer
 * 
 * @param size - The size of the buffer
 * @returns A new buffer instance
 */
circular_buf_t circular_buf_init(size_t size);

/**
 * Free a circular buffer instance.
 * 
 * @param cbuf - The buffer instance
 */
void circular_buf_free(circular_buf_t cbuf);

/**
 * Reset the buffer into an empty state.
 * Does not clear the data.
 * 
 * @param cbuf - The buffer instance
 */
void circular_buf_reset(circular_buf_t cbuf);

/**
 * Store data into the buffer while override existing data if the buffer is full.
 * 
 * @param cbuf - The buffer instance
 * @param data - Data to store
 */
void circular_buf_put_override(circular_buf_t cbuf, any_t data);

/**
 * Store data in the buffer except if it is full.
 * 
 * @param cbuf - The buffer instance
 * @param data - Data to store
 * @return -1 if buffer is full, 0 otherwise
 */
int circular_buf_put(circular_buf_t cbuf, any_t data);

/**
 * Store data in the buffer except if it is full.
 * 
 * @param cbuf - The buffer instance
 * @param data - Pointer to data that will be updated
 * @param peek - If true, will not pop the data
 * @return -1 if buffer is empty, 0 otherwise
 */
int circular_buf_get(circular_buf_t cbuf, any_t *data, bool peek);

/**
 * Check if the buffer is empty.
 * 
 * @param cbuf - The buffer instance
 * @returns True if buffer is empty
 */
bool circular_buf_empty(circular_buf_t cbuf);

/**
 * Check if the buffer is full.
 * 
 * @param cbuf - The buffer instance
 * @returns True if buffer is empty
 */
bool circular_buf_full(circular_buf_t cbuf);

/**
 * Get the maximum capacity of the buffer.
 * 
 * @param cbuf - The buffer instance
 * @returns The maximum capacity
 */
size_t circular_buf_capacity(circular_buf_t cbuf);

/**
 * Get the number of elements stored in the buffer.
 * 
 * @param cbuf - The buffer instance
 * @returns The number of stored elements
 */
size_t circular_buf_size(circular_buf_t cbuf);

#endif // CIRCULAR_BUFFER_H_
