# Cloud Setup Guide for NeuroSense Biometric Monitor

This document provides configuration steps for integrating the NeuroSense wearable with two cloud platforms:

- **Azure IoT Hub** (for secure telemetry via MQTT)
- **ThingSpeak** (for real-time dashboard and visualization)

---

## 1. Azure IoT Hub Setup

### 1.1 Prerequisites
- Azure account: https://portal.azure.com
- ESP8266/ESP32 device with Wi-Fi
- MQTT library in Arduino (PubSubClient)

---

### 1.2 Steps to Configure Azure IoT Hub

#### Step 1: Create an IoT Hub
1. Go to [Azure Portal](https://portal.azure.com)
2. Search for **IoT Hub** → Click **Create**
3. Fill in:
   - Subscription: Free Tier or Basic
   - Resource Group: Create new or select existing
   - IoT Hub Name: `MyProjectHub22` (must be globally unique)
   - Region: Closest to your location
4. Click **Review + Create**

---

#### Step 2: Register Your Device
1. Go to your created IoT Hub
2. Navigate to **IoT Devices** → **+ New**
3. Fill in:
   - Device ID: `1195`
   - Authentication Type: **Symmetric Key**
4. Click **Save**

> This creates two access keys (Primary and Secondary) for this device.

---

#### Step 3: Generate a SAS Token
Use an online SAS token generator like:
[https://darenmay.com/SasTokenGenerator](https://darenmay.com/SasTokenGenerator)

Fill in:
- Resource URI: `MyProjectHub22.azure-devices.net/devices/1195`
- Key: Copy from your device's **Primary Key**
- Expiry: e.g., `1 day` or `7 days`

Click **Generate** → Copy the SAS Token

---

#### Step 4: Update Arduino Code
In your Arduino `.ino` file:
```cpp
const char* mqttServer = "MyProjectHub22.azure-devices.net";
const char* deviceId = "1195";
const char* sasToken = "SharedAccessSignature sr=..."; // paste your token
const char* mqttTopic = "devices/1195/messages/events/";