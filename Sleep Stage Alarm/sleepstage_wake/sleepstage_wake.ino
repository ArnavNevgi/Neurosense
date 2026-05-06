#include <Wire.h>
#include <math.h>
#include <MPU6050_light.h>
#include "MAX30100_PulseOximeter.h"
#include "BluetoothSerial.h"
#include "../../include/sleep_classifier.h"

BluetoothSerial SerialBT;

// MPU6050 motion sensor
MPU6050 mpu(Wire);
unsigned long lastMotionCheck = 0;
float motionMagnitude = 0;

// MAX30100 heart rate sensor
PulseOximeter pox;
float heartRate = 0;
float SpO2 = 0;
bool poxReady = false;
bool mpuReady = false;
bool heartRateValid = false;
bool spo2Valid = false;

// Haptic Alarm
#define VIBRATION_PIN 25

// Sleep detection flags
bool alarmTriggered = false;
unsigned long sleepStartTime;
const unsigned long minSleepDuration = 60000; // 1 minute for demo
const unsigned long SAMPLE_INTERVAL_MS = 2000;
const unsigned long SLEEP_WINDOW_MS = 30000;
const size_t SLEEP_SAMPLE_BUFFER_SIZE = (SLEEP_WINDOW_MS / SAMPLE_INTERVAL_MS) + 1;

SleepClassifierConfig sleepConfig = defaultSleepClassifierConfig();
SleepSample sleepSamples[SLEEP_SAMPLE_BUFFER_SIZE];
SleepClassifierResult classifierResult = defaultSleepClassifierResult();
size_t sleepSampleCount = 0;
size_t sleepSampleWriteIndex = 0;

enum DeviceStatus {
  STATUS_OK = 0,
  STATUS_WIFI_OFFLINE = 1,
  STATUS_MQTT_OFFLINE = 2,
  STATUS_SENSOR_ERROR = 3
};

// Callback on heart beat
void onBeatDetected() {
  Serial.println("Beat detected");
}

void vibrate();

bool validRange(float value, float minValue, float maxValue) {
  return isfinite(value) && value >= minValue && value <= maxValue;
}

int boolToInt(bool value) {
  return value ? 1 : 0;
}

void logStatus(DeviceStatus status, const char* message) {
  Serial.print("[STATUS ");
  Serial.print((int)status);
  Serial.print("] ");
  Serial.println(message);
  SerialBT.print("[STATUS ");
  SerialBT.print((int)status);
  SerialBT.print("] ");
  SerialBT.println(message);
}

void addSleepSample(const SleepSample& sample) {
  sleepSamples[sleepSampleWriteIndex] = sample;
  sleepSampleWriteIndex = (sleepSampleWriteIndex + 1) % SLEEP_SAMPLE_BUFFER_SIZE;

  if (sleepSampleCount < SLEEP_SAMPLE_BUFFER_SIZE) {
    sleepSampleCount++;
  }
}

size_t copySleepWindow(SleepSample* output, size_t maxCount, unsigned long now) {
  size_t copied = 0;
  size_t available = sleepSampleCount < SLEEP_SAMPLE_BUFFER_SIZE ? sleepSampleCount : SLEEP_SAMPLE_BUFFER_SIZE;

  for (size_t i = 0; i < available && copied < maxCount; i++) {
    size_t index = (sleepSampleWriteIndex + SLEEP_SAMPLE_BUFFER_SIZE - available + i) % SLEEP_SAMPLE_BUFFER_SIZE;
    SleepSample sample = sleepSamples[index];

    if (now - sample.timestampMs <= sleepConfig.windowMs) {
      output[copied++] = sample;
    }
  }

  return copied;
}

void setup() {
  Serial.begin(115200);
  SerialBT.begin("NeuroSleepESP32");
  Wire.begin();
  sleepConfig = defaultSleepClassifierConfig();
  sleepConfig.windowMs = SLEEP_WINDOW_MS;

  pinMode(VIBRATION_PIN, OUTPUT);
  digitalWrite(VIBRATION_PIN, LOW);

  // Init MPU6050
  Serial.println("Initializing MPU6050...");
  byte mpuStatus = mpu.begin();
  mpuReady = (mpuStatus == 0);
  if (mpuReady) {
    delay(1000); // Allow the sensor to settle before the existing offset calibration.
    mpu.calcOffsets();
    Serial.println("MPU6050 ready.");
    logStatus(STATUS_OK, "MPU6050 ready.");
  } else {
    Serial.print("MPU6050 init failed, status=");
    Serial.println(mpuStatus);
    logStatus(STATUS_SENSOR_ERROR, "MPU6050 unavailable; continuing without motion reads.");
  }

  // Init MAX30100
  Serial.print("Initializing MAX30100... ");
  poxReady = pox.begin();
  if (!poxReady) {
    Serial.println("FAILED. Check wiring.");
    logStatus(STATUS_SENSOR_ERROR, "MAX30100 unavailable; continuing without HR/SpO2 reads.");
  } else {
    Serial.println("SUCCESS.");
    logStatus(STATUS_OK, "MAX30100 ready.");
    pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
    pox.setOnBeatDetectedCallback(onBeatDetected);
  }

  sleepStartTime = millis(); // start of simulated sleep
}

void loop() {
  if (poxReady) {
    pox.update();
  }

  if (mpuReady) {
    mpu.update();
  }

  unsigned long now = millis();

  // Sample motion every 2s
  if (now - lastMotionCheck > SAMPLE_INTERVAL_MS) {
    lastMotionCheck = now;

    if (mpuReady) {
      motionMagnitude = sqrt(mpu.getAccX() * mpu.getAccX() +
                             mpu.getAccY() * mpu.getAccY() +
                             mpu.getAccZ() * mpu.getAccZ());
    } else {
      motionMagnitude = 0;
    }

    if (poxReady) {
      heartRate = pox.getHeartRate();
      SpO2 = pox.getSpO2();
    } else {
      heartRate = 0;
      SpO2 = 0;
    }

    heartRateValid = poxReady && validRange(heartRate, sleepConfig.minValidHr, sleepConfig.maxValidHr);
    spo2Valid = poxReady && validRange(SpO2, 70.0, 100.0);

    SleepSample sample;
    sample.heartRate = heartRate;
    sample.heartRateValid = heartRateValid;
    sample.accelMagnitude = motionMagnitude;
    sample.motionValid = mpuReady && isfinite(motionMagnitude);
    sample.timestampMs = now;
    addSleepSample(sample);

    SleepSample windowSamples[SLEEP_SAMPLE_BUFFER_SIZE];
    size_t windowSampleCount = copySleepWindow(windowSamples, SLEEP_SAMPLE_BUFFER_SIZE, now);
    classifierResult = classifySleepWindow(windowSamples, windowSampleCount, sleepConfig);

    Serial.printf("Motion: %.2f valid=%d | HR: %.1f bpm valid=%d | SpO2: %.1f%% valid=%d\n",
                  motionMagnitude, boolToInt(sample.motionValid),
                  heartRate, boolToInt(heartRateValid),
                  SpO2, boolToInt(spo2Valid));
    Serial.printf("State: %s | avgHR: %.1f | HR trend: %.1f | accelVar: %.4f | movements: %u | enough=%d | valid=%d\n",
                  sleepStateToText(classifierResult.state),
                  classifierResult.avgHeartRate,
                  classifierResult.heartRateTrend,
                  classifierResult.accelVariance,
                  classifierResult.movementCount,
                  boolToInt(classifierResult.enoughSamples),
                  boolToInt(classifierResult.valid));
    SerialBT.printf("Motion: %.2f valid=%d | HR: %.1f bpm valid=%d | SpO2: %.1f%% valid=%d\n",
                    motionMagnitude, boolToInt(sample.motionValid),
                    heartRate, boolToInt(heartRateValid),
                    SpO2, boolToInt(spo2Valid));
    SerialBT.printf("State: %s | avgHR: %.1f | HR trend: %.1f | accelVar: %.4f | movements: %u | enough=%d | valid=%d\n",
                    sleepStateToText(classifierResult.state),
                    classifierResult.avgHeartRate,
                    classifierResult.heartRateTrend,
                    classifierResult.accelVariance,
                    classifierResult.movementCount,
                    boolToInt(classifierResult.enoughSamples),
                    boolToInt(classifierResult.valid));

    if (!mpuReady || !poxReady) {
      logStatus(STATUS_SENSOR_ERROR, "Classifier remains UNKNOWN until required sensors provide valid samples.");
    }

    // --- Sleep Stage Logic ---
    bool inLightSleep =
      classifierResult.valid &&
      classifierResult.enoughSamples &&
      classifierResult.state == SLEEP_STATE_LIGHT;

    // Trigger smart alarm during light sleep, after min duration
    if (!alarmTriggered && inLightSleep && (now - sleepStartTime > minSleepDuration)) {
      Serial.println("Light sleep detected. Triggering vibration alarm...");
      SerialBT.println("Light sleep detected. Triggering vibration alarm...");
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
