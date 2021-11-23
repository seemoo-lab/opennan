#ifndef NAN_IEEE80211_H_
#define NAN_IEEE80211_H_

#include <stdint.h>
#include <stdbool.h>
#include <net/ethernet.h>

#include "wire.h"

/* Some relevant Ethernet Protocol IDs */
#define ETH_P_IP 0x0800   /* Internet Protocol packet	*/
#define ETH_P_IPV6 0x86DD /* IPv6 over bluebook		*/

#define OUI_LEN 3

struct oui
{
    uint8_t byte[OUI_LEN];
} __attribute__((__packed__));

#define FCS_LEN 4

#define IEEE80211_FCTL_VERS 0x0003
#define IEEE80211_FCTL_FTYPE 0x000c
#define IEEE80211_FCTL_STYPE 0x00f0
#define IEEE80211_FCTL_TODS 0x0100
#define IEEE80211_FCTL_FROMDS 0x0200
#define IEEE80211_FCTL_MOREFRAGS 0x0400
#define IEEE80211_FCTL_RETRY 0x0800
#define IEEE80211_FCTL_PM 0x1000
#define IEEE80211_FCTL_MOREDATA 0x2000
#define IEEE80211_FCTL_PROTECTED 0x4000
#define IEEE80211_FCTL_ORDER 0x8000
#define IEEE80211_FCTL_CTL_EXT 0x0f00

#define IEEE80211_SCTL_FRAG 0x000F
#define IEEE80211_SCTL_SEQ 0xFFF0

#define IEEE80211_FTYPE_MGMT 0x0000
#define IEEE80211_FTYPE_CTL 0x0004
#define IEEE80211_FTYPE_DATA 0x0008
#define IEEE80211_FTYPE_EXT 0x000c

/* management */
#define IEEE80211_STYPE_ASSOC_REQ 0x0000
#define IEEE80211_STYPE_ASSOC_RESP 0x0010
#define IEEE80211_STYPE_REASSOC_REQ 0x0020
#define IEEE80211_STYPE_REASSOC_RESP 0x0030
#define IEEE80211_STYPE_PROBE_REQ 0x0040
#define IEEE80211_STYPE_PROBE_RESP 0x0050
#define IEEE80211_STYPE_BEACON 0x0080
#define IEEE80211_STYPE_ATIM 0x0090
#define IEEE80211_STYPE_DISASSOC 0x00A0
#define IEEE80211_STYPE_AUTH 0x00B0
#define IEEE80211_STYPE_DEAUTH 0x00C0
#define IEEE80211_STYPE_ACTION 0x00D0

#define IEEE80211_PUBLIC_ACTION_FRAME 0x04
#define IEEE80211_PUBLIC_ACTION_FRAME_VENDOR_SPECIFIC 0x09
#define IEEE80211_PROTECTED_DUAL_OF_PUBLIC_ACTION_FRAME 0x09

/* control */
#define IEEE80211_STYPE_CTL_EXT 0x0060
#define IEEE80211_STYPE_BACK_REQ 0x0080
#define IEEE80211_STYPE_BACK 0x0090
#define IEEE80211_STYPE_PSPOLL 0x00A0
#define IEEE80211_STYPE_RTS 0x00B0
#define IEEE80211_STYPE_CTS 0x00C0
#define IEEE80211_STYPE_ACK 0x00D0
#define IEEE80211_STYPE_CFEND 0x00E0
#define IEEE80211_STYPE_CFENDACK 0x00F0

/* data */
#define IEEE80211_STYPE_DATA 0x0000
#define IEEE80211_STYPE_DATA_CFACK 0x0010
#define IEEE80211_STYPE_DATA_CFPOLL 0x0020
#define IEEE80211_STYPE_DATA_CFACKPOLL 0x0030
#define IEEE80211_STYPE_NULLFUNC 0x0040
#define IEEE80211_STYPE_CFACK 0x0050
#define IEEE80211_STYPE_CFPOLL 0x0060
#define IEEE80211_STYPE_CFACKPOLL 0x0070
#define IEEE80211_STYPE_QOS_DATA 0x0080
#define IEEE80211_STYPE_QOS_DATA_CFACK 0x0090
#define IEEE80211_STYPE_QOS_DATA_CFPOLL 0x00A0
#define IEEE80211_STYPE_QOS_DATA_CFACKPOLL 0x00B0
#define IEEE80211_STYPE_QOS_NULLFUNC 0x00C0
#define IEEE80211_STYPE_QOS_CFACK 0x00D0
#define IEEE80211_STYPE_QOS_CFPOLL 0x00E0
#define IEEE80211_STYPE_QOS_CFACKPOLL 0x00F0

/* extension, added by 802.11ad */
#define IEEE80211_STYPE_DMG_BEACON 0x0000

/* control extension - for IEEE80211_FTYPE_CTL | IEEE80211_STYPE_CTL_EXT */
#define IEEE80211_CTL_EXT_POLL 0x2000
#define IEEE80211_CTL_EXT_SPR 0x3000
#define IEEE80211_CTL_EXT_GRANT 0x4000
#define IEEE80211_CTL_EXT_DMG_CTS 0x5000
#define IEEE80211_CTL_EXT_DMG_DTS 0x6000
#define IEEE80211_CTL_EXT_SSW 0x8000
#define IEEE80211_CTL_EXT_SSW_FBACK 0x9000
#define IEEE80211_CTL_EXT_SSW_ACK 0xa000

/* 
    Maximum size for the MA-UNITDATA primitive, 802.11 standard section
    6.2.1.1.2.
    802.11e clarifies the figure in section 7.1.2. The frame body is
    up to 2304 octets long (maximum MSDU size) plus any crypt overhead.
*/
#define IEEE80211_MAX_DATA_LEN 2304
/* 30 byte 4 addr hdr, 2 byte QoS, 2304 byte MSDU, 12 byte crypt, 4 byte FCS */
#define IEEE80211_MAX_FRAME_LEN 2352

#define IEEE80211_QOS_CTL_LEN 2
/* 1d tag mask */
#define IEEE80211_QOS_CTL_TAG1D_MASK 0x0007
/* TID mask */
#define IEEE80211_QOS_CTL_TID_MASK 0x000f
/* EOSP */
#define IEEE80211_QOS_CTL_EOSP 0x0010
/* ACK policy */
#define IEEE80211_QOS_CTL_ACK_POLICY_NORMAL 0x0000
#define IEEE80211_QOS_CTL_ACK_POLICY_NOACK 0x0020
#define IEEE80211_QOS_CTL_ACK_POLICY_NO_EXPL 0x0040
#define IEEE80211_QOS_CTL_ACK_POLICY_BLOCKACK 0x0060
#define IEEE80211_QOS_CTL_ACK_POLICY_MASK 0x0060
/* A-MSDU 802.11n */
#define IEEE80211_QOS_CTL_A_MSDU_PRESENT 0x0080
/* Mesh Control 802.11s */
#define IEEE80211_QOS_CTL_MESH_CONTROL_PRESENT 0x0100

/* Mesh Power Save Level */
#define IEEE80211_QOS_CTL_MESH_PS_LEVEL 0x0200
/* Mesh Receiver Service Period Initiated */
#define IEEE80211_QOS_CTL_RSPI 0x0400

/*
 * 802.11n Management Action Frames
 *
 * Adapted from 'ieee80211_hdr_3addr' in <linux/ieee80211.h>
 */
struct ieee80211_hdr
{
    uint16_t frame_control;
    uint16_t duration_id;
    /* dst */
    struct ether_addr addr1;
    /* src */
    struct ether_addr addr2;
    /* bssid */
    struct ether_addr addr3;
    uint16_t seq_ctrl;
} __attribute__((__packed__));

struct ieee80211_state
{
    /* IEEE 802.11 sequence number */
    uint16_t sequence_number;
    /* whether we need to add an fcs */
    bool fcs;
};

void ieee80211_init_state(struct ieee80211_state *state);

int ieee80211_channel_to_frequency(int chan);
int ieee80211_frequency_to_channel(int freq);

void ieee80211_add_nan_header(struct buf *buf, const struct ether_addr *src, const struct ether_addr *dst,
                              const struct ether_addr *bssid, struct ieee80211_state *state, const uint16_t type);

void ieee80211_add_radiotap_header(struct buf *buf, const struct ieee80211_state *state);
int ieee80211_parse_radiotap_header(struct buf *frame, signed char *rssi, uint8_t *flags, uint64_t *tsft);

void ieee80211_add_fcs(struct buf *buf);
int ieee80211_parse_fcs(struct buf *frame, const uint8_t radiotap_flags);

#endif //NAN_IEEE80211_H_
