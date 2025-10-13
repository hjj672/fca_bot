#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SHT31.h>

// =================================================================
// 1. GLOBAL CONFIGURATION
// =================================================================

// --- Humidifier Control (GPIO 14) ---
const int HUMIDIFIER_RELAY_PIN = 14;
const int HUMIDIFIER_ON_STATE = LOW; // Assumes Active-LOW.
const int HUMIDIFIER_OFF_STATE = !HUMIDIFIER_ON_STATE;
const float HUMIDITY_LOW_THRESHOLD  = 45.0; // Turn ON below this
const float HUMIDITY_HIGH_THRESHOLD = 50.0; // Turn OFF above this

// --- Random Cycle Relays (GPIO 25 & 26) ---
const int RELAY_RANDOM_1 = 25;
const int RELAY_RANDOM_2 = 26;
const int RANDOM_RELAY_ON_STATE = LOW; // Assumes Active-LOW.
const int RANDOM_RELAY_OFF_STATE = !RANDOM_RELAY_ON_STATE;

// --- SHT31 Sensor Configuration ---
constexpr uint8_t SDA_PIN = 21;
constexpr uint8_t SCL_PIN = 22;
Adafruit_SHT31 sht31_1 = Adafruit_SHT31();
Adafruit_SHT31 sht31_2 = Adafruit_SHT31();
constexpr uint8_t SHT31_ADDRESS_1 = 0x44;
constexpr uint8_t SHT31_ADDRESS_2 = 0x45;

// =================================================================
// 2. GLOBAL STATE VARIABLES
// =================================================================

// --- Timing ---
unsigned long lastSensorReadTime = 0;
unsigned long lastControlLogTime = 0;
const unsigned long SENSOR_READ_INTERVAL = 10000; // 10 seconds
const unsigned long CONTROL_LOG_INTERVAL = 60000;  // 1 minute

// --- Sensor Data ---
float hum1_sum = 0, hum2_sum = 0;
int readCount = 0;

// --- Humidifier State (GPIO 14) ---
bool isHumidifierOn = false;
unsigned long humidifierOnStartTime = 0;
unsigned long totalHumidifierOnTimeMillis = 0;

// --- Random Cycle State (GPIO 25 & 26) ---
enum RelayRandomState { STATE_OFF, STATE_ON };
RelayRandomState relayRandomState = STATE_OFF;
unsigned long relayRandomLastChangeTime = 0;
unsigned long currentRandomOnTime = 0;
unsigned long currentRandomOffTime = 0;

// =================================================================
// 3. HELPER FUNCTION DECLARATIONS
// =================================================================
void handleSensorReads(unsigned long currentTime);
void handleHumidityControl(unsigned long currentTime);
void handleRandomCycleRelays(unsigned long currentTime);

// =================================================================
// 4. SETUP FUNCTION
// =================================================================
void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  Serial.println("\n\n=== Combined Control System Initializing ===");

  // --- Initialize Humidifier Relay ---
  pinMode(HUMIDIFIER_RELAY_PIN, OUTPUT);
  digitalWrite(HUMIDIFIER_RELAY_PIN, HUMIDIFIER_OFF_STATE);
  Serial.println("✓ Humidifier relay (GPIO 14) initialized to OFF.");

  // --- Initialize Random Cycle Relays ---
  pinMode(RELAY_RANDOM_1, OUTPUT);
  pinMode(RELAY_RANDOM_2, OUTPUT);
  digitalWrite(RELAY_RANDOM_1, RANDOM_RELAY_OFF_STATE);
  digitalWrite(RELAY_RANDOM_2, RANDOM_RELAY_OFF_STATE);
  Serial.println("✓ Random cycle relays (GPIO 25, 26) initialized to OFF.");
  
  // --- Initialize Random Number Generator ---
  randomSeed(analogRead(0));
  currentRandomOffTime = random(2, 3) * 60 * 1000; // First OFF period: 5-7 mins
  Serial.printf("✓ Random cycle seeded. First OFF period: %lu ms\n", currentRandomOffTime);

  // --- Initialize SHT31 Sensors ---
  Wire.begin(SDA_PIN, SCL_PIN);
  if (!sht31_1.begin(SHT31_ADDRESS_1)) Serial.println("ERROR: SHT31 #1 not found!");
  else Serial.println("✓ SHT31 sensor #1 (0x44) initialized.");
  if (!sht31_2.begin(SHT31_ADDRESS_2)) Serial.println("ERROR: SHT31 #2 not found!");
  else Serial.println("✓ SHT31 sensor #2 (0x45) initialized.");

  Serial.println("\n=== Initialization Complete. Starting main loop. ===");
}

// =================================================================
// 5. MAIN LOOP (Orchestrator)
// =================================================================
void loop() {
  unsigned long currentTime = millis();
  
  // These functions run independently on every loop pass
  handleSensorReads(currentTime);
  handleHumidityControl(currentTime);
  handleRandomCycleRelays(currentTime);
}

// =================================================================
// 6. HELPER FUNCTION IMPLEMENTATIONS
// =================================================================

/**
 * @brief Reads SHT31 sensors every 10 seconds and accumulates humidity data.
 */
void handleSensorReads(unsigned long currentTime) {
  if (currentTime - lastSensorReadTime >= SENSOR_READ_INTERVAL) {
    lastSensorReadTime = currentTime;
    
    float h1 = sht31_1.readHumidity();
    if (!isnan(h1)) { hum1_sum += h1; }

    float h2 = sht31_2.readHumidity();
    if (!isnan(h2)) { hum2_sum += h2; }
    
    readCount++;
  }
}

/**
 * @brief Once per minute, calculates average humidity and controls the humidifier relay.
 */
void handleHumidityControl(unsigned long currentTime) {
  if (currentTime - lastControlLogTime >= CONTROL_LOG_INTERVAL) {
    lastControlLogTime = currentTime;

    if (readCount > 0) {
      // Calculate average humidity
      float avg_hum1 = hum1_sum / readCount;
      float avg_hum2 = hum2_sum / readCount;
      float averageHumidity = (avg_hum1 + avg_hum2) / 2.0f;

      // Apply Control Logic
      if (averageHumidity < HUMIDITY_LOW_THRESHOLD && !isHumidifierOn) {
        isHumidifierOn = true;
        digitalWrite(HUMIDIFIER_RELAY_PIN, HUMIDIFIER_ON_STATE);
        humidifierOnStartTime = currentTime;
        Serial.println("\n>>> HUMIDITY ACTION: Low humidity detected. Turning Humidifier ON.");
      } 
      else if (averageHumidity > HUMIDITY_HIGH_THRESHOLD && isHumidifierOn) {
        isHumidifierOn = false;
        digitalWrite(HUMIDIFIER_RELAY_PIN, HUMIDIFIER_OFF_STATE);
        totalHumidifierOnTimeMillis += (currentTime - humidifierOnStartTime);
        Serial.println("\n>>> HUMIDITY ACTION: High humidity reached. Turning Humidifier OFF.");
      }

      // Reporting
      float totalOnTimeMinutes = (float)totalHumidifierOnTimeMillis / 60000.0f;
      if (isHumidifierOn) {
          unsigned long currentSessionMillis = currentTime - humidifierOnStartTime;
          totalOnTimeMinutes = (float)(totalHumidifierOnTimeMillis + currentSessionMillis) / 60000.0f;
      }

      Serial.println("================ 1-MINUTE STATUS LOG ================");
      Serial.printf("Average Humidity: %.2f %% (S1: %.2f, S2: %.2f)\n", averageHumidity, avg_hum1, avg_hum2);
      Serial.printf("Humidifier (GPIO 14) Status: %s\n", isHumidifierOn ? "ON" : "OFF");
      Serial.printf("Humidifier Total ON Time: %.2f minutes\n", totalOnTimeMinutes);
      Serial.println("=====================================================\n");

      // Reset accumulators
      hum1_sum = 0;
      hum2_sum = 0;
      readCount = 0;
    }
  }
}

/**
 * @brief Manages relays on GPIO 25 & 26 with a non-blocking random timer.
 */
void handleRandomCycleRelays(unsigned long currentTime) {
  // Check if it's time to turn ON
  if (relayRandomState == STATE_OFF && (currentTime - relayRandomLastChangeTime >= currentRandomOffTime)) {
    relayRandomState = STATE_ON;
    relayRandomLastChangeTime = currentTime;
    digitalWrite(RELAY_RANDOM_1, RANDOM_RELAY_ON_STATE);
    digitalWrite(RELAY_RANDOM_2, RANDOM_RELAY_ON_STATE);
    currentRandomOnTime = random(30, 45) * 1000; // 60-90 seconds ON
    Serial.printf(">>> RANDOM CYCLE: Relays 25 & 26 are now ON for %lu ms\n", currentRandomOnTime);
  }
  // Check if it's time to turn OFF
  else if (relayRandomState == STATE_ON && (currentTime - relayRandomLastChangeTime >= currentRandomOnTime)) {
    relayRandomState = STATE_OFF;
    relayRandomLastChangeTime = currentTime;
    digitalWrite(RELAY_RANDOM_1, RANDOM_RELAY_OFF_STATE);
    digitalWrite(RELAY_RANDOM_2, RANDOM_RELAY_OFF_STATE);
    currentRandomOffTime = random(7, 9) * 60 * 1000; // 5-7 minutes OFF
    Serial.printf(">>> RANDOM CYCLE: Relays 25 & 26 are now OFF for %lu ms\n", currentRandomOffTime);
  }
}
