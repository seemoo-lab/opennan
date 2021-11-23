#include "frame.h"

int nan_get_beacon_type(uint16_t beacon_interval)
{
    switch (beacon_interval)
    {
    case NAN_DISCOVERY_BEACON_INTERVAL_TU:
        return NAN_DISCOVERY_BEACON;
    case NAN_SYNC_BEACON_INTERVAL_TU:
        return NAN_SYNC_BEACON;
    default:
        return -1;
    }
}

const char *nan_beacon_type_to_string(int type)
{
    switch (type)
    {
    case NAN_DISCOVERY_BEACON:
        return "DISCOVERY";
    case NAN_SYNC_BEACON:
        return "SYNC";
    default:
        return "UNKNOWN";
    }
}

const char *nan_action_frame_subtype_to_string(int subtype)
{
    switch (subtype)
    {
    case NAF_RANGING_REQUEST:
        return "NAF_RANGING_REQUEST";
    case NAF_RANGING_RESPONSE:
        return "NAF_RANGING_RESPONSE";
    case NAF_RANGING_TERMINATION:
        return "NAF_RANGING_TERMINATION";
    case NAN_RANGING_REPORT:
        return "NAN_RANGING_REPORT";
    case NAF_DATA_PATH_REQUEST:
        return "NAF_DATA_PATH_REQUEST";
    case NAF_DATA_PATH_RESPONSE:
        return "NAF_DATA_PATH_RESPONSE";
    case NAF_DATA_PATH_CONFIRM:
        return "NAF_DATA_PATH_CONFIRM";
    case NAF_DATA_PATH_KEY_INSTALLMENT:
        return "NAF_DATA_PATH_KEY_INSTALLMENT";
    case NAF_DATA_PATH_TERMINATION:
        return "NAF_DATA_PATH_TERMINATION";
    case NAF_SCHEDULE_REQUEST:
        return "NAF_SCHEDULE_REQUEST";
    case NAF_SCHEDULE_RESPONSE:
        return "NAF_SCHEDULE_RESPONSE";
    case NAF_SCHEDULE_CONFIRM:
        return "NAF_SCHEDULE_CONFIRM";
    case NAF_SCHEDULE_UPDATE_NOTIFICATION:
        return "NAF_SCHEDULE_UPDATE_NOTIFICATION";
    default:
        return "UNKNOWN";
    }
}