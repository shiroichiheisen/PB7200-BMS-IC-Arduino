/**
 * CompleteBMS.ino
 * 
 * Complete BMS system example with PB7200P80
 * 
 * This example demonstrates:
 * - Complete monitoring of all functions
 * - Detailed pack statistics
 * - Automatic balancing
 * - Configured protections
 * - Interactive serial interface
 * 
 * Available commands:
 * - 'v' : Show voltages
 * - 't' : Show temperatures
 * - 's' : Show statistics
 * - 'p' : Show protections
 * - 'b' : Toggle automatic balancing
 * - 'c' : Clear faults
 * - 'd' : Complete diagnostics
 */

#include <PB7200P80.h>

PB7200P80 bms(PB7200_INTERFACE_I2C, 0x55);

const uint8_t NUM_CELLS = 4;
bool autoBalanceEnabled = true;

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  
  printHeader();
  
  // Initialize BMS
  Serial.println(F("Initializing BMS..."));
  if (!bms.begin(NUM_CELLS)) {
    Serial.println(F("✗ ERROR: Initialization failed!"));
    while (1) delay(1000);
  }
  Serial.println(F("✓ BMS initialized!"));
  
  // Run self-test
  Serial.println();
  bms.selfTest();
  
  // Configure protections (Li-ion)
  Serial.println();
  Serial.println(F("Configuring protections..."));
  ProtectionConfig config = {
    .overVoltageThreshold = 4.25,
    .underVoltageThreshold = 2.80,
    .overCurrentThreshold = 10.0,
    .overTempThreshold = 60.0,
    .underTempThreshold = -10.0,
    .overVoltageDelay = 100,
    .underVoltageDelay = 100,
    .overCurrentDelay = 50
  };
  
  if (bms.setProtectionConfig(config)) {
    Serial.println(F("✓ Protections configured!"));
  } else {
    Serial.println(F("✗ Error configuring protections"));
  }
  
  // Enable automatic balancing
  if (autoBalanceEnabled) {
    Serial.println(F("Enabling automatic balancing..."));
    if (bms.setAutoBalancing(true, 50)) {
      Serial.println(F("✓ Automatic balancing active (threshold: 50mV)"));
    }
  }
  
  Serial.println();
  printMenu();
  Serial.println();
  
  delay(2000);
}

void loop() {
  // Update readings every 1 second
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate >= 1000) {
    lastUpdate = millis();
    bms.update();
    
    // Check critical alerts
    checkCriticalAlerts();
  }
  
  // Process serial commands
  if (Serial.available()) {
    char cmd = Serial.read();
    processCommand(cmd);
  }
  
  // Automatic display every 5 seconds
  static unsigned long lastDisplay = 0;
  if (millis() - lastDisplay >= 5000) {
    lastDisplay = millis();
    showQuickStatus();
  }
  
  delay(100);
}

void printHeader() {
  Serial.println(F("\n╔═══════════════════════════════════════╗"));
  Serial.println(F("║   COMPLETE BMS SYSTEM - PB7200P80     ║"));
  Serial.println(F("║        Battery Management System       ║"));
  Serial.println(F("╚═══════════════════════════════════════╝\n"));
}

void printMenu() {
  Serial.println(F("┌───────────────────────────────────────┐"));
  Serial.println(F("│        AVAILABLE COMMANDS              │"));
  Serial.println(F("├───────────────────────────────────────┤"));
  Serial.println(F("│ v - Show cell voltages                │"));
  Serial.println(F("│ t - Show temperatures                 │"));
  Serial.println(F("│ s - Show complete statistics          │"));
  Serial.println(F("│ p - Show protection status            │"));
  Serial.println(F("│ b - Toggle automatic balancing        │"));
  Serial.println(F("│ c - Clear faults                      │"));
  Serial.println(F("│ d - Complete diagnostics              │"));
  Serial.println(F("│ h - Show this menu                    │"));
  Serial.println(F("└───────────────────────────────────────┘"));
}

void processCommand(char cmd) {
  Serial.println();
  
  switch (cmd) {
    case 'v':
    case 'V':
      bms.printCellVoltages();
      break;
      
    case 't':
    case 'T':
      bms.printTemperatures();
      break;
      
    case 's':
    case 'S':
      showCompleteStats();
      break;
      
    case 'p':
    case 'P':
      bms.printStatus();
      break;
      
    case 'b':
    case 'B':
      autoBalanceEnabled = !autoBalanceEnabled;
      bms.setAutoBalancing(autoBalanceEnabled, 50);
      Serial.print(F("Automatic balancing: "));
      Serial.println(autoBalanceEnabled ? F("ENABLED") : F("DISABLED"));
      break;
      
    case 'c':
    case 'C':
      if (bms.clearFaults()) {
        Serial.println(F("✓ Faults cleared"));
      } else {
        Serial.println(F("✗ Error clearing faults"));
      }
      break;
      
    case 'd':
    case 'D':
      bms.printDiagnostics();
      break;
      
    case 'h':
    case 'H':
      printMenu();
      break;
      
    default:
      if (cmd != '\n' && cmd != '\r') {
        Serial.println(F("Invalid command. Type 'h' for help."));
      }
      break;
  }
  
  Serial.println();
}

void showQuickStatus() {
  PackStats stats;
  if (!bms.getPackStats(stats)) {
    return;
  }
  
  Serial.println(F("─────────────────────────────────────────"));
  Serial.print(F("Pack: "));
  Serial.print(stats.totalVoltage, 2);
  Serial.print(F("V │ "));
  Serial.print(stats.current, 2);
  Serial.print(F("A │ "));
  Serial.print(stats.power, 1);
  Serial.print(F("W │ Δ:"));
  Serial.print(stats.voltageDelta * 1000, 0);
  Serial.print(F("mV │ "));
  Serial.print(stats.maxTemp, 0);
  Serial.println(F("°C"));
}

void showCompleteStats() {
  PackStats stats;
  if (!bms.getPackStats(stats)) {
    Serial.println(F("Error getting statistics"));
    return;
  }
  
  Serial.println(F("╔═══════════════════════════════════════╗"));
  Serial.println(F("║      COMPLETE PACK STATISTICS          ║"));
  Serial.println(F("╚═══════════════════════════════════════╝"));
  
  Serial.println(F("\n┌─── Voltages ──────────────────────────┐"));
  Serial.print(F("│ Total:        "));
  printValue(stats.totalVoltage, 3, "V");
  Serial.print(F("│ Maximum:      "));
  printValue(stats.maxCellVoltage, 3, "V");
  Serial.print(F("│ Minimum:      "));
  printValue(stats.minCellVoltage, 3, "V");
  Serial.print(F("│ Average:      "));
  printValue(stats.avgCellVoltage, 3, "V");
  Serial.print(F("│ Delta:        "));
  printValue(stats.voltageDelta * 1000, 1, "mV");
  Serial.println(F("└───────────────────────────────────────┘"));
  
  Serial.println(F("\n┌─── Current and Power ─────────────────┐"));
  Serial.print(F("│ Current:      "));
  printValue(stats.current, 3, "A");
  Serial.print(F("│ Power:        "));
  printValue(stats.power, 2, "W");
  Serial.print(F("│ Status:       "));
  if (stats.current > 0.1) {
    Serial.println(F("CHARGING           │"));
  } else if (stats.current < -0.1) {
    Serial.println(F("DISCHARGING        │"));
  } else {
    Serial.println(F("IDLE               │"));
  }
  Serial.println(F("└───────────────────────────────────────┘"));
  
  Serial.println(F("\n┌─── Temperatures ──────────────────────┐"));
  Serial.print(F("│ Maximum:      "));
  printValue(stats.maxTemp, 1, "°C");
  Serial.print(F("│ Minimum:      "));
  printValue(stats.minTemp, 1, "°C");
  Serial.println(F("└───────────────────────────────────────┘"));
  
  Serial.println(F("\n┌─── Indices ───────────────────────────┐"));
  Serial.print(F("│ Max V Cell:   "));
  Serial.print(stats.maxCellIndex + 1);
  Serial.println(F("                    │"));
  Serial.print(F("│ Min V Cell:   "));
  Serial.print(stats.minCellIndex + 1);
  Serial.println(F("                    │"));
  Serial.print(F("│ Max T Sensor: "));
  Serial.print(stats.maxTempIndex + 1);
  Serial.println(F("                    │"));
  Serial.print(F("│ Min T Sensor: "));
  Serial.print(stats.minTempIndex + 1);
  Serial.println(F("                    │"));
  Serial.println(F("└───────────────────────────────────────┘"));
}

void printValue(float value, uint8_t decimals, const char* unit) {
  char buffer[20];
  dtostrf(value, 6, decimals, buffer);
  Serial.print(buffer);
  Serial.print(" ");
  Serial.print(unit);
  
  // Preenche espaços
  int len = strlen(buffer) + strlen(unit) + 1;
  for (int i = len; i < 19; i++) {
    Serial.print(" ");
  }
  Serial.println("│");
}

void checkCriticalAlerts() {
  static bool alertShown = false;
  
  bool hasAlert = false;
  
  if (bms.isOverVoltage() || bms.isUnderVoltage() || 
      bms.isOverCurrent() || bms.isOverTemperature()) {
    hasAlert = true;
  }
  
  if (hasAlert && !alertShown) {
    Serial.println(F("\n╔═══════════════════════════════════════╗"));
    Serial.println(F("║          ⚠  CRITICAL ALERT  ⚠         ║"));
    Serial.println(F("╚═══════════════════════════════════════╝"));
    bms.printStatus();
    alertShown = true;
  } else if (!hasAlert && alertShown) {
    Serial.println(F("\n✓ Alerts cleared - System normalized"));
    alertShown = false;
  }
}
