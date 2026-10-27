# Build and environment observations — 26 October 2026

Command: `ninja -C greenhouse/build`

Observed exit code: **0**. The existing ESP-IDF build compiled the current `greenhouse.c`, linked `greenhouse.elf`, generated `greenhouse.bin`, and passed the partition-size check. Application binary size was `0xe40e0` bytes, with `0x1bf20` bytes (11%) free in the smallest app partition. ESP-IDF printed Kconfig notices about invalid boolean defaults in Bluetooth/FATFS components; these did not prevent the build. This was an incremental build, not a clean rebuild or a device test.

Command: `node --check backend/index.js`

Observed exit code: **0**, no syntax errors reported.

No serial controller appeared under `/dev/ttyUSB*` or `/dev/ttyACM*`. No firmware was uploaded. Build success does not establish physical operation.

This file is a summary of the observed command output, not a verbatim terminal transcript.
