/*
 * Copyright 2026 Aaryan Anand
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ubx/ubx.h"

size_t ubx_frame_encoded_size(uint16_t payload_len)
{
    return (size_t)UBX_FRAME_OVERHEAD + (size_t)payload_len;
}

ubx_status_t ubx_frame_encode(uint8_t msg_class, uint8_t msg_id,
                              const uint8_t *payload, uint16_t payload_len,
                              uint8_t *out, size_t out_cap, size_t *out_len)
{
    size_t need;
    uint8_t ck_a;
    uint8_t ck_b;
    size_t i;

    if (out_len != NULL) {
        *out_len = 0u;
    }

    if (out == NULL || out_len == NULL) {
        return UBX_ERR_NULL;
    }
    if (payload == NULL && payload_len > 0u) {
        return UBX_ERR_NULL;
    }

    need = ubx_frame_encoded_size(payload_len);
    if (out_cap < need) {
        return UBX_ERR_BUFFER_TOO_SMALL;
    }

    out[0] = (uint8_t)UBX_SYNC1;
    out[1] = (uint8_t)UBX_SYNC2;
    out[2] = msg_class;
    out[3] = msg_id;
    out[4] = (uint8_t)(payload_len & 0xFFu);
    out[5] = (uint8_t)((payload_len >> 8) & 0xFFu);

    for (i = 0u; i < (size_t)payload_len; ++i) {
        out[UBX_HEADER_SIZE + i] = payload[i];
    }

    /* Checksum covers class, id, length, payload — not sync bytes. */
    ubx_checksum_compute(&out[2], (size_t)4u + (size_t)payload_len, &ck_a, &ck_b);
    out[UBX_HEADER_SIZE + payload_len] = ck_a;
    out[UBX_HEADER_SIZE + payload_len + 1u] = ck_b;

    *out_len = need;
    return UBX_OK;
}
