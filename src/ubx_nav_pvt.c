/*
 * Copyright 2026 Aaryan Anand
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * UBX-NAV-PVT (0x01 0x07) decode for the fixed 92-byte M10 payload layout.
 */

#include "ubx/ubx.h"

#include "ubx_endian.h"

ubx_status_t ubx_nav_pvt_decode(const uint8_t *payload, uint16_t length,
                                ubx_nav_pvt_t *out)
{
    if (out == NULL) {
        return UBX_ERR_NULL;
    }
    if (payload == NULL) {
        return UBX_ERR_NULL;
    }
    if (length != UBX_NAV_PVT_PAYLOAD_LEN) {
        return UBX_ERR_BAD_LENGTH;
    }

    out->i_tow = ubx_read_u32_le(&payload[0]);
    out->year = ubx_read_u16_le(&payload[4]);
    out->month = payload[6];
    out->day = payload[7];
    out->hour = payload[8];
    out->min = payload[9];
    out->sec = payload[10];
    out->valid = payload[11];
    out->t_acc = ubx_read_u32_le(&payload[12]);
    out->nano = ubx_read_i32_le(&payload[16]);
    out->fix_type = payload[20];
    out->flags = payload[21];
    out->flags2 = payload[22];
    out->num_sv = payload[23];
    out->lon = ubx_read_i32_le(&payload[24]);
    out->lat = ubx_read_i32_le(&payload[28]);
    out->height = ubx_read_i32_le(&payload[32]);
    out->h_msl = ubx_read_i32_le(&payload[36]);
    out->h_acc = ubx_read_u32_le(&payload[40]);
    out->v_acc = ubx_read_u32_le(&payload[44]);
    out->vel_n = ubx_read_i32_le(&payload[48]);
    out->vel_e = ubx_read_i32_le(&payload[52]);
    out->vel_d = ubx_read_i32_le(&payload[56]);
    out->g_speed = ubx_read_i32_le(&payload[60]);
    out->head_mot = ubx_read_i32_le(&payload[64]);
    out->s_acc = ubx_read_u32_le(&payload[68]);
    out->head_acc = ubx_read_u32_le(&payload[72]);
    out->p_dop = ubx_read_u16_le(&payload[76]);
    out->flags3 = ubx_read_u16_le(&payload[78]);
    /* payload[80..83] reserved0 — skipped */
    out->head_veh = ubx_read_i32_le(&payload[84]);
    out->mag_dec = ubx_read_i16_le(&payload[88]);
    out->mag_acc = ubx_read_u16_le(&payload[90]);

    return UBX_OK;
}
