/*
 * Copyright 2026 Aaryan Anand
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Host-side tests for CFG-VALSET / CFG-VALGET / MON-VER.
 * Wire vectors are checked against hand-computed little-endian layouts from
 * the u-blox M10 SPG interface description (not only encode/decode round-trips).
 */

#include "ubx/ubx.h"
#include "ubx/ubx_cfg.h"
#include "ubx/ubx_mon.h"

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

#define CHECK_MEM_EQ(a, b, n)                                                  \
    do {                                                                       \
        ++g_checks;                                                            \
        if (memcmp((a), (b), (n)) != 0) {                                      \
            ++g_failures;                                                      \
            fprintf(stderr, "FAIL %s:%d: memcmp %s vs %s (%zu bytes)\n",       \
                    __FILE__, __LINE__, #a, #b, (size_t)(n));                  \
        }                                                                      \
    } while (0)

static void test_key_value_sizes(void)
{
    /* Independently derived from key ID bits 30..28. */
    CHECK_EQ_INT(ubx_cfg_key_value_size(UBX_CFG_UART1_BAUDRATE), 4);   /* 0x4 */
    CHECK_EQ_INT(ubx_cfg_key_value_size(UBX_CFG_UART1OUTPROT_UBX), 1); /* 0x1 */
    CHECK_EQ_INT(ubx_cfg_key_value_size(UBX_CFG_RATE_MEAS), 2);        /* 0x3 */
    CHECK_EQ_INT(ubx_cfg_key_value_size(UBX_CFG_NAVSPG_DYNMODEL), 1);  /* 0x2 */
    CHECK_EQ_INT(ubx_cfg_key_value_size(UBX_CFG_MSGOUT_UBX_NAV_PVT_UART1), 1);
    CHECK_EQ_INT(ubx_cfg_key_value_size(0x50000001u), 8); /* size code 5 */
    CHECK_EQ_INT(ubx_cfg_key_value_size(0x00000001u), 0); /* size code 0 */
    CHECK_EQ_INT(ubx_cfg_key_value_size(0x70000001u), 0); /* size code 7 */
}

static void test_valset_single_key_baudrate_vector(void)
{
    /*
     * Hand-built VALSET v0, RAM only, CFG-UART1-BAUDRATE = 9600.
     * Key 0x40520001 LE: 01 00 52 40
     * Value 9600 = 0x00002580 LE: 80 25 00 00
     */
    static const uint8_t k_ref[] = {
        0x00u, 0x01u, 0x00u, 0x00u, 0x01u, 0x00u, 0x52u, 0x40u,
        0x80u, 0x25u, 0x00u, 0x00u};
    ubx_cfg_item_t item;
    uint8_t out[32];
    size_t out_len = 0u;

    item.key_id = UBX_CFG_UART1_BAUDRATE;
    item.value = 9600u;

    CHECK_EQ_INT(ubx_cfg_valset_encode(UBX_CFG_LAYER_RAM, &item, 1u, out,
                                       sizeof(out), &out_len),
                 UBX_OK);
    CHECK_EQ_INT(out_len, (int)sizeof(k_ref));
    CHECK_MEM_EQ(out, k_ref, sizeof(k_ref));
}

static void test_valset_multi_key_mixed_widths(void)
{
    /*
     * RAM|BBR, three items:
     *  - L:  UART1OUTPROT-UBX = 1          key 0x10740001
     *  - U2: RATE-MEAS = 1000 (1 Hz)       key 0x30210001
     *  - E1: DYNMODEL = AUTOMOT (4)        key 0x20110021
     *  - U4: BAUDRATE = 115200             key 0x40520001
     */
    static const uint8_t k_ref[] = {
        0x00u, 0x03u, 0x00u, 0x00u, /* ver, layers=RAM|BBR, reserved */
        0x01u, 0x00u, 0x74u, 0x10u, 0x01u,
        0x01u, 0x00u, 0x21u, 0x30u, 0xE8u, 0x03u,
        0x21u, 0x00u, 0x11u, 0x20u, 0x04u,
        0x01u, 0x00u, 0x52u, 0x40u, 0x00u, 0xC2u, 0x01u, 0x00u};
    ubx_cfg_item_t items[4];
    uint8_t out[64];
    size_t out_len = 0u;

    items[0].key_id = UBX_CFG_UART1OUTPROT_UBX;
    items[0].value = 1u;
    items[1].key_id = UBX_CFG_RATE_MEAS;
    items[1].value = 1000u;
    items[2].key_id = UBX_CFG_NAVSPG_DYNMODEL;
    items[2].value = UBX_CFG_DYNMODEL_AUTOMOT;
    items[3].key_id = UBX_CFG_UART1_BAUDRATE;
    items[3].value = 115200u;

    CHECK_EQ_INT(
        ubx_cfg_valset_encode((uint8_t)(UBX_CFG_LAYER_RAM | UBX_CFG_LAYER_BBR),
                              items, 4u, out, sizeof(out), &out_len),
        UBX_OK);
    CHECK_EQ_INT(out_len, (int)sizeof(k_ref));
    CHECK_MEM_EQ(out, k_ref, sizeof(k_ref));
}

static void test_valset_layers_and_limits(void)
{
    ubx_cfg_item_t item;
    ubx_cfg_item_t many[UBX_CFG_MAX_ITEMS + 1u];
    uint8_t out[16];
    size_t out_len = 0u;
    size_t i;

    item.key_id = UBX_CFG_UART1INPROT_UBX;
    item.value = 1u;

    CHECK_EQ_INT(ubx_cfg_valset_encode(0u, &item, 1u, out, sizeof(out), &out_len),
                 UBX_ERR_BAD_ARG);
    CHECK_EQ_INT(ubx_cfg_valset_encode(0x08u, &item, 1u, out, sizeof(out),
                                       &out_len),
                 UBX_ERR_BAD_ARG);
    CHECK_EQ_INT(ubx_cfg_valset_encode(UBX_CFG_LAYER_FLASH, &item, 1u, out, 4u,
                                       &out_len),
                 UBX_ERR_BUFFER_TOO_SMALL);

    item.key_id = 0x00000001u; /* bad size */
    CHECK_EQ_INT(ubx_cfg_valset_encode(UBX_CFG_LAYER_RAM, &item, 1u, out,
                                       sizeof(out), &out_len),
                 UBX_ERR_BAD_ARG);

    for (i = 0u; i < UBX_CFG_MAX_ITEMS + 1u; ++i) {
        many[i].key_id = UBX_CFG_SIGNAL_GPS_ENA;
        many[i].value = 1u;
    }
    {
        uint8_t big[4u + (UBX_CFG_MAX_ITEMS + 1u) * 5u];
        CHECK_EQ_INT(ubx_cfg_valset_encode(UBX_CFG_LAYER_RAM, many,
                                           UBX_CFG_MAX_ITEMS + 1u, big,
                                           sizeof(big), &out_len),
                     UBX_ERR_LIMIT);
        CHECK_EQ_INT(ubx_cfg_valset_encode(UBX_CFG_LAYER_RAM, many,
                                           UBX_CFG_MAX_ITEMS, big, sizeof(big),
                                           &out_len),
                     UBX_OK);
        CHECK_EQ_INT(out_len, (int)(4u + UBX_CFG_MAX_ITEMS * 5u));
    }
}

static void test_valget_poll_vector(void)
{
    /*
     * VALGET poll v0, layer=Default(7), position=0, one key BAUDRATE.
     * 00 07 00 00  01 00 52 40
     */
    static const uint8_t k_ref[] = {0x00u, 0x07u, 0x00u, 0x00u,
                                    0x01u, 0x00u, 0x52u, 0x40u};
    uint32_t key = UBX_CFG_UART1_BAUDRATE;
    uint8_t out[16];
    size_t out_len = 0u;

    CHECK_EQ_INT(ubx_cfg_valget_poll_encode(UBX_CFG_LAYER_GET_DEFAULT, 0u, &key,
                                            1u, out, sizeof(out), &out_len),
                 UBX_OK);
    CHECK_EQ_INT(out_len, (int)sizeof(k_ref));
    CHECK_MEM_EQ(out, k_ref, sizeof(k_ref));

    CHECK_EQ_INT(ubx_cfg_valget_poll_encode(3u, 0u, &key, 1u, out, sizeof(out),
                                            &out_len),
                 UBX_ERR_BAD_ARG);
}

static void test_valget_wildcard_selectors(void)
{
    /*
     * M10 SPG 5.10: all-groups wildcard has 0xfff in bits 16..27.
     * Documented representation 0x0FFF0000; little-endian wire 00 00 FF 0F.
     */
    static const uint8_t k_all_ref[] = {
        0x00u, 0x00u, 0x00u, 0x00u, /* ver, RAM, position=0 */
        0x00u, 0x00u, 0xFFu, 0x0Fu};
    /*
     * Group UART1 (0x52) wildcard: item bits 0xffff → 0x0052FFFF,
     * LE: FF FF 52 00
     */
    static const uint8_t k_grp_ref[] = {
        0x00u, 0x07u, 0x40u, 0x00u, /* Default, position=64 */
        0xFFu, 0xFFu, 0x52u, 0x00u};
    uint32_t key;
    uint8_t out[16];
    size_t out_len = 0u;
    ubx_cfg_item_t bad_item;

    CHECK(ubx_cfg_key_is_valget_wildcard(UBX_CFG_VALGET_WILDCARD_ALL));
    CHECK_EQ_INT(ubx_cfg_key_value_size(UBX_CFG_VALGET_WILDCARD_ALL), 0);
    CHECK(!ubx_cfg_key_is_valget_wildcard(UBX_CFG_UART1_BAUDRATE));

    key = UBX_CFG_VALGET_WILDCARD_ALL;
    CHECK_EQ_INT(ubx_cfg_valget_poll_encode(UBX_CFG_LAYER_GET_RAM, 0u, &key, 1u,
                                            out, sizeof(out), &out_len),
                 UBX_OK);
    CHECK_EQ_INT(out_len, (int)sizeof(k_all_ref));
    CHECK_MEM_EQ(out, k_all_ref, sizeof(k_all_ref));

    key = ubx_cfg_valget_wildcard_group(0x52u);
    CHECK_EQ_INT(key, 0x0052FFFFu);
    CHECK(ubx_cfg_key_is_valget_wildcard(key));
    CHECK_EQ_INT(ubx_cfg_valget_poll_encode(UBX_CFG_LAYER_GET_DEFAULT, 64u, &key,
                                            1u, out, sizeof(out), &out_len),
                 UBX_OK);
    CHECK_EQ_INT(out_len, (int)sizeof(k_grp_ref));
    CHECK_MEM_EQ(out, k_grp_ref, sizeof(k_grp_ref));

    CHECK_EQ_INT(ubx_cfg_valget_wildcard_group(0x00u), 0);
    CHECK_EQ_INT(ubx_cfg_valget_wildcard_group(0xFFu), 0);

    /* Reject undecodable non-wildcard / size-code-0 key. */
    key = 0x00000001u;
    CHECK_EQ_INT(ubx_cfg_valget_poll_encode(UBX_CFG_LAYER_GET_RAM, 0u, &key, 1u,
                                            out, sizeof(out), &out_len),
                 UBX_ERR_BAD_ARG);

    /* VALSET must still reject wildcards. */
    bad_item.key_id = UBX_CFG_VALGET_WILDCARD_ALL;
    bad_item.value = 0u;
    CHECK_EQ_INT(ubx_cfg_valset_encode(UBX_CFG_LAYER_RAM, &bad_item, 1u, out,
                                       sizeof(out), &out_len),
                 UBX_ERR_BAD_ARG);
    bad_item.key_id = ubx_cfg_valget_wildcard_group(0x52u);
    CHECK_EQ_INT(ubx_cfg_valset_encode(UBX_CFG_LAYER_RAM, &bad_item, 1u, out,
                                       sizeof(out), &out_len),
                 UBX_ERR_BAD_ARG);
}

static void test_valget_response_decode_vector(void)
{
    /*
     * Hand-built VALGET response v1, layer=RAM, position=0:
     * item1: OUTPROT-UBX=1 (L)
     * item2: RATE-MEAS=500 (U2)
     * item3: BAUDRATE=38400 (U4)  38400=0x9600 -> 00 96 00 00
     */
    static const uint8_t k_payload[] = {
        0x01u, 0x00u, 0x00u, 0x00u, 0x01u, 0x00u, 0x74u, 0x10u, 0x01u,
        0x01u, 0x00u, 0x21u, 0x30u, 0xF4u, 0x01u, 0x01u, 0x00u, 0x52u,
        0x40u, 0x00u, 0x96u, 0x00u, 0x00u};
    ubx_cfg_item_t items[4];
    uint8_t version = 0u;
    uint8_t layer = 0xFFu;
    uint16_t position = 0xFFFFu;
    size_t count = 0u;

    CHECK_EQ_INT(ubx_cfg_valget_decode(k_payload, (uint16_t)sizeof(k_payload),
                                       &version, &layer, &position, items, 4u,
                                       &count),
                 UBX_OK);
    CHECK_EQ_INT(version, UBX_CFG_VALGET_RESPONSE_VERSION);
    CHECK_EQ_INT(layer, UBX_CFG_LAYER_GET_RAM);
    CHECK_EQ_INT(position, 0);
    CHECK_EQ_INT(count, 3);
    CHECK_EQ_INT(items[0].key_id, UBX_CFG_UART1OUTPROT_UBX);
    CHECK_EQ_INT(items[0].value, 1);
    CHECK_EQ_INT(items[1].key_id, UBX_CFG_RATE_MEAS);
    CHECK_EQ_INT(items[1].value, 500);
    CHECK_EQ_INT(items[2].key_id, UBX_CFG_UART1_BAUDRATE);
    CHECK_EQ_INT(items[2].value, 38400);
}

static void test_valget_malformed(void)
{
    uint8_t version;
    uint8_t layer;
    uint16_t position;
    ubx_cfg_item_t items[2];
    size_t count = 0u;
    uint8_t short_hdr[] = {0x01u, 0x00u, 0x00u}; /* < 4 */
    uint8_t bad_ver[] = {0x00u, 0x00u, 0x00u, 0x00u};
    uint8_t trunc_value[] = {0x01u, 0x00u, 0x00u, 0x00u, 0x01u, 0x00u, 0x52u,
                             0x40u, 0x00u}; /* key U4 but only 1 value byte */
    uint8_t bad_key[] = {0x01u, 0x00u, 0x00u, 0x00u, 0x01u, 0x00u, 0x00u,
                         0x00u}; /* size code 0 */

    CHECK_EQ_INT(ubx_cfg_valget_decode(short_hdr, 3u, &version, &layer,
                                       &position, items, 2u, &count),
                 UBX_ERR_BAD_LENGTH);
    CHECK_EQ_INT(ubx_cfg_valget_decode(bad_ver, 4u, &version, &layer, &position,
                                       items, 2u, &count),
                 UBX_ERR_BAD_ARG);
    CHECK_EQ_INT(ubx_cfg_valget_decode(trunc_value, (uint16_t)sizeof(trunc_value),
                                       &version, &layer, &position, items, 2u,
                                       &count),
                 UBX_ERR_BAD_LENGTH);
    CHECK_EQ_INT(ubx_cfg_valget_decode(bad_key, (uint16_t)sizeof(bad_key),
                                       &version, &layer, &position, items, 2u,
                                       &count),
                 UBX_ERR_BAD_ARG);

    /* Truncated after partial multi-item stream. */
    {
        static const uint8_t partial[] = {
            0x01u, 0x00u, 0x00u, 0x00u, 0x01u, 0x00u, 0x74u, 0x10u, 0x01u,
            0x01u, 0x00u, 0x52u, 0x40u}; /* missing baud value bytes */
        CHECK_EQ_INT(ubx_cfg_valget_decode(partial, (uint16_t)sizeof(partial),
                                           &version, &layer, &position, items,
                                           2u, &count),
                     UBX_ERR_BAD_LENGTH);
    }
}

static void test_valget_capacity_overflow(void)
{
    static const uint8_t k_payload[] = {
        0x01u, 0x00u, 0x00u, 0x00u, 0x01u, 0x00u, 0x74u, 0x10u, 0x01u,
        0x02u, 0x00u, 0x74u, 0x10u, 0x00u};
    ubx_cfg_item_t items[1];
    uint8_t version;
    uint8_t layer;
    uint16_t position;
    size_t count = 0u;

    CHECK_EQ_INT(ubx_cfg_valget_decode(k_payload, (uint16_t)sizeof(k_payload),
                                       &version, &layer, &position, items, 1u,
                                       &count),
                 UBX_ERR_BUFFER_TOO_SMALL);
    CHECK_EQ_INT(count, 2);
    CHECK_EQ_INT(items[0].key_id, UBX_CFG_UART1OUTPROT_UBX);
}

static void test_eight_byte_value(void)
{
    /* Synthetic 8-byte key (size code 5): 0x50AA0001, value 0x0102030405060708 */
    static const uint8_t k_ref[] = {
        0x00u, 0x01u, 0x00u, 0x00u, 0x01u, 0x00u, 0xAAu, 0x50u,
        0x08u, 0x07u, 0x06u, 0x05u, 0x04u, 0x03u, 0x02u, 0x01u};
    ubx_cfg_item_t item;
    uint8_t out[32];
    size_t out_len = 0u;

    item.key_id = 0x50AA0001u;
    item.value = 0x0102030405060708ull;
    CHECK_EQ_INT(ubx_cfg_valset_encode(UBX_CFG_LAYER_RAM, &item, 1u, out,
                                       sizeof(out), &out_len),
                 UBX_OK);
    CHECK_EQ_INT(out_len, (int)sizeof(k_ref));
    CHECK_MEM_EQ(out, k_ref, sizeof(k_ref));
}

static void test_cfg_frame_roundtrip_identity(void)
{
    ubx_cfg_item_t item;
    uint8_t payload[32];
    uint8_t frame[48];
    size_t payload_len = 0u;
    size_t frame_len = 0u;
    uint8_t storage[64];
    ubx_parser_t parser;
    ubx_frame_t view;
    size_t consumed = 0u;

    item.key_id = UBX_CFG_MSGOUT_UBX_NAV_PVT_UART1;
    item.value = 1u;
    CHECK_EQ_INT(ubx_cfg_valset_encode(UBX_CFG_LAYER_RAM, &item, 1u, payload,
                                       sizeof(payload), &payload_len),
                 UBX_OK);
    CHECK_EQ_INT(ubx_frame_encode(UBX_CLASS_CFG, UBX_CFG_VALSET, payload,
                                  (uint16_t)payload_len, frame, sizeof(frame),
                                  &frame_len),
                 UBX_OK);
    CHECK(ubx_msg_is_cfg_valset(UBX_CLASS_CFG, UBX_CFG_VALSET));

    CHECK_EQ_INT(ubx_parser_init(&parser, storage, sizeof(storage), 64u), UBX_OK);
    CHECK_EQ_INT(ubx_parser_feed(&parser, frame, frame_len, &consumed),
                 UBX_FRAME_READY);
    CHECK_EQ_INT(ubx_parser_get_frame(&parser, &view), UBX_OK);
    CHECK(ubx_msg_is_cfg_valset(view.msg_class, view.msg_id));
    CHECK_EQ_INT(view.length, (int)payload_len);
    CHECK_MEM_EQ(view.payload, payload, payload_len);
}

static void test_mon_ver_poll_and_decode(void)
{
    uint8_t poll_payload[4];
    size_t poll_len = 1u;
    uint8_t frame[16];
    size_t frame_len = 0u;
    ubx_mon_ver_t ver;
    char exts[3][UBX_MON_VER_EXT_LEN + 1u];
    uint8_t payload[UBX_MON_VER_BASE_LEN + 2u * UBX_MON_VER_EXT_LEN];
    size_t i;

    CHECK_EQ_INT(ubx_mon_ver_poll_encode(poll_payload, sizeof(poll_payload),
                                         &poll_len),
                 UBX_OK);
    CHECK_EQ_INT(poll_len, 0);
    CHECK_EQ_INT(ubx_frame_encode(UBX_CLASS_MON, UBX_MON_VER, NULL, 0u, frame,
                                  sizeof(frame), &frame_len),
                 UBX_OK);
    CHECK_EQ_INT(frame_len, 8);
    CHECK(ubx_msg_is_mon_ver(UBX_CLASS_MON, UBX_MON_VER));

    memset(payload, 0, sizeof(payload));
    memcpy(&payload[0], "EXT CORE 5.30 (abc)", 19);
    /* hwVersion without NUL padding beyond content */
    memcpy(&payload[30], "000A0000", 8);
    memcpy(&payload[40], "PROTVER=34.10", 13);
    memcpy(&payload[70], "MOD=SAM-M10Q", 12);

    CHECK_EQ_INT(ubx_mon_ver_decode(payload, (uint16_t)sizeof(payload), &ver,
                                    exts, 3u),
                 UBX_OK);
    CHECK(strcmp(ver.sw_version, "EXT CORE 5.30 (abc)") == 0);
    CHECK(strcmp(ver.hw_version, "000A0000") == 0);
    CHECK_EQ_INT(ver.extension_count, 2);
    CHECK(strcmp(exts[0], "PROTVER=34.10") == 0);
    CHECK(strcmp(exts[1], "MOD=SAM-M10Q") == 0);

    /* Field with no embedded NUL: fill entire 30 bytes with 'A'. */
    memset(payload, 'A', UBX_MON_VER_SW_LEN);
    memset(&payload[30], 'B', UBX_MON_VER_HW_LEN);
    CHECK_EQ_INT(
        ubx_mon_ver_decode(payload, (uint16_t)UBX_MON_VER_BASE_LEN, &ver, NULL,
                           0u),
        UBX_OK);
    CHECK_EQ_INT(strlen(ver.sw_version), 30);
    CHECK_EQ_INT(strlen(ver.hw_version), 10);
    for (i = 0u; i < 30u; ++i) {
        CHECK(ver.sw_version[i] == 'A');
    }

    CHECK_EQ_INT(ubx_mon_ver_decode(payload, 39u, &ver, NULL, 0u),
                 UBX_ERR_BAD_LENGTH);
    CHECK_EQ_INT(ubx_mon_ver_decode(payload, 41u, &ver, NULL, 0u),
                 UBX_ERR_BAD_LENGTH);
}

static void test_mon_ver_extension_capacity(void)
{
    uint8_t payload[UBX_MON_VER_BASE_LEN + 2u * UBX_MON_VER_EXT_LEN];
    ubx_mon_ver_t ver;
    char exts[1][UBX_MON_VER_EXT_LEN + 1u];

    memset(payload, 0, sizeof(payload));
    memcpy(payload, "sw", 2);
    memcpy(&payload[30], "hw", 2);
    memcpy(&payload[40], "ext0", 4);
    memcpy(&payload[70], "ext1", 4);

    CHECK_EQ_INT(ubx_mon_ver_decode(payload, (uint16_t)sizeof(payload), &ver,
                                    exts, 1u),
                 UBX_ERR_BUFFER_TOO_SMALL);
    CHECK_EQ_INT(ver.extension_count, 2);
    CHECK(strcmp(exts[0], "ext0") == 0);
}

int main(void)
{
    test_key_value_sizes();
    test_valset_single_key_baudrate_vector();
    test_valset_multi_key_mixed_widths();
    test_valset_layers_and_limits();
    test_valget_poll_vector();
    test_valget_wildcard_selectors();
    test_valget_response_decode_vector();
    test_valget_malformed();
    test_valget_capacity_overflow();
    test_eight_byte_value();
    test_cfg_frame_roundtrip_identity();
    test_mon_ver_poll_and_decode();
    test_mon_ver_extension_capacity();

    if (g_failures != 0) {
        fprintf(stderr, "\n%d / %d checks failed\n", g_failures, g_checks);
        return 1;
    }

    printf("OK: %d checks passed\n", g_checks);
    return 0;
}
