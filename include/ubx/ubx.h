/*
 * Copyright 2026 Aaryan Anand
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Portable UBX binary protocol: checksums, frame encoding, streaming parser,
 * and selected message decoders. Transport I/O is out of scope.
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

/** UBX message class: Navigation. */
#define UBX_CLASS_NAV 0x01u
/** UBX message class: Acknowledgement. */
#define UBX_CLASS_ACK 0x05u
/** UBX message class: Configuration. */
#define UBX_CLASS_CFG 0x06u
/** UBX message class: Monitoring. */
#define UBX_CLASS_MON 0x0Au

/** UBX-NAV-PVT message ID. */
#define UBX_NAV_PVT 0x07u
/** UBX-ACK-NAK message ID. */
#define UBX_ACK_NAK 0x00u
/** UBX-ACK-ACK message ID. */
#define UBX_ACK_ACK 0x01u
/** UBX-CFG-VALSET message ID. */
#define UBX_CFG_VALSET 0x8Au
/** UBX-CFG-VALGET message ID. */
#define UBX_CFG_VALGET 0x8Bu
/** UBX-MON-VER message ID. */
#define UBX_MON_VER 0x04u

/** Fixed UBX-NAV-PVT payload length (u-blox M10 SPG interface description). */
#define UBX_NAV_PVT_PAYLOAD_LEN 92u
/** Fixed UBX-ACK-ACK / UBX-ACK-NAK payload length. */
#define UBX_ACK_PAYLOAD_LEN 2u

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
    UBX_ERR_NO_FRAME = -6,
    /** Payload length does not match the expected message size. */
    UBX_ERR_BAD_LENGTH = -7,
    /** Invalid configuration layer, key ID, or related argument. */
    UBX_ERR_BAD_ARG = -8,
    /** Exceeded a protocol limit (e.g. more than 64 configuration items). */
    UBX_ERR_LIMIT = -9
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

/* --- Message identification ----------------------------------------------- */

/** Non-zero if (msg_class, msg_id) is UBX-NAV-PVT. */
int ubx_msg_is_nav_pvt(uint8_t msg_class, uint8_t msg_id);
/** Non-zero if (msg_class, msg_id) is UBX-ACK-ACK. */
int ubx_msg_is_ack_ack(uint8_t msg_class, uint8_t msg_id);
/** Non-zero if (msg_class, msg_id) is UBX-ACK-NAK. */
int ubx_msg_is_ack_nak(uint8_t msg_class, uint8_t msg_id);

/* --- UBX-NAV-PVT (0x01 0x07) ---------------------------------------------- */

/**
 * Decoded UBX-NAV-PVT fields.
 *
 * Values use the receiver's native integer units from the u-blox M10 SPG
 * interface description. Scaling to physical units (degrees, etc.) is left to
 * the application. This decoder does not decide whether a fix is usable.
 *
 * Layout matches the fixed 92-byte payload (protocol versions with X2 flags3).
 */
typedef struct ubx_nav_pvt {
    uint32_t i_tow;     /**< GPS time of week of the navigation epoch [ms] */
    uint16_t year;      /**< Year (UTC) [y] */
    uint8_t month;      /**< Month, 1..12 (UTC) */
    uint8_t day;        /**< Day of month, 1..31 (UTC) */
    uint8_t hour;       /**< Hour of day, 0..23 (UTC) */
    uint8_t min;        /**< Minute of hour, 0..59 (UTC) */
    uint8_t sec;        /**< Seconds of minute, 0..60 (UTC) */
    uint8_t valid;      /**< UTC validity flags (see UBX_NAV_PVT_VALID_*) */
    uint32_t t_acc;     /**< Time accuracy estimate (UTC) [ns] */
    int32_t nano;       /**< Fraction of second (UTC) [ns], -1e9..1e9 */
    uint8_t fix_type;   /**< GNSS fix type (0=none .. 5=time only) */
    uint8_t flags;      /**< Fix status flags (see UBX_NAV_PVT_FLAGS_*) */
    uint8_t flags2;     /**< Additional flags (see UBX_NAV_PVT_FLAGS2_*) */
    uint8_t num_sv;     /**< Number of satellites used in the Nav solution */
    int32_t lon;        /**< Longitude [1e-7 deg] */
    int32_t lat;        /**< Latitude [1e-7 deg] */
    int32_t height;     /**< Height above ellipsoid [mm] */
    int32_t h_msl;      /**< Height above mean sea level [mm] */
    uint32_t h_acc;     /**< Horizontal accuracy estimate [mm] */
    uint32_t v_acc;     /**< Vertical accuracy estimate [mm] */
    int32_t vel_n;      /**< NED north velocity [mm/s] */
    int32_t vel_e;      /**< NED east velocity [mm/s] */
    int32_t vel_d;      /**< NED down velocity [mm/s] */
    int32_t g_speed;    /**< Ground speed (2-D) [mm/s] */
    int32_t head_mot;   /**< Heading of motion (2-D) [1e-5 deg] */
    uint32_t s_acc;     /**< Speed accuracy estimate [mm/s] */
    uint32_t head_acc;  /**< Heading accuracy estimate [1e-5 deg] */
    uint16_t p_dop;     /**< Position DOP [0.01] */
    uint16_t flags3;    /**< Additional flags (see UBX_NAV_PVT_FLAGS3_*) */
    int32_t head_veh;   /**< Heading of vehicle (2-D) [1e-5 deg] */
    int16_t mag_dec;    /**< Magnetic declination [1e-2 deg] */
    uint16_t mag_acc;   /**< Magnetic declination accuracy [1e-2 deg] */
} ubx_nav_pvt_t;

/** valid bit 0: valid UTC date */
#define UBX_NAV_PVT_VALID_DATE(v) (((uint8_t)(v)&0x01u) != 0u)
/** valid bit 1: valid UTC time of day */
#define UBX_NAV_PVT_VALID_TIME(v) (((uint8_t)(v)&0x02u) != 0u)
/** valid bit 2: UTC time of day fully resolved */
#define UBX_NAV_PVT_VALID_FULLY_RESOLVED(v) (((uint8_t)(v)&0x04u) != 0u)
/** valid bit 3: valid magnetic declination */
#define UBX_NAV_PVT_VALID_MAG(v) (((uint8_t)(v)&0x08u) != 0u)

/** flags bit 0: gnssFixOk */
#define UBX_NAV_PVT_FLAGS_GNSS_FIX_OK(f) (((uint8_t)(f)&0x01u) != 0u)
/** flags bit 1: differential corrections applied */
#define UBX_NAV_PVT_FLAGS_DIFF_SOLN(f) (((uint8_t)(f)&0x02u) != 0u)
/** flags bits 4..2: power save mode state */
#define UBX_NAV_PVT_FLAGS_PSM_STATE(f) ((uint8_t)(((uint8_t)(f) >> 2) & 0x07u))
/** flags bit 5: heading of vehicle valid */
#define UBX_NAV_PVT_FLAGS_HEAD_VEH_VALID(f) (((uint8_t)(f)&0x20u) != 0u)
/** flags bits 7..6: carrier phase range solution status */
#define UBX_NAV_PVT_FLAGS_CARR_SOLN(f) ((uint8_t)(((uint8_t)(f) >> 6) & 0x03u))

/** flags2 bit 5: UTC date/time confirmation info available */
#define UBX_NAV_PVT_FLAGS2_CONFIRMED_AVAI(f) (((uint8_t)(f)&0x20u) != 0u)
/** flags2 bit 6: UTC date validity confirmed */
#define UBX_NAV_PVT_FLAGS2_CONFIRMED_DATE(f) (((uint8_t)(f)&0x40u) != 0u)
/** flags2 bit 7: UTC time of day confirmed */
#define UBX_NAV_PVT_FLAGS2_CONFIRMED_TIME(f) (((uint8_t)(f)&0x80u) != 0u)

/** flags3 bit 0: invalid lon/lat/height/hMSL */
#define UBX_NAV_PVT_FLAGS3_INVALID_LLH(f) (((uint16_t)(f)&0x0001u) != 0u)

/**
 * Decode a UBX-NAV-PVT payload into `out`.
 *
 * Requires `length == UBX_NAV_PVT_PAYLOAD_LEN` (92). Does not interpret fix
 * quality for application use — callers must apply their own acceptance rules.
 */
ubx_status_t ubx_nav_pvt_decode(const uint8_t *payload, uint16_t length,
                                ubx_nav_pvt_t *out);

/* --- UBX-ACK-ACK / UBX-ACK-NAK -------------------------------------------- */

/**
 * Decoded acknowledgement payload (clsID, msgID of the referenced message).
 * Use ubx_msg_is_ack_ack() / ubx_msg_is_ack_nak() on the frame header to tell
 * ACK from NAK. This API does not perform command/response transactions.
 */
typedef struct ubx_ack {
    uint8_t cls_id; /**< Class ID of the acknowledged / not-acknowledged message */
    uint8_t msg_id; /**< Message ID of the acknowledged / not-acknowledged message */
} ubx_ack_t;

/**
 * Decode a UBX-ACK-ACK or UBX-ACK-NAK payload into `out`.
 * Requires `length == UBX_ACK_PAYLOAD_LEN` (2).
 */
ubx_status_t ubx_ack_decode(const uint8_t *payload, uint16_t length,
                            ubx_ack_t *out);

#ifdef __cplusplus
}
#endif

#endif /* UBX_UBX_H */
