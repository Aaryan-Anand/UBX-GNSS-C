/*
 * Copyright 2026 Aaryan Anand
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ubx/ubx.h"

void ubx_checksum_compute(const uint8_t *data, size_t len,
                          uint8_t *ck_a, uint8_t *ck_b)
{
    uint8_t a = 0u;
    uint8_t b = 0u;
    size_t i;

    if (ck_a == NULL || ck_b == NULL) {
        return;
    }

    if (data == NULL && len > 0u) {
        *ck_a = 0u;
        *ck_b = 0u;
        return;
    }

    for (i = 0u; i < len; ++i) {
        a = (uint8_t)(a + data[i]);
        b = (uint8_t)(b + a);
    }

    *ck_a = a;
    *ck_b = b;
}

ubx_status_t ubx_checksum_verify(const uint8_t *data, size_t len,
                                 uint8_t ck_a, uint8_t ck_b)
{
    uint8_t calc_a;
    uint8_t calc_b;

    if (data == NULL && len > 0u) {
        return UBX_ERR_NULL;
    }

    ubx_checksum_compute(data, len, &calc_a, &calc_b);
    if (calc_a == ck_a && calc_b == ck_b) {
        return UBX_OK;
    }
    return UBX_ERR_BAD_CHECKSUM;
}
