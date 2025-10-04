/**
 * CellBalancing.ino
 * 
 * Cell balancing example with PB7200P80
 * 
 * This example demonstrates:
 * - Manual cell balancing
 * - Automatic balancing
 * - Balancing process monitoring
 * 
 * Hardware:
 * - Arduino (any board)
 * - PB7200P80 connected via I2C
 */

#include <PB7200P80.h>

// Create PB7200P80 object
PB7200P80 bms(PB7200_INTERFACE_I2C, 0x55);

const uint8_t NUM_CELLS = 4;
const float BALANCE_THRESHOLD_MV = 50.0;  // Balance if difference > 50mV

// Operation mode
enum BalanceMode {
  MANUAL,
  AUTOMATIC
};

BalanceMode currentMode = AUTOMATIC;  // Change to MANUAL if desired

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  
  Serial.println(F("==========================================="));
  Serial.println(F("      PB7200P80 - Cell Balancing          "));
  Serial.println(F("==========================================="));
  Serial.println();
  
  // Initialize BMS
  if (!bms.begin(NUM_CELLS)) {
    Serial.println(F("ERROR: Failed to initialize PB7200P80!"));
    while (1) delay(1000);
  }
  
  Serial.println(F("PB7200P80 initialized!"));
  
  if (currentMode == AUTOMATIC) {
    Serial.println(F("Mode: AUTOMATIC BALANCING"));
    Serial.print(F("Threshold: "));
    Serial.print(BALANCE_THRESHOLD_MV, 0);
    Serial.println(F(" mV"));
    
    // Enable automatic balancing
    if (bms.setAutoBalancing(true, BALANCE_THRESHOLD_MV)) {
      Serial.println(F("Automatic balancing activated!"));
    } else {
      Serial.println(F("ERROR activating automatic balancing"));
    }
  } else {
    Serial.println(F("Mode: MANUAL BALANCING"));
    Serial.println(F("Enter cell number (1-4) to balance"));
    Serial.println(F("or '0' to stop all balancing"));
  }
  
  Serial.println();
  delay(2000);
}

void loop() {
  // Update readings
  bms.update();
  
  // Manual mode - process serial commands
  if (currentMode == MANUAL && Serial.available()) {
    char cmd = Serial.read();
    
    if (cmd >= '1' && cmd <= '4') {
      uint8_t cellIndex = cmd - '1';
      
      // Toggle cell balancing
      bool currentState = bms.isBalancing(cellIndex);
      bms.setBalancing(cellIndex, !currentState);
      
      Serial.print(F("Cell "));
      Serial.print(cellIndex + 1);
      Serial.print(F(": Balancing "));
      Serial.println(!currentState ? F("ENABLED") : F("DISABLED"));
    } else if (cmd == '0') {
      bms.stopAllBalancing();
      Serial.println(F("All balancing disabled"));
    }
  }
  
  // Display status every 3 seconds
  static unsigned long lastDisplay = 0;
  if (millis() - lastDisplay >= 3000) {
    lastDisplay = millis();
    displayBalancingStatus();
  }
  
  delay(100);
}

void displayBalancingStatus() {
  Serial.println(F("========================================"));
  Serial.println(F("       BALANCING STATUS                 "));
  Serial.println(F("========================================"));
  
  // Voltages and balancing status
  Serial.println(F("\nCell   | Voltage | Delta  | Balance"));
  Serial.println(F("-------|---------|--------|----------"));
  
  float avgVoltage = 0;
  for (uint8_t i = 0; i < NUM_CELLS; i++) {
    avgVoltage += bms.getCellVoltage(i);
  }
  avgVoltage /= NUM_CELLS;
  
  for (uint8_t i = 0; i < NUM_CELLS; i++) {
    float voltage = bms.getCellVoltage(i);
    float delta = (voltage - avgVoltage) * 1000;  // In mV
    bool balancing = bms.isBalancing(i);
    
    Serial.print(F("  "));
    Serial.print(i + 1);
    Serial.print(F("    | "));
    Serial.print(voltage, 3);
    Serial.print(F(" V | "));
    
    if (delta >= 0) Serial.print(F("+"));
    Serial.print(delta, 1);
    Serial.print(F(" mV | "));
    
    if (balancing) {
      Serial.println(F(">>> YES <<<"));
    } else {
      Serial.println(F("No"));
    }
  }
  
  // Statistics
  Serial.println(F("\nStatistics:"));
  Serial.print(F("  Average Voltage: "));
  Serial.print(avgVoltage, 3);
  Serial.println(F(" V"));
  
  Serial.print(F("  Maximum Voltage: "));
  Serial.print(bms.getMaxCellVoltage(), 3);
  Serial.println(F(" V"));
  
  Serial.print(F("  Minimum Voltage: "));
  Serial.print(bms.getMinCellVoltage(), 3);
  Serial.println(F(" V"));
  
  float delta = bms.getVoltageDelta() * 1000;
  Serial.print(F("  Total Delta:     "));
  Serial.print(delta, 1);
  Serial.println(F(" mV"));
  
  // Recommendation
  Serial.println();
  if (delta < BALANCE_THRESHOLD_MV) {
    Serial.println(F("✓ Cells well balanced!"));
  } else {
    Serial.println(F("⚠ Balancing needed"));
  }
  
  Serial.println(F("========================================\n"));
}
