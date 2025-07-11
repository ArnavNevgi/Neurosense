NeuroSense – Sleep Stage Detection with Smart Wake-Up

This project implements a wearable sleep stage detection system using ESP32, integrating motion and biometric sensors to classify sleep phases. The system activates a gentle haptic alarm when the user is in a light sleep phase, enabling more natural and less disruptive wake-ups.

Overview

The sleep stage detector is designed to function as part of the NeuroSense wearable platform. It leverages accelerometry and heart rate variability patterns to estimate sleep depth. Data is transmitted via Bluetooth for real-time monitoring. A vibration motor provides tactile feedback as a smart alarm when optimal wake-up conditions are detected.

Features

- Sleep phase estimation using motion and heart rate
- Real-time Bluetooth data streaming
- Smart alarm using vibration feedback
- Expandable for TinyML-based sleep classification

Sleep Stage Logic

The system categorizes sleep phases using the following criteria:
- **Light Sleep**: Moderate body movement and HR between 60–90 BPM
- **Deep Sleep**: Minimal movement and HR below 60 BPM

If the user remains in light sleep for a minimum duration, the device triggers a vibration alarm.

Hardware Requirements

| Component           | Description                               |
|--------------------|-------------------------------------------|
| ESP32               | Microcontroller with BLE and Wi-Fi       |
| MAX30100            | Pulse oximeter for heart rate + SpO₂     |
| MPU6050             | 6-axis accelerometer/gyroscope            |
| Vibration Motor     | Provides haptic feedback                  |
| NPN Transistor      | Motor driver (e.g., 2N2222 or S8050)      |
| Flyback Diode       | Motor protection (e.g., 1N4148)           |
| LiPo battery        | Power supply for wearable use             |

Wiring Summary

- **I²C Bus (shared)**:
  - SDA → GPIO21
  - SCL → GPIO22
- **Vibration Motor**:
  - GPIO25 through NPN transistor

Software Stack

- Arduino IDE using ESP32 core
- Sensor libraries:
  - `MPU6050_light`
  - `MAX30100_PulseOximeter`
  - `BluetoothSerial`
- Bluetooth serial output for Android/PC terminal apps
- On-device logic for wake-up detection

File Structure

- `sleep_detector.ino` – Main firmware code
- `ml_notes.md` – Notes on possible TinyML model integration
- `schematic.png` – Wiring diagram for hardware
- `README.md` – Project documentation

Future Extensions

- Integrate TinyML classifier for sleep stage recognition
- Add mobile app for alarm configuration and visualization
- Store sleep session logs using onboard EEPROM or SD card
- Cloud upload of sleep analysis results (via BLE or Wi-Fi)

Usage Notes

To test the system:
1. Upload code to ESP32 using Arduino IDE.
2. Open Bluetooth terminal on Android or PC.
3. Monitor motion, HR, and SpO₂ values.
4. Simulate sleep conditions to trigger smart alarm.