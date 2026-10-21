# ubx-gnss-c

A portable C99 library for the u-blox UBX binary GNSS protocol. Protocol logic is kept separate from UART, I²C, RTOS, and board-support code so the same core can be reused across embedded platforms.

## Features

- UBX frame encode with sync bytes `0xB5 0x62`, class, ID, little-endian length, payload, and CK_A/CK_B
- Checksum compute and verify
- Incremental streaming parser for arbitrary byte chunks (caller-owned bounded storage, multiple independent instances)
- Recovery from noise, truncated frames, bad checksums, oversized lengths, and overlapping sync patterns
- UBX-NAV-PVT (0x01 0x07) decode of the fixed 92-byte payload (u-blox M10 SPG layout), preserving native integer units
- UBX-ACK-ACK / UBX-ACK-NAK payload decode (acknowledged class and message ID)
- UBX-CFG-VALSET (v0) / UBX-CFG-VALGET poll and response encode/decode, with key size taken from each configuration key ID
- VALGET-only wildcard selectors (all-groups `0x0FFF0000`, group `0x00ggFFFF`) for configuration enumeration; rejected by VALSET
- Documented M10 configuration key constants for UART1, NAV-PVT output rate, measurement/navigation rate, dynamic model, and GNSS signal enables
- UBX-MON-VER poll and version-string decode (software, hardware, extensions)

No heap allocation. No platform or transport adapters in this library.

## Limitations

- Validated message layout follows the **u-blox M10 SPG** interface description (SAM-M10Q target). Other receivers are not claimed until tested.
- NAV-PVT decoding does **not** decide whether a fix is usable; applications must apply their own acceptance rules.
- Encoded configuration messages are not applied by this library. The application owns transport, timeouts, ACK/NAK handling, and VALGET readback. A well-formed VALSET does not imply the receiver accepted or stored the values.
- GLONASS signal keys are included for compatibility with M10 builds that expose them; some SPG revisions may NAK those keys.
- OTP programming and other irreversible operations are not provided.
- GNSS data may be inaccurate, stale, or invalid. This library is not safety-certified.

## Build and test

Requires a C99 toolchain and CMake 3.16+.

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Usage

| Header | Contents |
|--------|----------|
| [`include/ubx/ubx.h`](include/ubx/ubx.h) | Frame encode, checksum, parser, NAV-PVT, ACK |
| [`include/ubx/ubx_cfg.h`](include/ubx/ubx_cfg.h) | VALSET/VALGET and configuration key IDs |
| [`include/ubx/ubx_mon.h`](include/ubx/ubx_mon.h) | MON-VER poll and decode |

**Parse a byte stream**, then decode known messages:

```c
#include "ubx/ubx.h"
#include "ubx/ubx_cfg.h"
#include "ubx/ubx_mon.h"

uint8_t storage[256];
ubx_parser_t parser;
ubx_parser_init(&parser, storage, sizeof(storage), 256);

size_t consumed = 0;
if (ubx_parser_feed(&parser, bytes, len, &consumed) == UBX_FRAME_READY) {
    ubx_frame_t frame;
    ubx_parser_get_frame(&parser, &frame);

    if (ubx_msg_is_nav_pvt(frame.msg_class, frame.msg_id)) {
        ubx_nav_pvt_t pvt;
        ubx_nav_pvt_decode(frame.payload, frame.length, &pvt);
    } else if (ubx_msg_is_cfg_valget(frame.msg_class, frame.msg_id)) {
        ubx_cfg_item_t items[16];
        uint8_t version, layer;
        uint16_t position;
        size_t n = 0;
        ubx_cfg_valget_decode(frame.payload, frame.length, &version, &layer,
                              &position, items, 16, &n);
    } else if (ubx_msg_is_mon_ver(frame.msg_class, frame.msg_id)) {
        ubx_mon_ver_t ver;
        ubx_mon_ver_decode(frame.payload, frame.length, &ver, NULL, 0);
    }

    ubx_parser_clear_frame(&parser);
}
```

**Build a configuration set** (application sends the frame and checks ACK/NAK):

```c
ubx_cfg_item_t items[] = {
    { UBX_CFG_UART1_BAUDRATE, 115200u },
    { UBX_CFG_UART1OUTPROT_UBX, 1u },
    { UBX_CFG_UART1OUTPROT_NMEA, 0u },
    { UBX_CFG_MSGOUT_UBX_NAV_PVT_UART1, 1u },
};
uint8_t payload[64];
size_t payload_len = 0;
ubx_cfg_valset_encode(UBX_CFG_LAYER_RAM, items, 4, payload, sizeof(payload),
                      &payload_len);

uint8_t frame[80];
size_t frame_len = 0;
ubx_frame_encode(UBX_CLASS_CFG, UBX_CFG_VALSET, payload,
                 (uint16_t)payload_len, frame, sizeof(frame), &frame_len);
```

Frame payload pointers remain valid until the next `ubx_parser_feed()` after `ubx_parser_clear_frame()`, or until `ubx_parser_reset()`.

## License

Copyright 2026 Aaryan Anand

Licensed under the Apache License, Version 2.0. See [`LICENSE`](LICENSE).

C sources use `SPDX-License-Identifier: Apache-2.0`. Third-party code added later must be checked for license compatibility and attribution.
