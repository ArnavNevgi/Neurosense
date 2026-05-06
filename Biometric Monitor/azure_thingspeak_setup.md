# Azure IoT Hub and ThingSpeak Setup

This guide configures cloud endpoints for the NeuroSense biometric monitor.

Do not hardcode real credentials into committed firmware or documentation. Put local values in `include/secrets.h`, which is ignored by Git.

## Azure IoT Hub

1. Create an Azure IoT Hub in the Azure Portal.
2. Register a device in the IoT Hub.
3. Use a device ID of your choice, then copy that value into local secrets:

```cpp
#define NEUROSENSE_AZURE_DEVICE_ID "YOUR_AZURE_DEVICE_ID"
```

4. Set the MQTT host and topic in local secrets:

```cpp
#define NEUROSENSE_AZURE_MQTT_SERVER "YOUR_AZURE_IOT_HUB.azure-devices.net"
#define NEUROSENSE_AZURE_MQTT_TOPIC "devices/YOUR_AZURE_DEVICE_ID/messages/events/"
```

5. Generate a SAS token using a trusted local or Azure-supported workflow.
6. Put the SAS token only in `include/secrets.h`:

```cpp
#define NEUROSENSE_AZURE_SAS_TOKEN "YOUR_AZURE_SAS_TOKEN"
```

Azure SAS tokens expire. Regenerate them as needed and do not commit them.

## ThingSpeak

1. Create or open a ThingSpeak channel.
2. Copy the channel number and write API key into local secrets:

```cpp
#define NEUROSENSE_THINGSPEAK_CHANNEL_NUMBER 0UL
#define NEUROSENSE_THINGSPEAK_WRITE_API_KEY "YOUR_THINGSPEAK_WRITE_API_KEY"
```

3. Field mapping used by the firmware:

| Field | Value |
| --- | --- |
| 1 | BPM |
| 2 | SpO2 |
| 3 | Object temperature |
| 4 | Ambient temperature |
| 5 | HR validity, 1 or 0 |
| 6 | SpO2 validity, 1 or 0 |
| 7 | status code |
| 8 | Wi-Fi RSSI, or -127 when offline |

## TLS Limitation

The current ESP8266 firmware still calls `espClient.setInsecure()`. This is development-only TLS mode and does not validate the Azure IoT Hub server certificate. Add CA certificate validation before any production deployment.

## Build Check

After configuring local secrets, build from the repo root:

```powershell
pio run -e esp8266_biometric_monitor
```
