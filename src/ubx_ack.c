/*
 * Copyright 2026 Aaryan Anand
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * UBX-ACK-ACK (0x05 0x01) and UBX-ACK-NAK (0x05 0x00) payload decode.
 * Distinguish ACK vs NAK using the frame message ID (see ubx_msg_is_ack_*).
 */

#include "ubx/ubx.h"

ubx_status_t ubx_ack_decode(const uint8_t *payload, uint16_t length,
                            ubx_ack_t *out)
{
    if (out == NULL || payload == NULL) {
        return UBX_ERR_NULL;
    }
    if (length != UBX_ACK_PAYLOAD_LEN) {
        return UBX_ERR_BAD_LENGTH;
    }

    out->cls_id = payload[0];
    out->msg_id = payload[1];
    return UBX_OK;
}

int ubx_msg_is_nav_pvt(uint8_t msg_class, uint8_t msg_id)
{
    return (msg_class == (uint8_t)UBX_CLASS_NAV &&
            msg_id == (uint8_t)UBX_NAV_PVT)
               ? 1
               : 0;
}

int ubx_msg_is_ack_ack(uint8_t msg_class, uint8_t msg_id)
{
    return (msg_class == (uint8_t)UBX_CLASS_ACK &&
            msg_id == (uint8_t)UBX_ACK_ACK)
               ? 1
               : 0;
}

int ubx_msg_is_ack_nak(uint8_t msg_class, uint8_t msg_id)
{
    return (msg_class == (uint8_t)UBX_CLASS_ACK &&
            msg_id == (uint8_t)UBX_ACK_NAK)
               ? 1
               : 0;
}
