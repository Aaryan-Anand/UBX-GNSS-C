/*
 * Copyright 2026 Aaryan Anand
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Portable UBX binary protocol core: checksums, frame encoding, and a
 * streaming parser. Transport I/O and message semantics are out of scope.
 */

#ifndef UBX_UBX_H
#define UBX_UBX_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** First sync byte of a UBX frame. */
#define UBX_SYNC1 0xB5u
/** Second sync byte of a UBX frame. */
#define UBX_SYNC2 0x62u

/** Bytes before the payload: sync1, sync2, class, id, length (LE). */
#define UBX_HEADER_SIZE 6u
/** Trailing checksum bytes: CK_A, CK_B. */
#define UBX_CHECKSUM_SIZE 2u
/** Fixed overhead excluding payload: header + checksum. */
#define UBX_FRAME_OVERHEAD (UBX_HEADER_SIZE + UBX_CHECKSUM_SIZE)

/**
 * Status and result codes.
 *
 * UBX_OK and UBX_FRAME_READY are success outcomes. All other values are errors.
 */
typedef enum ubx_status {
    UBX_OK = 0,
    /** A complete, checksum-validated frame is available. */
    UBX_FRAME_READY = 1,
    UBX_ERR_NULL = -1,
    UBX_ERR_BUFFER_TOO_SMALL = -2,
    UBX_ERR_PAYLOAD_TOO_LARGE = -3,
    UBX_ERR_BAD_CHECKSUM = -4,
    UBX_ERR_STATE = -5,
    UBX_ERR_NO_FRAME = -6
} ubx_status_t;

/**
 * View of a decoded UBX frame.
 *
 * When obtained from a parser, `payload` points into the caller-owned parser
 * storage. That pointer remains valid until the next successful
 * ubx_parser_feed() after ubx_parser_clear_frame(), or until ubx_parser_reset().
 * Do not free or resize the storage while a frame view is in use.
 */
typedef struct ubx_frame {
    uint8_t msg_class;
    uint8_t msg_id;
    uint16_t length;
    const uint8_t *payload;
} ubx_frame_t;

/**
 * Streaming UBX parser.
 *
 * Instances are independent. All mutable state lives in this structure and in
 * the caller-provided storage buffer passed to ubx_parser_init().
 */
typedef struct ubx_parser {
    uint8_t *storage;
    uint16_t max_payload;
    uint16_t storage_cap;

    uint8_t state;
    uint8_t msg_class;
    uint8_t msg_id;
    uint16_t length;
    uint16_t payload_index;
    uint8_t ck_a;
    uint8_t ck_b;
    uint8_t frame_ready;
} ubx_parser_t;

/* --- Checksum ------------------------------------------------------------- */

/**
 * Compute UBX CK_A / CK_B over `data[0 .. len)`.
 *
 * The covered bytes are class, id, length (2 LE bytes), and payload — not the
 * sync bytes and not the checksum itself.
 */
void ubx_checksum_compute(const uint8_t *data, size_t len,
                          uint8_t *ck_a, uint8_t *ck_b);

/**
 * Return UBX_OK if (ck_a, ck_b) match the checksum of `data`, else
 * UBX_ERR_BAD_CHECKSUM. Null data with len == 0 is allowed.
 */
ubx_status_t ubx_checksum_verify(const uint8_t *data, size_t len,
                                 uint8_t ck_a, uint8_t ck_b);

/* --- Frame encode --------------------------------------------------------- */

/** Total encoded frame size for a given payload length (overhead + payload). */
size_t ubx_frame_encoded_size(uint16_t payload_len);

/**
 * Encode one UBX frame into `out`.
 *
 * Writes sync, class, id, little-endian length, payload, and checksum.
 * `payload` may be NULL only when `payload_len` is 0.
 * On success, sets `*out_len` to the number of bytes written and returns UBX_OK.
 */
ubx_status_t ubx_frame_encode(uint8_t msg_class, uint8_t msg_id,
                              const uint8_t *payload, uint16_t payload_len,
                              uint8_t *out, size_t out_cap, size_t *out_len);

/* --- Streaming parser ----------------------------------------------------- */

/**
 * Minimum storage size (bytes) required for a parser with the given maximum
 * accepted payload length. Storage holds only the payload; header fields are
 * kept in ubx_parser_t.
 */
size_t ubx_parser_storage_size(uint16_t max_payload);

/**
 * Initialize a parser with caller-owned storage.
 *
 * `storage_size` must be at least ubx_parser_storage_size(max_payload).
 * `max_payload` is the largest payload length the parser will accept; larger
 * length fields cause the candidate frame to be abandoned and sync search to
 * resume.
 */
ubx_status_t ubx_parser_init(ubx_parser_t *parser, uint8_t *storage,
                             size_t storage_size, uint16_t max_payload);

/** Clear parser state and any pending frame. Storage contents are not zeroed. */
void ubx_parser_reset(ubx_parser_t *parser);

/**
 * Feed an arbitrary chunk of bytes into the parser.
 *
 * Does not assume one call equals one frame. May return:
 * - UBX_OK: all of `len` consumed; no complete frame yet
 * - UBX_FRAME_READY: a validated frame is available; `*consumed` may be less
 *   than `len`. Call ubx_parser_get_frame(), then ubx_parser_clear_frame(),
 *   then feed any remaining bytes.
 * - UBX_ERR_STATE: a previous frame is still pending (clear it first)
 * - UBX_ERR_NULL: invalid arguments
 *
 * Noise, truncated frames, bad checksums, and oversized lengths are handled
 * internally by abandoning the candidate and continuing sync search within
 * the fed data when possible.
 */
ubx_status_t ubx_parser_feed(ubx_parser_t *parser, const uint8_t *data,
                             size_t len, size_t *consumed);

/** Non-zero if a validated frame is available. */
int ubx_parser_has_frame(const ubx_parser_t *parser);

/**
 * Fill `frame` with a view of the pending validated frame.
 * Returns UBX_ERR_NO_FRAME if none is ready.
 */
ubx_status_t ubx_parser_get_frame(const ubx_parser_t *parser, ubx_frame_t *frame);

/**
 * Discard the pending frame so parsing can continue.
 * Safe to call when no frame is pending.
 */
void ubx_parser_clear_frame(ubx_parser_t *parser);

#ifdef __cplusplus
}
#endif

#endif /* UBX_UBX_H */
