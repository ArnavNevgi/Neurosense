# NeuroSense: A Wearable System for Health Monitoring and Sleep Optimization

**NeuroSense** is an embedded systems project that combines physiological sensing, real-time analytics, and cloud connectivity in a wearable format. The system is divided into two core modules—one for continuous biometric health monitoring, and the other for intelligent sleep stage detection with a smart alarm interface. Both systems are designed to operate on resource-constrained microcontrollers (ESP8266/ESP32) and can be deployed independently or together as a unified wearable platform.

---

## Project Structure

### 1. biometric_monitor/
This module implements a wearable device for continuous biometric monitoring using the **MAX30100** (for heart rate and SpO₂) and **MLX90614** (for body and ambient temperature). Data is uploaded to **Azure IoT Hub** via secure MQTT, and also to **ThingSpeak** for real-time visualization.

- Sensors: MAX30100, MLX90614
- Cloud: Azure IoT Hub (MQTT) + ThingSpeak (HTTP)
- Language: Arduino (C++)
- Libraries: PubSubClient, ThingSpeak, Adafruit MLX90614

### 2. sleep_stage_alarm/
This module implements sleep phase detection using **heart rate variability** and **motion sensing (MPU6050)**. It activates a haptic vibration motor when light sleep is detected, enabling smart wake-ups. Bluetooth is used to stream real-time sleep stage data to a terminal or mobile device.

- Sensors: MAX30100, MPU6050
- Actuation: Vibration motor via transistor driver
- Communication: Bluetooth Serial (ESP32)
- Logic: On-device sleep classification

---

## Combined Use Case
Together, both modules enable full-cycle wearable health tracking—**from daytime biometric data logging to overnight sleep optimization**—making this system suitable for personalized wellness devices, remote patient monitoring, or IoT-based research tools.

---

## Requirements
- ESP8266 or ESP32 microcontroller
- Arduino IDE with required libraries
- Wi-Fi network for cloud sync
- ThingSpeak and Azure IoT Hub accounts

---
