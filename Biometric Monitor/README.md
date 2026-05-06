# NeuroSense Biometric Monitor

ESP8266 firmware for reading biometric and temperature sensors, then publishing telemetry to Azure IoT Hub and ThingSpeak.

This module is for prototyping only. It is not for medical use or clinical monitoring.

## Implementation

- Main file: `Biometric_Monitor/Biometric_Monitor.ino`
- Board: ESP8266 NodeMCU
- Sensors: MAX30100 and MLX90614
- Cloud outputs:
  - Azure IoT Hub via MQTT
  - ThingSpeak via HTTP
- Local output: Serial logs

The current Azure client still uses development-only TLS mode via `espClient.setInsecure()`. Add CA certificate validation before production deployment.

## Current Behavior

- Reads heart rate and SpO2 from MAX30100.
- Reads object and ambient temperature from MLX90614.
- Publishes Azure JSON telemetry with:
  - BPM, SpO2, ObjectTemp, AmbientTemp
  - `device_id`, `firmware_version`, `timestamp_ms`, `wifi_rssi`, `status_code`
  - `mlx_ready`, `pox_ready`
  - `hr_valid`, `spo2_valid`, `object_temp_valid`, `ambient_temp_valid`
- Sends ThingSpeak numeric fields:
  - Field 1: BPM
  - Field 2: SpO2
  - Field 3: ObjectTemp
  - Field 4: AmbientTemp
  - Field 5: HR validity, 1 or 0
  - Field 6: SpO2 validity, 1 or 0
  - Field 7: status code
  - Field 8: Wi-Fi RSSI, or -127 when offline
- Uses bounded Wi-Fi connection attempts and periodic Wi-Fi retry.
- Uses MQTT retry backoff instead of a permanent reconnect loop.
- Uses `mlxReady` and `poxReady` flags so sensor init failures do not hard-lock the firmware.

## Wiring

| Component | ESP8266 NodeMCU Pin | Notes |
| --- | --- | --- |
| MAX30100 SDA | D2 / GPIO4 | Shared I2C |
| MAX30100 SCL | D1 / GPIO5 | Shared I2C |
| MLX90614 SDA | D2 / GPIO4 | Shared I2C |
| MLX90614 SCL | D1 / GPIO5 | Shared I2C |
| Sensor VCC | 3.3V | Check breakout requirements |
| Sensor GND | GND | Common ground |

Use suitable I2C pullups for your sensor breakout boards.

## Required Software

Board package:

- ESP8266 platform, NodeMCU 1.0 / ESP-12E compatible target

Libraries:

- `MAX30100lib` by OXullo Intersecans, vendored under `Biometric Monitor/lib/MAX30100_PulseOximeter/Arduino-MAX30100-master`
- `Adafruit MLX90614 Library`
- `Adafruit BusIO`
- `PubSubClient`
- `ThingSpeak`

Core-provided libraries:

- `Wire`
- `SPI`
- `ESP8266WiFi`
- `WiFiClientSecure`

## Secrets Setup

For PlatformIO:

```powershell
Copy-Item include\secrets.h.example include\secrets.h
```

Edit only `include/secrets.h` with local values. Do not commit it.

For Arduino IDE, if the nested sketch cannot find the repo-level `include/secrets.h`, copy the same local file into `Biometric Monitor/Biometric_Monitor/secrets.h`.

## Build

From the repo root:

```powershell
pio run -e esp8266_biometric_monitor
```

## Upload

With PlatformIO, connect the ESP8266 board and run:

```powershell
pio run -e esp8266_biometric_monitor -t upload
```

With Arduino IDE, open `Biometric_Monitor/Biometric_Monitor.ino`, select an ESP8266 NodeMCU-compatible board, install the required libraries, configure local secrets, and upload.

## Troubleshooting

- `MAX30100_PulseOximeter.h` not found: install `MAX30100lib` or make sure the vendored library is available to your build system.
- `Adafruit_MLX90614.h` not found: install the Adafruit MLX90614 library and Adafruit BusIO.
- Wi-Fi does not connect: check `include/secrets.h`; firmware will continue offline and retry.
- Azure publish fails: check IoT Hub host, device ID, SAS token expiry, and MQTT topic.
- TLS warning: current firmware uses development-only TLS mode. Add CA validation before production use.
- ThingSpeak update fails: check channel number, write key, network connectivity, and ThingSpeak rate limits.
