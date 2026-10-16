#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_DRV2605.h>

// System config
#define AUDIO_IN_PIN 14          // Bone conducting microphone input pin
#define SAMPLE_RATE 8000         // Sampling freq in Hz
#define BUFFER_SIZE 256          // Number of samples per frame
#define SAMPLE_PERIOD_US (1000000 / SAMPLE_RATE) // 125 microseconds per sample

// YIN algo parameters/pitch config
#define PITCH_THRESHOLD_HZ 175.0f // Trigger threshold for pitch
#define YIN_THRESHOLD 0.15        // Difference threshold
#define MAX_LAG (BUFFER_SIZE / 2) // Max lag to search
#define MIN_LAG 16               // Min lag to search

// Noise Gate & Range Limits
#define MIN_PITCH_HZ 120.0f
#define MAX_PITCH_HZ 300.0f
#define MIN_RMS_AMPLITUDE 15.0f  // Adjust based on quiet baseline ADC noise

// Pins for the SparkFun Thing Plus Qwiic connector
#define QWIIC_SDA 8
#define QWIIC_SCL 9

// Haptic Feedback Waveform
#define WAVEFORM 47

// 3-sample median filter
float pitchHistory[3] = {0, 0, 0};
uint8_t historyIdx = 0;

// Buffers
int16_t rawBuffer[BUFFER_SIZE];
float signalBuffer[BUFFER_SIZE]; 
float yinBuffer[MAX_LAG]; 



// Cooldown for haptic feedback
unsigned long lastTriggerTime = 0;
const unsigned long HAPTIC_COOLDOWN_MS = 400; 

// Function declarations
void captureAudioBuffer();
void removeDCOffset();
float getPitchYIN();
void hapticFeedback();
float getMedianPitch(float newPitch);
void clearPitchHistory();
float calculateRMS();
void processVocalPitch();

Adafruit_DRV2605 drv;

void setup() {
  Serial.begin(115200);
  
  // Wait up to 2 seconds for Serial Monitor to connect
  unsigned long start = millis();
  while (!Serial && (millis() - start < 2000));

  // Initialize Wire on SparkFun Thing Plus Qwiic pins
  Wire.begin(QWIIC_SDA, QWIIC_SCL);

  if (!drv.begin(&Wire)) {
    Serial.println("Could not find DRV2605L driver! Check Qwiic connection.");
  } else {
    Serial.println("DRV2605L Haptic Driver initialized successfully.");
    drv.selectLibrary(1);              // Standard ERM library
    drv.setMode(DRV2605_MODE_INTTRIG);  // Internal trigger mode
  }

  // 12-bit ADC resolution
  analogReadResolution(12);
  pinMode(AUDIO_IN_PIN, INPUT);
}

void loop() {
  captureAudioBuffer(); 
  removeDCOffset();     
  processVocalPitch();  
  delay(10); // Reduced delay to keep pitch tracking responsive
}

void clearPitchHistory() {
  pitchHistory[0] = 0.0f;
  pitchHistory[1] = 0.0f;
  pitchHistory[2] = 0.0f;
  historyIdx = 0;
}

void hapticFeedback() {
  unsigned long now = millis();
  
  if (now - lastTriggerTime >= HAPTIC_COOLDOWN_MS) {
    Serial.println(">>> PITCH BELOW THRESHOLD: TRIGGERING HAPTIC <<<");
    
    drv.setWaveform(0, WAVEFORM); // Strong sharp buzz
    drv.setWaveform(1, 0);  
    drv.go();               

    lastTriggerTime = now;

    // Flush the median history and raw audio so the physical motor buzz
    // doesn't acoustically bleed into the next frame
    clearPitchHistory();
    memset(rawBuffer, 0, sizeof(rawBuffer));
    memset(signalBuffer, 0, sizeof(signalBuffer));
  }
}

void captureAudioBuffer() {
  unsigned long nextSampleTime = micros();

  for (int i = 0; i < BUFFER_SIZE; i++) {
    while (micros() < nextSampleTime) {
      // Precise sample timing wait loop
    }
    rawBuffer[i] = analogRead(AUDIO_IN_PIN);
    nextSampleTime += SAMPLE_PERIOD_US;
  }
}

void removeDCOffset() {
  long sum = 0;
  for (int i = 0; i < BUFFER_SIZE; i++) {
    sum += rawBuffer[i];
  }
  float dcBias = (float)sum / BUFFER_SIZE;

  for (int i = 0; i < BUFFER_SIZE; i++) {
    signalBuffer[i] = ((float)rawBuffer[i] - dcBias) * 2.5f;
  }
}

float getPitchYIN() {
  yinBuffer[0] = 1.0;
  float runningSum = 0.0;

  for (int tau = 1; tau < MAX_LAG; tau++) {
    float diffSum = 0.0;
    for (int i = 0; i < MAX_LAG; i++) {
      float delta = signalBuffer[i] - signalBuffer[i + tau];
      diffSum += delta * delta;
    }
    
    runningSum += diffSum;
    
    if (runningSum > 0) {
      yinBuffer[tau] = diffSum * tau / runningSum;
    } else {
      yinBuffer[tau] = 1.0;
    }
  }

  int tauFound = -1;
  for (int tau = MIN_LAG; tau < MAX_LAG; tau++) {
    if (yinBuffer[tau] < YIN_THRESHOLD) {
      while (tau + 1 < MAX_LAG && yinBuffer[tau + 1] < yinBuffer[tau]) {
        tau++;
      }
      tauFound = tau;
      break;
    }
  }

  if (tauFound == -1) {
    return -1.0;
  }

  float betterTau = tauFound;
  int x0 = tauFound - 1;
  int x2 = tauFound + 1;

  if (x0 >= 0 && x2 < MAX_LAG) {
    float s0 = yinBuffer[x0];
    float s1 = yinBuffer[tauFound];
    float s2 = yinBuffer[x2];
    
    float denominator = (2.0 * s1 - s2 - s0);
    if (denominator != 0.0) {
      betterTau = tauFound + (s2 - s0) / (2.0 * denominator);
    }
  }

  return (float)SAMPLE_RATE / betterTau;
}

float getMedianPitch(float newPitch) {
  // If the filter was just flushed, seed all slots with the new pitch
  if (pitchHistory[0] == 0.0f) {
    pitchHistory[0] = newPitch;
    pitchHistory[1] = newPitch;
    pitchHistory[2] = newPitch;
    return newPitch;
  }

  pitchHistory[historyIdx] = newPitch;
  historyIdx = (historyIdx + 1) % 3;

  float a = pitchHistory[0];
  float b = pitchHistory[1];
  float c = pitchHistory[2];

  if ((a <= b && b <= c) || (c <= b && b <= a)) return b;
  if ((b <= a && a <= c) || (c <= a && a <= b)) return a;
  return c;
}

float calculateRMS() {
  float sum = 0;
  for (int i = 0; i < BUFFER_SIZE; i++) {
    sum += signalBuffer[i] * signalBuffer[i];
  }
  return sqrt(sum / BUFFER_SIZE);
}

void processVocalPitch() {
  float rms = calculateRMS();

  // 1. Noise Gate Check
  if (rms < MIN_RMS_AMPLITUDE) {
    // Silence detected: flush previous history so old pitches can't carry over
    clearPitchHistory();
    return;  
  }

  float rawPitch = getPitchYIN();

  // 2. Frequency Range Check
  if (rawPitch >= MIN_PITCH_HZ && rawPitch <= MAX_PITCH_HZ) {
    
    // 3. Smooth out harmonic glitches
    float filteredPitch = getMedianPitch(rawPitch);

    Serial.print("RMS: ");
    Serial.print(rms, 1);
    Serial.print(" | Pitch: ");
    Serial.print(filteredPitch, 1);
    Serial.println(" Hz");

    // 4. Trigger Haptic Alert if fundamental is below threshold
    if (filteredPitch < PITCH_THRESHOLD_HZ) {
      hapticFeedback();
    }
  }
}