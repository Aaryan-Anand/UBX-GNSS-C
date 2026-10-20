/*
 * Copyright 2026 Aaryan Anand
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Internal little-endian readers. Not part of the public API.
 */

#ifndef UBX_ENDIAN_H
#define UBX_ENDIAN_H

#include <stdint.h>

static uint16_t ubx_read_u16_le(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static int16_t ubx_read_i16_le(const uint8_t *p)
{
    return (int16_t)ubx_read_u16_le(p);
}

static uint32_t ubx_read_u32_le(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static int32_t ubx_read_i32_le(const uint8_t *p)
{
    return (int32_t)ubx_read_u32_le(p);
}

#endif /* UBX_ENDIAN_H */
