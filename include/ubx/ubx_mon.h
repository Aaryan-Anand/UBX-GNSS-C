/*
 * Copyright 2026 Aaryan Anand
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * UBX-MON-VER poll construction and response decoding. Timeouts and transport
 * belong to the application.
 */

#ifndef UBX_UBX_MON_H
#define UBX_UBX_MON_H

#include "ubx/ubx.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Fixed software-version field width in the MON-VER payload. */
#define UBX_MON_VER_SW_LEN 30u
/** Fixed hardware-version field width in the MON-VER payload. */
#define UBX_MON_VER_HW_LEN 10u
/** Fixed extension-string field width in the MON-VER payload. */
#define UBX_MON_VER_EXT_LEN 30u
/** Minimum MON-VER response payload length (sw + hw, no extensions). */
#define UBX_MON_VER_BASE_LEN (UBX_MON_VER_SW_LEN + UBX_MON_VER_HW_LEN)

/**
 * Decoded UBX-MON-VER identification strings (always NUL-terminated).
 * Extension strings are returned separately via the extensions array.
 */
typedef struct ubx_mon_ver {
    char sw_version[UBX_MON_VER_SW_LEN + 1u];
    char hw_version[UBX_MON_VER_HW_LEN + 1u];
    size_t extension_count;
} ubx_mon_ver_t;

/** Non-zero if (msg_class, msg_id) is UBX-MON-VER. */
int ubx_msg_is_mon_ver(uint8_t msg_class, uint8_t msg_id);

/**
 * Encode an empty UBX-MON-VER poll-request payload (length 0).
 * Sets `*out_len` to 0. `out` may be NULL when `out_cap` is 0.
 */
ubx_status_t ubx_mon_ver_poll_encode(uint8_t *out, size_t out_cap,
                                     size_t *out_len);

/**
 * Decode a UBX-MON-VER response payload.
 *
 * Payload length must be UBX_MON_VER_BASE_LEN + N * UBX_MON_VER_EXT_LEN.
 * Copies sw/hw into `out` with forced NUL termination (fields may lack a NUL
 * in the wire format). Optional extension strings are copied into
 * `extensions[i]` (each buffer must hold at least UBX_MON_VER_EXT_LEN + 1).
 * If `extensions` is NULL, only `extension_count` is reported.
 */
ubx_status_t ubx_mon_ver_decode(const uint8_t *payload, uint16_t length,
                                ubx_mon_ver_t *out,
                                char extensions[][UBX_MON_VER_EXT_LEN + 1u],
                                size_t extensions_cap);

#ifdef __cplusplus
}
#endif

#endif /* UBX_UBX_MON_H */
