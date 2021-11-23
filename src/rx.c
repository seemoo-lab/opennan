#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <radiotap.h>
#include <radiotap_iter.h>

#include "rx.h"
#include "attributes.h"
#include "utils.h"
#include "log.h"
#include "ieee80211.h"
#include "timer.h"
#include "frame.h"
#include "list.h"
#include "tx.h"
#include "sync.h"

const char *nan_rx_result_to_string(const int result)
{
    switch (result)
    {
    case RX_IGNORE_SYNC_OUTSIDE_DW:
        return "ignore sync beacon outside dw";
    case RX_IGNORE_OUI:
        return "ignore oui";
    case RX_IGNORE_PEER:
        return "ignore peer";
    case RX_IGNORE_RSSI:
        return "ignore rssi";
    case RX_IGNORE_FAILED_CRC:
        return "ignore failed crc";
    case RX_IGNORE_NOPROMISC:
        return "ignore nopromisc";
    case RX_IGNORE_FROM_SELF:
        return "ignore from self";
    case RX_IGNORE:
        return "ignore";
    case RX_OK:
        return "ok";
    case RX_UNEXPECTED_FORMAT:
        return "unexpected format";
    case RX_UNEXPECTED_TYPE:
        return "unexpected type";
    case RX_UNEXPECTED_VALUE:
        return "unexpected value";
    case RX_TOO_SHORT:
        return "too short";
    case RX_MISSING_MANDATORY_ATTRIBUTE:
        return "missing mandatory attribute";
    default:
        return "unknown result";
    }
}

/**
 * Parse the data of master indication attribute from a 
 * received beacon and write it to the given peer.
 * 
 * @param buf - The buffer that contains the attribute's data
 * @param peer - The peer that has send the beacon
 * @returns - 0 on success, a negative value otherwise
 */
int nan_parse_master_indication_attribute(struct buf *buf, struct nan_peer *peer)
{
    uint8_t master_preference;
    uint8_t random_factor;
    read_u8(buf, &master_preference);
    read_u8(buf, &random_factor);

    if (buf_error(buf))
        return RX_TOO_SHORT;

    nan_peer_set_master_indication(peer, master_preference, random_factor);

    return RX_OK;
}

/**
 * Parse the data of cluster attribute from a 
 * received beacon and write it to the given peer.
 * 
 * @param buf - The buffer that contains the attribute's data
 * @param peer - The peer that has send the beacon
 * @returns - 0 on success, a negative value otherwise
 */
int nan_parse_cluster_attribute(struct buf *buf, struct nan_peer *peer)
{
    uint64_t anchor_master_rank;
    uint32_t ambtt;
    uint8_t hop_count;

    read_le64(buf, &anchor_master_rank);
    read_u8(buf, &hop_count);
    read_le32(buf, &ambtt);

    if (buf_error(buf))
        return RX_TOO_SHORT;

    nan_peer_set_anchor_master_information(peer, anchor_master_rank, ambtt, hop_count);

    return RX_OK;
}

/**
 * Parse the data of service descriptor attribute and add it to the given list.
 * 
 * @param buf - The buffer that contains the attribute's data
 * @param service_descriptors - A list of service descriptors
 * @returns - 0 on success, a negative value otherwise
 */
int nan_parse_sda(struct buf *buf, list_t service_descriptors)
{
    struct nan_service_descriptor_attribute *attribute =
        malloc(sizeof(struct nan_service_descriptor_attribute));

    read_bytes_copy(buf, (uint8_t *)&attribute->service_id, NAN_SERVICE_ID_LENGTH);
    read_u8(buf, &attribute->instance_id);
    read_u8(buf, &attribute->requestor_instance_id);
    read_u8(buf, (uint8_t *)&attribute->control);

    if (attribute->control.binding_bitmap_present)
        buf_advance(buf, 2);

    if (attribute->control.matching_filter_present)
    {
        uint8_t length;
        read_u8(buf, &length);
        buf_advance(buf, length);
    }

    if (attribute->control.service_response_filter_present)
    {
        uint8_t length;
        read_u8(buf, &length);
        buf_advance(buf, length);
    }

    if (attribute->control.service_info_present)
    {
        read_u8(buf, &attribute->service_info_length);
        attribute->service_info = malloc(attribute->service_info_length);
        read_bytes_copy(buf, (uint8_t *)attribute->service_info, attribute->service_info_length);
    }

    if (buf_error(buf))
    {
        free(attribute);
        return RX_TOO_SHORT;
    }

    if (service_descriptors)
        list_add(service_descriptors, (any_t)attribute);

    return RX_OK;
}

/**
 * Parse the data of service descriptor extension attribute and add it to the given list.
 * 
 * @param buf - The buffer that contains the attribute's data
 * @param service_descriptor_extensions - A list of service descriptor extensions
 * @returns - 0 on success, a negative value otherwise
 */
int nan_parse_sdea(struct buf *buf, size_t length, list_t service_descriptor_extensions)
{
    struct nan_service_descriptor_extension_attribute *attribute =
        malloc(sizeof(struct nan_service_descriptor_extension_attribute));

    read_u8(buf, &attribute->instance_id);
    read_le16(buf, (uint16_t *)&attribute->control);

    if (attribute->control.range_limit_present)
        buf_advance(buf, 4);

    if (attribute->control.service_update_indicator_present)
        read_u8(buf, &attribute->service_update_indicator);

    if (buf_position(buf) + 2 < length)
    {
        uint16_t length;
        read_le16(buf, &length);
        read_bytes_copy(buf, (uint8_t *)&attribute->oui, OUI_LEN);
        buf_advance(buf, 1);

        attribute->service_specific_info_length = length - 4;
        read_bytes(buf, (const uint8_t **)&attribute->service_specific_info, attribute->service_specific_info_length);
    }

    if (buf_error(buf))
    {
        free(attribute);
        return RX_TOO_SHORT;
    }

    if (service_descriptor_extensions)
        list_add(service_descriptor_extensions, (any_t)attribute);

    return RX_OK;
}
/*
int nan_handle_availability_attribute(struct nan_state *state, struct buf *buf,
                                      struct nan_peer *peer, size_t length)
{
    unsigned int offset = 0;
    uint8_t sequence_id;
    struct nan_availability_attribute_control *attribute_control;

    read_u8(buf, sequence_id);
    read_le16(buf, (uint16_t *)attribute_control);

    while (offset < length)
    {
        uint16_t entry_length;
        struct nan_availability_entry_control *entry_control;

        read_le16(buf, &entry_length);
        read_le16(buf, (uint16_t *)entry_control);

        if (entry_control->time_bitmap_present == 0)
        {
        }
        else
        {
            struct nan_availability_time_bitmap_control *time_bitmap_control;
            uint8_t time_bitmap_length;

            read_le16(buf, (uint16_t *)time_bitmap_control);
            read_u8(buf, &time_bitmap_length);
        }
    }
}
*/

/**
 * Read the next attribute from the frame
 * 
 * @param frame The frame buffer pointing to the start of a NAN attribute
 * @param attribute_id Pointer that will be set to the parsed attribtue id
 * @param attribute_length Pointer that will be set to the parsed attribtue length
 * @param data Pointer that will be set to start of the attribute's data
 */
int nan_attribute_read_next(struct buf *frame, uint8_t *attribute_id,
                            uint16_t *attribute_length, const uint8_t **data)
{
    read_u8(frame, attribute_id);
    read_le16(frame, attribute_length);
    read_bytes(frame, data, *attribute_length);

    if (buf_error(frame) < 0)
        return RX_TOO_SHORT;

    return 3 + (int)*attribute_length;
};

/**
 * Mactro to iterate through the NAN attribtues of a frame. 
 * Available variables in handle function:
 *  * attribute_id - Parsed attribute id
 *  * attribute_length - Parsed attribute length
 *  * attribute_buf - Buffer of attribute data
 * 
 * @param handle Handle function called for each attribute
 */
#define NAN_ITERATE_ATTRIBUTES(frame, handle)                           \
    do                                                                  \
    {                                                                   \
        const uint8_t *attribute_data;                                  \
        uint8_t attribute_id;                                           \
        uint16_t attribute_length;                                      \
        int length;                                                     \
        while (0 < (length = nan_attribute_read_next(frame,             \
                                                     &attribute_id,     \
                                                     &attribute_length, \
                                                     &attribute_data))) \
        {                                                               \
            struct buf *attribute_buf =                                 \
                buf_new_const(attribute_data, attribute_length);        \
            handle;                                                     \
            buf_free(attribute_buf);                                    \
            if (result < 0)                                             \
            {                                                           \
                log_warn("Could not parse nan attribute: %s",           \
                         nan_attribute_type_as_string(attribute_id));   \
                break;                                                  \
            }                                                           \
        }                                                               \
        if (result < 0)                                                 \
            break;                                                      \
        if (buf_rest(frame) > 0)                                        \
        {                                                               \
            result = RX_UNEXPECTED_FORMAT;                              \
            break;                                                      \
        }                                                               \
        result = RX_OK;                                                 \
    } while (0);

int nan_parse_beacon_header(struct buf *frame, int *beacon_type, uint64_t *timestamp)
{
    uint16_t beacon_interval;
    uint16_t capability;
    uint8_t element_id;
    uint8_t length;
    struct oui oui;
    uint8_t oui_type;

    read_le64(frame, timestamp);
    read_le16(frame, &beacon_interval);
    read_le16(frame, &capability);
    read_u8(frame, &element_id);
    read_u8(frame, &length);
    read_bytes_copy(frame, (uint8_t *)&oui, OUI_LEN);
    read_u8(frame, &oui_type);

    if (buf_error(frame))
        return RX_TOO_SHORT;

    if (!oui_equal(oui, NAN_OUI) || oui_type != NAN_OUI_TYPE_BEACON)
        return RX_IGNORE_OUI;

    *beacon_type = nan_get_beacon_type(beacon_interval);
    if (*beacon_type < 0)
    {
        log_warn("Unknown beacon interval %d", beacon_interval);
        return RX_UNEXPECTED_TYPE;
    }

    return RX_OK;
}

int nan_rx_beacon(struct buf *frame, struct nan_state *state,
                  const struct ether_addr *peer_address, const struct ether_addr *cluster_id,
                  const signed char rssi, const uint64_t now_usec)
{
    uint64_t timestamp = 0;
    int beacon_type = 0;
    int result = 0;

    if ((result = nan_parse_beacon_header(frame, &beacon_type, &timestamp)) != RX_OK)
        return result;

    log_trace("nan_beacon: received %s beacon from cluster %s",
              nan_beacon_type_to_string(beacon_type),
              ether_addr_to_string(cluster_id));

    struct nan_peer *peer = NULL;
    enum peer_status peer_status = nan_peer_add(&state->peers, peer_address,
                                                cluster_id, now_usec);
    if (peer_status < 0)
    {
        log_warn("nan_beacon: could not add peer: %s (%d)",
                 ether_addr_to_string(peer_address), peer_status);
        return RX_IGNORE;
    }

    nan_peer_get(&state->peers, peer_address, &peer);
    if (peer == NULL)
    {
        log_warn("nan_beacon: could not get peer: %s (%d)",
                 ether_addr_to_string(peer_address), peer_status);
        return RX_IGNORE;
    }

    if (peer_status == PEER_ADD)
    {
        nan_timer_state_init(&peer->timer, state->timer.base_time_usec, state->timer.average_error_state);
        nan_timer_state_init(&peer->old_timer, state->timer.base_time_usec, state->timer.average_error_state);
    }

    log_trace("nan_beacon: received %s beacon from peer %s",
              nan_beacon_type_to_string(beacon_type),
              ether_addr_to_string(peer_address));

    if (!nan_timer_initial_scan_done(&state->timer, now_usec))
        nan_timer_initial_scan_cancel(&state->timer);

    NAN_ITERATE_ATTRIBUTES(frame, {
        switch (attribute_id)
        {
        case MASTER_INDICATION_ATTRIBUTE:
            result = nan_parse_master_indication_attribute(attribute_buf, peer);
            break;
        case CLUSTER_ATTRIBUTE:
            result = nan_parse_cluster_attribute(attribute_buf, peer);
            break;
        default:
            log_trace("Unhandled attribute: %s", nan_attribute_type_as_string(attribute_id));
            result = RX_IGNORE;
        }
    });

    if (result < 0)
        return result;

    if (peer->anchor_master_rank != peer->last_anchor_master_rank)
    {
        if (nan_is_master_rank_issuer(&state->self_address, peer->anchor_master_rank))
        {
            log_debug("Peer %s selected us as anchor master", ether_addr_to_string(&peer->addr));
        }
        else if (nan_is_master_rank_issuer(&peer->addr, peer->anchor_master_rank))
        {
            log_debug("Peer %s selected itself as anchor master", ether_addr_to_string(&peer->addr));
        }
        else
        {
            log_debug("Peer %s selected other peer %s as achor master:",
                      ether_addr_to_string(&peer->addr),
                      ether_addr_to_string(nan_get_address_from_master_rank(&peer->anchor_master_rank)));
        }
    }

    nan_peer_set_beacon_information(peer, rssi, timestamp);
    nan_update_master_preference(&state->sync, peer, now_usec);
    nan_check_master_candidate(&state->sync, peer);

    peer->last_beacon_time = now_usec;

    bool is_new_cluster = !ether_addr_equal(cluster_id, &state->cluster.cluster_id);
    bool in_initial_cluster = list_len(state->peers.peers) == 1 && peer_status == PEER_ADD;
    if (is_new_cluster || in_initial_cluster)
    {
        uint64_t synced_time_usec = nan_timer_get_synced_time_usec(&state->timer, now_usec);
        int result = nan_cluster_compare_grade(state->sync.master_preference, synced_time_usec,
                                               peer->master_preference, timestamp);

        if (result > 0)
        {
            state->cluster.cluster_id = *cluster_id;
            nan_timer_sync_time(&state->timer, now_usec, timestamp);
            log_debug("Joined new cluster: %s", ether_addr_to_string(cluster_id));
        }
        else
        {
            log_trace("Found cluster with lower cluster grade: %s", ether_addr_to_string(cluster_id));
        }
    }
    else if (state->desync)
    {
        if (beacon_type == NAN_SYNC_BEACON)
            peer->count_sync++;
        
        nan_timer_sync_error(&peer->timer, now_usec, timestamp);

        nan_timer_sync_time(&peer->old_timer, now_usec, timestamp);
        peer->old_timer_send_count = 0;

        log_debug("Peer %s not in sync", ether_addr_to_string(&peer->addr));
    }
    else if (beacon_type == NAN_SYNC_BEACON)
    {
        if (nan_is_anchor_master(&state->sync, &peer->addr))
            nan_timer_sync_time(&state->timer, now_usec, timestamp);
        else
            nan_timer_sync_error(&state->timer, now_usec, timestamp);

        uint64_t synced_time_tu = nan_timer_get_synced_time_tu(&state->timer, now_usec);
        nan_anchor_master_selection(&state->sync, peer, synced_time_tu);
    }
    else if (beacon_type == NAN_DISCOVERY_BEACON)
    {
        if (!nan_is_anchor_master(&state->sync, &peer->addr))
            nan_timer_sync_error(&state->timer, now_usec, timestamp);
    }

    return RX_OK;
}

int nan_rx_service_discovery(struct buf *frame, struct nan_state *state,
                             const struct ether_addr *destination_address,
                             const struct ether_addr *cluster_id,
                             struct nan_peer *peer, uint64_t now_usec)
{
    (void)state;
    (void)cluster_id;

    if (peer->forward)
    {
        struct nan_peer *target_peer = NULL;
        if (ether_addr_equal(destination_address, &NAN_NETWORK_ID))
        {
            LIST_FIND(state->peers.peers, target_peer,
                      !ether_addr_equal(&target_peer->addr, &peer->addr));
        }
        else
        {
            LIST_FIND(state->peers.peers, target_peer,
                      ether_addr_equal(&target_peer->addr, destination_address));
        }

        if (target_peer != NULL && circular_buf_empty(target_peer->frame_buffer))
        {
            struct buf *frame_copy = buf_new_copy(buf_data(frame), buf_size(frame));
            struct buf *frame_forward = buf_new_owned(buf_size(frame));

            // Strip radiotap header from frame copy
            ieee80211_parse_radiotap_header(frame_copy, NULL, NULL, NULL);

            // Add radiotap header to forwarded frame
            struct ieee80211_state ieee80211_state = {.fcs = true};
            ieee80211_add_radiotap_header(frame_forward, &ieee80211_state);

            // Copy ieee80211 header
            //struct ieee80211_hdr *ieee80211 = (struct ieee80211_hdr *)buf_current(frame_copy);
            //ieee80211->addr2 = state->self_address;
            write_bytes(frame_forward, buf_current(frame_copy), sizeof(struct ieee80211_hdr));
            buf_advance(frame_copy, sizeof(struct ieee80211_hdr));

            // Copy sdf header
            write_bytes(frame_forward, buf_current(frame_copy), sizeof(struct nan_service_discovery_frame));
            buf_advance(frame_copy, sizeof(struct nan_service_discovery_frame));

            // Use vendor specific attribute to mark frame as forwarded
            if (*buf_current(frame_copy) != VENDOR_SPECIFIC_ATTRIBUTE)
            {
                write_u8(frame_forward, VENDOR_SPECIFIC_ATTRIBUTE);
                write_le16(frame_forward, 3);
                write_bytes(frame_forward, (uint8_t *)&((struct oui){{0xa2, 0xdf, 0xff}}), 3);

                int result;
                NAN_ITERATE_ATTRIBUTES(frame_copy, {
                    switch (attribute_id)
                    {
                    case SERVICE_DESCRIPTOR_ATTRIBUTE:
                    {
                        struct nan_service_descriptor_attribute *attribute =
                            malloc(sizeof(struct nan_service_descriptor_attribute));

                        write_u8(frame_forward, attribute_id);
                        write_le16(frame_forward, attribute_length);

                        // copy service id, instance id, requestor instance id, control
                        write_bytes(frame_forward, buf_current(attribute_buf), NAN_SERVICE_ID_LENGTH + 3);

                        // read control
                        buf_advance(attribute_buf, NAN_SERVICE_ID_LENGTH + 2);
                        read_u8(attribute_buf, (uint8_t *)&attribute->control);

                        if (attribute->control.binding_bitmap_present)
                        {
                            write_bytes(frame_forward, buf_current(attribute_buf), 2);
                            buf_advance(attribute_buf, 2);
                        }

                        if (attribute->control.matching_filter_present)
                        {
                            uint8_t length;
                            read_u8(attribute_buf, &length);

                            write_u8(frame_forward, length);
                            write_bytes(frame_forward, buf_current(attribute_buf), length);
                            buf_advance(attribute_buf, length);
                        }

                        if (attribute->control.service_response_filter_present)
                        {
                            uint8_t length;
                            read_u8(attribute_buf, &length);

                            write_u8(frame_forward, length);
                            write_bytes(frame_forward, buf_current(attribute_buf), length);
                            buf_advance(attribute_buf, length);
                        }

                        if (attribute->control.service_info_present)
                        {
                            uint8_t length = 0;
                            char *message;
                            read_u8(attribute_buf, &length);
                            read_bytes(attribute_buf, (const uint8_t **)&message, length);

                            if (peer->modify)
                            {
                                write_u8(frame_forward, 7);
                                write_bytes(frame_forward, (uint8_t *)"#0000ff", 7);
                            }
                            else
                            {
                                write_u8(frame_forward, length);
                                write_bytes(frame_forward, (const uint8_t *)message, length);
                            }
                        }

                        free(attribute);
                        break;
                    }
                    default:
                        result = RX_IGNORE;
                    }
                });

                ieee80211_add_fcs(frame_forward);

                if (circular_buf_put(target_peer->frame_buffer, frame_forward) < 0)
                {
                    log_warn("Could not add frame to circular buffer");
                    buf_free(frame_forward);
                    return -1;
                }
            }
        }
    }

    list_t service_descriptors = list_init();
    list_t service_descriptor_extensions = list_init();
    int result = 0;

    NAN_ITERATE_ATTRIBUTES(frame, {
        switch (attribute_id)
        {
        case SERVICE_DESCRIPTOR_ATTRIBUTE:
            result = nan_parse_sda(attribute_buf, service_descriptors);
            break;
        case SERVICE_DESCRIPTOR_EXTENSION_ATTRIBUTE:
            result = nan_parse_sdea(attribute_buf, attribute_length,
                                    service_descriptor_extensions);
            break;
        default:
            log_trace("Unhandled attribute: %s", nan_attribute_type_as_string(attribute_id));
            result = RX_IGNORE;
        }
    })

    if (result < 0)
    {
        log_error("Error while parsing attributes: %d", result);
        return result;
    }

    struct nan_service_descriptor_attribute *service_descriptor;
    LIST_FOR_EACH(
        service_descriptors, service_descriptor, {
            if (service_descriptor->control.service_control_type == PUBLISHED && !peer->publisher)
            {
                peer->publisher = true;
                log_debug("Publisher: %s", ether_addr_to_string(&peer->addr));
            }

            log_trace("Received service discovery for %u of type %d",
                      nan_service_id_to_string(&service_descriptor->service_id),
                      service_descriptor->control.service_control_type);
            nan_handle_received_service_discovery(&state->services, &state->events, &state->interface_address,
                                                  &peer->addr, destination_address, service_descriptor);

            if (service_descriptor->control.service_control_type == CONTROL_TYPE_FOLLOW_UP) {
                peer->last_follow_up_time = now_usec;
            }
        })

    list_free(service_descriptors, true);
    list_free(service_descriptor_extensions, true);

    return result;
}

int nan_rx_action(struct buf *frame, struct nan_state *state,
                  const struct ether_addr *source_address, const struct ether_addr *destination_address,
                  const struct ether_addr *cluster_id, const uint64_t now_usec)
{
    if (buf_rest(frame) < (int)sizeof(struct nan_action_frame))
    {
        log_trace("nan_action: frame to short");
        return RX_TOO_SHORT;
    }
    const struct nan_action_frame *action_frame = (const struct nan_action_frame *)buf_current(frame);

    if (!oui_equal(action_frame->oui, NAN_OUI))
        return RX_IGNORE_OUI;

    struct nan_peer *peer = NULL;
    enum peer_status peer_status = nan_peer_add(&state->peers, source_address, cluster_id, now_usec);
    if (peer_status < 0)
    {
        log_warn("nan_action: could not add peer: %s (%d)", ether_addr_to_string(source_address), peer_status);
        return RX_IGNORE;
    }

    nan_peer_get(&state->peers, source_address, &peer);
    if (peer == NULL)
    {
        log_warn("nan_action: could not get peer: %s (%d)", ether_addr_to_string(source_address), peer_status);
        return RX_IGNORE;
    }

    if (peer_status == PEER_ADD)
    {
        nan_timer_state_init(&peer->timer, state->timer.base_time_usec, state->timer.average_error_state);
        nan_timer_state_init(&peer->old_timer, state->timer.base_time_usec, state->timer.average_error_state);

        log_debug("peer init %s %p", ether_addr_to_string(&peer->addr), peer->timer.average_error_state);
    }

    if (action_frame->oui_type == NAN_OUI_TYPE_SERVICE_DISCOVERY)
    {
        // service discovery frame is just one byte shorter than action frame
        buf_advance(frame, sizeof(struct nan_service_discovery_frame));
        return nan_rx_service_discovery(frame, state, destination_address, cluster_id, peer, now_usec);
    }
    if (action_frame->oui_type != NAN_OUI_TYPE_ACTION)
    {
        log_warn("Unknown action frame out type: %d", action_frame->oui_type);
        return RX_IGNORE;
    }

    buf_advance(frame, sizeof(struct nan_action_frame));
    log_trace("nan_action: received %s from %s",
              nan_action_frame_subtype_to_string(action_frame->oui_subtype),
              ether_addr_to_string(source_address));

    return RX_OK;
}

int nan_rx(struct buf *frame, struct nan_state *state)
{
    signed char rssi;
    uint8_t flags;

    uint64_t now_usec = clock_time_usec();
    if (ieee80211_parse_radiotap_header(frame, &rssi, &flags, NULL /*&now_usec*/) < 0)
    {
        log_trace("radiotap: cannot parse header");
        return RX_UNEXPECTED_FORMAT;
    }

    if (ieee80211_parse_fcs(frame, flags) < 0)
    {
        log_trace("CRC failed");
        return RX_IGNORE_FAILED_CRC;
    }

    if (buf_rest(frame) < (int)sizeof(struct ieee80211_hdr))
    {
        log_trace("ieee80211: header to short");
        return RX_TOO_SHORT;
    }

    const struct ieee80211_hdr *ieee80211 = (const struct ieee80211_hdr *)buf_current(frame);
    const struct ether_addr *destination_address = &ieee80211->addr1;
    const struct ether_addr *source_address = &ieee80211->addr2;
    const struct ether_addr *cluster_id = &ieee80211->addr3;
    uint16_t frame_control = le16toh(ieee80211->frame_control);

    if (ether_addr_equal(source_address, &state->self_address))
        return RX_IGNORE_FROM_SELF;

    if (buf_advance(frame, sizeof(struct ieee80211_hdr)) < 0)
        return RX_TOO_SHORT;

    switch (frame_control & (IEEE80211_FCTL_FTYPE | IEEE80211_FCTL_STYPE))
    {
    case IEEE80211_FTYPE_MGMT | IEEE80211_STYPE_BEACON:
        return nan_rx_beacon(frame, state, source_address, cluster_id, rssi, now_usec);
    case IEEE80211_FTYPE_MGMT | IEEE80211_STYPE_ACTION:
        log_trace("Received action frame");
        return nan_rx_action(frame, state, source_address, destination_address, cluster_id, now_usec);
    default:
        log_trace("ieee80211: cannot handle type %x and subtype %x of received frame from %s",
                  frame_control & IEEE80211_FCTL_FTYPE, frame_control & IEEE80211_FCTL_STYPE, ether_addr_to_string(source_address));
        return RX_UNEXPECTED_TYPE;
    }
}
