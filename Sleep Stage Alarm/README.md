# NeuroSense Sleep Stage Alarm

ESP32 firmware for a heuristic sleep-state alarm prototype. It reads heart rate and motion, streams diagnostics over Bluetooth Serial, and triggers a vibration motor during a detected light-sleep wake window.

This module is not medical sleep staging and is not clinically validated. It is a heuristic prototype.

## Implementation

- Main file: `sleepstage_wake/sleepstage_wake.ino`
- Board: ESP32 Dev Module
- Sensors:
  - MAX30100 for heart rate and SpO2
  - MPU6050 for accelerometer motion
- Output: Bluetooth Serial device named `NeuroSleepESP32`
- Actuator: vibration motor on GPIO25 through a transistor driver
- Classifier: heuristic rolling-window sleep-state classifier

## Classifier

The classifier API lives in `include/sleep_classifier.h`. The firmware samples about every 2 seconds and stores a rolling 30-second window, currently 16 samples.

Each sample includes:

- heart rate
- heart-rate validity
- acceleration magnitude
- motion validity
- timestamp in milliseconds

The classifier can return:

- `SLEEP_STATE_UNKNOWN`
- `SLEEP_STATE_AWAKE`
- `SLEEP_STATE_LIGHT`
- `SLEEP_STATE_DEEP`

The heuristic uses movement count, acceleration variance, average heart rate, and heart-rate trend. The vibration alert still only triggers after the minimum sleep duration and when the classifier returns a valid, sample-backed `SLEEP_STATE_LIGHT` result.

## Wiring

| Component | ESP32 Pin | Notes |
| --- | --- | --- |
| MAX30100 SDA | GPIO21 | Shared I2C |
| MAX30100 SCL | GPIO22 | Shared I2C |
| MPU6050 SDA | GPIO21 | Shared I2C |
| MPU6050 SCL | GPIO22 | Shared I2C |
| Vibration motor driver input | GPIO25 | Drive through NPN transistor or suitable driver |
| Motor supply | External or board supply as appropriate | Do not drive motor directly from GPIO |
| Flyback diode | Across motor | Required for inductive load protection |
| GND | GND | Common ground |

## Required Software

Board package:

- ESP32 platform, ESP32 Dev Module compatible target

Libraries:

- `MAX30100lib` by OXullo Intersecans, vendored under `Biometric Monitor/lib/MAX30100_PulseOximeter/Arduino-MAX30100-master`
- `MPU6050_light`

Core-provided libraries:

- `Wire`
- `BluetoothSerial`

## Build

From the repo root:

```powershell
pio run -e esp32_sleep_stage_alarm
```

## Upload

With PlatformIO, connect the ESP32 board and run:

```powershell
pio run -e esp32_sleep_stage_alarm -t upload
```

With Arduino IDE, open `sleepstage_wake/sleepstage_wake.ino`, select an ESP32 Dev Module-compatible board, install the required libraries, and upload.

## Runtime Output

Serial and Bluetooth output include:

- motion magnitude and validity
- heart rate and validity
- SpO2 and validity
- classifier state
- average HR
- HR trend
- acceleration variance
- movement count
- enough-samples flag
- classifier validity flag

## Tests

`tests/test_sleep_classifier.cpp` contains standalone host-readable classifier vectors for light, deep, awake, invalid-HR, and boundary cases. The current repo does not configure PlatformIO unit tests.

## Troubleshooting

- `BluetoothSerial.h` not found: select an ESP32 board/core.
- `MPU6050_light.h` not found: install the `MPU6050_light` library.
- `MAX30100_PulseOximeter.h` not found: install `MAX30100lib` or make sure the vendored library is available.
- No Bluetooth output: pair with `NeuroSleepESP32` and open a Bluetooth Serial terminal.
- Motor does not vibrate: check GPIO25, transistor wiring, motor supply, common ground, and flyback diode orientation.
- Classifier stays `UNKNOWN`: wait for enough samples and confirm both sensors report valid readings.
