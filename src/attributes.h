#ifndef NAN_ATTRIBUTES_H_
#define NAN_ATTRIBUTES_H_

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <net/ethernet.h>

#include "ieee80211.h"
#include "wire.h"

#define NAN_SERVICE_ID_LENGTH 6

/**
 * Structure for the nan service id that consists of the first 6 octets 
 * of the sha256 hash of the lower-case service name.
 */
struct nan_service_id
{
    uint8_t byte[NAN_SERVICE_ID_LENGTH];
} __attribute__((__packed__));

enum nan_attribute_type
{
    MASTER_INDICATION_ATTRIBUTE = 0x00,
    CLUSTER_ATTRIBUTE = 0x01,
    SERVICE_ID_LIST_ATTRIBUTE = 0x02,
    SERVICE_DESCRIPTOR_ATTRIBUTE = 0x03,
    NAN_CONNECTION_CAPABILITY_ATTRIBUTE = 0x04,
    WLAN_INFRASTRUCTURE_ATTRIBUTE = 0x05,
    P2P_OPERATION_ATTRIBUTE = 0x06,
    IBSS_ATTRIBUTE = 0x07,
    MESH_ATTRIBUTE = 0x08,
    FURTHER_NAN_SERVICE_DISCOVERY_ATTRIBUTE = 0x09,
    FURTHER_AVAILABILITY_MAP_ATTRIBUTE = 0x0a,
    COUNTRY_CODE_ATTRIBUTE = 0x0b,
    RANGING_ATTRIBUTE = 0x0c,
    CLUSTER_DISCOVERY_ATTRIBUTE = 0x0d,
    SERVICE_DESCRIPTOR_EXTENSION_ATTRIBUTE = 0x0e,
    DEVICE_CAPABILITY_ATTRIBUTE = 0x0f,
    NDP_ATTRIBUTE = 0x10,
    NMSG_ATTRIBUTE = 0x11, // Reserved
    NAN_AVAILABILITY_ATTRIBUTE = 0x12,
    NDC_ATTRIBUTE = 0x13,
    NDL_ATTRIBUTE = 0x14,
    NDL_QOS_ATTRIBUTE = 0x15,
    MULTICAST_SCHEDULE_ATTRIBUTE = 0x16, // Reserved
    UNALIGNED_SCHEDULE_ATTRIBUTE = 0x17,
    PAGING_UNICAST_ATTRIBUTE = 0x18,   // Reserved
    PAGING_MULTICAST_ATTRIBUTE = 0x19, // Reserved
    RANGING_INFORMATION_ATTRIBUTE = 0x1a,
    RANGING_SETUP_ATTRIBUTE = 0x1b,
    FTM_RANGING_REPORT_ATTRIBUTE = 0x1c,
    ELEMENT_CONTAINER_ATTRIBUTE = 0x1d,
    EXTENDED_WLAN_INFRASTRUCTURE_ATTRIBUTE = 0x1e,
    EXTENDED_P2P_OPERATION_ATTRIBUTE = 0x1f,
    EXTENDED_IBSS_ATTRIBUTE = 0x20,
    EXTENDED_MESH_ATTRIBUTE = 0x21,
    CIPHER_SUITE_INFO_ATTRIBUTE = 0x22,
    SECURITY_CONTEXT_INFO_ATTRIBUTE = 0x23,
    SHARED_KEY_DESCRIPTOR_ATTRIBUTE = 0x24,
    MULTICAST_SCHEDULE_CHANGE_ATTRIBUTE = 0x25,       // Reserved
    MULTICAST_SCHEDULE_OWNER_CHANGE_ATTRIBUTE = 0x26, // Reserved
    PUBLIC_AVAILABILITY_ATTRIBUTE = 0x27,
    SUBSCRIBE_SERVICE_ID_LIST_ATTRIBUTE = 0x28,
    NDP_EXTENSION_ATTRIBUTE = 0x29,
    VENDOR_SPECIFIC_ATTRIBUTE = 0xdd,
};

struct nan_attribute_header
{
    uint8_t id;
    uint16_t length;
} __attribute__((__packed__));

struct nan_master_indication_attribute
{
    uint8_t id;
    uint16_t length;
    uint8_t master_preference;
    uint8_t random_factor;
} __attribute__((__packed__));

struct nan_cluster_attribute
{
    uint8_t id;
    uint16_t length;
    uint64_t anchor_master_rank;
    uint8_t hop_count;
    uint32_t anchor_master_beacon_transmission_time;
} __attribute__((__packed__));

enum nan_service_control_type
{
    CONTROL_TYPE_PUBLISH = 0,
    CONTROL_TYPE_SUBSCRIBE = 1,
    CONTROL_TYPE_FOLLOW_UP = 2,
};

struct nan_service_descriptor_control
{
    unsigned service_control_type : 2;
    unsigned matching_filter_present : 1;
    unsigned service_response_filter_present : 1;
    unsigned service_info_present : 1;
    unsigned discovery_range_limited : 1;
    unsigned binding_bitmap_present : 1;
    unsigned reserved : 1;
} __attribute__((__packed__));

struct nan_service_descriptor_attribute
{
    struct nan_service_id service_id;
    uint8_t instance_id;
    uint8_t requestor_instance_id;
    struct nan_service_descriptor_control control;
    char *service_info;
    uint8_t service_info_length;
};

struct nan_service_descriptor_extension_control
{
    unsigned further_service_discovery_required : 1;
    unsigned further_service_discovery_with_gas : 1;
    unsigned data_path_required : 1;
    unsigned data_path_type : 1;
    unsigned multicast_type : 1;
    unsigned qos_required : 1;
    unsigned security_required : 1;
    unsigned ranging_required : 1;
    unsigned range_limit_present : 1;
    unsigned service_update_indicator_present : 1;
    unsigned reserved : 6;
} __attribute__((__packed__));

struct nan_service_descriptor_extension_attribute
{
    uint8_t instance_id;
    struct nan_service_descriptor_extension_control control;
    uint32_t range_limit;
    uint8_t service_update_indicator;
    uint16_t service_info_length;
    struct oui oui;
    uint8_t service_protocol_type;
    char *service_specific_info;
    size_t service_specific_info_length;
};

struct nan_device_capability_attribute
{
    uint8_t id;
    uint16_t length;
    struct
    {
        unsigned specified : 1;
        unsigned value : 4;
        unsigned reserved : 3;
    } __attribute__((__packed__)) map_id;
    struct
    {
        // Wake up in 2.4 GHz each 2^(n-1)th dw or no wake up if n = 0
        unsigned dw_2_4_ghz : 3;
        // Wake up in 5 GHz each 2^(n-1)th dw or no wake up if n = 0
        unsigned dw_5_ghz : 3;
        // Map ID that overrides wake up in 2.4 GHz
        unsigned overwrite_dw_2_4_ghz : 4;
        // Map ID that overrides wake up in 5 GHz
        unsigned overwrite_dw_5_ghz : 4;
        unsigned reserved : 2;
    } __attribute__((__packed__)) committed_dw_info;
    struct
    {
        unsigned tv_white_spaces : 1;
        unsigned sub_1_ghz : 1;
        unsigned band_2_4_ghz : 1;
        unsigned band_3_6_ghz : 1;
        unsigned band_4_9_and_5_ghz : 1;
        unsigned band_60_ghz : 1;
        unsigned reserved : 2;
    } __attribute__((__packed__)) supported_bands;
    struct
    {
        // 1 = VHT, 0 = HT
        unsigned phy_mode : 1;
        // 1 = VHT 80+80 support
        unsigned vht_80_plus_80 : 1;
        // 1 = VHT 160 support
        unsigned vht_160 : 1;
        // 1 = P-NDL support
        unsigned paging_ndl_support : 1;
        unsigned reserved : 4;
    } __attribute__((__packed__)) operation_mode;
    // 0 = no information available
    unsigned number_of_tx_antennas : 4;
    // 0 = no information available
    unsigned number_of_rx_antennas : 4;
    // switch time in microseconds, 0 = no information available
    uint16_t max_channel_switch_time;
    struct
    {
        // 1 = device is a DSF master
        unsigned dsf_master : 1;
        // 1 = device supports IEEE 802.11 extended key id mechanism
        unsigned extended_key_id : 1;
        // 1 = device supports receiving NDP packets from same NDI pair in multiple channels
        unsigned simultaneous_ndp_data_reception : 1;
        // 1 = device supports NDPE attribute
        unsigned ndpe_attribute_support : 1;
        unsigned reserved : 4;
    } __attribute__((__packed__)) capabilities;

} __attribute__((__packed__));

enum nan_availability_band_entry
{
    BAND_TV_WHITE_SPACES = 0,
    BAND_SUB_1_GHZ = 1,
    BAND_2_4_GHZ = 2,
    BAND_3_6_GHZ = 3,
    BAND_4_9_AND_5_GHZ = 4,
    BAND_60_GHZ = 5
};

struct nan_availability_channel_entry
{
    // Global operating class
    uint8_t operating_class;
    uint16_t channel_bitmap;
    uint8_t primary_channel_bitmap;
    uint16_t auxiliary_channel_bitmap;
} __attribute__((__packed__));

struct nan_availability_band_channel_entry_control
{
    // 0 = list of indicated bands, 1 = list of operating classes and channel entries
    unsigned type : 1;
    // Set to 1 if at least one channel entry indicates non-contiguous bandwidth
    unsigned non_contiguous_bandwidth : 1;
    unsigned reserved : 2;
    // Number of band or channel entries, 0 is reserved
    unsigned number_of_entries : 4;
} __attribute__((__packed__));

enum nan_availability_type
{
    AVAILABILITY_COMITTED = 1,
    AVAILABILITY_POTENTIAL = 2,
    AVAILABILITY_CONDITIONAL = 4,
};

struct nan_availability_entry_control
{
    // At least one bit must be set; 101, 111 are reserved
    unsigned availability_type : 3;
    // represents the preference of being available in the associated FAWs
    unsigned usage_preference : 2;
    // 0-5 indicating 20% proportion within FAWs already utilized, 7 indicates unknown utilization
    unsigned utilization : 3;
    // Max number of spartial streams device can receive during FAWs
    unsigned max_spartial_streams : 4;
    // 0 = all NAN slots available, 1 = time bitmap control, length, and fields are present
    unsigned time_bitmap_present : 1;
    unsigned reserved : 3;
} __attribute__((__packed__));

struct nan_availability_time_bitmap_control
{
    // Duration in TU calculated as 2^(n+4), 0 <= n <= 3
    unsigned duration : 3;
    // Repeat period calculated as 2^(n+6), 1 <= p <= n, 0 == no repetition
    unsigned period : 3;
    // Start offset in TU calculated as DW0 + 16*n
    unsigned start_offset : 9;
    unsigned reserved : 1;
} __attribute__((__packed__));

struct nan_availability_attribute_control
{
    // Identify associated availability attribute
    unsigned map_id : 4;
    // Set to 1 if committed availability has changed
    unsigned committed_changed : 1;
    // Set to 1 if potential availability has changed
    unsigned potential_changed : 1;
    // Set to 1 if public availability attribute has changed
    unsigned public_availability_attribute_changed : 1;
    // Set to 1 if ndc attribute has changed
    unsigned ndc_attribute_changed : 1;
    // Set to 1 if multicast schedule attribute has changed
    unsigned multicast_schedule_attribute_changed : 1;
    // Set to 1 if multicast schedule change attribute has changed
    unsigned multicast_schedule_change_attribute_changed : 1;
    unsigned reserved : 6;
} __attribute__((__packed__));

enum nan_data_path_attribute_type
{
    REQUEST = 0,
    RESPONSE = 1,
    CONFIRM = 2,
    SECURITY_INSTALL = 3,
    TERMINATE = 4
};

enum nan_data_path_attribute_status
{
    CONTINUED = 0,
    ACCEPTED = 1,
    REJECTED = 2,
};

struct nan_data_oath_attribute_control
{
    unsigned confirm_required : 1;
    unsigned reserved_1 : 1;
    unsigned security_present : 1;
    unsigned publish_id_present : 1;
    unsigned responder_interface_address_present : 1;
    unsigned data_path_specific_info_present : 1;
    unsigned reserved_2 : 2;
} __attribute__((__packed__));

struct nan_data_path_attribute_fixed
{
    uint8_t id;
    uint16_t length;
    uint8_t dialog_token;
    unsigned type : 4;
    unsigned status : 4;
    uint8_t reason_code;
    struct ether_addr initiator_address;
    uint8_t data_path_id;
    struct nan_data_oath_attribute_control control;
} __attribute__((__packed__));

const char *nan_attribute_type_as_string(uint8_t attribute_type);

#endif // NAN_ATTRIBUTES_H_