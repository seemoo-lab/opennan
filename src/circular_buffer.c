#include "circular_buffer.h"

#include <stdlib.h>

// The definition of our circular buffer structure is hidden from the user
struct circular_buf
{
	void **buffer;
	size_t head;
	size_t tail;
	size_t capacity;
	bool full;
};

static void advance_pointer(circular_buf_t cbuf)
{
	if (cbuf->full)
		cbuf->tail = (cbuf->tail + 1) % cbuf->capacity;

	cbuf->head = (cbuf->head + 1) % cbuf->capacity;
	cbuf->full = (cbuf->head == cbuf->tail);
}

static void retreat_pointer(circular_buf_t cbuf)
{
	cbuf->full = 0;
	cbuf->tail = (cbuf->tail + 1) % cbuf->capacity;
}

circular_buf_t circular_buf_init(size_t size)
{
	circular_buf_t cbuf = malloc(sizeof(struct circular_buf));

	cbuf->buffer = malloc(size * sizeof(any_t));
	cbuf->capacity = size;
	circular_buf_reset(cbuf);

	return cbuf;
}

void circular_buf_free(circular_buf_t cbuf)
{
	free(cbuf->buffer);
	free(cbuf);
}

void circular_buf_reset(circular_buf_t cbuf)
{
	cbuf->head = 0;
	cbuf->tail = 0;
	cbuf->full = 0;
}

size_t circular_buf_size(circular_buf_t cbuf)
{
	if (circular_buf_full(cbuf))
		return cbuf->capacity;

	if (cbuf->head >= cbuf->tail)
		return (cbuf->head - cbuf->tail);

	return (cbuf->capacity + cbuf->head - cbuf->tail);
}

size_t circular_buf_capacity(circular_buf_t cbuf)
{
	return cbuf->capacity;
}

void circular_buf_put_override(circular_buf_t cbuf, any_t data)
{
	cbuf->buffer[cbuf->head] = data;
	advance_pointer(cbuf);
}

int circular_buf_put(circular_buf_t cbuf, any_t data)
{
	if (circular_buf_full(cbuf))
		return -1;

	cbuf->buffer[cbuf->head] = data;
	advance_pointer(cbuf);
	return 0;
}

int circular_buf_get(circular_buf_t cbuf, any_t *data, bool peek)
{
	if (*data)
		*data = NULL;

	if (circular_buf_empty(cbuf))
		return -1;

	*data = cbuf->buffer[cbuf->tail];
	if (!peek)
		retreat_pointer(cbuf);

	return 0;
}

bool circular_buf_empty(circular_buf_t cbuf)
{
	return (!cbuf->full && (cbuf->head == cbuf->tail));
}

bool circular_buf_full(circular_buf_t cbuf)
{
	return cbuf->full;
}
