#include <Wire.h>
#include <Adafruit_MLX90614.h>
#include "MAX30100_PulseOximeter.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ThingSpeak.h>

// --- Sensor Objects ---
Adafruit_MLX90614 mlx = Adafruit_MLX90614();          // Temperature sensor
PulseOximeter pox;                                     // Pulse Oximeter

// --- WiFi Configuration ---
const char* ssid = "realme 9 Pro+";
const char* password = "69696969";
WiFiClientSecure espClient;
WiFiClient thingSpeakClient;

// --- Azure IoT Hub Configuration ---
const char* mqttServer = "MyProjectHub22.azure-devices.net";
const int mqttPort = 8883;
const char* deviceId = "1195";
const char* sasToken = "SharedAccessSignature sr=MyProjectHub22.azure-devices.net%2Fdevices%2F1195&sig=pBIxloNUqwo8Ia8uF50YTw652cKbHLXooQkDXL3qJvc%3D&se=1732436672";
const char* mqttTopic = "devices/1195/messages/events/";
PubSubClient client(espClient);

// --- ThingSpeak Configuration ---
unsigned long myChannelNumber = 2669052;
const char* myWriteAPIKey = "O0FHJWE26VR75S83";

// --- Timers ---
uint32_t lastReportTime = 0;
#define REPORTING_PERIOD_MS 5000

// --- Live Data Variables ---
float bpm = 0.0, spo2 = 0.0;
float temp_obj = 0.0, temp_amb = 0.0;

// --- Beat Detection Callback ---
void onBeatDetected() {
  Serial.println("❤️ Beat detected!");
}

void setup() {
  Serial.begin(115200);
  Wire.begin(D2, D1);  // SDA = D2 (GPIO4), SCL = D1 (GPIO5)

  // --- Initialize Temperature Sensor ---
  Serial.print("Initializing MLX90614... ");
  if (!mlx.begin()) {
    Serial.println("Failed! Check connections.");
    while (1);
  }
  Serial.println("Success.");

  // --- Initialize Pulse Oximeter ---
  Serial.print("Initializing MAX30100... ");
  if (!pox.begin()) {
    Serial.println("Failed! Check wiring.");
    while (1);
  }
  Serial.println("Success.");
  pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
  pox.setOnBeatDetectedCallback(onBeatDetected);

  // --- Connect to WiFi ---
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected. IP: " + WiFi.localIP().toString());

  // --- Secure MQTT (TLS) ---
  espClient.setInsecure(); // For development only

  // --- Connect to Azure IoT Hub ---
  client.setServer(mqttServer, mqttPort);
  String username = String(mqttServer) + "/" + deviceId + "/?api-version=2021-04-12";
  Serial.print("Connecting to Azure IoT Hub... ");
  if (client.connect(deviceId, username.c_str(), sasToken)) {
    Serial.println("Success!");
  } else {
    Serial.print("Failed. State: ");
    Serial.println(client.state());
    while (1);
  }

  // --- Initialize ThingSpeak ---
  ThingSpeak.begin(thingSpeakClient);
}

void loop() {
  pox.update();  // Update pulse oximeter readings

  // --- Read Sensor Data ---
  bpm = pox.getHeartRate();
  spo2 = pox.getSpO2();
  temp_obj = mlx.readObjectTempC();
  temp_amb = mlx.readAmbientTempC();

  // --- Print to Serial Monitor ---
  Serial.println("=== LIVE DATA ===");
  Serial.printf("💓 BPM: %.1f | SpO2: %.1f%%\n", bpm, spo2);
  Serial.printf("🌡️ Body Temp: %.2f°C | Ambient Temp: %.2f°C\n", temp_obj, temp_amb);
  Serial.println("==================");

  // --- Every 5s: Upload to Cloud ---
  if (millis() - lastReportTime > REPORTING_PERIOD_MS) {
    // JSON Payload for Azure
    String payload = String("{\"BPM\":") + bpm +
                     ",\"SpO2\":" + spo2 +
                     ",\"ObjectTemp\":" + temp_obj +
                     ",\"AmbientTemp\":" + temp_amb + "}";

    if (client.publish(mqttTopic, payload.c_str())) {
      Serial.println("📤 Sent to Azure IoT Hub.");
    } else {
      Serial.println("❌ Failed to send to Azure.");
    }

    // ThingSpeak Update
    ThingSpeak.setField(1, bpm);
    ThingSpeak.setField(2, spo2);
    ThingSpeak.setField(3, temp_obj);
    ThingSpeak.setField(4, temp_amb);

    int status = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
    if (status == 200) {
      Serial.println("✅ Data sent to ThingSpeak.");
    } else {
      Serial.println("❌ ThingSpeak update failed. Code: " + String(status));
    }

    lastReportTime = millis();
  }

  client.loop();  // Maintain MQTT connection
}
