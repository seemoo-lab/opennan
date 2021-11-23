#include "channel.h"

void nan_channel_state_init(struct nan_channel_state *state, int channel)
{
    state->master = channel;
};

/* adapted from iw/util.c */
int ieee80211_channel_to_frequency(int chan)
{
    /* see 802.11 17.3.8.3.2 and Annex J
	 * there are overlapping channel numbers in 5GHz and 2GHz bands */
    if (chan <= 0)
        return 0; /* not supported */

    /* 2 GHz band */
    if (chan == 14)
        return 2484;
    else if (chan < 14)
        return 2407 + chan * 5;

    /* 5 GHz band */
    if (chan < 32)
        return 0; /* not supported */

    if (chan >= 182 && chan <= 196)
        return 4000 + chan * 5;
    else
        return 5000 + chan * 5;
}

/* from iw/util.c */
int ieee80211_frequency_to_channel(int freq)
{
    /* see 802.11-2007 17.3.8.3.2 and Annex J */
    if (freq == 2484)
        return 14;
    else if (freq < 2484)
        return (freq - 2407) / 5;
    else if (freq >= 4910 && freq <= 4980)
        return (freq - 4000) / 5;
    else if (freq <= 45000) /* DMG band lower limit */
        return (freq - 5000) / 5;
    else if (freq >= 58320 && freq <= 64800)
        return (freq - 56160) / 2160;
    else
        return 0;
}