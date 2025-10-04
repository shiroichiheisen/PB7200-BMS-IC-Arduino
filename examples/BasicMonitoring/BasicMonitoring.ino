/**
 * BasicMonitoring.ino
 * 
 * Basic PB7200P80 monitoring example
 * 
 * This example demonstrates:
 * - PB7200P80 initialization
 * - Cell voltage reading
 * - Temperature reading
 * - Current reading
 * - Data display on Serial Monitor
 * 
 * Hardware:
 * - Arduino (any board)
 * - PB7200P80 connected via I2C
 *   - SDA -> A4 (Uno/Nano) or SDA (other boards)
 *   - SCL -> A5 (Uno/Nano) or SCL (other boards)
 *   - VCC -> 3.3V or 5V
 *   - GND -> GND
 */

#include <PB7200P80.h>

// Create PB7200P80 object with I2C interface at default address 0x55
PB7200P80 bms(PB7200_INTERFACE_I2C, 0x55);

// Number of cells configuration
const uint8_t NUM_CELLS = 4;  // Change according to your pack

void setup() {
  // Initialize Serial
  Serial.begin(115200);
  while (!Serial) delay(10);
  
  Serial.println(F("==========================================="));
  Serial.println(F("      PB7200P80 - Basic Monitoring         "));
  Serial.println(F("==========================================="));
  Serial.println();
  
  // Initialize PB7200P80
  Serial.print(F("Initializing PB7200P80 with "));
  Serial.print(NUM_CELLS);
  Serial.println(F(" cells..."));
  
  if (!bms.begin(NUM_CELLS)) {
    Serial.println(F("ERROR: Failed to initialize PB7200P80!"));
    Serial.println(F("Check connections:"));
    Serial.println(F("  - SDA/SCL connected correctly"));
    Serial.println(F("  - PB7200P80 power supply"));
    Serial.println(F("  - Correct I2C address"));
    while (1) delay(1000);
  }
  
  Serial.println(F("PB7200P80 initialized successfully!"));
  Serial.println();
  
  // Run self-test
  bms.selfTest();
  Serial.println();
  
  delay(1000);
}

void loop() {
  // Update all readings
  bms.update();
  
  // Header
  Serial.println(F("========================================"));
  Serial.println(F("            BMS READINGS                "));
  Serial.println(F("========================================"));
  
  // Cell voltages
  Serial.println(F("\nCell Voltages:"));
  Serial.println(F("----------------------------------------"));
  for (uint8_t i = 0; i < NUM_CELLS; i++) {
    float voltage = bms.getCellVoltage(i);
    Serial.print(F("Cell "));
    Serial.print(i + 1);
    Serial.print(F(": "));
    Serial.print(voltage, 3);
    Serial.println(F(" V"));
  }
  
  // Voltage statistics
  Serial.println(F("\nVoltage Statistics:"));
  Serial.println(F("----------------------------------------"));
  Serial.print(F("Total Voltage:  "));
  Serial.print(bms.getTotalVoltage(), 3);
  Serial.println(F(" V"));
  
  Serial.print(F("Maximum Voltage:"));
  Serial.print(bms.getMaxCellVoltage(), 3);
  Serial.println(F(" V"));
  
  Serial.print(F("Minimum Voltage:"));
  Serial.print(bms.getMinCellVoltage(), 3);
  Serial.println(F(" V"));
  
  Serial.print(F("Delta:          "));
  Serial.print(bms.getVoltageDelta() * 1000, 1);
  Serial.println(F(" mV"));
  
  // Temperatures
  Serial.println(F("\nTemperatures:"));
  Serial.println(F("----------------------------------------"));
  for (uint8_t i = 0; i < 4; i++) {  // Show 4 sensors
    float temp = bms.getTemperature(i);
    Serial.print(F("Sensor "));
    Serial.print(i + 1);
    Serial.print(F(": "));
    Serial.print(temp, 1);
    Serial.println(F(" °C"));
  }
  
  Serial.print(F("Max. Temp.:     "));
  Serial.print(bms.getMaxTemperature(), 1);
  Serial.println(F(" °C"));
  
  Serial.print(F("Min. Temp.:     "));
  Serial.print(bms.getMinTemperature(), 1);
  Serial.println(F(" °C"));
  
  // Current and Power
  Serial.println(F("\nCurrent and Power:"));
  Serial.println(F("----------------------------------------"));
  float current = bms.getCurrent();
  Serial.print(F("Current:        "));
  Serial.print(current, 3);
  Serial.println(F(" A"));
  
  float power = bms.getPower();
  Serial.print(F("Power:          "));
  Serial.print(power, 2);
  Serial.println(F(" W"));
  
  if (current > 0) {
    Serial.println(F("Status:         CHARGING"));
  } else if (current < -0.1) {
    Serial.println(F("Status:         DISCHARGING"));
  } else {
    Serial.println(F("Status:         IDLE"));
  }
  
  // System Status and Protections
  Serial.println(F("\nSystem Status:"));
  Serial.println(F("----------------------------------------"));
  
  Serial.print(F("Overvoltage:    "));
  Serial.println(bms.isOverVoltage() ? F("ALERT!") : F("OK"));
  
  Serial.print(F("Undervoltage:   "));
  Serial.println(bms.isUnderVoltage() ? F("ALERT!") : F("OK"));
  
  Serial.print(F("Overcurrent:    "));
  Serial.println(bms.isOverCurrent() ? F("ALERT!") : F("OK"));
  
  Serial.print(F("Over-temp.:     "));
  Serial.println(bms.isOverTemperature() ? F("ALERT!") : F("OK"));
  
  Serial.println(F("\n========================================\n"));
  
  // Wait 2 seconds before next reading
  delay(2000);
}
