# NeuroSense

NeuroSense is an Arduino embedded firmware repo with two wearable-style prototype modules:

- An ESP8266 biometric monitor that reads heart rate, SpO2, and temperature, then publishes telemetry to Azure IoT Hub and ThingSpeak.
- An ESP32 sleep-stage alarm that reads heart rate and motion, classifies sleep state with a heuristic rolling-window classifier, streams output over Bluetooth Serial, and drives a vibration motor during light-sleep wake windows.

This project is for education, prototyping, and engineering experimentation only. It is not for medical use, diagnosis, treatment, safety-critical monitoring, or clinical sleep staging.

## Current Architecture

```text
MAX30100 / MLX90614 / MPU6050
    -> ESP32 / ESP8266 firmware
    -> filtering + heuristic sleep classifier
    -> MQTT / HTTP / Bluetooth Serial
    -> Azure IoT Hub / ThingSpeak / terminal review
```

The biometric firmware uses placeholder-safe configuration, sensor-ready flags, bounded Wi-Fi/MQTT retry behavior, and validity-tagged Azure telemetry. The sleep alarm firmware uses guarded sensor handling and a 30-second heuristic classifier window. Production TLS certificate validation is still a TODO; the ESP8266 Azure client currently uses development-only TLS mode.

## Repo Tree

```text
C:\Neurosense
|   README.md
|   platformio.ini
|   .gitignore
|
+---Biometric Monitor
|   |   README.md
|   |   azure_thingspeak_setup.md
|   |
|   +---Biometric_Monitor
|   |       Biometric_Monitor.ino
|   |
|   \---lib
|       \---MAX30100_PulseOximeter
|           \---Arduino-MAX30100-master
|
+---Sleep Stage Alarm
|   |   README.md
|   |
|   \---sleepstage_wake
|           sleepstage_wake.ino
|
+---include
|       secrets.h.example
|       sleep_classifier.h
|
+---src
|       biometric_monitor.cpp
|       sleep_stage_alarm.cpp
|       sleep_classifier.cpp
|
+---tests
|       test_sleep_classifier.cpp
|
\---docs
        build_notes.md
        phase_summary.md
        security_notes.md
```

## Modules

### Biometric Monitor

- Main file: `Biometric Monitor/Biometric_Monitor/Biometric_Monitor.ino`
- Board: ESP8266 NodeMCU
- Sensors: MAX30100 and MLX90614
- Outputs: Azure IoT Hub MQTT telemetry and ThingSpeak HTTP updates
- Reliability: bounded Wi-Fi setup/retry, MQTT backoff, sensor-ready flags
- Telemetry: BPM, SpO2, object temperature, ambient temperature, metadata, status code, sensor-ready flags, and validity flags

### Sleep Stage Alarm

- Main file: `Sleep Stage Alarm/sleepstage_wake/sleepstage_wake.ino`
- Board: ESP32 Dev Module
- Sensors: MAX30100 and MPU6050
- Outputs: Bluetooth Serial diagnostic stream
- Actuator: vibration motor on GPIO25 through a transistor driver
- Classifier: heuristic rolling-window classifier over about 30 seconds
- Tests: `tests/test_sleep_classifier.cpp` contains standalone host-readable classifier vectors

## Build With PlatformIO

From the repo root:

```powershell
pio run -e esp8266_biometric_monitor
pio run -e esp32_sleep_stage_alarm
```

The PlatformIO environments are defined in `platformio.ini`. They use wrapper files in `src/` so the original Arduino `.ino` files stay in place.

## Arduino IDE Compatibility

The original `.ino` sketches remain in their Arduino-friendly folder layout:

- `Biometric Monitor/Biometric_Monitor/Biometric_Monitor.ino`
- `Sleep Stage Alarm/sleepstage_wake/sleepstage_wake.ino`

For Arduino IDE builds, install the board package and libraries listed in each module README. If the IDE does not see the repo-level `include/` folder, copy your local secrets file into the biometric sketch folder as `secrets.h`.

## Secrets Setup

1. Copy `include/secrets.h.example` to `include/secrets.h`.
2. Fill in local Wi-Fi, Azure IoT Hub, SAS token, ThingSpeak, and firmware version values.
3. Never commit `include/secrets.h`.

`include/secrets.h` is ignored by Git. Azure SAS tokens expire and should be regenerated as needed.

## Implemented

- PlatformIO build environments for ESP8266 and ESP32.
- Placeholder-based secret handling via `include/secrets.h.example`.
- Biometric metadata and validity-tagged Azure payloads.
- ThingSpeak fields for biometric readings plus validity/status fields.
- Bounded Wi-Fi setup/retry and MQTT retry backoff in the biometric firmware.
- Sensor-ready flags and non-locking sensor failure handling.
- ESP32 heuristic sleep classifier using a 30-second rolling sample window.
- Standalone classifier test vectors.

## Planned TODOs

- Replace ESP8266 `setInsecure()` with CA certificate validation before production use.
- Add a configured PlatformIO test target for the sleep classifier.
- Tune sleep classifier thresholds using real labeled data.
- Reduce noisy Serial output if needed during longer sessions.
- Add optional offline buffering only after defining storage limits and failure behavior.

## Known Limitations

- Not medical-grade and not clinically validated.
- Sleep states are heuristic estimates, not diagnostic sleep stages.
- Azure TLS certificate validation is not yet implemented.
- No battery voltage telemetry is implemented because no battery sensing circuit is defined in firmware.
- No mobile app, dashboard screenshots, or cloud analytics are included in this repo.
