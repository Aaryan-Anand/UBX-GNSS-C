/*
 * Copyright 2026 Aaryan Anand
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Portable UBX configuration interface (CFG-VALSET / CFG-VALGET) and a small
 * set of documented M10 configuration key IDs. Transport I/O and ACK/NAK
 * session handling are owned by the application.
 */

#ifndef UBX_UBX_CFG_H
#define UBX_UBX_CFG_H

#include "ubx/ubx.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum key IDs / key-value pairs per VALSET or VALGET message. */
#define UBX_CFG_MAX_ITEMS 64u

/** VALSET version 0 (transactionless). */
#define UBX_CFG_VALSET_VERSION 0x00u
/** VALGET poll request version. */
#define UBX_CFG_VALGET_POLL_VERSION 0x00u
/** VALGET response version. */
#define UBX_CFG_VALGET_RESPONSE_VERSION 0x01u

/* --- Layers ---------------------------------------------------------------- */

/** VALSET layers bitfield: update RAM (effective immediately). */
#define UBX_CFG_LAYER_RAM 0x01u
/** VALSET layers bitfield: update battery-backed RAM. */
#define UBX_CFG_LAYER_BBR 0x02u
/** VALSET layers bitfield: update flash (if present). */
#define UBX_CFG_LAYER_FLASH 0x04u

/** VALGET single-layer select: RAM. */
#define UBX_CFG_LAYER_GET_RAM 0u
/** VALGET single-layer select: BBR. */
#define UBX_CFG_LAYER_GET_BBR 1u
/** VALGET single-layer select: Flash. */
#define UBX_CFG_LAYER_GET_FLASH 2u
/** VALGET single-layer select: Default (read-only). */
#define UBX_CFG_LAYER_GET_DEFAULT 7u

/* --- Configuration key IDs (u-blox M10 SPG) -------------------------------- */
/*
 * Key IDs encode value storage size in bits 30..28:
 *   0x1 = 1 byte (L / one-bit boolean stored as U1)
 *   0x2 = 1 byte (U1/I1/E1/X1)
 *   0x3 = 2 bytes
 *   0x4 = 4 bytes
 *   0x5 = 8 bytes
 * Callers may use any valid key; the helpers below are a convenience set.
 */

/** CFG-UART1-BAUDRATE (U4): UART1 baud rate [baud]. */
#define UBX_CFG_UART1_BAUDRATE 0x40520001u

/** CFG-UART1OUTPROT-UBX (L): output UBX on UART1. */
#define UBX_CFG_UART1OUTPROT_UBX 0x10740001u
/** CFG-UART1OUTPROT-NMEA (L): output NMEA on UART1. */
#define UBX_CFG_UART1OUTPROT_NMEA 0x10740002u

/** CFG-UART1INPROT-UBX (L): accept UBX input on UART1. */
#define UBX_CFG_UART1INPROT_UBX 0x10730001u

/** CFG-MSGOUT-UBX_NAV_PVT_UART1 (U1): NAV-PVT output rate on UART1. */
#define UBX_CFG_MSGOUT_UBX_NAV_PVT_UART1 0x20910007u

/** CFG-RATE-MEAS (U2): measurement period [0.001 s], e.g. 1000 = 1 Hz. */
#define UBX_CFG_RATE_MEAS 0x30210001u
/** CFG-RATE-NAV (U2): navigation rate ratio (measurements per solution), 1..127. */
#define UBX_CFG_RATE_NAV 0x30210002u

/** CFG-NAVSPG-DYNMODEL (E1): dynamic platform model. */
#define UBX_CFG_NAVSPG_DYNMODEL 0x20110021u

/** DYNMODEL: portable. */
#define UBX_CFG_DYNMODEL_PORT 0u
/** DYNMODEL: stationary. */
#define UBX_CFG_DYNMODEL_STAT 2u
/** DYNMODEL: pedestrian. */
#define UBX_CFG_DYNMODEL_PED 3u
/** DYNMODEL: automotive. */
#define UBX_CFG_DYNMODEL_AUTOMOT 4u
/** DYNMODEL: sea. */
#define UBX_CFG_DYNMODEL_SEA 5u
/** DYNMODEL: airborne <1g. */
#define UBX_CFG_DYNMODEL_AIR1 6u
/** DYNMODEL: airborne <2g. */
#define UBX_CFG_DYNMODEL_AIR2 7u
/** DYNMODEL: airborne <4g. */
#define UBX_CFG_DYNMODEL_AIR4 8u

/** CFG-SIGNAL-GPS_ENA (L). */
#define UBX_CFG_SIGNAL_GPS_ENA 0x1031001fu
/** CFG-SIGNAL-GPS_L1CA_ENA (L). */
#define UBX_CFG_SIGNAL_GPS_L1CA_ENA 0x10310001u
/** CFG-SIGNAL-SBAS_ENA (L). */
#define UBX_CFG_SIGNAL_SBAS_ENA 0x10310020u
/** CFG-SIGNAL-SBAS_L1CA_ENA (L). */
#define UBX_CFG_SIGNAL_SBAS_L1CA_ENA 0x10310005u
/** CFG-SIGNAL-GAL_ENA (L). */
#define UBX_CFG_SIGNAL_GAL_ENA 0x10310021u
/** CFG-SIGNAL-GAL_E1_ENA (L). */
#define UBX_CFG_SIGNAL_GAL_E1_ENA 0x10310007u
/** CFG-SIGNAL-BDS_ENA (L). */
#define UBX_CFG_SIGNAL_BDS_ENA 0x10310022u
/** CFG-SIGNAL-BDS_B1_ENA (L) BeiDou B1I. */
#define UBX_CFG_SIGNAL_BDS_B1_ENA 0x1031000du
/** CFG-SIGNAL-BDS_B1C_ENA (L). */
#define UBX_CFG_SIGNAL_BDS_B1C_ENA 0x1031000fu
/** CFG-SIGNAL-QZSS_ENA (L). */
#define UBX_CFG_SIGNAL_QZSS_ENA 0x10310024u
/** CFG-SIGNAL-QZSS_L1CA_ENA (L). */
#define UBX_CFG_SIGNAL_QZSS_L1CA_ENA 0x10310012u
/** CFG-SIGNAL-QZSS_L1S_ENA (L). */
#define UBX_CFG_SIGNAL_QZSS_L1S_ENA 0x10310014u
/**
 * CFG-SIGNAL-GLO_ENA (L). Present in M10 SPG 5.10; omitted from SPG 5.30
 * SIGNAL tables — receivers without GLONASS may NAK this key.
 */
#define UBX_CFG_SIGNAL_GLO_ENA 0x10310025u
/** CFG-SIGNAL-GLO_L1_ENA (L). Same availability note as GLO_ENA. */
#define UBX_CFG_SIGNAL_GLO_L1_ENA 0x10310018u

/* --- Generic key/value API ------------------------------------------------ */

/**
 * One configuration item.
 *
 * `value` holds the numeric payload in host form; when encoded, only the
 * little-endian width implied by `key_id` is written (1/2/4/8 bytes).
 * Logical (L) and enumeration (E*) values use the same integer encoding.
 */
typedef struct ubx_cfg_item {
    uint32_t key_id;
    uint64_t value;
} ubx_cfg_item_t;

/**
 * Return the value storage size in bytes encoded in `key_id` bits 30..28,
 * or 0 if the size field is not a supported identifier (0x1..0x5).
 */
size_t ubx_cfg_key_value_size(uint32_t key_id);

/** Non-zero if (msg_class, msg_id) is UBX-CFG-VALSET. */
int ubx_msg_is_cfg_valset(uint8_t msg_class, uint8_t msg_id);
/** Non-zero if (msg_class, msg_id) is UBX-CFG-VALGET. */
int ubx_msg_is_cfg_valget(uint8_t msg_class, uint8_t msg_id);

/**
 * Encode a transactionless UBX-CFG-VALSET payload (version 0).
 *
 * `layers` is a bitfield of UBX_CFG_LAYER_RAM / BBR / FLASH (at least one bit
 * required; unknown bits rejected). At most UBX_CFG_MAX_ITEMS pairs.
 *
 * Building a valid packet does not mean the receiver applied it — verify with
 * ACK/NAK and VALGET readback in the application.
 */
ubx_status_t ubx_cfg_valset_encode(uint8_t layers, const ubx_cfg_item_t *items,
                                   size_t item_count, uint8_t *out,
                                   size_t out_cap, size_t *out_len);

/**
 * Encode a UBX-CFG-VALGET poll-request payload (version 0).
 *
 * `layer` must be UBX_CFG_LAYER_GET_* (RAM/BBR/Flash/Default).
 * `position` skips that many key-value pairs when wildcards expand.
 * `keys` may be NULL only when `key_count` is 0.
 */
ubx_status_t ubx_cfg_valget_poll_encode(uint8_t layer, uint16_t position,
                                        const uint32_t *keys, size_t key_count,
                                        uint8_t *out, size_t out_cap,
                                        size_t *out_len);

/**
 * Decode a UBX-CFG-VALGET response payload (version 1).
 *
 * Writes up to `items_cap` decoded items. On success, sets version/layer/
 * position and `*item_count`. Returns UBX_ERR_BUFFER_TOO_SMALL if more items
 * are present than `items_cap` (partial fill still written).
 */
ubx_status_t ubx_cfg_valget_decode(const uint8_t *payload, uint16_t length,
                                   uint8_t *version, uint8_t *layer,
                                   uint16_t *position, ubx_cfg_item_t *items,
                                   size_t items_cap, size_t *item_count);

#ifdef __cplusplus
}
#endif

#endif /* UBX_UBX_CFG_H */
