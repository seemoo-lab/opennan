#include "ieee80211.h"

#include <stdlib.h>
#include <errno.h>
#include <radiotap.h>
#include <radiotap_iter.h>

#include "rx.h"
#include "crc32.h"
#include "log.h"

void ieee80211_init_state(struct ieee80211_state *state)
{
    state->sequence_number = 0;
    state->fcs = true;
}

unsigned int ieee80211_state_next_sequence_number(struct ieee80211_state *state)
{
    return state->sequence_number++;
}

void ieee80211_add_nan_header(struct buf *buf, const struct ether_addr *src, const struct ether_addr *dst,
                              const struct ether_addr *bssid, struct ieee80211_state *state, const uint16_t type)
{
    struct ieee80211_hdr *hdr = (struct ieee80211_hdr *)buf_current(buf);

    hdr->frame_control = htole16(type);
    hdr->duration_id = htole16(0);
    hdr->addr1 = *dst;
    hdr->addr2 = *src;
    hdr->addr3 = *bssid;
    hdr->seq_ctrl = htole16(ieee80211_state_next_sequence_number(state) << 4);

    buf_advance(buf, sizeof(struct ieee80211_hdr));
}

inline static int ieee80211_radiotap_type_to_mask(int type)
{
    return 1 << type;
}

void ieee80211_add_radiotap_header(struct buf *buf, const struct ieee80211_state *state)
{
    /*
	 * TX radiotap headers and mac80211
	 * https://www.kernel.org/doc/Documentation/networking/mac80211-injection.txt
	 */
    struct ieee80211_radiotap_header *hdr = (struct ieee80211_radiotap_header *)buf_current(buf);
    buf_advance(buf, sizeof(struct ieee80211_radiotap_header));
    size_t length = sizeof(struct ieee80211_radiotap_header);

    hdr->it_version = 0;
    hdr->it_pad = 0;

    /* TODO Adjust PHY parameters based on receiver capabilities */
    uint32_t present = 0;

    if (state->fcs)
    {
        present |= ieee80211_radiotap_type_to_mask(IEEE80211_RADIOTAP_FLAGS);
        length += write_u8(buf, IEEE80211_RADIOTAP_F_FCS);
    }

    present |= ieee80211_radiotap_type_to_mask(IEEE80211_RADIOTAP_RATE);
    length += write_u8(buf, 2);

    present |= ieee80211_radiotap_type_to_mask(IEEE80211_RADIOTAP_DBM_ANTSIGNAL);
    length += write_u8(buf, 200);

    hdr->it_len = htole16((uint16_t)length);
    hdr->it_present = htole32(present);
}

int ieee80211_parse_radiotap_header(struct buf *frame, signed char *rssi, uint8_t *flags, uint64_t *tsft)
{
    struct ieee80211_radiotap_header *header = (struct ieee80211_radiotap_header *)buf_current(frame);
    struct ieee80211_radiotap_iterator iter;

    int err = ieee80211_radiotap_iterator_init(&iter, (struct ieee80211_radiotap_header *)buf_current(frame),
                                               buf_rest(frame), NULL);
    if (err < 0)
        return RX_UNEXPECTED_FORMAT;

    while (!(err = ieee80211_radiotap_iterator_next(&iter)))
    {
        if (iter.is_radiotap_ns)
        {
            switch (iter.this_arg_index)
            {

            case IEEE80211_RADIOTAP_TSFT:
                /* https://www.radiotap.org/fields/TSFT.html */
                if (tsft)
                    *tsft = le64toh(*(uint64_t *)iter.this_arg);
                break;
            case IEEE80211_RADIOTAP_FLAGS:
                if (flags)
                    *flags = *(unsigned char *)iter.this_arg;
                break;
            case IEEE80211_RADIOTAP_DBM_ANTSIGNAL:
                /* https://www.radiotap.org/fields/Antenna%20signal.html */
                if (rssi)
                    *rssi = *(signed char *)iter.this_arg;
                break;
            default:
                /* ignore */
                break;
            }
        }
    }

    if (err != -ENOENT)
        return RX_UNEXPECTED_FORMAT;

    return buf_advance(frame, le16toh(header->it_len));
}

int ieee80211_parse_fcs(struct buf *frame, const uint8_t radiotap_flags)
{
    if (radiotap_flags & IEEE80211_RADIOTAP_F_BADFCS)
        return -1;
    if (radiotap_flags & IEEE80211_RADIOTAP_F_FCS)
        return buf_take(frame, 4); /* strip FCS */
    return 0;
}

void ieee80211_add_fcs(struct buf *buf)
{
    write_le32(buf, crc32(buf_data(buf), buf_position(buf)));
}