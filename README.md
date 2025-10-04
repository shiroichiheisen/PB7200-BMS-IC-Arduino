# PB7200P80 Arduino Library

## Version 1.0.0

Complete Arduino library for the PB7200P80 Battery Management System (BMS) AFE chip.

---

## Table of Contents

1. [Introduction](#introduction)
2. [Installation](#installation)
3. [Hardware Connections](#hardware-connections)
4. [Quick Start](#quick-start)
5. [API Reference](#api-reference)
6. [Usage Examples](#usage-examples)
7. [Protection Configuration](#protection-configuration)
8. [Cell Balancing](#cell-balancing)
9. [Troubleshooting](#troubleshooting)
10. [FAQ](#faq)

---

## Introduction

The **PB7200P80** library provides a complete and easy-to-use interface for communication with the PB7200P80 chip, an AFE (Analog Front End) for Battery Management Systems (BMS).

### Main Features

✓ **Cell Monitoring**: Precise reading of up to 20 cells in series  
✓ **Temperature Measurement**: Support for up to 8 temperature sensors  
✓ **Current Measurement**: Charge/discharge current reading  
✓ **Configurable Protections**: Overvoltage, undervoltage, overcurrent, overtemperature  
✓ **Cell Balancing**: Manual and automatic  
✓ **I2C Interface**: Simple and reliable communication  
✓ **Diagnostic Functions**: Tools for debugging and validation  

### Compatibility

- **Platforms**: Arduino Uno, Nano, Mega, ESP32, ESP8266, STM32
- **IDE**: Arduino IDE 1.8+, PlatformIO
- **Communication**: I2C (UART in development)

---

## Installation

### Method 1: Manual Installation

1. Download the library or copy the `PB7200P80` folder to:
   - Windows: `Documents\Arduino\libraries\`
   - Mac: `~/Documents/Arduino/libraries/`
   - Linux: `~/Arduino/libraries/`

2. Restart Arduino IDE

3. Check if `PB7200P80` appears in **Sketch → Include Library**

### Method 2: PlatformIO

Add to your `platformio.ini`:

```ini
lib_deps = 
    Wire
    PB7200P80
```

---

## Hardware Connections

### Basic I2C Connection

```
PB7200P80          Arduino Uno/Nano
---------          ----------------
VCC      ────────  3.3V or 5V
GND      ────────  GND
SDA      ────────  A4 (SDA)
SCL      ────────  A5 (SCL)
```

### ESP32 Connection

```
PB7200P80          ESP32
---------          -----
VCC      ────────  3.3V
GND      ────────  GND
SDA      ────────  GPIO21 (SDA)
SCL      ────────  GPIO22 (SCL)
```

### Important Notes

⚠ **Operating Voltage**: Check if PB7200P80 supports 5V or only 3.3V  
⚠ **Pull-ups**: Most Arduino boards have internal pull-up resistors. If needed, add 4.7kΩ resistors between SDA/SCL and VCC  
⚠ **Cable Length**: Keep I2C cables short (< 30cm) to avoid communication issues  

---

## Quick Start

### Minimal Example

```cpp
#include <PB7200P80.h>

// Create BMS object
PB7200P80 bms;

void setup() {
  Serial.begin(115200);
  
  // Initialize BMS with 4 cells
  if (!bms.begin(4)) {
    Serial.println("Error initializing BMS!");
    while(1);
  }
  
  Serial.println("BMS initialized!");
}

void loop() {
  // Update all readings
  bms.update();
  
  // Display total voltage
  Serial.print("Pack Voltage: ");
  Serial.print(bms.getTotalVoltage());
  Serial.println(" V");
  
  delay(1000);
}
```

---

## API Reference

### Initialization

#### `begin()`
```cpp
bool begin(uint8_t cellCount)
```
Initialize communication with PB7200P80.

**Parameters:**
- `cellCount`: Number of connected cells (1-20)

**Returns:** `true` if successfully initialized

### Voltage Reading

#### `getCellVoltage()`
```cpp
float getCellVoltage(uint8_t cellIndex)
```
Read voltage of a specific cell.

**Returns:** Voltage in volts

#### `getTotalVoltage()`
```cpp
float getTotalVoltage()
```
Returns the sum of all cell voltages.

#### `getMaxCellVoltage()` / `getMinCellVoltage()`
Returns maximum/minimum cell voltage.

#### `getVoltageDelta()`
```cpp
float getVoltageDelta()
```
Returns difference between max and min voltage.

### Temperature Reading

#### `getTemperature()`
```cpp
float getTemperature(uint8_t tempIndex)
```
Read temperature from specific sensor (0-7).

**Returns:** Temperature in °C

### Current and Power

#### `getCurrent()`
```cpp
float getCurrent()
```
Read pack current (positive = charging, negative = discharging).

#### `getPower()`
```cpp
float getPower()
```
Returns calculated power (voltage × current) in watts.

### Status and Protections

#### `isOverVoltage()`
Check for overvoltage condition.

#### `isUnderVoltage()`
Check for undervoltage condition.

#### `isOverCurrent()`
Check for overcurrent condition.

#### `isOverTemperature()`
Check for overtemperature condition.

#### `clearFaults()`
Clear all fault flags.

### Cell Balancing

#### `setBalancing()`
```cpp
bool setBalancing(uint8_t cellIndex, bool enable)
```
Enable/disable balancing for a specific cell.

#### `setAutoBalancing()`
```cpp
bool setAutoBalancing(bool enable, uint16_t threshold = 50)
```
Enable automatic balancing with voltage threshold (mV).

#### `isBalancing()`
Check if a cell is being balanced.

### Configuration

#### `setProtectionConfig()`
```cpp
bool setProtectionConfig(const ProtectionConfig &config)
```
Configure protection limits.

**Example:**
```cpp
ProtectionConfig config;
config.overVoltageThreshold = 4.25;     // 4.25V per cell
config.underVoltageThreshold = 2.80;    // 2.80V per cell
config.overCurrentThreshold = 10.0;     // 10A
config.overTempThreshold = 60.0;        // 60°C
bms.setProtectionConfig(config);
```

### Statistics

#### `getPackStats()`
```cpp
bool getPackStats(PackStats &stats)
```
Get complete pack statistics in a single call.

**Fields:**
- `totalVoltage`: Total pack voltage
- `maxCellVoltage`: Maximum cell voltage
- `minCellVoltage`: Minimum cell voltage
- `voltageDelta`: Max-min difference
- `current`: Current
- `power`: Power
- `maxTemp`: Maximum temperature

#### `update()`
```cpp
bool update()
```
Update all readings at once (optimized).

### Diagnostic Functions

#### `selfTest()`
Run self-test and print results.

#### `printDiagnostics()`
Print complete diagnostic information.

#### `printCellVoltages()`
Print all cell voltages formatted.

---

## Usage Examples

### Example 1: Simple Serial Monitor

```cpp
#include <PB7200P80.h>

PB7200P80 bms;

void setup() {
  Serial.begin(115200);
  bms.begin(4);
}

void loop() {
  bms.update();
  
  Serial.print("Pack: ");
  Serial.print(bms.getTotalVoltage(), 2);
  Serial.print("V | ");
  Serial.print(bms.getCurrent(), 2);
  Serial.print("A | ");
  Serial.print(bms.getMaxTemperature(), 1);
  Serial.println("°C");
  
  delay(1000);
}
```

### Example 2: Monitoring with Alerts

```cpp
void loop() {
  bms.update();
  
  // Check critical alerts
  if (bms.isOverVoltage()) {
    Serial.println("⚠ ALERT: OVERVOLTAGE!");
    // Action: disconnect charger
  }
  
  if (bms.isUnderVoltage()) {
    Serial.println("⚠ ALERT: UNDERVOLTAGE!");
    // Action: disconnect load
  }
  
  if (bms.isOverTemperature()) {
    Serial.println("⚠ ALERT: HIGH TEMPERATURE!");
    // Action: reduce current or shut down
  }
  
  // Show voltage delta
  float delta = bms.getVoltageDelta() * 1000;
  if (delta > 100) {  // > 100mV
    Serial.print("⚠ High delta: ");
    Serial.print(delta);
    Serial.println(" mV - Consider balancing");
  }
  
  delay(2000);
}
```

### Example 3: Protection Relay Control

```cpp
const int CHARGE_RELAY_PIN = 7;
const int DISCHARGE_RELAY_PIN = 8;

void loop() {
  bms.update();
  
  // Charge relay control
  if (bms.isOverVoltage() || bms.isOverTemperature()) {
    digitalWrite(CHARGE_RELAY_PIN, LOW);  // Turn off charger
  } else {
    digitalWrite(CHARGE_RELAY_PIN, HIGH);  // Allow charging
  }
  
  // Discharge relay control
  if (bms.isUnderVoltage() || bms.isOverCurrent()) {
    digitalWrite(DISCHARGE_RELAY_PIN, LOW);  // Block discharge
  } else {
    digitalWrite(DISCHARGE_RELAY_PIN, HIGH);  // Allow discharge
  }
  
  delay(500);
}
```

---

## Protection Configuration

### Li-ion Battery Protection (18650)

```cpp
ProtectionConfig liionConfig = {
  .overVoltageThreshold = 4.25,      // Safe maximum
  .underVoltageThreshold = 2.80,     // Safe minimum
  .overCurrentThreshold = 10.0,      // Depends on cell
  .overTempThreshold = 60.0,         // Temperature limit
  .underTempThreshold = 0.0,         // Don't charge below 0°C
  .overVoltageDelay = 100,
  .underVoltageDelay = 100,
  .overCurrentDelay = 50
};

bms.setProtectionConfig(liionConfig);
```

### LiFePO4 Battery Protection

```cpp
ProtectionConfig lifepo4Config = {
  .overVoltageThreshold = 3.65,      // Maximum for LiFePO4
  .underVoltageThreshold = 2.50,     // Minimum for LiFePO4
  .overCurrentThreshold = 20.0,      // LiFePO4 handles more current
  .overTempThreshold = 60.0,
  .underTempThreshold = -20.0,       // LiFePO4 more cold-tolerant
  .overVoltageDelay = 100,
  .underVoltageDelay = 100,
  .overCurrentDelay = 50
};

bms.setProtectionConfig(lifepo4Config);
```

---

## Cell Balancing

### When to Balance?

Balancing is necessary when there's significant difference between cell voltages:

- **Difference < 30mV**: Well-balanced cells, no action required
- **Difference 30-50mV**: Balancing recommended during charge
- **Difference > 50mV**: Balancing necessary
- **Difference > 100mV**: Urgent balancing, investigate weak cells

### Automatic Balancing

```cpp
void setup() {
  bms.begin(4);
  
  // Activate automatic balancing
  // 50mV threshold
  bms.setAutoBalancing(true, 50);
}

void loop() {
  bms.update();
  
  // Monitor progress
  for (uint8_t i = 0; i < 4; i++) {
    if (bms.isBalancing(i)) {
      Serial.print("Cell ");
      Serial.print(i + 1);
      Serial.println(" balancing...");
    }
  }
  
  delay(5000);
}
```

---

## Troubleshooting

### Problem: BMS doesn't initialize

**Symptoms:** `begin()` returns `false`

**Solutions:**
1. Check I2C connections (SDA/SCL)
2. Confirm PB7200P80 power supply
3. Test I2C address:
```cpp
Wire.begin();
Wire.beginTransmission(0x55);
byte error = Wire.endTransmission();
if (error == 0) {
  Serial.println("Device found!");
}
```
4. Add 4.7kΩ pull-up resistors on SDA and SCL
5. Reduce I2C speed:
```cpp
Wire.setClock(50000);  // 50kHz instead of 100kHz
```

### Problem: Wrong voltage readings

**Symptoms:** 0V voltages or absurd values

**Solutions:**
1. Check if cells are connected to PB7200P80
2. Confirm number of cells in `begin()`:
```cpp
bms.begin(4);  // Must match actual number
```
3. Run diagnostics:
```cpp
bms.printDiagnostics();
```
4. Check if cell is faulty (multimeter)

### Problem: Unstable I2C communication

**Symptoms:** Intermittent readings, lockups

**Solutions:**
1. Shorten I2C cables (< 20cm ideal)
2. Add 100nF capacitor between VCC and GND near PB7200P80
3. Use shielded cables for I2C
4. Avoid routes near noise sources (motors, PWM)
5. Add small delay between readings:
```cpp
bms.update();
delay(50);
```

---

## FAQ

### Q: How many cells can I monitor?

**A:** PB7200P80 supports up to 20 cells in series. The library supports 1 to 20 cells.

### Q: Can I use with ESP32/ESP8266?

**A:** Yes! The library is compatible. On ESP32, use default I2C pins (GPIO21/22) or configure others:
```cpp
Wire.begin(SDA_PIN, SCL_PIN);
bms.begin(4);
```

### Q: Does balancing completely drain cells?

**A:** No. Balancing only dissipates energy from higher cells to equal the lower ones. It's a passive process using resistors.

### Q: Can I use in 24V/48V system?

**A:** Yes, as long as the number of series cells is compatible:
- 24V: ~6-7 Li-ion cells (6S)
- 48V: ~13-14 Li-ion cells (13S)

### Q: Memory consumption of library?

**A:** The library uses approximately:
- Flash: ~8KB
- RAM: ~250 bytes

Compatible with Arduino Uno and similar boards.

---

## Changelog

### Version 1.0.0 (2025-10-04)
- Initial release
- Complete I2C support
- Monitoring of up to 20 cells
- 8 temperature sensors
- Manual and automatic balancing
- Configurable protections
- Complete examples

---

## License

This library is distributed under the MIT license. See LICENSE file for details.

---

## Author

PB7200P80 Library  
Version 1.0.0  
Date: 10/04/2025

---

## Legal Notices

⚠ **WARNING**: Lithium batteries can be dangerous if mismanaged. This library is provided "as is" without warranties. The author is not responsible for damages caused by using this library.

✓ Always use adequate protections in battery systems  
✓ Monitor temperature constantly  
✓ Use redundant protection circuits in critical applications  
✓ Consult experts for commercial applications  
✓ Follow local regulations about lithium batteries  

---

**Happy using the PB7200P80 library!** 🔋⚡
