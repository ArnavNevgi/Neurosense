#include <Wire.h>
#include <Adafruit_MLX90614.h>
#include "MAX30100_PulseOximeter.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ThingSpeak.h>

// MLX90614: Temperature sensor
Adafruit_MLX90614 mlx = Adafruit_MLX90614();

// MAX30100: Pulse oximeter (HR + SpO2)
PulseOximeter pox;

// Live data
float bpm = 0.0, spo2 = 0.0;
float temp_obj = 0.0, temp_amb = 0.0;

// WiFi credentials
const char* ssid = "realme 9 Pro+";
const char* password = "69696969";

// MQTT (Azure)
const char* mqttServer = "MyProjectHub22.azure-devices.net";
const int mqttPort = 8883;
const char* deviceId = "1195";

// ⚠️ REPLACE WITH NEW TOKEN
const char* sasToken =
"SharedAccessSignature sr=MyProjectHub22.azure-devices.net%2Fdevices%2F1195&sig=REPLACE_ME&se=REPLACE_ME";

const char* mqttTopic = "devices/1195/messages/events/";

WiFiClientSecure espClient;
PubSubClient mqttClient(espClient);
WiFiClient thingSpeakClient;

// ThingSpeak Config
unsigned long myChannelNumber = 2669052;
const char* myWriteAPIKey = "X472RER1LA9F60YU";

// Timers
uint32_t lastUpload = 0;
#define REPORT_INTERVAL_MS 15000   // ThingSpeak minimum = 15 sec

// Beat detection callback
void onBeatDetected() {
  Serial.println("Beat detected.");
}

// MQTT reconnect logic
void reconnectMQTT() {
  while (!mqttClient.connected()) {
    Serial.println("Reconnecting to Azure IoT Hub...");
    String username = String(mqttServer) + "/" + deviceId + "/?api-version=2021-04-12";

    if (mqttClient.connect(deviceId, username.c_str(), sasToken)) {
      Serial.println("Reconnected to Azure.");
    } else {
      Serial.print("Retry failed, rc=");
      Serial.println(mqttClient.state());
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  // I2C (NodeMCU pins)
  Wire.begin(D2, D1); // SDA = D2, SCL = D1

  // MLX90614 initialization
  Serial.print("Initializing temperature sensor... ");
  if (!mlx.begin()) {
    Serial.println("Failed to detect MLX90614.");
    while (1);
  }
  Serial.println("Success.");

  // MAX30100 initialization
  Serial.print("Initializing MAX30100... ");
  if (!pox.begin()) {
    Serial.println("Failed. Check wiring.");
    while (1);
  }
  Serial.println("Success.");
  pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
  pox.setOnBeatDetectedCallback(onBeatDetected);

  // WiFi connection
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected. IP: " + WiFi.localIP().toString());

  // Azure MQTT TLS (dev mode)
  espClient.setInsecure();  // Replace with CA cert for production
  mqttClient.setServer(mqttServer, mqttPort);

  reconnectMQTT();

  // ThingSpeak initialization
  ThingSpeak.begin(thingSpeakClient);
}

void loop() {
  // Maintain MQTT connection
  if (!mqttClient.connected()) {
    reconnectMQTT();
  }
  mqttClient.loop();

  // Update MAX30100
  pox.update();

  // Read sensors
  bpm = pox.getHeartRate();
  spo2 = pox.getSpO2();
  temp_obj = mlx.readObjectTempC();
  temp_amb = mlx.readAmbientTempC();

  // Sanity filtering
  if (bpm < 30 || bpm > 200) bpm = 0;
  if (spo2 < 70 || spo2 > 100) spo2 = 0;

  // Display data
  Serial.println("------ Live Data ------");
  Serial.printf("BPM: %.1f | SpO2: %.1f%%\n", bpm, spo2);
  Serial.printf("Object Temp: %.2f°C | Ambient Temp: %.2f°C\n", temp_obj, temp_amb);
  Serial.println("------------------------");

  // Upload every 15 seconds
  if (millis() - lastUpload > REPORT_INTERVAL_MS) {

    // Azure payload
    String payload = String("{\"BPM\":") + bpm +
                     ",\"SpO2\":" + spo2 +
                     ",\"ObjectTemp\":" + temp_obj +
                     ",\"AmbientTemp\":" + temp_amb + "}";

    // Send to Azure
    if (mqttClient.publish(mqttTopic, payload.c_str())) {
      Serial.println("Data sent to Azure IoT Hub.");
    } else {
      Serial.println("Failed to send data to Azure.");
    }

    // Send to ThingSpeak
    ThingSpeak.setField(1, bpm);
    ThingSpeak.setField(2, spo2);
    ThingSpeak.setField(3, temp_obj);
    ThingSpeak.setField(4, temp_amb);

    int status = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
    if (status == 200) {
      Serial.println("Data sent to ThingSpeak.");
    } else {
      Serial.println("ThingSpeak update failed. HTTP code: " + String(status));
    }

    lastUpload = millis();
  }

  delay(100);
}
