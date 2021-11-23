#ifndef NAN_FRAME_H_
#define NAN_FRAME_H_

#include <stdint.h>
#include <stdbool.h>
#include <netinet/ether.h>

#include "ieee80211.h"

#define NAN_SYNC_BEACON_FRAME_MAX_LENGTH 128
#define NAN_DISCOVERY_BEACON_FRAME_MAX_LENGTH 350
#define NAN_SYNC_BEACON_INTERVAL_TU 512
#define NAN_DISCOVERY_BEACON_INTERVAL_TU 100

#define NAN_INFORMATION_ELEMENT_FIXED_LENGTH 4
#define NAN_ATTRIBUTE_FIXED_LENGTH 3

#define NAN_BROADCAST_ADDRESS (struct ether_addr) {{ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }}
#define NAN_NETWORK_ID (struct ether_addr) {{ 0x51, 0x6f, 0x9a, 0x01, 0x00, 0x00 }}
#define NAN_CLUSTER_ID_BASE (struct ether_addr) {{ 0x50, 0x6f, 0x9a, 0x01, 0x00, 0x00 }}
#define NAN_OUI (struct oui) {{ 0x50, 0x6f, 0x9a }}
#define NAN_OUI_TYPE_BEACON 0x13
#define NAN_OUT_TYPE_SERVICE_DISCOVERY 0x13
#define NAN_OUI_TYPE_ACTION 0x18

enum nan_beacon_type
{
    NAN_SYNC_BEACON,
    NAN_DISCOVERY_BEACON
};

/** 
 * NAN synchronization/discovery beacon frame
 * sync frame limited to 128 bytes
 * discovery frame limited to 350 bytes
 */
struct nan_beacon_frame
{
    uint64_t time_stamp;
    // 512 TUs for sync, 100 TUs for discovery
    uint16_t beacon_interval;
    uint16_t capability;
    // 0xdd vendor specific information element
    uint8_t element_id;
    uint8_t length;
    struct oui oui;
    uint8_t oui_type;
} __attribute__((__packed__));

enum nan_action_frame_subtype
{
    NAF_RANGING_REQUEST = 1,
    NAF_RANGING_RESPONSE = 2,
    NAF_RANGING_TERMINATION = 3,
    NAN_RANGING_REPORT = 4,
    NAF_DATA_PATH_REQUEST = 5,
    NAF_DATA_PATH_RESPONSE = 6,
    NAF_DATA_PATH_CONFIRM = 7,
    NAF_DATA_PATH_KEY_INSTALLMENT = 8,
    NAF_DATA_PATH_TERMINATION = 9,
    NAF_SCHEDULE_REQUEST = 10,
    NAF_SCHEDULE_RESPONSE = 11,
    NAF_SCHEDULE_CONFIRM = 12,
    NAF_SCHEDULE_UPDATE_NOTIFICATION = 13
};

/**
 * NAN action frame
 */
struct nan_action_frame
{
    uint8_t category;
    uint8_t action;
    struct oui oui;
    uint8_t oui_type;
    uint8_t oui_subtype;
} __attribute__((__packed__));

/**
 * NAN service discovery frame
 */
struct nan_service_discovery_frame
{
    uint8_t category;
    uint8_t action;
    struct oui oui;
    uint8_t oui_type;
} __attribute__((__packed__));

/**
 * NAN Action Frame
 * */
struct nan_naf
{
    uint8_t category;
    uint8_t action;
    struct oui oui;
    uint8_t oui_type;
    uint8_t oui_subtype;
} __attribute__((__packed__));

int nan_get_beacon_type(uint16_t beacon_interval);

const char *nan_beacon_type_to_string(int type);

const char *nan_action_frame_subtype_to_string(int subtype);

#endif // NAN_FRAME_H_
