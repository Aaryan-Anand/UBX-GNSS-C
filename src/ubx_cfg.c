/*
 * Copyright 2026 Aaryan Anand
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * UBX-CFG-VALSET (v0) and UBX-CFG-VALGET encode/decode.
 */

#include "ubx/ubx_cfg.h"

static uint16_t ubx_cfg_read_u16_le(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static uint32_t ubx_cfg_read_u32_le(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static void ubx_cfg_write_u16_le(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)((v >> 8) & 0xFFu);
}

static void ubx_cfg_write_u32_le(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)((v >> 8) & 0xFFu);
    p[2] = (uint8_t)((v >> 16) & 0xFFu);
    p[3] = (uint8_t)((v >> 24) & 0xFFu);
}

static void ubx_cfg_write_value_le(uint8_t *p, uint64_t value, size_t nbytes)
{
    size_t i;
    for (i = 0u; i < nbytes; ++i) {
        p[i] = (uint8_t)((value >> (8u * i)) & 0xFFu);
    }
}

static uint64_t ubx_cfg_read_value_le(const uint8_t *p, size_t nbytes)
{
    uint64_t v = 0u;
    size_t i;
    for (i = 0u; i < nbytes; ++i) {
        v |= (uint64_t)p[i] << (8u * i);
    }
    return v;
}

size_t ubx_cfg_key_value_size(uint32_t key_id)
{
    switch ((key_id >> 28) & 0x7u) {
    case 0x1u: /* L: one bit stored as one byte */
    case 0x2u: /* one byte */
        return 1u;
    case 0x3u:
        return 2u;
    case 0x4u:
        return 4u;
    case 0x5u:
        return 8u;
    default:
        return 0u;
    }
}

uint32_t ubx_cfg_valget_wildcard_group(uint8_t group_id)
{
    if (group_id < 0x01u || group_id > 0xFEu) {
        return 0u;
    }
    return ((uint32_t)group_id << 16) | (uint32_t)UBX_CFG_VALGET_WILDCARD_ITEM;
}

int ubx_cfg_key_is_valget_wildcard(uint32_t key_id)
{
    /* All groups: bits 16..27 == 0xfff (documented 0x0FFF0000). */
    if (((key_id >> 16) & 0xFFFu) == 0xFFFu) {
        return 1;
    }
    /* Group wildcard: bits 0..15 == 0xffff and group ID in 0x01..0xfe. */
    if ((key_id & (uint32_t)UBX_CFG_VALGET_WILDCARD_ITEM) ==
        (uint32_t)UBX_CFG_VALGET_WILDCARD_ITEM) {
        uint8_t group = (uint8_t)((key_id >> 16) & 0xFFu);
        if (group >= 0x01u && group <= 0xFEu) {
            return 1;
        }
    }
    return 0;
}

static int ubx_cfg_valget_key_ok(uint32_t key_id)
{
    return (ubx_cfg_key_value_size(key_id) != 0u ||
            ubx_cfg_key_is_valget_wildcard(key_id) != 0)
               ? 1
               : 0;
}

int ubx_msg_is_cfg_valset(uint8_t msg_class, uint8_t msg_id)
{
    return (msg_class == (uint8_t)UBX_CLASS_CFG &&
            msg_id == (uint8_t)UBX_CFG_VALSET)
               ? 1
               : 0;
}

int ubx_msg_is_cfg_valget(uint8_t msg_class, uint8_t msg_id)
{
    return (msg_class == (uint8_t)UBX_CLASS_CFG &&
            msg_id == (uint8_t)UBX_CFG_VALGET)
               ? 1
               : 0;
}

static int ubx_cfg_layers_valset_ok(uint8_t layers)
{
    if (layers == 0u) {
        return 0;
    }
    if ((layers & (uint8_t)~(UBX_CFG_LAYER_RAM | UBX_CFG_LAYER_BBR |
                             UBX_CFG_LAYER_FLASH)) != 0u) {
        return 0;
    }
    return 1;
}

static int ubx_cfg_layer_valget_ok(uint8_t layer)
{
    return (layer == UBX_CFG_LAYER_GET_RAM || layer == UBX_CFG_LAYER_GET_BBR ||
            layer == UBX_CFG_LAYER_GET_FLASH ||
            layer == UBX_CFG_LAYER_GET_DEFAULT)
               ? 1
               : 0;
}

ubx_status_t ubx_cfg_valset_encode(uint8_t layers, const ubx_cfg_item_t *items,
                                   size_t item_count, uint8_t *out,
                                   size_t out_cap, size_t *out_len)
{
    size_t need = 4u;
    size_t i;
    size_t off;

    if (out_len != NULL) {
        *out_len = 0u;
    }
    if (out_len == NULL || (items == NULL && item_count > 0u)) {
        return UBX_ERR_NULL;
    }
    if (out == NULL && out_cap > 0u) {
        return UBX_ERR_NULL;
    }
    if (!ubx_cfg_layers_valset_ok(layers)) {
        return UBX_ERR_BAD_ARG;
    }
    if (item_count > UBX_CFG_MAX_ITEMS) {
        return UBX_ERR_LIMIT;
    }

    for (i = 0u; i < item_count; ++i) {
        size_t vsize = ubx_cfg_key_value_size(items[i].key_id);
        if (vsize == 0u) {
            return UBX_ERR_BAD_ARG;
        }
        need += 4u + vsize;
    }

    if (out_cap < need) {
        return UBX_ERR_BUFFER_TOO_SMALL;
    }

    out[0] = (uint8_t)UBX_CFG_VALSET_VERSION;
    out[1] = layers;
    out[2] = 0u;
    out[3] = 0u;
    off = 4u;

    for (i = 0u; i < item_count; ++i) {
        size_t vsize = ubx_cfg_key_value_size(items[i].key_id);
        ubx_cfg_write_u32_le(&out[off], items[i].key_id);
        off += 4u;
        ubx_cfg_write_value_le(&out[off], items[i].value, vsize);
        off += vsize;
    }

    *out_len = need;
    return UBX_OK;
}

ubx_status_t ubx_cfg_valget_poll_encode(uint8_t layer, uint16_t position,
                                        const uint32_t *keys, size_t key_count,
                                        uint8_t *out, size_t out_cap,
                                        size_t *out_len)
{
    size_t need;
    size_t i;

    if (out_len != NULL) {
        *out_len = 0u;
    }
    if (out_len == NULL || (keys == NULL && key_count > 0u)) {
        return UBX_ERR_NULL;
    }
    if (out == NULL && out_cap > 0u) {
        return UBX_ERR_NULL;
    }
    if (!ubx_cfg_layer_valget_ok(layer)) {
        return UBX_ERR_BAD_ARG;
    }
    if (key_count > UBX_CFG_MAX_ITEMS) {
        return UBX_ERR_LIMIT;
    }

    for (i = 0u; i < key_count; ++i) {
        /* Normal keys or documented VALGET wildcards only. */
        if (!ubx_cfg_valget_key_ok(keys[i])) {
            return UBX_ERR_BAD_ARG;
        }
    }

    need = 4u + (key_count * 4u);
    if (out_cap < need) {
        return UBX_ERR_BUFFER_TOO_SMALL;
    }

    out[0] = (uint8_t)UBX_CFG_VALGET_POLL_VERSION;
    out[1] = layer;
    ubx_cfg_write_u16_le(&out[2], position);
    for (i = 0u; i < key_count; ++i) {
        ubx_cfg_write_u32_le(&out[4u + (i * 4u)], keys[i]);
    }

    *out_len = need;
    return UBX_OK;
}

ubx_status_t ubx_cfg_valget_decode(const uint8_t *payload, uint16_t length,
                                   uint8_t *version, uint8_t *layer,
                                   uint16_t *position, ubx_cfg_item_t *items,
                                   size_t items_cap, size_t *item_count)
{
    size_t off;
    size_t count = 0u;
    ubx_status_t status = UBX_OK;

    if (item_count != NULL) {
        *item_count = 0u;
    }
    if (payload == NULL || version == NULL || layer == NULL ||
        position == NULL || item_count == NULL) {
        return UBX_ERR_NULL;
    }
    if (items == NULL && items_cap > 0u) {
        return UBX_ERR_NULL;
    }
    if (length < 4u) {
        return UBX_ERR_BAD_LENGTH;
    }

    *version = payload[0];
    *layer = payload[1];
    *position = ubx_cfg_read_u16_le(&payload[2]);

    if (*version != (uint8_t)UBX_CFG_VALGET_RESPONSE_VERSION) {
        return UBX_ERR_BAD_ARG;
    }
    if (!ubx_cfg_layer_valget_ok(*layer)) {
        return UBX_ERR_BAD_ARG;
    }

    off = 4u;
    while (off < (size_t)length) {
        uint32_t key;
        size_t vsize;

        if ((size_t)length - off < 4u) {
            return UBX_ERR_BAD_LENGTH;
        }
        key = ubx_cfg_read_u32_le(&payload[off]);
        off += 4u;
        vsize = ubx_cfg_key_value_size(key);
        if (vsize == 0u) {
            return UBX_ERR_BAD_ARG;
        }
        if ((size_t)length - off < vsize) {
            return UBX_ERR_BAD_LENGTH;
        }

        if (count < items_cap) {
            items[count].key_id = key;
            items[count].value = ubx_cfg_read_value_le(&payload[off], vsize);
        } else {
            status = UBX_ERR_BUFFER_TOO_SMALL;
        }
        off += vsize;
        ++count;
        if (count > UBX_CFG_MAX_ITEMS) {
            return UBX_ERR_LIMIT;
        }
    }

    /* On overflow, report total items present; only items_cap were written. */
    *item_count = count;
    return status;
}
