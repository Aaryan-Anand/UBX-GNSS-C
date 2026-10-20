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

## Hardware target

Initial development and validation target the **u-blox SAM-M10Q**. Support for other UBX-capable receivers is not claimed until explicitly validated.

## Status

**Pre-implementation.** The repository foundation is in place; library source, build files, and tests have not yet been added.

## Development

This project is independently maintained by Aaryan Anand. It is public and freely redistributable, including for commercial use, under the Apache License 2.0.

Future development should follow straightforward standards: clear, portable C; tests for parsing and edge cases as they are introduced; and documentation that stays accurate as the API evolves. Contributions, if accepted, should match that bar.

## Safety and disclaimer

GNSS position, velocity, and timing data may be inaccurate, stale, incomplete, or invalid. This library is **not** safety-certified and provides no warranty of correctness for navigation or control use. Applications must independently validate navigation data and apply appropriate engineering safeguards for their use case.

See the `LICENSE` for the full warranty disclaimer and limitation of liability.

## License

Copyright 2026 Aaryan Anand

Licensed under the Apache License, Version 2.0. See [`LICENSE`](LICENSE) for the full text.

Future C source files should carry SPDX identifiers (`SPDX-License-Identifier: Apache-2.0`) and an appropriate copyright statement. Third-party code incorporated later must be checked for license compatibility and attribution requirements.
