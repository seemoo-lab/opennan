#ifndef NAN_MOVING_AVERAGE_H
#define NAN_MOVING_AVERAGE_H

typedef struct moving_average_state
{
    void *buffer;
    int position;
    int buffer_size;
    bool buffer_full;
} moving_average_t;

#define moving_average_init(state, average, type, size) \
    do                                                  \
    {                                                   \
        average = 0;                                    \
        state.buffer = calloc(size, sizeof(type));      \
        state.position = 0;                             \
        state.buffer_full = false;                      \
        state.buffer_size = size;                       \
    } while (0)

#define moving_average_add(state, average, type, value)                            \
    do                                                                             \
    {                                                                              \
        if (state.buffer_full)                                                     \
        {                                                                          \
            type diff = (value - ((type *)state.buffer)[state.position]);          \
            average += diff / state.buffer_size;                                   \
        }                                                                          \
        else                                                                       \
        {                                                                          \
            average = ((average * state.position) + value) / (state.position + 1); \
        }                                                                          \
        ((type *)state.buffer)[state.position] = value;                            \
        state.position++;                                                          \
        if (state.position == state.buffer_size)                                   \
        {                                                                          \
            state.position = 0;                                                    \
            state.buffer_full = true;                                              \
        }                                                                          \
    } while (0)

#endif //NAN_MOVING_AVERAGE_H