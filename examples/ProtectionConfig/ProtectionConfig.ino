/**
 * ProtectionConfig.ino
 * 
 * PB7200P80 protection configuration example
 * 
 * This example demonstrates:
 * - Protection limits configuration
 * - Reading current settings
 * - Monitoring protection events
 * 
 * WARNING: Adjust values according to your
 *          battery pack specifications!
 */

#include <PB7200P80.h>

PB7200P80 bms(PB7200_INTERFACE_I2C, 0x55);

const uint8_t NUM_CELLS = 4;

// Protection settings for typical Li-ion cells
// ADJUST ACCORDING TO YOUR BATTERY TYPE!
ProtectionConfig protection = {
  .overVoltageThreshold = 4.25,      // Overvoltage at 4.25V
  .underVoltageThreshold = 2.80,     // Undervoltage at 2.80V
  .overCurrentThreshold = 10.0,      // Overcurrent at 10A
  .overTempThreshold = 60.0,         // Over-temperature at 60°C
  .underTempThreshold = -10.0,       // Under-temperature at -10°C
  .overVoltageDelay = 100,           // 100ms delay
  .underVoltageDelay = 100,          // 100ms delay
  .overCurrentDelay = 50             // 50ms delay
};

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  
  Serial.println(F("==========================================="));
  Serial.println(F("   PB7200P80 - Protection Configuration   "));
  Serial.println(F("==========================================="));
  Serial.println();
  
  // Initialize BMS
  if (!bms.begin(NUM_CELLS)) {
    Serial.println(F("ERROR: Failed to initialize PB7200P80!"));
    while (1) delay(1000);
  }
  
  Serial.println(F("PB7200P80 initialized!"));
  Serial.println();
  
  // Read current configuration
  Serial.println(F("CURRENT Protection Configuration:"));
  Serial.println(F("----------------------------------------"));
  displayProtectionConfig();
  Serial.println();
  
  // Ask if user wants to apply new configuration
  Serial.println(F("Do you want to apply the NEW configuration? (Y/N)"));
  Serial.println(F("(Type 'Y' in the next 10 seconds)"));
  
  unsigned long startTime = millis();
  bool applyConfig = false;
  
  while (millis() - startTime < 10000) {
    if (Serial.available()) {
      char response = Serial.read();
      if (response == 'Y' || response == 'y') {
        applyConfig = true;
        break;
      } else if (response == 'N' || response == 'n') {
        break;
      }
    }
    delay(100);
  }
  
  if (applyConfig) {
    Serial.println(F("\nApplying new configuration..."));
    
    if (bms.setProtectionConfig(protection)) {
      Serial.println(F("✓ Configuration applied successfully!"));
      delay(1000);
      
      Serial.println(F("\nNEW Configuration:"));
      Serial.println(F("----------------------------------------"));
      displayProtectionConfig();
    } else {
      Serial.println(F("✗ ERROR applying configuration!"));
    }
  } else {
    Serial.println(F("\nKeeping current configuration."));
  }
  
  Serial.println();
  Serial.println(F("Starting protection monitoring..."));
  Serial.println();
  delay(2000);
}

void loop() {
  // Update readings
  bms.update();
  
  // Check protections
  static bool lastOVP = false;
  static bool lastUVP = false;
  static bool lastOCP = false;
  static bool lastOTP = false;
  
  bool ovp = bms.isOverVoltage();
  bool uvp = bms.isUnderVoltage();
  bool ocp = bms.isOverCurrent();
  bool otp = bms.isOverTemperature();
  
  // Detect changes and alert
  if (ovp && !lastOVP) {
    Serial.println(F("\n⚠⚠⚠ ALERT: OVERVOLTAGE DETECTED! ⚠⚠⚠"));
    showProtectionDetails();
  }
  if (uvp && !lastUVP) {
    Serial.println(F("\n⚠⚠⚠ ALERT: UNDERVOLTAGE DETECTED! ⚠⚠⚠"));
    showProtectionDetails();
  }
  if (ocp && !lastOCP) {
    Serial.println(F("\n⚠⚠⚠ ALERT: OVERCURRENT DETECTED! ⚠⚠⚠"));
    showProtectionDetails();
  }
  if (otp && !lastOTP) {
    Serial.println(F("\n⚠⚠⚠ ALERT: OVER-TEMPERATURE DETECTED! ⚠⚠⚠"));
    showProtectionDetails();
  }
  
  lastOVP = ovp;
  lastUVP = uvp;
  lastOCP = ocp;
  lastOTP = otp;
  
  // Periodic display
  static unsigned long lastDisplay = 0;
  if (millis() - lastDisplay >= 5000) {
    lastDisplay = millis();
    displayMonitoring();
  }
  
  delay(100);
}

void displayProtectionConfig() {
  ProtectionConfig config;
  if (bms.getProtectionConfig(config)) {
    Serial.print(F("Overvoltage:      "));
    Serial.print(config.overVoltageThreshold, 2);
    Serial.println(F(" V"));
    
    Serial.print(F("Undervoltage:     "));
    Serial.print(config.underVoltageThreshold, 2);
    Serial.println(F(" V"));
    
    Serial.print(F("Overcurrent:      "));
    Serial.print(config.overCurrentThreshold, 1);
    Serial.println(F(" A"));
    
    Serial.print(F("Over-temperature: "));
    Serial.print(config.overTempThreshold, 1);
    Serial.println(F(" °C"));
    
    Serial.print(F("Under-temperature:"));
    Serial.print(config.underTempThreshold, 1);
    Serial.println(F(" °C"));
  } else {
    Serial.println(F("ERROR reading configuration!"));
  }
}

void displayMonitoring() {
  Serial.println(F("========================================"));
  Serial.println(F("       PROTECTION MONITORING            "));
  Serial.println(F("========================================"));
  
  // Protection status
  Serial.println(F("\nProtection Status:"));
  Serial.print(F("  Overvoltage:      "));
  Serial.println(bms.isOverVoltage() ? F("⚠ ACTIVE") : F("✓ OK"));
  
  Serial.print(F("  Undervoltage:     "));
  Serial.println(bms.isUnderVoltage() ? F("⚠ ACTIVE") : F("✓ OK"));
  
  Serial.print(F("  Overcurrent:      "));
  Serial.println(bms.isOverCurrent() ? F("⚠ ACTIVE") : F("✓ OK"));
  
  Serial.print(F("  Over-temperature: "));
  Serial.println(bms.isOverTemperature() ? F("⚠ ACTIVE") : F("✓ OK"));
  
  Serial.print(F("  Under-temperature:"));
  Serial.println(bms.isUnderTemperature() ? F("⚠ ACTIVE") : F("✓ OK"));
  
  // Current values
  Serial.println(F("\nCurrent Values:"));
  Serial.print(F("  Max Cell Voltage: "));
  Serial.print(bms.getMaxCellVoltage(), 3);
  Serial.println(F(" V"));
  
  Serial.print(F("  Min Cell Voltage: "));
  Serial.print(bms.getMinCellVoltage(), 3);
  Serial.println(F(" V"));
  
  Serial.print(F("  Current:          "));
  Serial.print(bms.getCurrent(), 3);
  Serial.println(F(" A"));
  
  Serial.print(F("  Max Temperature:  "));
  Serial.print(bms.getMaxTemperature(), 1);
  Serial.println(F(" °C"));
  
  Serial.println(F("========================================\n"));
}

void showProtectionDetails() {
  Serial.println(F("\nProtection Details:"));
  Serial.println(F("----------------------------------------"));
  
  for (uint8_t i = 0; i < NUM_CELLS; i++) {
    Serial.print(F("Cell "));
    Serial.print(i + 1);
    Serial.print(F(": "));
    Serial.print(bms.getCellVoltage(i), 3);
    Serial.println(F(" V"));
  }
  
  Serial.print(F("Current: "));
  Serial.print(bms.getCurrent(), 3);
  Serial.println(F(" A"));
  
  for (uint8_t i = 0; i < 4; i++) {
    Serial.print(F("Temp "));
    Serial.print(i + 1);
    Serial.print(F(": "));
    Serial.print(bms.getTemperature(i), 1);
    Serial.println(F(" °C"));
  }
  
  Serial.println(F("----------------------------------------\n"));
}
