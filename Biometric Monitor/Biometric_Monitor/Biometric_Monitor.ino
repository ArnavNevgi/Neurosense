#include <Wire.h>
#include <SPI.h>
#include <math.h>
#include <Adafruit_MLX90614.h>
#include "MAX30100_PulseOximeter.h"
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ThingSpeak.h>

#if defined(__has_include)
  #if __has_include(<secrets.h>)
    #include <secrets.h>
  #elif __has_include("secrets.h")
    #include "secrets.h"
  #endif
#endif

#ifndef NEUROSENSE_WIFI_SSID
#define NEUROSENSE_WIFI_SSID "YOUR_WIFI_SSID"
#endif

#ifndef NEUROSENSE_WIFI_PASSWORD
#define NEUROSENSE_WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#endif

#ifndef NEUROSENSE_AZURE_MQTT_SERVER
#define NEUROSENSE_AZURE_MQTT_SERVER "YOUR_AZURE_IOT_HUB.azure-devices.net"
#endif

#ifndef NEUROSENSE_AZURE_DEVICE_ID
#define NEUROSENSE_AZURE_DEVICE_ID "YOUR_AZURE_DEVICE_ID"
#endif

#ifndef NEUROSENSE_AZURE_SAS_TOKEN
#define NEUROSENSE_AZURE_SAS_TOKEN "YOUR_AZURE_SAS_TOKEN"
#endif

#ifndef NEUROSENSE_AZURE_MQTT_TOPIC
#define NEUROSENSE_AZURE_MQTT_TOPIC "devices/YOUR_AZURE_DEVICE_ID/messages/events/"
#endif

#ifndef NEUROSENSE_THINGSPEAK_CHANNEL_NUMBER
#define NEUROSENSE_THINGSPEAK_CHANNEL_NUMBER 0UL
#endif

#ifndef NEUROSENSE_THINGSPEAK_WRITE_API_KEY
#define NEUROSENSE_THINGSPEAK_WRITE_API_KEY "YOUR_THINGSPEAK_WRITE_API_KEY"
#endif

#ifndef BIOMETRIC_FIRMWARE_VERSION
#define BIOMETRIC_FIRMWARE_VERSION "0.1.0"
#endif

// MLX90614: Temperature sensor
Adafruit_MLX90614 mlx = Adafruit_MLX90614();

// MAX30100: Pulse oximeter (HR + SpO2)
PulseOximeter pox;

// Live data
float bpm = 0.0, spo2 = 0.0;
float temp_obj = 0.0, temp_amb = 0.0;
bool hrValid = false;
bool spo2Valid = false;
bool objectTempValid = false;
bool ambientTempValid = false;

// WiFi credentials
const char* ssid = NEUROSENSE_WIFI_SSID;
const char* password = NEUROSENSE_WIFI_PASSWORD;

// MQTT (Azure)
const char* mqttServer = NEUROSENSE_AZURE_MQTT_SERVER;
const int mqttPort = 8883;
const char* deviceId = NEUROSENSE_AZURE_DEVICE_ID;

// Replace with a valid Azure IoT Hub SAS token before uploading telemetry.
const char* sasToken = NEUROSENSE_AZURE_SAS_TOKEN;

const char* mqttTopic = NEUROSENSE_AZURE_MQTT_TOPIC;

WiFiClientSecure espClient;
PubSubClient mqttClient(espClient);
WiFiClient thingSpeakClient;

// ThingSpeak Config
unsigned long myChannelNumber = NEUROSENSE_THINGSPEAK_CHANNEL_NUMBER;
const char* myWriteAPIKey = NEUROSENSE_THINGSPEAK_WRITE_API_KEY;

// Timers
uint32_t lastUpload = 0;
#define REPORT_INTERVAL_MS 15000   // ThingSpeak minimum = 15 sec

enum DeviceStatus {
  STATUS_OK = 0,
  STATUS_WIFI_OFFLINE = 1,
  STATUS_MQTT_OFFLINE = 2,
  STATUS_SENSOR_ERROR = 3
};

bool mlxReady = false;
bool poxReady = false;
int statusCode = STATUS_SENSOR_ERROR;

const uint32_t WIFI_CONNECT_TIMEOUT_MS = 20000;
const uint32_t WIFI_RETRY_INTERVAL_MS = 30000;
bool wifiAttemptInProgress = false;
uint32_t wifiAttemptStarted = 0;
uint32_t nextWifiRetry = 0;

const uint32_t MQTT_RETRY_INITIAL_MS = 5000;
const uint32_t MQTT_RETRY_MAX_MS = 60000;
uint32_t mqttRetryDelay = MQTT_RETRY_INITIAL_MS;
uint32_t nextMqttRetry = 0;

// Beat detection callback
void onBeatDetected() {
  Serial.println("Beat detected.");
}

void logStatus(DeviceStatus status, const char* message) {
  Serial.print("[STATUS ");
  Serial.print((int)status);
  Serial.print("] ");
  Serial.println(message);
}

bool validRange(float value, float minValue, float maxValue) {
  return isfinite(value) && value >= minValue && value <= maxValue;
}

int boolToInt(bool value) {
  return value ? 1 : 0;
}

int currentWifiRssi() {
  return WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : -127;
}

DeviceStatus calculateStatusCode() {
  // Priority is intentionally simple for Phase 4: failed sensors first,
  // then Wi-Fi, then MQTT. Phase 4 only reports this status.
  if (!mlxReady || !poxReady) {
    return STATUS_SENSOR_ERROR;
  }

  if (WiFi.status() != WL_CONNECTED) {
    return STATUS_WIFI_OFFLINE;
  }

  if (!mqttClient.connected()) {
    return STATUS_MQTT_OFFLINE;
  }

  return STATUS_OK;
}

void startWiFiAttempt() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  logStatus(STATUS_WIFI_OFFLINE, "Starting WiFi connection attempt.");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  wifiAttemptStarted = millis();
  wifiAttemptInProgress = true;
}

void waitForInitialWiFi(uint32_t timeoutMs) {
  startWiFiAttempt();
  uint32_t start = millis();
  uint32_t lastDot = 0;

  while (WiFi.status() != WL_CONNECTED && millis() - start < timeoutMs) {
    if (poxReady) {
      pox.update();
    }

    if (millis() - lastDot >= 500) {
      Serial.print(".");
      lastDot = millis();
    }

    delay(50);
    yield();
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifiAttemptInProgress = false;
    Serial.println("\nWiFi connected. IP: " + WiFi.localIP().toString());
    logStatus(STATUS_OK, "WiFi connected.");
  } else {
    wifiAttemptInProgress = false;
    nextWifiRetry = millis() + WIFI_RETRY_INTERVAL_MS;
    Serial.println("\nWiFi offline; continuing boot.");
    logStatus(STATUS_WIFI_OFFLINE, "WiFi unavailable after bounded setup attempt.");
  }
}

void maintainWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    if (wifiAttemptInProgress) {
      wifiAttemptInProgress = false;
      Serial.println("\nWiFi connected. IP: " + WiFi.localIP().toString());
      logStatus(STATUS_OK, "WiFi connected.");
    }
    return;
  }

  uint32_t now = millis();

  if (wifiAttemptInProgress) {
    if (now - wifiAttemptStarted >= WIFI_CONNECT_TIMEOUT_MS) {
      wifiAttemptInProgress = false;
      nextWifiRetry = now + WIFI_RETRY_INTERVAL_MS;
      WiFi.disconnect();
      logStatus(STATUS_WIFI_OFFLINE, "WiFi retry timed out; will retry later.");
    }
    return;
  }

  if ((int32_t)(now - nextWifiRetry) >= 0) {
    startWiFiAttempt();
  }
}

// MQTT reconnect logic
void reconnectMQTT() {
  if (mqttClient.connected()) {
    mqttRetryDelay = MQTT_RETRY_INITIAL_MS;
    return;
  }

  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  uint32_t now = millis();
  if ((int32_t)(now - nextMqttRetry) < 0) {
    return;
  }

  Serial.println("Reconnecting to Azure IoT Hub...");
  String username = String(mqttServer) + "/" + deviceId + "/?api-version=2021-04-12";

  if (mqttClient.connect(deviceId, username.c_str(), sasToken)) {
    Serial.println("Reconnected to Azure.");
    logStatus(STATUS_OK, "MQTT connected.");
    mqttRetryDelay = MQTT_RETRY_INITIAL_MS;
    nextMqttRetry = now;
  } else {
    Serial.print("MQTT retry failed, rc=");
    Serial.println(mqttClient.state());
    logStatus(STATUS_MQTT_OFFLINE, "MQTT offline; retry scheduled.");
    nextMqttRetry = now + mqttRetryDelay;

    if (mqttRetryDelay < MQTT_RETRY_MAX_MS / 2) {
      mqttRetryDelay *= 2;
    } else {
      mqttRetryDelay = MQTT_RETRY_MAX_MS;
    }
  }
}

void readBiometricSensors() {
  float rawBpm = 0.0;
  float rawSpo2 = 0.0;
  float rawObjectTemp = 0.0;
  float rawAmbientTemp = 0.0;

  if (poxReady) {
    rawBpm = pox.getHeartRate();
    rawSpo2 = pox.getSpO2();
  }

  if (mlxReady) {
    rawObjectTemp = mlx.readObjectTempC();
    rawAmbientTemp = mlx.readAmbientTempC();
  }

  hrValid = poxReady && validRange(rawBpm, 30.0, 200.0);
  spo2Valid = poxReady && validRange(rawSpo2, 70.0, 100.0);
  objectTempValid = mlxReady && validRange(rawObjectTemp, -20.0, 120.0);
  ambientTempValid = mlxReady && validRange(rawAmbientTemp, -20.0, 80.0);

  bpm = hrValid ? rawBpm : 0;
  spo2 = spo2Valid ? rawSpo2 : 0;
  temp_obj = objectTempValid ? rawObjectTemp : 0;
  temp_amb = ambientTempValid ? rawAmbientTemp : 0;
}

void setup() {
  Serial.begin(115200);

  // I2C (NodeMCU pins)
  Wire.begin(D2, D1); // SDA = D2, SCL = D1

  // MLX90614 initialization
  Serial.print("Initializing temperature sensor... ");
  mlxReady = mlx.begin();
  if (!mlxReady) {
    Serial.println("Failed to detect MLX90614.");
    logStatus(STATUS_SENSOR_ERROR, "MLX90614 unavailable; continuing without temperature reads.");
  } else {
    Serial.println("Success.");
  }

  // MAX30100 initialization
  Serial.print("Initializing MAX30100... ");
  poxReady = pox.begin();
  if (!poxReady) {
    Serial.println("Failed. Check wiring.");
    logStatus(STATUS_SENSOR_ERROR, "MAX30100 unavailable; continuing without HR/SpO2 reads.");
  } else {
    Serial.println("Success.");
    pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
    pox.setOnBeatDetectedCallback(onBeatDetected);
  }

  // WiFi connection
  Serial.println("Connecting to WiFi...");
  waitForInitialWiFi(WIFI_CONNECT_TIMEOUT_MS);

  // Development-only TLS mode. Replace with CA certificate validation before production use.
  espClient.setInsecure();
  mqttClient.setServer(mqttServer, mqttPort);

  reconnectMQTT();

  // ThingSpeak initialization
  ThingSpeak.begin(thingSpeakClient);
}

void loop() {
  maintainWiFi();
  reconnectMQTT();

  if (mqttClient.connected()) {
    mqttClient.loop();
  }

  // Update MAX30100
  if (poxReady) {
    pox.update();
  }

  readBiometricSensors();
  statusCode = calculateStatusCode();

  // Display data
  Serial.println("------ Live Data ------");
  Serial.printf("BPM: %.1f valid=%d | SpO2: %.1f%% valid=%d\n", bpm, boolToInt(hrValid), spo2, boolToInt(spo2Valid));
  Serial.printf("Object Temp: %.2f C valid=%d | Ambient Temp: %.2f C valid=%d\n", temp_obj, boolToInt(objectTempValid), temp_amb, boolToInt(ambientTempValid));
  Serial.print("Status code: ");
  Serial.print(statusCode);
  Serial.print(" | WiFi RSSI: ");
  Serial.println(currentWifiRssi());
  Serial.println("------------------------");

  // Upload every 15 seconds
  if (millis() - lastUpload > REPORT_INTERVAL_MS) {
    // Azure payload
    String payload = String("{\"BPM\":") + bpm +
                     ",\"SpO2\":" + spo2 +
                     ",\"ObjectTemp\":" + temp_obj +
                     ",\"AmbientTemp\":" + temp_amb +
                     ",\"device_id\":\"" + deviceId + "\"" +
                     ",\"firmware_version\":\"" + BIOMETRIC_FIRMWARE_VERSION + "\"" +
                     ",\"timestamp_ms\":" + millis() +
                     ",\"wifi_rssi\":" + currentWifiRssi() +
                     ",\"status_code\":" + statusCode +
                     ",\"mlx_ready\":" + boolToInt(mlxReady) +
                     ",\"pox_ready\":" + boolToInt(poxReady) +
                     ",\"hr_valid\":" + boolToInt(hrValid) +
                     ",\"spo2_valid\":" + boolToInt(spo2Valid) +
                     ",\"object_temp_valid\":" + boolToInt(objectTempValid) +
                     ",\"ambient_temp_valid\":" + boolToInt(ambientTempValid) +
                     "}";

    // Send to Azure
    Serial.println("Azure payload:");
    Serial.println(payload);

    if (mqttClient.connected() && mqttClient.publish(mqttTopic, payload.c_str())) {
      Serial.println("Data sent to Azure IoT Hub.");
    } else {
      Serial.println("Failed to send data to Azure.");
      logStatus(STATUS_MQTT_OFFLINE, "Azure MQTT publish skipped or failed.");
    }

    // Send to ThingSpeak
    ThingSpeak.setField(1, bpm);
    ThingSpeak.setField(2, spo2);
    ThingSpeak.setField(3, temp_obj);
    ThingSpeak.setField(4, temp_amb);
    ThingSpeak.setField(5, boolToInt(hrValid));
    ThingSpeak.setField(6, boolToInt(spo2Valid));
    ThingSpeak.setField(7, statusCode);
    ThingSpeak.setField(8, currentWifiRssi());

    if (WiFi.status() == WL_CONNECTED) {
      int status = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
      if (status == 200) {
        Serial.println("Data sent to ThingSpeak.");
      } else {
        Serial.println("ThingSpeak update failed. HTTP code: " + String(status));
      }
    } else {
      logStatus(STATUS_WIFI_OFFLINE, "ThingSpeak update skipped; WiFi offline.");
    }

    lastUpload = millis();
  }

  delay(100);
}
