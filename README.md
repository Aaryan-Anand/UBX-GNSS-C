# ubx-gnss-c

A framework-agnostic C library for the u-blox UBX GNSS binary protocol.

## Motivation

Embedded projects often need reliable UBX parsing without tying protocol logic to a specific MCU, RTOS, or hardware abstraction layer. This library aims to provide a portable, reusable implementation that can be integrated into applications independently of platform I/O.

## Design goals

- **Portability** — portable C99 with no hard dependency on a particular toolchain, RTOS, or board support package
- **Minimal dependencies** — keep the core protocol layer free of external libraries where practical
- **Bounded memory use** — avoid unbounded buffering and dynamic allocation in the protocol path
- **Deterministic binary parsing** — parse UBX messages in a predictable, stream-oriented manner
- **I/O separation** — isolate protocol logic from platform-specific UART, I2C, or other transport code

## Status

**UBX protocol core.** The library provides portable frame encoding, checksum handling, and an incremental streaming parser. Message decoding (for example NAV-PVT), receiver configuration, and transport adapters are not implemented yet.

## What is implemented

| Area | Contents |
|------|----------|
| Checksum | CK_A / CK_B compute and verify over class, ID, length, and payload |
| Frame encode | Sync `0xB5 0x62`, class, ID, 16-bit little-endian length, payload, checksum, with output capacity checks |
| Streaming parser | Incremental feed of arbitrary byte chunks; multiple independent instances; caller-owned bounded storage; configurable max payload length |
| Recovery | Noise, truncated frames, bad checksums, oversized lengths, overlapping sync patterns |
| Build / test | CMake library target and host-side CTest suite (no GNSS receiver or platform SDK required) |

Public API header: [`include/ubx/ubx.h`](include/ubx/ubx.h).

### Buffer lifetimes

Parser frame views returned by `ubx_parser_get_frame()` point into the caller-owned storage passed to `ubx_parser_init()`. The payload pointer remains valid until the next successful `ubx_parser_feed()` after `ubx_parser_clear_frame()`, or until `ubx_parser_reset()`. Clear a pending frame before feeding more data.

### Build and test

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Development

This project is independently maintained by Aaryan Anand. It is public and freely redistributable, including for commercial use, under the Apache License 2.0.


## Safety and disclaimer

GNSS position, velocity, and timing data may be inaccurate, stale, incomplete, or invalid. This library is **not** safety-certified and provides no warranty of correctness for navigation or control use. Applications must independently validate navigation data and apply appropriate engineering safeguards for their use case.

See the `LICENSE` for the full warranty disclaimer and limitation of liability.

## License

Copyright 2026 Aaryan Anand

Licensed under the Apache License, Version 2.0. See [`LICENSE`](LICENSE) for the full text.

C source and headers use SPDX identifiers (`SPDX-License-Identifier: Apache-2.0`) with an appropriate copyright statement. Third-party code incorporated later must be checked for license compatibility and attribution requirements.
