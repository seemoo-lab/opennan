#include "tx.h"

#include <string.h>
#include <radiotap.h>

#include "attributes.h"
#include "ieee80211.h"
#include "timer.h"
#include "log.h"
#include "utils.h"
#include "circular_buffer.h"
#include "peer.h"

bool nan_can_send_discovery_beacon(const struct nan_state *state, uint64_t now_usec)
{
    if (state->sync.role != MASTER)
        return false;

    if (!nan_timer_initial_scan_done(&state->timer, now_usec))
        return false;

    return nan_timer_can_send_discovery_beacon(&state->timer, now_usec);
}

int nan_add_master_indication_attribute(struct buf *buf, const struct nan_state *state)
{
    struct nan_master_indication_attribute *attribute = (struct nan_master_indication_attribute *)buf_current(buf);
    attribute->id = MASTER_INDICATION_ATTRIBUTE;
    attribute->length = htole32(2);
    attribute->master_preference = state->sync.master_preference;
    attribute->random_factor = state->sync.random_factor;

    buf_advance(buf, sizeof(struct nan_master_indication_attribute));
    return sizeof(struct nan_master_indication_attribute);
}

int nan_add_cluster_attribute(struct buf *buf, const struct nan_state *state)
{
    struct nan_cluster_attribute *attribute = (struct nan_cluster_attribute *)buf_current(buf);
    attribute->id = CLUSTER_ATTRIBUTE;
    attribute->length = htole32(13);
    attribute->anchor_master_rank = htole64(state->sync.anchor_master_rank);
    attribute->hop_count = state->sync.hop_count;
    attribute->anchor_master_beacon_transmission_time = htole32(state->sync.ambtt);

    buf_advance(buf, sizeof(struct nan_cluster_attribute));
    return sizeof(struct nan_cluster_attribute);
}

int nan_add_service_id_list_attribute(struct buf *buf, const struct nan_state *state)
{
    struct nan_attribute_header *header = (struct nan_attribute_header *)buf_current(buf);
    header->id = SERVICE_ID_LIST_ATTRIBUTE;
    buf_advance(buf, sizeof(struct nan_attribute_header));

    size_t attribute_length = 0;
    struct nan_service *service;
    LIST_FILTER_FOR_EACH(
        state->services.published_services, service, nan_should_announce_service(service), {
            attribute_length += write_bytes(buf, (uint8_t *)service->service_id.byte, NAN_SERVICE_ID_LENGTH);
        });
    LIST_FILTER_FOR_EACH(
        state->services.subscribed_services, service, nan_should_announce_service(service), {
            attribute_length += write_bytes(buf, (uint8_t *)service->service_id.byte, NAN_SERVICE_ID_LENGTH);
        });

    header->length = htole32(attribute_length);
    return attribute_length += sizeof(struct nan_attribute_header);
}

int nan_add_service_descriptor_attribute(struct buf *buf, const struct nan_service *service,
                                         const enum nan_service_control_type control_type,
                                         const uint8_t requestor_instance_id,
                                         const char *service_specific_info, const size_t service_specific_info_length)
{
    struct nan_attribute_header *header = (struct nan_attribute_header *)buf_current(buf);
    header->id = SERVICE_DESCRIPTOR_ATTRIBUTE;
    buf_advance(buf, sizeof(struct nan_attribute_header));

    size_t attribute_length = 0;
    attribute_length += write_bytes(buf, (uint8_t *)service->service_id.byte, NAN_SERVICE_ID_LENGTH);
    attribute_length += write_u8(buf, service->instance_id);
    attribute_length += write_u8(buf, requestor_instance_id);

    struct nan_service_descriptor_control *control =
        (struct nan_service_descriptor_control *)buf_current(buf);
    attribute_length += buf_advance(buf, sizeof(struct nan_service_descriptor_control));

    memset(control, 0, sizeof(struct nan_service_descriptor_control));
    control->service_control_type = control_type;

    if (service_specific_info && service_specific_info_length < 256)
    {
        control->service_info_present = 1;
        attribute_length += write_u8(buf, (uint8_t)service_specific_info_length);
        attribute_length += write_bytes(buf, (uint8_t *)service_specific_info, service_specific_info_length);
    }

    header->length = htole32(attribute_length);
    return attribute_length += sizeof(struct nan_attribute_header);
}

int nan_add_service_descriptor_extension_attribute(struct buf *buf, const struct nan_service *service,
                                                   const char *service_specific_info, const size_t service_specific_info_length)
{
    struct nan_attribute_header *header = (struct nan_attribute_header *)buf_current(buf);
    header->id = SERVICE_DESCRIPTOR_EXTENSION_ATTRIBUTE;
    buf_advance(buf, sizeof(struct nan_attribute_header));

    size_t attribute_length = 0;
    attribute_length += write_u8(buf, service->instance_id);

    struct nan_service_descriptor_extension_control *control =
        (struct nan_service_descriptor_extension_control *)buf_current(buf);
    attribute_length += buf_advance(buf, sizeof(struct nan_service_descriptor_extension_control));

    memset(control, 0, sizeof(struct nan_service_descriptor_extension_control));

    if (service->type == PUBLISHED)
    {
        control->service_update_indicator_present = 1;
        attribute_length += write_u8(buf, service->service_update_indicator);
    }

    if (service_specific_info && service_specific_info_length >= 256)
    {
        attribute_length += write_le16(buf, (uint16_t)service_specific_info_length);
        attribute_length += write_bytes(buf, (uint8_t *)service_specific_info, service_specific_info_length);
    }

    header->length = htole32(attribute_length);
    return attribute_length += sizeof(struct nan_attribute_header);
}

int nan_add_device_capability_attribute(struct buf *buf)
{
    struct nan_device_capability_attribute *attribute = (struct nan_device_capability_attribute *)buf_current(buf);
    uint16_t attribute_length = sizeof(struct nan_device_capability_attribute);

    memset(attribute, 0, attribute_length);

    attribute->id = DEVICE_CAPABILITY_ATTRIBUTE;
    attribute->length = htole32(attribute_length - sizeof(struct nan_attribute_header));

    attribute->committed_dw_info.dw_2_4_ghz = 1;
    attribute->supported_bands.band_2_4_ghz = 1;

    buf_advance(buf, attribute_length);
    return attribute_length;
}

int nan_add_availability_attribute(struct buf *buf)
{
    struct nan_attribute_header *header = (struct nan_attribute_header *)buf_current(buf);
    header->id = NAN_AVAILABILITY_ATTRIBUTE;
    buf_advance(buf, sizeof(struct nan_attribute_header));

    size_t attribute_length = 0;

    // sequence id
    attribute_length += write_u8(buf, 0);

    struct nan_availability_attribute_control attribute_control = {0};
    attribute_control.map_id = 1;
    attribute_length += write_bytes(buf, (uint8_t *)&attribute_control, sizeof(struct nan_availability_attribute_control));

    // availability entry length
    attribute_length += write_le16(buf, (uint16_t)5);

    struct nan_availability_entry_control entry_control = {0};
    entry_control.availability_type = AVAILABILITY_COMITTED;
    attribute_length += write_le16(buf, *(uint16_t *)&entry_control);

    struct nan_availability_time_bitmap_control time_bitmap_control = {0};
    attribute_length += write_le16(buf, *(uint16_t *)&time_bitmap_control);
    // time bitmap length
    attribute_length += write_u8(buf, 0);

    header->length = htole32(attribute_length);
    return attribute_length += sizeof(struct nan_attribute_header);
}

int nan_add_data_path_attribute(struct buf *buf, const struct nan_data_path *data_path,
                                const struct ether_addr *initiator_address,
                                const enum nan_data_path_attribute_status status,
                                const enum nan_data_path_attribute_type type,
                                const uint8_t publish_id, const struct ether_addr *responder_address,
                                const char *ndp_specific_info, const size_t ndp_specific_info_length)
{
    struct nan_data_path_attribute_fixed *attribute = (struct nan_data_path_attribute_fixed *)buf_current(buf);
    attribute->id = NDP_ATTRIBUTE;
    buf_advance(buf, sizeof(struct nan_data_path_attribute_fixed));

    attribute->dialog_token = 0;
    attribute->type = type;
    attribute->status = status;
    attribute->reason_code = 0;
    attribute->initiator_address = *initiator_address;
    attribute->data_path_id = data_path->data_path_id;

    memset(&attribute->control, 0, sizeof(attribute->control));

    size_t attribute_length = sizeof(struct nan_data_path_attribute_fixed);

    if (type == REQUEST && publish_id != 0)
    {
        attribute->control.publish_id_present = 1;
        attribute_length += write_u8(buf, publish_id);
    }

    if (type == RESPONSE && responder_address != NULL)
    {
        attribute->control.responder_interface_address_present = 1;
        attribute_length += write_ether_addr(buf, responder_address);
    }

    if (ndp_specific_info != NULL && ndp_specific_info_length != 0)
    {
        attribute->control.data_path_specific_info_present = 1;
        attribute_length += write_bytes(buf, (const uint8_t *)ndp_specific_info, ndp_specific_info_length);
    }

    attribute->length = htole32(attribute_length - sizeof(struct nan_attribute_header));
    return attribute_length;
}

void nan_add_beacon_header(struct buf *buf, struct nan_state *state, const enum nan_beacon_type type,
                           uint8_t **data_length, const uint64_t now_usec, const struct nan_peer *peer)
{
    const struct nan_timer_state *timer = peer != NULL ? &peer->timer : &state->timer;
    const struct ether_addr *dest = peer != NULL
                                        ? &peer->addr
                                        : &NAN_BROADCAST_ADDRESS;

    ieee80211_add_radiotap_header(buf, &state->ieee80211);
    ieee80211_add_nan_header(buf, &state->interface_address, dest, &state->cluster.cluster_id,
                             &state->ieee80211, IEEE80211_FTYPE_MGMT | IEEE80211_STYPE_BEACON);

    struct nan_beacon_frame *beacon_header = (struct nan_beacon_frame *)buf_current(buf);
    uint64_t synced_time = nan_timer_get_synced_time_usec(timer, now_usec);

    beacon_header->time_stamp = htole64(synced_time);
    beacon_header->capability = htole16(0x0420);
    beacon_header->element_id = 0xdd;
    beacon_header->length = 4;
    beacon_header->oui = NAN_OUI;
    beacon_header->oui_type = NAN_OUI_TYPE_BEACON;

    if (type == NAN_SYNC_BEACON)
        beacon_header->beacon_interval = NAN_SYNC_BEACON_INTERVAL_TU;
    else
        beacon_header->beacon_interval = NAN_DISCOVERY_BEACON_INTERVAL_TU;

    *data_length = &beacon_header->length;

    buf_advance(buf, sizeof(struct nan_beacon_frame));
}

void nan_build_beacon_frame(struct buf *buf, struct nan_state *state,
                            const enum nan_beacon_type type, const uint64_t now_usec,
                            struct nan_peer *peer)
{
    uint8_t *data_length;
    nan_add_beacon_header(buf, state, type, &data_length, now_usec, peer);

    uint8_t attributes_length = 0;
    attributes_length += nan_add_master_indication_attribute(buf, state);
    attributes_length += nan_add_cluster_attribute(buf, state);
    *data_length += attributes_length;

    if (state->ieee80211.fcs)
        ieee80211_add_fcs(buf);
}

void nan_add_service_discovery_header(struct buf *buf, struct nan_state *state, const struct ether_addr *destination)
{
    ieee80211_add_radiotap_header(buf, &state->ieee80211);
    ieee80211_add_nan_header(buf, &state->interface_address, destination, &state->cluster.cluster_id,
                             &state->ieee80211, IEEE80211_FTYPE_MGMT | IEEE80211_STYPE_ACTION);

    struct nan_service_discovery_frame *service_discovery_frame =
        (struct nan_service_discovery_frame *)buf_current(buf);

    service_discovery_frame->category = IEEE80211_PUBLIC_ACTION_FRAME;
    service_discovery_frame->action = IEEE80211_PUBLIC_ACTION_FRAME_VENDOR_SPECIFIC;
    service_discovery_frame->oui = NAN_OUI;
    service_discovery_frame->oui_type = NAN_OUI_TYPE_SERVICE_DISCOVERY;

    buf_advance(buf, sizeof(struct nan_service_discovery_frame));
}

void nan_build_service_discovery_frame(struct buf *buf, struct nan_state *state,
                                       const struct ether_addr *destination, const list_t announced_services)
{
    nan_add_service_discovery_header(buf, state, destination);
    nan_add_device_capability_attribute(buf);
    nan_add_availability_attribute(buf);

    if (announced_services)
    {
        struct nan_service *service;
        LIST_FOR_EACH(announced_services, service, {
            enum nan_service_control_type control_type =
                service->type == SUBSCRIBED ? CONTROL_TYPE_SUBSCRIBE : CONTROL_TYPE_PUBLISH;

            nan_add_service_descriptor_attribute(buf, service, control_type, 0,
                                                 service->service_specific_info,
                                                 service->service_specific_info_length);
            nan_add_service_descriptor_extension_attribute(buf, service,
                                                           service->service_specific_info,
                                                           service->service_specific_info_length);
        });
    }

    if (state->ieee80211.fcs)
        ieee80211_add_fcs(buf);
}

int nan_transmit(struct nan_state *state, const struct ether_addr *destination,
                 const uint8_t instance_id, const uint8_t requestor_instance_id,
                 const char *service_specific_info, const size_t service_specific_info_length)
{
    struct nan_service *service = nan_get_service_by_instance_id(&state->services, instance_id, -1);

    if (service == NULL)
    {
        log_error("Called transmit for unknown service: %u", instance_id);
        return -1;
    }

    struct buf *buf = buf_new_owned(BUF_MAX_LENGTH);

    nan_add_service_discovery_header(buf, state, destination);
    nan_add_service_descriptor_attribute(buf, service, CONTROL_TYPE_FOLLOW_UP,
                                         requestor_instance_id, service_specific_info, service_specific_info_length);

    if (service_specific_info_length >= 256)
        nan_add_service_descriptor_extension_attribute(buf, service, service_specific_info, service_specific_info_length);

    if (state->ieee80211.fcs)
        ieee80211_add_fcs(buf);

    if (state->desync)
    {
        struct nan_peer *peer = NULL;
        if (nan_peer_get(&state->peers, destination, &peer) < 0)
        {
            log_warn("Unknown peer %s", ether_addr_to_string(destination));
            goto error;
        }

        if (circular_buf_put(peer->frame_buffer, buf) < 0)
        {
            log_warn("Could not add follow up frame to buffer");
            goto error;
        }
    }
    else
    {
        if (circular_buf_put(state->buffer, (any_t)buf) < 0)
        {
            log_warn("Could not add follow up frame to buffer");
            goto error;
        }
    }

    return 0;

error:
    buf_free(buf);
    return -1;
}
