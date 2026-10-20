/*
 * Copyright 2026 Aaryan Anand
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Host-side tests for the UBX protocol core. No receiver or platform SDK.
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
        int _a = (int)(a);                                                     \
        int _b = (int)(b);                                                     \
        ++g_checks;                                                            \
        if (_a != _b) {                                                        \
            ++g_failures;                                                      \
            fprintf(stderr, "FAIL %s:%d: %s (%d) != %s (%d)\n", __FILE__,      \
                    __LINE__, #a, _a, #b, _b);                                 \
        }                                                                      \
    } while (0)

/* Known vector: CFG-MSG poll (class 0x06, id 0x01, empty payload).
 * Covered bytes: 06 01 00 00 -> CK_A=0x07, CK_B=0x1B
 * Full frame: B5 62 06 01 00 00 07 1B
 */
static const uint8_t k_frame_cfg_msg_poll[] = {
    0xB5u, 0x62u, 0x06u, 0x01u, 0x00u, 0x00u, 0x07u, 0x1Bu
};

/* ACK-ACK with 2-byte payload (cls=0x06, id=0x01): class 0x05, id 0x01.
 * Payload: 06 01
 * Covered: 05 01 02 00 06 01
 */
static void make_ack_ack(uint8_t out[10])
{
    const uint8_t payload[2] = {0x06u, 0x01u};
    size_t n = 0u;
    ubx_status_t st =
        ubx_frame_encode(0x05u, 0x01u, payload, 2u, out, 10u, &n);
    (void)st;
    (void)n;
}

static void test_checksum_known_vectors(void)
{
    uint8_t ck_a;
    uint8_t ck_b;
    const uint8_t covered[] = {0x06u, 0x01u, 0x00u, 0x00u};

    ubx_checksum_compute(covered, sizeof(covered), &ck_a, &ck_b);
    CHECK_EQ_INT(ck_a, 0x07);
    CHECK_EQ_INT(ck_b, 0x1B);
    CHECK_EQ_INT(ubx_checksum_verify(covered, sizeof(covered), 0x07u, 0x1Bu),
                 UBX_OK);
    CHECK_EQ_INT(ubx_checksum_verify(covered, sizeof(covered), 0x00u, 0x1Bu),
                 UBX_ERR_BAD_CHECKSUM);
    CHECK_EQ_INT(ubx_checksum_verify(covered, sizeof(covered), 0x07u, 0x00u),
                 UBX_ERR_BAD_CHECKSUM);

    /* Empty covered region. */
    ubx_checksum_compute(NULL, 0u, &ck_a, &ck_b);
    CHECK_EQ_INT(ck_a, 0);
    CHECK_EQ_INT(ck_b, 0);
}

static void test_frame_encode_valid_and_bounds(void)
{
    uint8_t out[16];
    size_t out_len = 0u;
    ubx_status_t st;
    const uint8_t payload[2] = {0xAAu, 0x55u};

    CHECK_EQ_INT(ubx_frame_encoded_size(0u), 8);
    CHECK_EQ_INT(ubx_frame_encoded_size(2u), 10);

    st = ubx_frame_encode(0x06u, 0x01u, NULL, 0u, out, sizeof(out), &out_len);
    CHECK_EQ_INT(st, UBX_OK);
    CHECK_EQ_INT(out_len, 8);
    CHECK(memcmp(out, k_frame_cfg_msg_poll, sizeof(k_frame_cfg_msg_poll)) == 0);

    st = ubx_frame_encode(0x01u, 0x02u, payload, 2u, out, sizeof(out), &out_len);
    CHECK_EQ_INT(st, UBX_OK);
    CHECK_EQ_INT(out_len, 10);
    CHECK_EQ_INT(out[0], UBX_SYNC1);
    CHECK_EQ_INT(out[1], UBX_SYNC2);
    CHECK_EQ_INT(out[2], 0x01);
    CHECK_EQ_INT(out[3], 0x02);
    CHECK_EQ_INT(out[4], 0x02);
    CHECK_EQ_INT(out[5], 0x00);
    CHECK_EQ_INT(out[6], 0xAA);
    CHECK_EQ_INT(out[7], 0x55);
    CHECK_EQ_INT(ubx_checksum_verify(&out[2], 6u, out[8], out[9]), UBX_OK);

    /* Capacity too small. */
    st = ubx_frame_encode(0x06u, 0x01u, NULL, 0u, out, 7u, &out_len);
    CHECK_EQ_INT(st, UBX_ERR_BUFFER_TOO_SMALL);
    CHECK_EQ_INT(out_len, 0);

    /* Exact capacity. */
    st = ubx_frame_encode(0x06u, 0x01u, NULL, 0u, out, 8u, &out_len);
    CHECK_EQ_INT(st, UBX_OK);

    /* Null payload with non-zero length. */
    st = ubx_frame_encode(0x01u, 0x02u, NULL, 1u, out, sizeof(out), &out_len);
    CHECK_EQ_INT(st, UBX_ERR_NULL);

    st = ubx_frame_encode(0x01u, 0x02u, payload, 1u, NULL, 8u, &out_len);
    CHECK_EQ_INT(st, UBX_ERR_NULL);
}

static ubx_status_t feed_all(ubx_parser_t *p, const uint8_t *data, size_t len)
{
    size_t off = 0u;
    ubx_status_t last = UBX_OK;

    while (off < len) {
        size_t consumed = 0u;
        last = ubx_parser_feed(p, data + off, len - off, &consumed);
        if (last == UBX_FRAME_READY) {
            return UBX_FRAME_READY;
        }
        if (last != UBX_OK) {
            return last;
        }
        if (consumed == 0u) {
            return UBX_ERR_STATE;
        }
        off += consumed;
    }
    return last;
}

static void test_parser_valid_frame(void)
{
    uint8_t storage[64];
    ubx_parser_t parser;
    ubx_frame_t frame;
    size_t consumed = 0u;
    ubx_status_t st;

    CHECK_EQ_INT(ubx_parser_init(&parser, storage, sizeof(storage), 64u), UBX_OK);
    st = ubx_parser_feed(&parser, k_frame_cfg_msg_poll,
                         sizeof(k_frame_cfg_msg_poll), &consumed);
    CHECK_EQ_INT(st, UBX_FRAME_READY);
    CHECK_EQ_INT(consumed, (int)sizeof(k_frame_cfg_msg_poll));
    CHECK(ubx_parser_has_frame(&parser));
    CHECK_EQ_INT(ubx_parser_get_frame(&parser, &frame), UBX_OK);
    CHECK_EQ_INT(frame.msg_class, 0x06);
    CHECK_EQ_INT(frame.msg_id, 0x01);
    CHECK_EQ_INT(frame.length, 0);
    CHECK(frame.payload == NULL);

    /* Must clear before feeding more. */
    CHECK_EQ_INT(ubx_parser_feed(&parser, k_frame_cfg_msg_poll, 1u, &consumed),
                 UBX_ERR_STATE);
    ubx_parser_clear_frame(&parser);
    CHECK(!ubx_parser_has_frame(&parser));
}

static void test_parser_byte_at_a_time(void)
{
    uint8_t storage[32];
    ubx_parser_t parser;
    ubx_frame_t frame;
    size_t i;
    ubx_status_t st = UBX_OK;

    CHECK_EQ_INT(ubx_parser_init(&parser, storage, sizeof(storage), 32u), UBX_OK);

    for (i = 0u; i < sizeof(k_frame_cfg_msg_poll); ++i) {
        size_t consumed = 0u;
        st = ubx_parser_feed(&parser, &k_frame_cfg_msg_poll[i], 1u, &consumed);
        if (i + 1u < sizeof(k_frame_cfg_msg_poll)) {
            CHECK_EQ_INT(st, UBX_OK);
            CHECK_EQ_INT(consumed, 1);
        }
    }
    CHECK_EQ_INT(st, UBX_FRAME_READY);
    CHECK_EQ_INT(ubx_parser_get_frame(&parser, &frame), UBX_OK);
    CHECK_EQ_INT(frame.msg_class, 0x06);
    CHECK_EQ_INT(frame.msg_id, 0x01);
}

static void test_parser_split_chunks(void)
{
    uint8_t storage[32];
    ubx_parser_t parser;
    ubx_frame_t frame;
    size_t consumed = 0u;
    ubx_status_t st;
    /* Split after sync, mid-header, and across checksum (sums to 8). */
    const size_t cuts[] = {1u, 2u, 3u, 1u, 1u};
    size_t off = 0u;
    size_t c;

    CHECK_EQ_INT(ubx_parser_init(&parser, storage, sizeof(storage), 32u), UBX_OK);

    for (c = 0u; c < sizeof(cuts) / sizeof(cuts[0]); ++c) {
        st = ubx_parser_feed(&parser, k_frame_cfg_msg_poll + off, cuts[c],
                             &consumed);
        off += consumed;
        if (c + 1u < sizeof(cuts) / sizeof(cuts[0])) {
            CHECK_EQ_INT(st, UBX_OK);
            CHECK_EQ_INT(consumed, (int)cuts[c]);
        }
    }
    CHECK_EQ_INT(st, UBX_FRAME_READY);
    CHECK_EQ_INT(off, (int)sizeof(k_frame_cfg_msg_poll));
    CHECK_EQ_INT(ubx_parser_get_frame(&parser, &frame), UBX_OK);
    CHECK_EQ_INT(frame.length, 0);
}

static void test_parser_concatenated_frames(void)
{
    uint8_t storage[64];
    ubx_parser_t parser;
    ubx_frame_t frame;
    uint8_t buf[32];
    uint8_t ack[10];
    size_t n = 0u;
    size_t consumed = 0u;
    ubx_status_t st;

    make_ack_ack(ack);
    memcpy(buf, k_frame_cfg_msg_poll, sizeof(k_frame_cfg_msg_poll));
    memcpy(buf + sizeof(k_frame_cfg_msg_poll), ack, sizeof(ack));
    n = sizeof(k_frame_cfg_msg_poll) + sizeof(ack);

    CHECK_EQ_INT(ubx_parser_init(&parser, storage, sizeof(storage), 64u), UBX_OK);

    st = ubx_parser_feed(&parser, buf, n, &consumed);
    CHECK_EQ_INT(st, UBX_FRAME_READY);
    CHECK_EQ_INT(consumed, (int)sizeof(k_frame_cfg_msg_poll));
    CHECK_EQ_INT(ubx_parser_get_frame(&parser, &frame), UBX_OK);
    CHECK_EQ_INT(frame.msg_class, 0x06);
    ubx_parser_clear_frame(&parser);

    st = ubx_parser_feed(&parser, buf + consumed, n - consumed, &consumed);
    CHECK_EQ_INT(st, UBX_FRAME_READY);
    CHECK_EQ_INT(ubx_parser_get_frame(&parser, &frame), UBX_OK);
    CHECK_EQ_INT(frame.msg_class, 0x05);
    CHECK_EQ_INT(frame.msg_id, 0x01);
    CHECK_EQ_INT(frame.length, 2);
    CHECK(frame.payload != NULL);
    CHECK_EQ_INT(frame.payload[0], 0x06);
    CHECK_EQ_INT(frame.payload[1], 0x01);
}

static void test_parser_noise_before_and_between(void)
{
    uint8_t storage[64];
    ubx_parser_t parser;
    ubx_frame_t frame;
    uint8_t stream[64];
    size_t n = 0u;
    size_t consumed = 0u;
    ubx_status_t st;
    uint8_t ack[10];

    make_ack_ack(ack);

    /* Noise, frame, noise, frame. */
    stream[n++] = 0x00u;
    stream[n++] = 0xFFu;
    stream[n++] = 0x24u; /* '$' — NMEA-like noise */
    memcpy(stream + n, k_frame_cfg_msg_poll, sizeof(k_frame_cfg_msg_poll));
    n += sizeof(k_frame_cfg_msg_poll);
    stream[n++] = 0xAAu;
    stream[n++] = 0x55u;
    memcpy(stream + n, ack, sizeof(ack));
    n += sizeof(ack);

    CHECK_EQ_INT(ubx_parser_init(&parser, storage, sizeof(storage), 64u), UBX_OK);

    st = ubx_parser_feed(&parser, stream, n, &consumed);
    CHECK_EQ_INT(st, UBX_FRAME_READY);
    CHECK_EQ_INT(ubx_parser_get_frame(&parser, &frame), UBX_OK);
    CHECK_EQ_INT(frame.msg_class, 0x06);
    ubx_parser_clear_frame(&parser);

    st = ubx_parser_feed(&parser, stream + consumed, n - consumed, &consumed);
    CHECK_EQ_INT(st, UBX_FRAME_READY);
    CHECK_EQ_INT(ubx_parser_get_frame(&parser, &frame), UBX_OK);
    CHECK_EQ_INT(frame.msg_class, 0x05);
}

static void test_parser_corrupt_checksum_recovery(void)
{
    uint8_t storage[64];
    ubx_parser_t parser;
    ubx_frame_t frame;
    uint8_t bad[8];
    uint8_t stream[32];
    size_t n = 0u;
    size_t consumed = 0u;
    ubx_status_t st;

    memcpy(bad, k_frame_cfg_msg_poll, sizeof(bad));
    bad[6] ^= 0xFFu; /* corrupt CK_A */

    memcpy(stream + n, bad, sizeof(bad));
    n += sizeof(bad);
    memcpy(stream + n, k_frame_cfg_msg_poll, sizeof(k_frame_cfg_msg_poll));
    n += sizeof(k_frame_cfg_msg_poll);

    CHECK_EQ_INT(ubx_parser_init(&parser, storage, sizeof(storage), 64u), UBX_OK);
    st = feed_all(&parser, stream, n);
    CHECK_EQ_INT(st, UBX_FRAME_READY);
    CHECK_EQ_INT(ubx_parser_get_frame(&parser, &frame), UBX_OK);
    CHECK_EQ_INT(frame.msg_class, 0x06);
    CHECK_EQ_INT(frame.msg_id, 0x01);

    /* Also corrupt CK_B only. */
    ubx_parser_reset(&parser);
    memcpy(bad, k_frame_cfg_msg_poll, sizeof(bad));
    bad[7] ^= 0x01u;
    n = 0u;
    memcpy(stream + n, bad, sizeof(bad));
    n += sizeof(bad);
    memcpy(stream + n, k_frame_cfg_msg_poll, sizeof(k_frame_cfg_msg_poll));
    n += sizeof(k_frame_cfg_msg_poll);
    CHECK_EQ_INT(feed_all(&parser, stream, n), UBX_FRAME_READY);
    CHECK_EQ_INT(ubx_parser_get_frame(&parser, &frame), UBX_OK);
    CHECK_EQ_INT(frame.msg_id, 0x01);
    (void)consumed;
}

static void test_parser_oversized_and_truncated(void)
{
    uint8_t storage[8];
    ubx_parser_t parser;
    ubx_frame_t frame;
    uint8_t oversized[16];
    uint8_t stream[32];
    size_t n = 0u;
    size_t out_len = 0u;
    const uint8_t big_payload[4] = {1u, 2u, 3u, 4u};

    /* max_payload = 2, but frame has payload length 4. */
    CHECK_EQ_INT(
        ubx_frame_encode(0x01u, 0x02u, big_payload, 4u, oversized, sizeof(oversized),
                         &out_len),
        UBX_OK);

    CHECK_EQ_INT(ubx_parser_init(&parser, storage, sizeof(storage), 2u), UBX_OK);

    memcpy(stream, oversized, out_len);
    n = out_len;
    memcpy(stream + n, k_frame_cfg_msg_poll, sizeof(k_frame_cfg_msg_poll));
    n += sizeof(k_frame_cfg_msg_poll);

    CHECK_EQ_INT(feed_all(&parser, stream, n), UBX_FRAME_READY);
    CHECK_EQ_INT(ubx_parser_get_frame(&parser, &frame), UBX_OK);
    CHECK_EQ_INT(frame.msg_class, 0x06);

    /* Truncated empty frame (header only), then a valid frame.
     * The next sync byte fails as CK_A and is replayed to resync. */
    {
        uint8_t trunc_storage[32];
        uint8_t trunc_stream[24];
        size_t tn = 0u;
        size_t consumed = 0u;
        ubx_status_t st;

        CHECK_EQ_INT(
            ubx_parser_init(&parser, trunc_storage, sizeof(trunc_storage), 32u),
            UBX_OK);

        memcpy(trunc_stream, k_frame_cfg_msg_poll, 6u); /* no checksum */
        tn = 6u;
        st = ubx_parser_feed(&parser, trunc_stream, tn, &consumed);
        CHECK_EQ_INT(st, UBX_OK);
        CHECK(!ubx_parser_has_frame(&parser));

        memcpy(trunc_stream, k_frame_cfg_msg_poll,
               sizeof(k_frame_cfg_msg_poll));
        CHECK_EQ_INT(feed_all(&parser, trunc_stream, sizeof(k_frame_cfg_msg_poll)),
                     UBX_FRAME_READY);
        CHECK_EQ_INT(ubx_parser_get_frame(&parser, &frame), UBX_OK);
        CHECK_EQ_INT(frame.msg_class, 0x06);
    }
}

static void test_parser_overlapping_sync(void)
{
    uint8_t storage[32];
    ubx_parser_t parser;
    ubx_frame_t frame;
    uint8_t stream[16];
    size_t n = 0u;

    /* Lone 0xB5, then another 0xB5 leading a real frame (overlapping sync). */
    stream[n++] = 0xB5u;
    memcpy(stream + n, k_frame_cfg_msg_poll, sizeof(k_frame_cfg_msg_poll));
    n += sizeof(k_frame_cfg_msg_poll);

    CHECK_EQ_INT(ubx_parser_init(&parser, storage, sizeof(storage), 32u), UBX_OK);
    CHECK_EQ_INT(feed_all(&parser, stream, n), UBX_FRAME_READY);
    CHECK_EQ_INT(ubx_parser_get_frame(&parser, &frame), UBX_OK);
    CHECK_EQ_INT(frame.msg_class, 0x06);

    /* B5 B5 62 ... */
    ubx_parser_reset(&parser);
    n = 0u;
    stream[n++] = 0xB5u;
    stream[n++] = 0xB5u;
    memcpy(stream + n, k_frame_cfg_msg_poll + 1u,
           sizeof(k_frame_cfg_msg_poll) - 1u);
    n += sizeof(k_frame_cfg_msg_poll) - 1u;
    CHECK_EQ_INT(feed_all(&parser, stream, n), UBX_FRAME_READY);
    CHECK_EQ_INT(ubx_parser_get_frame(&parser, &frame), UBX_OK);
    CHECK_EQ_INT(frame.msg_id, 0x01);
}

static void test_parser_independent_instances(void)
{
    uint8_t storage_a[32];
    uint8_t storage_b[32];
    ubx_parser_t a;
    ubx_parser_t b;
    ubx_frame_t frame;
    uint8_t ack[10];
    size_t consumed = 0u;

    make_ack_ack(ack);
    CHECK_EQ_INT(ubx_parser_init(&a, storage_a, sizeof(storage_a), 32u), UBX_OK);
    CHECK_EQ_INT(ubx_parser_init(&b, storage_b, sizeof(storage_b), 32u), UBX_OK);

    /* Partial feed to A only. */
    CHECK_EQ_INT(ubx_parser_feed(&a, k_frame_cfg_msg_poll, 4u, &consumed), UBX_OK);
    CHECK(!ubx_parser_has_frame(&a));
    CHECK(!ubx_parser_has_frame(&b));

    /* Complete a frame on B. */
    CHECK_EQ_INT(ubx_parser_feed(&b, ack, sizeof(ack), &consumed), UBX_FRAME_READY);
    CHECK(ubx_parser_has_frame(&b));
    CHECK(!ubx_parser_has_frame(&a));
    CHECK_EQ_INT(ubx_parser_get_frame(&b, &frame), UBX_OK);
    CHECK_EQ_INT(frame.msg_class, 0x05);

    /* Finish A. */
    CHECK_EQ_INT(
        ubx_parser_feed(&a, k_frame_cfg_msg_poll + 4u,
                        sizeof(k_frame_cfg_msg_poll) - 4u, &consumed),
        UBX_FRAME_READY);
    CHECK_EQ_INT(ubx_parser_get_frame(&a, &frame), UBX_OK);
    CHECK_EQ_INT(frame.msg_class, 0x06);
}

static void test_parser_storage_and_init_bounds(void)
{
    uint8_t storage[4];
    ubx_parser_t parser;

    CHECK_EQ_INT(ubx_parser_storage_size(0u), 0);
    CHECK_EQ_INT(ubx_parser_storage_size(100u), 100);

    CHECK_EQ_INT(ubx_parser_init(NULL, storage, sizeof(storage), 4u),
                 UBX_ERR_NULL);
    CHECK_EQ_INT(ubx_parser_init(&parser, NULL, sizeof(storage), 4u),
                 UBX_ERR_NULL);
    CHECK_EQ_INT(ubx_parser_init(&parser, storage, 3u, 4u),
                 UBX_ERR_BUFFER_TOO_SMALL);
    CHECK_EQ_INT(ubx_parser_init(&parser, storage, 4u, 4u), UBX_OK);

    /* Zero max payload: empty frames only; NULL storage allowed. */
    CHECK_EQ_INT(ubx_parser_init(&parser, NULL, 0u, 0u), UBX_OK);
    {
        size_t consumed = 0u;
        ubx_frame_t frame;
        CHECK_EQ_INT(ubx_parser_feed(&parser, k_frame_cfg_msg_poll,
                                     sizeof(k_frame_cfg_msg_poll), &consumed),
                     UBX_FRAME_READY);
        CHECK_EQ_INT(ubx_parser_get_frame(&parser, &frame), UBX_OK);
        CHECK_EQ_INT(frame.length, 0);
    }
}

static void test_invalid_frame_not_published(void)
{
    uint8_t storage[32];
    ubx_parser_t parser;
    uint8_t bad[8];
    size_t consumed = 0u;

    memcpy(bad, k_frame_cfg_msg_poll, sizeof(bad));
    bad[7] = (uint8_t)(bad[7] + 1u);

    CHECK_EQ_INT(ubx_parser_init(&parser, storage, sizeof(storage), 32u), UBX_OK);
    CHECK_EQ_INT(ubx_parser_feed(&parser, bad, sizeof(bad), &consumed), UBX_OK);
    CHECK(!ubx_parser_has_frame(&parser));
    {
        ubx_frame_t frame;
        CHECK_EQ_INT(ubx_parser_get_frame(&parser, &frame), UBX_ERR_NO_FRAME);
    }
}

int main(void)
{
    test_checksum_known_vectors();
    test_frame_encode_valid_and_bounds();
    test_parser_valid_frame();
    test_parser_byte_at_a_time();
    test_parser_split_chunks();
    test_parser_concatenated_frames();
    test_parser_noise_before_and_between();
    test_parser_corrupt_checksum_recovery();
    test_parser_oversized_and_truncated();
    test_parser_overlapping_sync();
    test_parser_independent_instances();
    test_parser_storage_and_init_bounds();
    test_invalid_frame_not_published();

    if (g_failures != 0) {
        fprintf(stderr, "\n%d / %d checks failed\n", g_failures, g_checks);
        return 1;
    }

    printf("OK: %d checks passed\n", g_checks);
    return 0;
}
