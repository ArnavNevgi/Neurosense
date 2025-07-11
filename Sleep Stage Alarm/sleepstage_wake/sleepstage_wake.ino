#include <Wire.h>
#include <MPU6050_light.h>
#include "MAX30100_PulseOximeter.h"
#include "BluetoothSerial.h"

BluetoothSerial SerialBT;

// MPU6050 motion sensor
MPU6050 mpu(Wire);
unsigned long lastMotionCheck = 0;
float motionMagnitude = 0;

// MAX30100 heart rate sensor
PulseOximeter pox;
float heartRate = 0;
float SpO2 = 0;

// Haptic Alarm
#define VIBRATION_PIN 25

// Sleep detection flags
bool alarmTriggered = false;
unsigned long sleepStartTime;
const unsigned long minSleepDuration = 60000; // 1 minute for demo

// Callback on heart beat
void onBeatDetected() {
  Serial.println("❤️ Beat detected");
}

void setup() {
  Serial.begin(115200);
  SerialBT.begin("NeuroSleepESP32");
  Wire.begin();

  pinMode(VIBRATION_PIN, OUTPUT);
  digitalWrite(VIBRATION_PIN, LOW);

  // Init MPU6050
  Serial.println("Initializing MPU6050...");
  mpu.begin();
  delay(1000);
  mpu.calcOffsets();
  Serial.println("MPU6050 ready.");

  // Init MAX30100
  Serial.print("Initializing MAX30100... ");
  if (!pox.begin()) {
    Serial.println("FAILED. Check wiring.");
    while (1);
  } else {
    Serial.println("SUCCESS.");
  }
  pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
  pox.setOnBeatDetectedCallback(onBeatDetected);

  sleepStartTime = millis(); // start of simulated sleep
}

void loop() {
  pox.update();
  mpu.update();

  unsigned long now = millis();

  // Sample motion every 2s
  if (now - lastMotionCheck > 2000) {
    lastMotionCheck = now;
    motionMagnitude = sqrt(mpu.getAccX() * mpu.getAccX() +
                           mpu.getAccY() * mpu.getAccY() +
                           mpu.getAccZ() * mpu.getAccZ());

    heartRate = pox.getHeartRate();
    SpO2 = pox.getSpO2();

    Serial.printf("Motion: %.2f | HR: %.1f bpm | SpO₂: %.1f%%\n", motionMagnitude, heartRate, SpO2);
    SerialBT.printf("Motion: %.2f | HR: %.1f bpm | SpO₂: %.1f%%\n", motionMagnitude, heartRate, SpO2);

    // --- Sleep Stage Logic ---
    bool inLightSleep = (motionMagnitude > 0.15 && heartRate > 60 && heartRate < 90);
    bool inDeepSleep = (motionMagnitude <= 0.15 && heartRate < 60);

    // Trigger smart alarm during light sleep, after min duration
    if (!alarmTriggered && inLightSleep && (now - sleepStartTime > minSleepDuration)) {
      Serial.println("💡 Light sleep detected. Triggering vibration alarm...");
      SerialBT.println("💡 Light sleep detected. Triggering vibration alarm...");
      vibrate();
      alarmTriggered = true;
    }

    // Optional: reset alarm after long time
    if (now - sleepStartTime > 5 * 60 * 1000) {
      alarmTriggered = false;
      sleepStartTime = now;
    }
  }

  delay(100); // minimal delay
}

void vibrate() {
  digitalWrite(VIBRATION_PIN, HIGH);
  delay(1500); // 1.5s vibration
  digitalWrite(VIBRATION_PIN, LOW);
}