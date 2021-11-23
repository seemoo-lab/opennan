#ifndef NAN_RX_H_
#define NAN_RX_H_

#include <netinet/ether.h>

#include "state.h"
#include "wire.h"
#include "attributes.h"
#include "service.h"

enum RX_RESULT
{
    RX_IGNORE_SYNC_OUTSIDE_DW = 8,
    RX_IGNORE_OUI = 7,
    RX_IGNORE_PEER = 6,
    RX_IGNORE_RSSI = 5,
    RX_IGNORE_FAILED_CRC = 4,
    RX_IGNORE_NOPROMISC = 3,
    RX_IGNORE_FROM_SELF = 2,
    RX_IGNORE = 1,
    RX_OK = 0,
    RX_TOO_SHORT = -1,
    RX_UNEXPECTED_FORMAT = -2,
    RX_UNEXPECTED_TYPE = -3,
    RX_UNEXPECTED_VALUE = -4,
    RX_MISSING_MANDATORY_ATTRIBUTE = -5,
    RX_ATTRIBUTE_NOT_FOUND = -6,
    RX_OTHER_ERROR = -7,
};

typedef int (*nan_parse_attribute)(struct buf *frame, void *data);

const char *nan_rx_result_to_string(const int result);

int nan_rx(struct buf *frame, struct nan_state *state);

#endif // NAN_RX_H_