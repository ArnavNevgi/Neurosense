NeuroSense – Biometric Health Monitor

This project implements a wearable biometric monitoring system using the ESP8266 microcontroller. It collects and transmits real-time physiological data such as heart rate, SpO₂ (oxygen saturation), and body temperature to cloud platforms for storage and visualization.

Overview

The biometric monitor is part of the NeuroSense suite, designed to support continuous health monitoring using low-cost sensors. Data is captured from the body through integrated sensors and uploaded to both Azure IoT Hub (for secure telemetry streaming) and ThingSpeak (for real-time visualization and analytics).

Features

- Real-time monitoring of:
  - Heart rate (BPM)
  - SpO₂ (oxygen saturation)
  - Object and ambient temperature
- Secure MQTT data transmission to Azure IoT Hub
- HTTP API integration with ThingSpeak
- Lightweight and battery-operable design for wearable use

Hardware Requirements

| Component               | Description                               |
|------------------------|-------------------------------------------|
| ESP8266 NodeMCU        | Microcontroller with Wi-Fi                |
| MAX30100               | Pulse oximeter (HR + SpO₂) sensor         |
| MLX90614               | IR-based object and ambient temp sensor   |
| LiPo battery + charger | Power supply (optional for portability)   |

Wiring Summary

- **MAX30100 & MLX90614** share the I²C bus:
  - SDA → D2 (GPIO4)
  - SCL → D1 (GPIO5)
- Power from 3.3V or USB

Software Stack

- Arduino framework (C/C++)
- MQTT client via PubSubClient
- Secure connection using TLS over Wi-Fi
- ThingSpeak HTTP API
- Libraries used:
  - `MAX30100_PulseOximeter`
  - `Adafruit_MLX90614`
  - `ThingSpeak`
  - `ESP8266WiFi`

Cloud Architecture

- **Azure IoT Hub** receives JSON telemetry via MQTT over TLS
- **ThingSpeak** receives periodic updates using field mappings:
  - Field 1: Heart Rate
  - Field 2: SpO₂
  - Field 3: Object Temperature
  - Field 4: Ambient Temperature

File Structure

- `biometric_uploader.ino` – Main Arduino code
- `lib/` – Optional: Embedded libraries for portable use
- `azure_thingspeak_setup.md` – Configuration for cloud services
- `README.md` – Project documentation

Future Work

- Add onboard alerts using vibration or LEDs
- Integrate SD card logging for offline data backup
- Deploy ML-based anomaly detection for health trends
