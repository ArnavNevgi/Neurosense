# Phase Summary

## Phase 1: Compile Readiness

- Added PlatformIO environments for the ESP8266 biometric monitor and ESP32 sleep alarm.
- Kept original `.ino` files in place.
- Added wrapper source files for PlatformIO builds.
- Removed Unicode Serial strings that could cause toolchain encoding issues.
- Added missing compile includes and forward declarations.

## Phase 2: Security Cleanup

- Replaced committed secrets with placeholders.
- Added `include/secrets.h.example`.
- Ignored `include/secrets.h`, root `secrets.h`, certificate files, key files, and `.env` files.
- Documented that `setInsecure()` is development-only TLS mode.

## Phase 3: Firmware Reliability

- Replaced blocking Wi-Fi setup with a bounded attempt and retry behavior.
- Replaced blocking MQTT reconnect loops with scheduled retry and backoff.
- Added sensor-ready flags.
- Removed permanent sensor failure locks.

## Phase 4: Telemetry Quality

- Added biometric validity flags.
- Added Azure metadata fields such as device ID, firmware version, timestamp, RSSI, status code, sensor-ready flags, and validity flags.
- Extended ThingSpeak numeric fields 5 through 8 for validity/status/RSSI.
- Did not add battery telemetry or NTP.

## Phase 5: Sleep Classifier

- Added a heuristic sleep classifier API in `include/sleep_classifier.h`.
- Added a 30-second rolling sample window in the ESP32 sleep firmware.
- Added standalone host-readable classifier vectors in `tests/test_sleep_classifier.cpp`.
- A standalone `g++` vector check was run during Phase 5; PlatformIO unit tests are not configured.

## Remaining TODOs

- Add CA certificate validation for Azure MQTT.
- Add a configured PlatformIO test target for the sleep classifier.
- Tune heuristic thresholds with real labeled data.
- Decide whether offline buffering is needed and define storage limits first.
- Keep documentation clear that this is not medical or clinical sleep staging.
