/*
 * Copyright 2026 Aaryan Anand
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * UBX-MON-VER poll and response decode.
 */

#include "ubx/ubx_mon.h"

#include <string.h>

int ubx_msg_is_mon_ver(uint8_t msg_class, uint8_t msg_id)
{
    return (msg_class == (uint8_t)UBX_CLASS_MON &&
            msg_id == (uint8_t)UBX_MON_VER)
               ? 1
               : 0;
}

ubx_status_t ubx_mon_ver_poll_encode(uint8_t *out, size_t out_cap,
                                     size_t *out_len)
{
    (void)out;
    (void)out_cap;

    if (out_len == NULL) {
        return UBX_ERR_NULL;
    }
    if (out == NULL && out_cap > 0u) {
        return UBX_ERR_NULL;
    }

    *out_len = 0u;
    return UBX_OK;
}

static void ubx_copy_fixed_string(char *dst, size_t dst_cap, const uint8_t *src,
                                  size_t src_len)
{
    size_t i;
    size_t n;

    if (dst == NULL || dst_cap == 0u) {
        return;
    }

    n = (dst_cap - 1u < src_len) ? (dst_cap - 1u) : src_len;
    for (i = 0u; i < n; ++i) {
        if (src[i] == 0u) {
            break;
        }
        dst[i] = (char)src[i];
    }
    dst[i] = '\0';
}

ubx_status_t ubx_mon_ver_decode(const uint8_t *payload, uint16_t length,
                                ubx_mon_ver_t *out,
                                char extensions[][UBX_MON_VER_EXT_LEN + 1u],
                                size_t extensions_cap)
{
    size_t ext_bytes;
    size_t ext_count;
    size_t i;

    if (payload == NULL || out == NULL) {
        return UBX_ERR_NULL;
    }
    if (extensions == NULL && extensions_cap > 0u) {
        return UBX_ERR_NULL;
    }
    if ((size_t)length < UBX_MON_VER_BASE_LEN) {
        return UBX_ERR_BAD_LENGTH;
    }

    ext_bytes = (size_t)length - UBX_MON_VER_BASE_LEN;
    if ((ext_bytes % UBX_MON_VER_EXT_LEN) != 0u) {
        return UBX_ERR_BAD_LENGTH;
    }
    ext_count = ext_bytes / UBX_MON_VER_EXT_LEN;

    memset(out, 0, sizeof(*out));
    ubx_copy_fixed_string(out->sw_version, sizeof(out->sw_version), &payload[0],
                          UBX_MON_VER_SW_LEN);
    ubx_copy_fixed_string(out->hw_version, sizeof(out->hw_version),
                          &payload[UBX_MON_VER_SW_LEN], UBX_MON_VER_HW_LEN);
    out->extension_count = ext_count;

    if (extensions != NULL) {
        size_t copy_n =
            (ext_count < extensions_cap) ? ext_count : extensions_cap;
        for (i = 0u; i < copy_n; ++i) {
            ubx_copy_fixed_string(
                extensions[i], UBX_MON_VER_EXT_LEN + 1u,
                &payload[UBX_MON_VER_BASE_LEN + (i * UBX_MON_VER_EXT_LEN)],
                UBX_MON_VER_EXT_LEN);
        }
        if (ext_count > extensions_cap) {
            return UBX_ERR_BUFFER_TOO_SMALL;
        }
    }

    return UBX_OK;
}
