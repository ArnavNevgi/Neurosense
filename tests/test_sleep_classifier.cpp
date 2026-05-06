// Host-readable test vectors for the heuristic sleep classifier.
//
// Optional standalone check:
//   g++ -std=c++11 -DSLEEP_CLASSIFIER_STANDALONE_TEST tests/test_sleep_classifier.cpp -o sleep_classifier_test
//
// These vectors are not wired into the current PlatformIO firmware build.

#include "../include/sleep_classifier.h"

#if defined(SLEEP_CLASSIFIER_STANDALONE_TEST)
#include <assert.h>

static SleepSample makeSample(float hr, bool hrValid, float accel, bool motionValid, unsigned long t) {
  SleepSample sample;
  sample.heartRate = hr;
  sample.heartRateValid = hrValid;
  sample.accelMagnitude = accel;
  sample.motionValid = motionValid;
  sample.timestampMs = t;
  return sample;
}

static void fillWindow(SleepSample* samples, float hr, bool hrValid, const float* accelValues, size_t count) {
  for (size_t i = 0; i < count; i++) {
    samples[i] = makeSample(hr, hrValid, accelValues[i], true, (unsigned long)i * 2000UL);
  }
}

int main() {
  SleepClassifierConfig config = defaultSleepClassifierConfig();
  SleepSample samples[16];

  const float lightAccel[16] = {
    1.00, 1.00, 1.07, 1.00,
    1.00, 0.93, 1.00, 1.00,
    1.07, 1.00, 1.00, 0.93,
    1.00, 1.00, 1.00, 1.00
  };
  fillWindow(samples, 72.0, true, lightAccel, 16);
  assert(classifySleepWindow(samples, 16, config).state == SLEEP_STATE_LIGHT);

  const float deepAccel[16] = {
    1.000, 1.004, 0.996, 1.002,
    1.000, 0.998, 1.003, 1.000,
    0.997, 1.002, 1.000, 0.999,
    1.001, 1.000, 0.998, 1.002
  };
  fillWindow(samples, 55.0, true, deepAccel, 16);
  assert(classifySleepWindow(samples, 16, config).state == SLEEP_STATE_DEEP);

  const float awakeAccel[16] = {
    1.00, 1.45, 0.55, 1.38,
    0.62, 1.50, 0.58, 1.42,
    0.60, 1.35, 0.64, 1.40,
    0.56, 1.48, 0.61, 1.44
  };
  fillWindow(samples, 88.0, true, awakeAccel, 16);
  assert(classifySleepWindow(samples, 16, config).state == SLEEP_STATE_AWAKE);

  fillWindow(samples, 0.0, false, lightAccel, 16);
  assert(classifySleepWindow(samples, 16, config).state == SLEEP_STATE_UNKNOWN);

  fillWindow(samples, 60.0, true, lightAccel, 16);
  assert(classifySleepWindow(samples, 16, config).state == SLEEP_STATE_LIGHT);

  return 0;
}
#endif
