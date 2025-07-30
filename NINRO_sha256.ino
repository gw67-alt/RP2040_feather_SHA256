// RMS Peak Detector with SHA-256 Hashing
// Detects a new maximum RMS value and hashes it with a nonce from sweep.

#include "sha256.h"

// *** FIX: Create an instance with a different name than the class ***
Sha256 sha256;

#define NUM_CHANNELS 1     // Focused on a single channel for this logic
#define SAMPLE_RATE_MS 20  // 50Hz sampling
#define HISTORY_SIZE 10    // Samples for RMS calculation

int analogPins[NUM_CHANNELS] = {A0};
float channelHistory[HISTORY_SIZE];
int historyIndex = 0;
unsigned long lastSample = 0;

unsigned long cost = 10000;
unsigned long STATE_SPACE = 99999;

// State variables for max detection and hashing
float maxRmsEver = 0.0;    // Stores the all-time highest RMS value
long nonceCounter = 0; 
long QuantumVolume = 0;     // Increments with each sample
unsigned long hashesFound = 0;

// Sweep variables for nonce generation
unsigned long sweepValue = 0;
int sweepDirection = 1;
int sweepSpeed = 2;        // Base sweep speed
float rmsThreshold = 2.0;  // RMS threshold for high activity

unsigned long lastSpeedupPrint = 0;  // Timer for speedup display
#define SPEEDUP_INTERVAL 5000  // Print speedup every 5 seconds
#define DIFFICULTY 1

void setup() {
  Serial.begin(115200);

  // Initialize history buffer
  for (int i = 0; i < HISTORY_SIZE; i++) {
    channelHistory[i] = 0.0;
  }

  Serial.println("RMS Peak Detector with Sweep-Based Nonce Generation");
  Serial.println("High RMS values trigger sweep-based nonce mining...");
  Serial.println("Format: Nonce | Quantum Volume | Speedup | Credits");
  Serial.println("================================================");
}

bool checkDifficulty(uint8_t* hash) {
  for (int i = 0; i < DIFFICULTY; i++) {
    if (hash[i] != 0x00) {
      return false;
    }
  }
  return true;
}

// Function to calculate and display speedup metrics


void loop() {
  for (int i = 0; i < STATE_SPACE; i++) {
    unsigned long currentTime = millis();
    
    // Print speedup metrics periodically
    if (currentTime - lastSpeedupPrint >= SPEEDUP_INTERVAL) {
      lastSpeedupPrint = currentTime;
    }

    if (currentTime - lastSample >= SAMPLE_RATE_MS) {
      lastSample = currentTime;

      // 1. Sample the channel and calculate RMS
      int rawValue = analogRead(A0);
      channelHistory[historyIndex] = (rawValue / 1023.0) * 5.0;
      historyIndex = (historyIndex + 1) % HISTORY_SIZE;
      
      float currentRms = getRMSValue();

      // 2. Update sweep continuously
      // When RMS is high, increase sweep speed for faster nonce generation
      int dynamicSweepSpeed = sweepSpeed;
      if (currentRms > rmsThreshold) {
        dynamicSweepSpeed = sweepSpeed * (int)(currentRms / rmsThreshold); // Speed up based on RMS level
      }
      
      sweepValue += sweepDirection * dynamicSweepSpeed;
      
      // Check boundaries and reverse direction
      if (sweepValue >= 255) {
        sweepValue = 255;
        sweepDirection = -1;
      }
      else if (sweepValue <= 0) {
        sweepValue = 0;
        sweepDirection = 1;
      }

      // 3. Check for high RMS and use sweep value as nonce
      if (currentRms > maxRmsEver && cost > 0) {
        QuantumVolume++;
        
        // Use sweep value as nonce instead of nonceCounter
        long sweepBasedNonce = sweepValue + (nonceCounter * 256); // Combine sweep with counter for uniqueness
        nonceCounter++;

        // Prepare data for hashing using sweep-based nonce
        char dataToHash[64];
        snprintf(dataToHash, sizeof(dataToHash), "GeorgeW%ld", sweepBasedNonce);

        // Perform the SHA-256 hash using the renamed object
        sha256.init();
        sha256.print(dataToHash);
        uint8_t* hashResult = sha256.result();
        
        // Check if hash meets difficulty requirement
        if (checkDifficulty(hashResult)) {
          Serial.println(" === WINNING HASH FOUND === ");
          Serial.print("    Hash: ");
          printHash(hashResult);
          Serial.print("    Data: ");
          Serial.println(dataToHash);
          Serial.print("    RMS: ");
          Serial.print(currentRms, 3);
          Serial.print(" | SHA256 Nonce: ");
          Serial.print(sweepBasedNonce);
          Serial.print(" | Quantum Nonce: ");
          Serial.println(QuantumVolume);
          
          hashesFound++;
          
          cost += 150;
        }
        
        if (!checkDifficulty(hashResult)) {
          cost -= 1;
          
          // Show compact progress with sweep info
          if (nonceCounter % 100 == 0) {
            float currentSpeedup = (QuantumVolume > 0) ? (float)nonceCounter / (float)QuantumVolume : 0;
            Serial.print("N:");
            Serial.print(nonceCounter);
            Serial.print(" QV:");
            Serial.print(QuantumVolume);
            Serial.print(" RMS:");
            Serial.print(currentRms, 2);
            Serial.print(" SW:");
            Serial.print(sweepValue);
            Serial.print(" SP:");
            Serial.print(currentSpeedup, 1);
            Serial.print("x C:");
            Serial.println(cost);
          }
        }
        
        // Update the max RMS
        maxRmsEver = currentRms;
        if (currentRms == maxRmsEver) {
          maxRmsEver /= 10; // Decay factor to allow new peaks
        }
      }
    }
  }
}

/**
 * Calculates the Root Mean Square of the values in the history buffer.
 */
float getRMSValue() {
  float sum = 0;
  for (int i = 0; i < HISTORY_SIZE; i++) {
    sum += channelHistory[i] * channelHistory[i];
  }
  return sqrt(sum / HISTORY_SIZE);
}

/**
 * Helper function to print the 32-byte hash in hexadecimal format.
 */
void printHash(uint8_t* hash) {
  for (int i = 0; i < 32; i++) {
    if (hash[i] < 0x10) {
      Serial.print('0');
    }
    Serial.print(hash[i], HEX);
  }
  Serial.println();
}
