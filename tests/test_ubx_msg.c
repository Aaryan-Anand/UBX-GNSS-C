/*
 * Copyright 2026 Aaryan Anand
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Host-side tests for UBX-NAV-PVT and UBX-ACK decoding.
 * Fixtures are constructed field-by-field with known little-endian values.
 */

#include "ubx/ubx.h"

#include <stdio.h>
#include <string.h>

static int g_failures = 0;
static int g_checks = 0;

#define CHECK(cond)                                                            \
    do {                                                                       \
        ++g_checks;                                                            \
        if (!(cond)) {                                                         \
            ++g_failures;                                                      \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);    \
        }                                                                      \
    } while (0)

#define CHECK_EQ_INT(a, b)                                                     \
    do {                                                                       \
        long long _a = (long long)(a);                                         \
        long long _b = (long long)(b);                                         \
        ++g_checks;                                                            \
        if (_a != _b) {                                                        \
            ++g_failures;                                                      \
            fprintf(stderr, "FAIL %s:%d: %s (%lld) != %s (%lld)\n", __FILE__,  \
                    __LINE__, #a, _a, #b, _b);                                 \
        }                                                                      \
    } while (0)

static void put_u16_le(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)((v >> 8) & 0xFFu);
}

static void put_i16_le(uint8_t *p, int16_t v)
{
    put_u16_le(p, (uint16_t)v);
}

static void put_u32_le(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)((v >> 8) & 0xFFu);
    p[2] = (uint8_t)((v >> 16) & 0xFFu);
    p[3] = (uint8_t)((v >> 24) & 0xFFu);
}

static void put_i32_le(uint8_t *p, int32_t v)
{
    put_u32_le(p, (uint32_t)v);
}

/*
 * Build a NAV-PVT payload with independently chosen field values.
 * Offsets follow u-blox M10 SPG UBX-NAV-PVT (92 bytes).
 */
static void fill_nav_pvt_payload(uint8_t payload[UBX_NAV_PVT_PAYLOAD_LEN])
{
    memset(payload, 0, UBX_NAV_PVT_PAYLOAD_LEN);

    put_u32_le(&payload[0], 123456789u);          /* iTOW ms */
    put_u16_le(&payload[4], 2026u);               /* year */
    payload[6] = 10u;                             /* month */
    payload[7] = 20u;                             /* day */
    payload[8] = 14u;                             /* hour */
    payload[9] = 30u;                             /* min */
    payload[10] = 45u;                            /* sec */
    payload[11] = 0x07u; /* validDate|validTime|fullyResolved */
    put_u32_le(&payload[12], 50u);                /* tAcc ns */
    put_i32_le(&payload[16], -250000000);         /* nano ns */
    payload[20] = 3u;                             /* fixType 3D */
    payload[21] = 0x01u;                          /* gnssFixOk */
    payload[22] = 0xE0u; /* confirmedAvai|Date|Time */
    payload[23] = 12u;                            /* numSV */
    put_i32_le(&payload[24], -1221234567);        /* lon 1e-7 deg (W) */
    put_i32_le(&payload[28], 374567890);          /* lat 1e-7 deg (N) */
    put_i32_le(&payload[32], 123450);             /* height mm */
    put_i32_le(&payload[36], 100000);             /* hMSL mm */
    put_u32_le(&payload[40], 1500u);              /* hAcc mm */
    put_u32_le(&payload[44], 2200u);              /* vAcc mm */
    put_i32_le(&payload[48], -1500);              /* velN mm/s */
    put_i32_le(&payload[52], 2500);               /* velE mm/s */
    put_i32_le(&payload[56], -100);               /* velD mm/s */
    put_i32_le(&payload[60], 3000);               /* gSpeed mm/s */
    put_i32_le(&payload[64], 4500000);            /* headMot 1e-5 deg */
    put_u32_le(&payload[68], 400u);               /* sAcc mm/s */
    put_u32_le(&payload[72], 500000u);            /* headAcc 1e-5 deg */
    put_u16_le(&payload[76], 145u);               /* pDOP 0.01 */
    put_u16_le(&payload[78], 0x0000u);            /* flags3 */
    /* reserved0 [80..83] left zero */
    put_i32_le(&payload[84], 4600000);            /* headVeh */
    put_i16_le(&payload[88], -1250);              /* magDec 1e-2 deg */
    put_u16_le(&payload[90], 50u);                /* magAcc 1e-2 deg */
}

static void test_nav_pvt_positive_decode(void)
{
    uint8_t payload[UBX_NAV_PVT_PAYLOAD_LEN];
    ubx_nav_pvt_t pvt;
    ubx_status_t st;

    fill_nav_pvt_payload(payload);
    st = ubx_nav_pvt_decode(payload, UBX_NAV_PVT_PAYLOAD_LEN, &pvt);
    CHECK_EQ_INT(st, UBX_OK);

    CHECK_EQ_INT(pvt.i_tow, 123456789);
    CHECK_EQ_INT(pvt.year, 2026);
    CHECK_EQ_INT(pvt.month, 10);
    CHECK_EQ_INT(pvt.day, 20);
    CHECK_EQ_INT(pvt.hour, 14);
    CHECK_EQ_INT(pvt.min, 30);
    CHECK_EQ_INT(pvt.sec, 45);
    CHECK(UBX_NAV_PVT_VALID_DATE(pvt.valid));
    CHECK(UBX_NAV_PVT_VALID_TIME(pvt.valid));
    CHECK(UBX_NAV_PVT_VALID_FULLY_RESOLVED(pvt.valid));
    CHECK(!UBX_NAV_PVT_VALID_MAG(pvt.valid));
    CHECK_EQ_INT(pvt.t_acc, 50);
    CHECK_EQ_INT(pvt.nano, -250000000);
    CHECK_EQ_INT(pvt.fix_type, 3);
    CHECK(UBX_NAV_PVT_FLAGS_GNSS_FIX_OK(pvt.flags));
    CHECK(!UBX_NAV_PVT_FLAGS_DIFF_SOLN(pvt.flags));
    CHECK_EQ_INT(UBX_NAV_PVT_FLAGS_PSM_STATE(pvt.flags), 0);
    CHECK(UBX_NAV_PVT_FLAGS2_CONFIRMED_AVAI(pvt.flags2));
    CHECK(UBX_NAV_PVT_FLAGS2_CONFIRMED_DATE(pvt.flags2));
    CHECK(UBX_NAV_PVT_FLAGS2_CONFIRMED_TIME(pvt.flags2));
    CHECK_EQ_INT(pvt.num_sv, 12);
    CHECK_EQ_INT(pvt.lon, -1221234567);
    CHECK_EQ_INT(pvt.lat, 374567890);
    CHECK_EQ_INT(pvt.height, 123450);
    CHECK_EQ_INT(pvt.h_msl, 100000);
    CHECK_EQ_INT(pvt.h_acc, 1500);
    CHECK_EQ_INT(pvt.v_acc, 2200);
    CHECK_EQ_INT(pvt.vel_n, -1500);
    CHECK_EQ_INT(pvt.vel_e, 2500);
    CHECK_EQ_INT(pvt.vel_d, -100);
    CHECK_EQ_INT(pvt.g_speed, 3000);
    CHECK_EQ_INT(pvt.head_mot, 4500000);
    CHECK_EQ_INT(pvt.s_acc, 400);
    CHECK_EQ_INT(pvt.head_acc, 500000);
    CHECK_EQ_INT(pvt.p_dop, 145);
    CHECK_EQ_INT(pvt.flags3, 0);
    CHECK_EQ_INT(pvt.head_veh, 4600000);
    CHECK_EQ_INT(pvt.mag_dec, -1250);
    CHECK_EQ_INT(pvt.mag_acc, 50);
}

static void test_nav_pvt_southern_western_coords(void)
{
    uint8_t payload[UBX_NAV_PVT_PAYLOAD_LEN];
    ubx_nav_pvt_t pvt;

    fill_nav_pvt_payload(payload);
    put_i32_le(&payload[24], -580123456); /* lon W */
    put_i32_le(&payload[28], -345678901); /* lat S */

    CHECK_EQ_INT(ubx_nav_pvt_decode(payload, UBX_NAV_PVT_PAYLOAD_LEN, &pvt),
                 UBX_OK);
    CHECK_EQ_INT(pvt.lon, -580123456);
    CHECK_EQ_INT(pvt.lat, -345678901);
}

static void test_nav_pvt_signed_velocities(void)
{
    uint8_t payload[UBX_NAV_PVT_PAYLOAD_LEN];
    ubx_nav_pvt_t pvt;

    fill_nav_pvt_payload(payload);
    put_i32_le(&payload[48], -999999);
    put_i32_le(&payload[52], -1);
    put_i32_le(&payload[56], 2147483647); /* max int32 down */
    put_i32_le(&payload[60], -50);

    CHECK_EQ_INT(ubx_nav_pvt_decode(payload, UBX_NAV_PVT_PAYLOAD_LEN, &pvt),
                 UBX_OK);
    CHECK_EQ_INT(pvt.vel_n, -999999);
    CHECK_EQ_INT(pvt.vel_e, -1);
    CHECK_EQ_INT(pvt.vel_d, 2147483647);
    CHECK_EQ_INT(pvt.g_speed, -50);
}

static void test_nav_pvt_invalid_fix_flags(void)
{
    uint8_t payload[UBX_NAV_PVT_PAYLOAD_LEN];
    ubx_nav_pvt_t pvt;

    fill_nav_pvt_payload(payload);
    payload[11] = 0u;  /* no valid date/time */
    payload[20] = 0u;  /* no fix */
    payload[21] = 0u;  /* gnssFixOk clear */
    payload[23] = 0u;
    put_u16_le(&payload[78], 0x0001u); /* invalidLlh */

    CHECK_EQ_INT(ubx_nav_pvt_decode(payload, UBX_NAV_PVT_PAYLOAD_LEN, &pvt),
                 UBX_OK);
    CHECK_EQ_INT(pvt.fix_type, 0);
    CHECK(!UBX_NAV_PVT_FLAGS_GNSS_FIX_OK(pvt.flags));
    CHECK(!UBX_NAV_PVT_VALID_DATE(pvt.valid));
    CHECK(!UBX_NAV_PVT_VALID_TIME(pvt.valid));
    CHECK(UBX_NAV_PVT_FLAGS3_INVALID_LLH(pvt.flags3));
}

static void test_nav_pvt_flag_bitfields(void)
{
    uint8_t payload[UBX_NAV_PVT_PAYLOAD_LEN];
    ubx_nav_pvt_t pvt;

    fill_nav_pvt_payload(payload);
    /* diffSoln | psmState=3 | headVehValid | carrSoln=2 */
    payload[21] = (uint8_t)(0x02u | (3u << 2) | 0x20u | (2u << 6));
    payload[22] = 0x40u; /* confirmedDate only */

    CHECK_EQ_INT(ubx_nav_pvt_decode(payload, UBX_NAV_PVT_PAYLOAD_LEN, &pvt),
                 UBX_OK);
    CHECK(UBX_NAV_PVT_FLAGS_DIFF_SOLN(pvt.flags));
    CHECK_EQ_INT(UBX_NAV_PVT_FLAGS_PSM_STATE(pvt.flags), 3);
    CHECK(UBX_NAV_PVT_FLAGS_HEAD_VEH_VALID(pvt.flags));
    CHECK_EQ_INT(UBX_NAV_PVT_FLAGS_CARR_SOLN(pvt.flags), 2);
    CHECK(!UBX_NAV_PVT_FLAGS2_CONFIRMED_AVAI(pvt.flags2));
    CHECK(UBX_NAV_PVT_FLAGS2_CONFIRMED_DATE(pvt.flags2));
    CHECK(!UBX_NAV_PVT_FLAGS2_CONFIRMED_TIME(pvt.flags2));
}

static void test_nav_pvt_bad_length(void)
{
    uint8_t payload[UBX_NAV_PVT_PAYLOAD_LEN];
    ubx_nav_pvt_t pvt;

    fill_nav_pvt_payload(payload);
    CHECK_EQ_INT(ubx_nav_pvt_decode(payload, 91u, &pvt), UBX_ERR_BAD_LENGTH);
    CHECK_EQ_INT(ubx_nav_pvt_decode(payload, 93u, &pvt), UBX_ERR_BAD_LENGTH);
    CHECK_EQ_INT(ubx_nav_pvt_decode(payload, 0u, &pvt), UBX_ERR_BAD_LENGTH);
    CHECK_EQ_INT(ubx_nav_pvt_decode(NULL, UBX_NAV_PVT_PAYLOAD_LEN, &pvt),
                 UBX_ERR_NULL);
    CHECK_EQ_INT(ubx_nav_pvt_decode(payload, UBX_NAV_PVT_PAYLOAD_LEN, NULL),
                 UBX_ERR_NULL);
}

static void test_nav_pvt_via_parser_frame(void)
{
    uint8_t payload[UBX_NAV_PVT_PAYLOAD_LEN];
    uint8_t frame[UBX_FRAME_OVERHEAD + UBX_NAV_PVT_PAYLOAD_LEN];
    uint8_t storage[UBX_NAV_PVT_PAYLOAD_LEN];
    ubx_parser_t parser;
    ubx_frame_t view;
    ubx_nav_pvt_t pvt;
    size_t frame_len = 0u;
    size_t consumed = 0u;

    fill_nav_pvt_payload(payload);
    CHECK_EQ_INT(ubx_frame_encode(UBX_CLASS_NAV, UBX_NAV_PVT, payload,
                                  UBX_NAV_PVT_PAYLOAD_LEN, frame, sizeof(frame),
                                  &frame_len),
                 UBX_OK);
    CHECK(ubx_msg_is_nav_pvt(UBX_CLASS_NAV, UBX_NAV_PVT));

    CHECK_EQ_INT(ubx_parser_init(&parser, storage, sizeof(storage),
                                 UBX_NAV_PVT_PAYLOAD_LEN),
                 UBX_OK);
    CHECK_EQ_INT(ubx_parser_feed(&parser, frame, frame_len, &consumed),
                 UBX_FRAME_READY);
    CHECK_EQ_INT(ubx_parser_get_frame(&parser, &view), UBX_OK);
    CHECK(ubx_msg_is_nav_pvt(view.msg_class, view.msg_id));
    CHECK_EQ_INT(view.length, UBX_NAV_PVT_PAYLOAD_LEN);
    CHECK_EQ_INT(ubx_nav_pvt_decode(view.payload, view.length, &pvt), UBX_OK);
    CHECK_EQ_INT(pvt.num_sv, 12);
    CHECK_EQ_INT(pvt.lon, -1221234567);
}

static void test_ack_ack_and_nak(void)
{
    uint8_t payload[2];
    ubx_ack_t ack;
    uint8_t frame[16];
    size_t frame_len = 0u;
    uint8_t storage[8];
    ubx_parser_t parser;
    ubx_frame_t view;
    size_t consumed = 0u;

    payload[0] = UBX_CLASS_NAV;
    payload[1] = UBX_NAV_PVT;
    CHECK_EQ_INT(ubx_ack_decode(payload, UBX_ACK_PAYLOAD_LEN, &ack), UBX_OK);
    CHECK_EQ_INT(ack.cls_id, UBX_CLASS_NAV);
    CHECK_EQ_INT(ack.msg_id, UBX_NAV_PVT);

    CHECK(ubx_msg_is_ack_ack(UBX_CLASS_ACK, UBX_ACK_ACK));
    CHECK(ubx_msg_is_ack_nak(UBX_CLASS_ACK, UBX_ACK_NAK));
    CHECK(!ubx_msg_is_ack_ack(UBX_CLASS_ACK, UBX_ACK_NAK));
    CHECK(!ubx_msg_is_ack_nak(UBX_CLASS_ACK, UBX_ACK_ACK));

    /* ACK-ACK frame through parser */
    payload[0] = 0x06u;
    payload[1] = 0x8Au;
    CHECK_EQ_INT(ubx_frame_encode(UBX_CLASS_ACK, UBX_ACK_ACK, payload, 2u, frame,
                                  sizeof(frame), &frame_len),
                 UBX_OK);
    CHECK_EQ_INT(ubx_parser_init(&parser, storage, sizeof(storage), 8u), UBX_OK);
    CHECK_EQ_INT(ubx_parser_feed(&parser, frame, frame_len, &consumed),
                 UBX_FRAME_READY);
    CHECK_EQ_INT(ubx_parser_get_frame(&parser, &view), UBX_OK);
    CHECK(ubx_msg_is_ack_ack(view.msg_class, view.msg_id));
    CHECK_EQ_INT(ubx_ack_decode(view.payload, view.length, &ack), UBX_OK);
    CHECK_EQ_INT(ack.cls_id, 0x06);
    CHECK_EQ_INT(ack.msg_id, 0x8A);

    /* ACK-NAK */
    ubx_parser_clear_frame(&parser);
    payload[0] = 0x06u;
    payload[1] = 0x01u;
    CHECK_EQ_INT(ubx_frame_encode(UBX_CLASS_ACK, UBX_ACK_NAK, payload, 2u, frame,
                                  sizeof(frame), &frame_len),
                 UBX_OK);
    CHECK_EQ_INT(ubx_parser_feed(&parser, frame, frame_len, &consumed),
                 UBX_FRAME_READY);
    CHECK_EQ_INT(ubx_parser_get_frame(&parser, &view), UBX_OK);
    CHECK(ubx_msg_is_ack_nak(view.msg_class, view.msg_id));
    CHECK_EQ_INT(ubx_ack_decode(view.payload, view.length, &ack), UBX_OK);
    CHECK_EQ_INT(ack.cls_id, 0x06);
    CHECK_EQ_INT(ack.msg_id, 0x01);
}

static void test_ack_bad_length(void)
{
    uint8_t payload[4] = {0x01u, 0x07u, 0x00u, 0x00u};
    ubx_ack_t ack;

    CHECK_EQ_INT(ubx_ack_decode(payload, 1u, &ack), UBX_ERR_BAD_LENGTH);
    CHECK_EQ_INT(ubx_ack_decode(payload, 3u, &ack), UBX_ERR_BAD_LENGTH);
    CHECK_EQ_INT(ubx_ack_decode(NULL, 2u, &ack), UBX_ERR_NULL);
    CHECK_EQ_INT(ubx_ack_decode(payload, 2u, NULL), UBX_ERR_NULL);
}

int main(void)
{
    test_nav_pvt_positive_decode();
    test_nav_pvt_southern_western_coords();
    test_nav_pvt_signed_velocities();
    test_nav_pvt_invalid_fix_flags();
    test_nav_pvt_flag_bitfields();
    test_nav_pvt_bad_length();
    test_nav_pvt_via_parser_frame();
    test_ack_ack_and_nak();
    test_ack_bad_length();

    if (g_failures != 0) {
        fprintf(stderr, "\n%d / %d checks failed\n", g_failures, g_checks);
        return 1;
    }

    printf("OK: %d checks passed\n", g_checks);
    return 0;
}
