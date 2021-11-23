#ifndef NAN_CHANNEL_H_
#define NAN_CHANNEL_H_

struct nan_channel_state
{
    int master;
};

void nan_channel_state_init(struct nan_channel_state *state, int channel);

#endif // NAN_CHANNEL_H_