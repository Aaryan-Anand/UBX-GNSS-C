# ubx-gnss-c

A portable C99 library for the u-blox UBX binary GNSS protocol. Protocol logic is kept separate from UART, I²C, RTOS, and board-support code so the same core can be reused across embedded platforms.

## Features

- UBX frame encode with sync bytes `0xB5 0x62`, class, ID, little-endian length, payload, and CK_A/CK_B
- Checksum compute and verify
- Incremental streaming parser for arbitrary byte chunks (caller-owned bounded storage, multiple independent instances)
- Recovery from noise, truncated frames, bad checksums, oversized lengths, and overlapping sync patterns
- UBX-NAV-PVT (0x01 0x07) decode of the fixed 92-byte payload (u-blox M10 SPG layout), preserving native integer units
- UBX-ACK-ACK / UBX-ACK-NAK payload decode (acknowledged class and message ID)

No heap allocation. No platform or transport adapters in this library.

## Limitations

- Validated message layout follows the **u-blox M10 SPG** interface description (SAM-M10Q target). Other receivers are not claimed until tested.
- NAV-PVT decoding does **not** decide whether a fix is usable; applications must apply their own acceptance rules using `fix_type`, flags, and accuracy fields.
- Receiver configuration, blocking command transactions, and transport I/O are not provided.
- GNSS data may be inaccurate, stale, or invalid. This library is not safety-certified.

## Build and test

Requires a C99 toolchain and CMake 3.16+.

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Usage

Public API: [`include/ubx/ubx.h`](include/ubx/ubx.h).

**Parse a byte stream**, then decode known messages:

```c
uint8_t storage[256];
ubx_parser_t parser;
ubx_parser_init(&parser, storage, sizeof(storage), 256);

size_t consumed = 0;
ubx_status_t st = ubx_parser_feed(&parser, bytes, len, &consumed);
if (st == UBX_FRAME_READY) {
    ubx_frame_t frame;
    ubx_parser_get_frame(&parser, &frame);

    if (ubx_msg_is_nav_pvt(frame.msg_class, frame.msg_id)) {
        ubx_nav_pvt_t pvt;
        if (ubx_nav_pvt_decode(frame.payload, frame.length, &pvt) == UBX_OK) {
            /* pvt.lon / pvt.lat are 1e-7 deg; heights are mm; etc. */
        }
    } else if (ubx_msg_is_ack_ack(frame.msg_class, frame.msg_id) ||
               ubx_msg_is_ack_nak(frame.msg_class, frame.msg_id)) {
        ubx_ack_t ack;
        ubx_ack_decode(frame.payload, frame.length, &ack);
    }

    ubx_parser_clear_frame(&parser);
    /* feed any remaining bytes after `consumed` */
}
```

Frame payload pointers remain valid until the next `ubx_parser_feed()` after `ubx_parser_clear_frame()`, or until `ubx_parser_reset()`.

**Encode** an outbound frame with `ubx_frame_encode()`.

## License

Copyright 2026 Aaryan Anand

Licensed under the Apache License, Version 2.0. See [`LICENSE`](LICENSE).

C sources use `SPDX-License-Identifier: Apache-2.0`. Third-party code added later must be checked for license compatibility and attribution.
