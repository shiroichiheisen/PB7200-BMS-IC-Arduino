/**
 * @file PB7200P80.cpp
 * @brief Implementation of Arduino library for PB7200P80 AFE
 * @author PB7200P80 Library
 * @version 1.0.0
 * @date 2025-10-04
 */

#include "PB7200P80.h"

/**
 * @brief Class constructor
 */
PB7200P80::PB7200P80(PB7200_Interface interface, uint8_t address, TwoWire *wire) {
    _interface = interface;
    _i2cAddress = address;
    _wire = wire;
    _serial = nullptr;
    _cellCount = 0;
    _tempSensorCount = 8;
    _current = 0.0;
    _status = 0;
    _faultStatus = 0;
    _lastUpdate = 0;
    
    // Initialize arrays
    for (uint8_t i = 0; i < PB7200_MAX_CELLS; i++) {
        _cellVoltages[i] = 0.0;
    }
    for (uint8_t i = 0; i < PB7200_MAX_TEMPS; i++) {
        _temperatures[i] = 0.0;
    }
}

/**
 * @brief Initialize communication with PB7200P80
 */
bool PB7200P80::begin(uint8_t cellCount) {
    if (cellCount == 0 || cellCount > PB7200_MAX_CELLS) {
        return false;
    }
    
    _cellCount = cellCount;
    
    // Initialize communication interface
    if (_interface == PB7200_INTERFACE_I2C) {
        _wire->begin();
        _wire->setClock(100000); // 100kHz default
    } else {
        // TODO: Implement UART interface if needed
        return false;
    }
    
    delay(100); // Wait for stabilization
    
    // Check communication
    if (!isConnected()) {
        return false;
    }
    
    // Initial configuration
    uint8_t ctrlValue = 0x01; // Normal mode, ADC enabled
    if (!writeRegister(PB7200_REG_CONTROL, ctrlValue)) {
        return false;
    }
    
    delay(50);
    
    // First reading
    update();
    
    return true;
}

/**
 * @brief Check if device is responding
 */
bool PB7200P80::isConnected() {
    uint8_t deviceID;
    if (readRegister(PB7200_REG_DEVICE_ID, deviceID)) {
        return (deviceID != 0x00 && deviceID != 0xFF);
    }
    return false;
}

/**
 * @brief Read device ID
 */
uint8_t PB7200P80::getDeviceID() {
    uint8_t deviceID = 0;
    readRegister(PB7200_REG_DEVICE_ID, deviceID);
    return deviceID;
}

// ========== Voltage Reading ==========

/**
 * @brief Read voltage of a specific cell
 */
float PB7200P80::getCellVoltage(uint8_t cellIndex) {
    if (!isValidCellIndex(cellIndex)) {
        return 0.0;
    }
    
    uint8_t regAddr = PB7200_REG_CELL_VOLTAGE_BASE + (cellIndex * 2);
    uint8_t data[2];
    
    if (readRegisters(regAddr, data, 2)) {
        uint16_t raw = (data[0] << 8) | data[1];
        _cellVoltages[cellIndex] = rawToVoltage(raw);
        return _cellVoltages[cellIndex];
    }
    
    return 0.0;
}

/**
 * @brief Read all cell voltages
 */
bool PB7200P80::getAllCellVoltages(float *voltages, uint8_t count) {
    if (count > _cellCount || count > PB7200_MAX_CELLS) {
        return false;
    }
    
    uint8_t dataLength = count * 2;
    uint8_t data[PB7200_MAX_CELLS * 2];
    
    if (readRegisters(PB7200_REG_CELL_VOLTAGE_BASE, data, dataLength)) {
        for (uint8_t i = 0; i < count; i++) {
            uint16_t raw = (data[i * 2] << 8) | data[i * 2 + 1];
            voltages[i] = rawToVoltage(raw);
            _cellVoltages[i] = voltages[i];
        }
        return true;
    }
    
    return false;
}

/**
 * @brief Read complete cell data
 */
bool PB7200P80::getCellData(uint8_t cellIndex, CellData &data) {
    if (!isValidCellIndex(cellIndex)) {
        return false;
    }
    
    data.voltage = getCellVoltage(cellIndex);
    data.balancing = isBalancing(cellIndex);
    
    // Check protections
    uint8_t faults = getFaultStatus();
    data.overvoltage = (faults & PB7200_STATUS_OVP) != 0;
    data.undervoltage = (faults & PB7200_STATUS_UVP) != 0;
    
    return true;
}

/**
 * @brief Get total pack voltage
 */
float PB7200P80::getTotalVoltage() {
    float total = 0.0;
    for (uint8_t i = 0; i < _cellCount; i++) {
        total += _cellVoltages[i];
    }
    return total;
}

/**
 * @brief Get maximum cell voltage
 */
float PB7200P80::getMaxCellVoltage() {
    float maxVoltage = 0.0;
    for (uint8_t i = 0; i < _cellCount; i++) {
        if (_cellVoltages[i] > maxVoltage) {
            maxVoltage = _cellVoltages[i];
        }
    }
    return maxVoltage;
}

/**
 * @brief Get minimum cell voltage
 */
float PB7200P80::getMinCellVoltage() {
    float minVoltage = 5.0; // High initial value
    for (uint8_t i = 0; i < _cellCount; i++) {
        if (_cellVoltages[i] < minVoltage && _cellVoltages[i] > 0.0) {
            minVoltage = _cellVoltages[i];
        }
    }
    return minVoltage;
}

/**
 * @brief Get difference between max and min voltage
 */
float PB7200P80::getVoltageDelta() {
    return getMaxCellVoltage() - getMinCellVoltage();
}

// ========== Temperature Reading ==========

/**
 * @brief Read temperature from specific sensor
 */
float PB7200P80::getTemperature(uint8_t tempIndex) {
    if (!isValidTempIndex(tempIndex)) {
        return 0.0;
    }
    
    uint8_t regAddr = PB7200_REG_TEMP_BASE + (tempIndex * 2);
    uint8_t data[2];
    
    if (readRegisters(regAddr, data, 2)) {
        int16_t raw = (data[0] << 8) | data[1];
        _temperatures[tempIndex] = rawToTemp(raw);
        return _temperatures[tempIndex];
    }
    
    return 0.0;
}

/**
 * @brief Read all temperatures
 */
bool PB7200P80::getAllTemperatures(float *temperatures, uint8_t count) {
    if (count > PB7200_MAX_TEMPS) {
        return false;
    }
    
    uint8_t dataLength = count * 2;
    uint8_t data[PB7200_MAX_TEMPS * 2];
    
    if (readRegisters(PB7200_REG_TEMP_BASE, data, dataLength)) {
        for (uint8_t i = 0; i < count; i++) {
            int16_t raw = (data[i * 2] << 8) | data[i * 2 + 1];
            temperatures[i] = rawToTemp(raw);
            _temperatures[i] = temperatures[i];
        }
        return true;
    }
    
    return false;
}

/**
 * @brief Get maximum temperature
 */
float PB7200P80::getMaxTemperature() {
    float maxTemp = -100.0;
    for (uint8_t i = 0; i < _tempSensorCount; i++) {
        if (_temperatures[i] > maxTemp) {
            maxTemp = _temperatures[i];
        }
    }
    return maxTemp;
}

/**
 * @brief Get minimum temperature
 */
float PB7200P80::getMinTemperature() {
    float minTemp = 200.0;
    for (uint8_t i = 0; i < _tempSensorCount; i++) {
        if (_temperatures[i] < minTemp && _temperatures[i] > -50.0) {
            minTemp = _temperatures[i];
        }
    }
    return minTemp;
}

// ========== Current Reading ==========

/**
 * @brief Read pack current
 */
float PB7200P80::getCurrent() {
    uint8_t data[2];
    
    if (readRegisters(PB7200_REG_CURRENT_H, data, 2)) {
        int16_t raw = (data[0] << 8) | data[1];
        _current = rawToCurrent(raw);
        return _current;
    }
    
    return 0.0;
}

/**
 * @brief Read pack power
 */
float PB7200P80::getPower() {
    return getTotalVoltage() * _current;
}

// ========== Status and Protections ==========

/**
 * @brief Read status register
 */
uint8_t PB7200P80::getStatus() {
    readRegister(PB7200_REG_STATUS, _status);
    return _status;
}

/**
 * @brief Read fault register
 */
uint8_t PB7200P80::getFaultStatus() {
    readRegister(PB7200_REG_FAULT_STATUS, _faultStatus);
    return _faultStatus;
}

/**
 * @brief Check for overvoltage
 */
bool PB7200P80::isOverVoltage() {
    return (getFaultStatus() & PB7200_STATUS_OVP) != 0;
}

/**
 * @brief Check for undervoltage
 */
bool PB7200P80::isUnderVoltage() {
    return (getFaultStatus() & PB7200_STATUS_UVP) != 0;
}

/**
 * @brief Check for overcurrent
 */
bool PB7200P80::isOverCurrent() {
    return (getFaultStatus() & PB7200_STATUS_OCP) != 0;
}

/**
 * @brief Check for overtemperature
 */
bool PB7200P80::isOverTemperature() {
    return (getFaultStatus() & PB7200_STATUS_OTP) != 0;
}

/**
 * @brief Check for undertemperature
 */
bool PB7200P80::isUnderTemperature() {
    return (getFaultStatus() & PB7200_STATUS_UTP) != 0;
}

/**
 * @brief Clear fault flags
 */
bool PB7200P80::clearFaults() {
    return writeRegister(PB7200_REG_FAULT_STATUS, 0x00);
}

// ========== Cell Balancing ==========

/**
 * @brief Enable cell balancing
 */
bool PB7200P80::setBalancing(uint8_t cellIndex, bool enable) {
    if (!isValidCellIndex(cellIndex)) {
        return false;
    }
    
    // Determine which control register to use (3 registers for 20 cells)
    uint8_t regOffset = cellIndex / 8;
    uint8_t bitOffset = cellIndex % 8;
    uint8_t regAddr = PB7200_REG_BALANCE_CTRL1 + regOffset;
    
    uint8_t currentValue;
    if (!readRegister(regAddr, currentValue)) {
        return false;
    }
    
    if (enable) {
        currentValue |= (1 << bitOffset);
    } else {
        currentValue &= ~(1 << bitOffset);
    }
    
    return writeRegister(regAddr, currentValue);
}

/**
 * @brief Enable automatic balancing
 */
bool PB7200P80::setAutoBalancing(bool enable, uint16_t threshold) {
    // Simplified implementation - consult datasheet for actual chip
    if (enable) {
        // Enable automatic mode
        uint8_t ctrlValue;
        if (!readRegister(PB7200_REG_CONTROL, ctrlValue)) {
            return false;
        }
        ctrlValue |= 0x10; // Auto-balancing bit
        return writeRegister(PB7200_REG_CONTROL, ctrlValue);
    } else {
        return stopAllBalancing();
    }
}

/**
 * @brief Check if a cell is being balanced
 */
bool PB7200P80::isBalancing(uint8_t cellIndex) {
    if (!isValidCellIndex(cellIndex)) {
        return false;
    }
    
    uint8_t regOffset = cellIndex / 8;
    uint8_t bitOffset = cellIndex % 8;
    uint8_t regAddr = PB7200_REG_BALANCE_CTRL1 + regOffset;
    
    uint8_t value;
    if (readRegister(regAddr, value)) {
        return (value & (1 << bitOffset)) != 0;
    }
    
    return false;
}

/**
 * @brief Disable balancing for all cells
 */
bool PB7200P80::stopAllBalancing() {
    bool success = true;
    success &= writeRegister(PB7200_REG_BALANCE_CTRL1, 0x00);
    success &= writeRegister(PB7200_REG_BALANCE_CTRL2, 0x00);
    success &= writeRegister(PB7200_REG_BALANCE_CTRL3, 0x00);
    return success;
}

// ========== Configuration ==========

/**
 * @brief Configure protections
 */
bool PB7200P80::setProtectionConfig(const ProtectionConfig &config) {
    bool success = true;
    
    // Convert and write voltage limits
    uint16_t ovpRaw = voltageToRaw(config.overVoltageThreshold);
    uint16_t uvpRaw = voltageToRaw(config.underVoltageThreshold);
    
    success &= writeRegister(PB7200_REG_CONFIG_OVP, (ovpRaw >> 8) & 0xFF);
    success &= writeRegister(PB7200_REG_CONFIG_OVP + 1, ovpRaw & 0xFF);
    success &= writeRegister(PB7200_REG_CONFIG_UVP, (uvpRaw >> 8) & 0xFF);
    success &= writeRegister(PB7200_REG_CONFIG_UVP + 1, uvpRaw & 0xFF);
    
    // Convert and write current limit
    int16_t ocpRaw = currentToRaw(config.overCurrentThreshold);
    success &= writeRegister(PB7200_REG_CONFIG_OCP, (ocpRaw >> 8) & 0xFF);
    success &= writeRegister(PB7200_REG_CONFIG_OCP + 1, ocpRaw & 0xFF);
    
    // Convert and write temperature limits
    int16_t otpRaw = tempToRaw(config.overTempThreshold);
    int16_t utpRaw = tempToRaw(config.underTempThreshold);
    
    success &= writeRegister(PB7200_REG_CONFIG_OTP, (otpRaw >> 8) & 0xFF);
    success &= writeRegister(PB7200_REG_CONFIG_OTP + 1, otpRaw & 0xFF);
    success &= writeRegister(PB7200_REG_CONFIG_UTP, (utpRaw >> 8) & 0xFF);
    success &= writeRegister(PB7200_REG_CONFIG_UTP + 1, utpRaw & 0xFF);
    
    return success;
}

/**
 * @brief Read protection configuration
 */
bool PB7200P80::getProtectionConfig(ProtectionConfig &config) {
    uint8_t data[2];
    bool success = true;
    
    // Read voltage limits
    if (readRegisters(PB7200_REG_CONFIG_OVP, data, 2)) {
        uint16_t raw = (data[0] << 8) | data[1];
        config.overVoltageThreshold = rawToVoltage(raw);
    } else {
        success = false;
    }
    
    if (readRegisters(PB7200_REG_CONFIG_UVP, data, 2)) {
        uint16_t raw = (data[0] << 8) | data[1];
        config.underVoltageThreshold = rawToVoltage(raw);
    } else {
        success = false;
    }
    
    // Read current limit
    if (readRegisters(PB7200_REG_CONFIG_OCP, data, 2)) {
        int16_t raw = (data[0] << 8) | data[1];
        config.overCurrentThreshold = rawToCurrent(raw);
    } else {
        success = false;
    }
    
    // Read temperature limits
    if (readRegisters(PB7200_REG_CONFIG_OTP, data, 2)) {
        int16_t raw = (data[0] << 8) | data[1];
        config.overTempThreshold = rawToTemp(raw);
    } else {
        success = false;
    }
    
    if (readRegisters(PB7200_REG_CONFIG_UTP, data, 2)) {
        int16_t raw = (data[0] << 8) | data[1];
        config.underTempThreshold = rawToTemp(raw);
    } else {
        success = false;
    }
    
    return success;
}

/**
 * @brief Set operation mode
 */
bool PB7200P80::setMode(PB7200_Mode mode) {
    uint8_t ctrlValue;
    if (!readRegister(PB7200_REG_CONTROL, ctrlValue)) {
        return false;
    }
    
    // Clear mode bits and set new mode
    ctrlValue &= 0xFC;
    ctrlValue |= (mode & 0x03);
    
    return writeRegister(PB7200_REG_CONTROL, ctrlValue);
}

/**
 * @brief Reset device
 */
bool PB7200P80::reset() {
    // Write reset command
    if (writeRegister(PB7200_REG_CONTROL, 0x80)) {
        delay(100);
        return begin(_cellCount);
    }
    return false;
}

/**
 * @brief Enter sleep mode
 */
bool PB7200P80::sleep() {
    return setMode(PB7200_MODE_SLEEP);
}

/**
 * @brief Wake up from sleep mode
 */
bool PB7200P80::wakeup() {
    return setMode(PB7200_MODE_NORMAL);
}

/**
 * @brief Shutdown device
 */
bool PB7200P80::shutdown() {
    return writeRegister(PB7200_REG_SHUTDOWN, 0x01);
}

// ========== Statistics ==========

/**
 * @brief Get complete pack statistics
 */
bool PB7200P80::getPackStats(PackStats &stats) {
    // Update readings
    if (!update()) {
        return false;
    }
    
    stats.totalVoltage = 0.0;
    stats.maxCellVoltage = 0.0;
    stats.minCellVoltage = 5.0;
    stats.avgCellVoltage = 0.0;
    stats.maxCellIndex = 0;
    stats.minCellIndex = 0;
    
    // Process voltages
    for (uint8_t i = 0; i < _cellCount; i++) {
        stats.totalVoltage += _cellVoltages[i];
        
        if (_cellVoltages[i] > stats.maxCellVoltage) {
            stats.maxCellVoltage = _cellVoltages[i];
            stats.maxCellIndex = i;
        }
        
        if (_cellVoltages[i] < stats.minCellVoltage && _cellVoltages[i] > 0.0) {
            stats.minCellVoltage = _cellVoltages[i];
            stats.minCellIndex = i;
        }
    }
    
    stats.avgCellVoltage = stats.totalVoltage / _cellCount;
    stats.voltageDelta = stats.maxCellVoltage - stats.minCellVoltage;
    
    // Process temperatures
    stats.maxTemp = -100.0;
    stats.minTemp = 200.0;
    stats.maxTempIndex = 0;
    stats.minTempIndex = 0;
    
    for (uint8_t i = 0; i < _tempSensorCount; i++) {
        if (_temperatures[i] > stats.maxTemp) {
            stats.maxTemp = _temperatures[i];
            stats.maxTempIndex = i;
        }
        
        if (_temperatures[i] < stats.minTemp && _temperatures[i] > -50.0) {
            stats.minTemp = _temperatures[i];
            stats.minTempIndex = i;
        }
    }
    
    // Current and power
    stats.current = _current;
    stats.power = stats.totalVoltage * _current;
    
    return true;
}

/**
 * @brief Update all readings (optimized)
 */
bool PB7200P80::update() {
    bool success = true;
    
    // Read all voltages
    float voltages[PB7200_MAX_CELLS];
    if (getAllCellVoltages(voltages, _cellCount)) {
        for (uint8_t i = 0; i < _cellCount; i++) {
            _cellVoltages[i] = voltages[i];
        }
    } else {
        success = false;
    }
    
    // Read all temperatures
    float temps[PB7200_MAX_TEMPS];
    if (getAllTemperatures(temps, _tempSensorCount)) {
        for (uint8_t i = 0; i < _tempSensorCount; i++) {
            _temperatures[i] = temps[i];
        }
    } else {
        success = false;
    }
    
    // Read current
    getCurrent();
    
    // Read status
    getStatus();
    getFaultStatus();
    
    _lastUpdate = millis();
    
    return success;
}

// ========== Diagnostics ==========

/**
 * @brief Run self-test
 */
bool PB7200P80::selfTest() {
    // Check communication
    if (!isConnected()) {
        Serial.println(F("FAIL: No communication with PB7200P80"));
        return false;
    }
    
    Serial.println(F("OK: Communication established"));
    
    // Check device ID
    uint8_t deviceID = getDeviceID();
    Serial.print(F("Device ID: 0x"));
    Serial.println(deviceID, HEX);
    
    // Check voltage reading
    update();
    float totalV = getTotalVoltage();
    if (totalV < 0.1) {
        Serial.println(F("WARNING: Total voltage too low"));
    } else {
        Serial.print(F("OK: Total voltage = "));
        Serial.print(totalV);
        Serial.println(F(" V"));
    }
    
    return true;
}

/**
 * @brief Print diagnostic information
 */
void PB7200P80::printDiagnostics() {
    Serial.println(F("========== PB7200P80 Diagnostics =========="));
    Serial.print(F("Device ID: 0x"));
    Serial.println(getDeviceID(), HEX);
    Serial.print(F("Configured cells: "));
    Serial.println(_cellCount);
    Serial.print(F("Status: 0x"));
    Serial.println(getStatus(), HEX);
    Serial.print(F("Faults: 0x"));
    Serial.println(getFaultStatus(), HEX);
    Serial.println();
    
    printCellVoltages();
    Serial.println();
    printTemperatures();
    Serial.println();
    
    Serial.print(F("Current: "));
    Serial.print(_current, 3);
    Serial.println(F(" A"));
    Serial.print(F("Power: "));
    Serial.print(getPower(), 2);
    Serial.println(F(" W"));
    Serial.println(F("=========================================="));
}

/**
 * @brief Print all cell voltages
 */
void PB7200P80::printCellVoltages() {
    Serial.println(F("Cell Voltages:"));
    for (uint8_t i = 0; i < _cellCount; i++) {
        Serial.print(F("  Cell "));
        Serial.print(i + 1);
        Serial.print(F(": "));
        Serial.print(_cellVoltages[i], 3);
        Serial.print(F(" V"));
        if (isBalancing(i)) {
            Serial.print(F(" [BAL]"));
        }
        Serial.println();
    }
    Serial.print(F("Total: "));
    Serial.print(getTotalVoltage(), 3);
    Serial.print(F(" V | Delta: "));
    Serial.print(getVoltageDelta() * 1000, 1);
    Serial.println(F(" mV"));
}

/**
 * @brief Print all temperatures
 */
void PB7200P80::printTemperatures() {
    Serial.println(F("Temperatures:"));
    for (uint8_t i = 0; i < _tempSensorCount; i++) {
        Serial.print(F("  Sensor "));
        Serial.print(i + 1);
        Serial.print(F(": "));
        Serial.print(_temperatures[i], 1);
        Serial.println(F(" °C"));
    }
}

/**
 * @brief Print complete status
 */
void PB7200P80::printStatus() {
    uint8_t status = getStatus();
    uint8_t faults = getFaultStatus();
    
    Serial.println(F("System Status:"));
    Serial.print(F("  Overvoltage: "));
    Serial.println((faults & PB7200_STATUS_OVP) ? F("YES") : F("NO"));
    Serial.print(F("  Undervoltage: "));
    Serial.println((faults & PB7200_STATUS_UVP) ? F("YES") : F("NO"));
    Serial.print(F("  Overcurrent: "));
    Serial.println((faults & PB7200_STATUS_OCP) ? F("YES") : F("NO"));
    Serial.print(F("  Overtemperature: "));
    Serial.println((faults & PB7200_STATUS_OTP) ? F("YES") : F("NO"));
    Serial.print(F("  Balancing: "));
    Serial.println((status & PB7200_STATUS_BALANCING) ? F("ACTIVE") : F("INACTIVE"));
    Serial.print(F("  Charging: "));
    Serial.println((status & PB7200_STATUS_CHARGING) ? F("YES") : F("NO"));
}

// ========== Private Communication Methods ==========

/**
 * @brief Write a register
 */
bool PB7200P80::writeRegister(uint8_t reg, uint8_t value) {
    if (_interface == PB7200_INTERFACE_I2C) {
        _wire->beginTransmission(_i2cAddress);
        _wire->write(reg);
        _wire->write(value);
        return (_wire->endTransmission() == 0);
    }
    return false;
}

/**
 * @brief Write multiple registers
 */
bool PB7200P80::writeRegisters(uint8_t reg, uint8_t *values, uint8_t length) {
    if (_interface == PB7200_INTERFACE_I2C) {
        _wire->beginTransmission(_i2cAddress);
        _wire->write(reg);
        for (uint8_t i = 0; i < length; i++) {
            _wire->write(values[i]);
        }
        return (_wire->endTransmission() == 0);
    }
    return false;
}

/**
 * @brief Read a register
 */
bool PB7200P80::readRegister(uint8_t reg, uint8_t &value) {
    if (_interface == PB7200_INTERFACE_I2C) {
        _wire->beginTransmission(_i2cAddress);
        _wire->write(reg);
        if (_wire->endTransmission(false) != 0) {
            return false;
        }
        
        if (_wire->requestFrom(_i2cAddress, (uint8_t)1) == 1) {
            value = _wire->read();
            return true;
        }
    }
    return false;
}

/**
 * @brief Read multiple registers
 */
bool PB7200P80::readRegisters(uint8_t reg, uint8_t *values, uint8_t length) {
    if (_interface == PB7200_INTERFACE_I2C) {
        _wire->beginTransmission(_i2cAddress);
        _wire->write(reg);
        if (_wire->endTransmission(false) != 0) {
            return false;
        }
        
        uint8_t received = _wire->requestFrom(_i2cAddress, length);
        if (received == length) {
            for (uint8_t i = 0; i < length; i++) {
                values[i] = _wire->read();
            }
            return true;
        }
    }
    return false;
}

// ========== Helper Methods ==========

uint16_t PB7200P80::voltageToRaw(float voltage) {
    return (uint16_t)(voltage / PB7200_VOLTAGE_LSB);
}

float PB7200P80::rawToVoltage(uint16_t raw) {
    return raw * PB7200_VOLTAGE_LSB;
}

int16_t PB7200P80::currentToRaw(float current) {
    return (int16_t)(current / PB7200_CURRENT_LSB);
}

float PB7200P80::rawToCurrent(int16_t raw) {
    return raw * PB7200_CURRENT_LSB;
}

int16_t PB7200P80::tempToRaw(float temp) {
    return (int16_t)(temp / PB7200_TEMP_LSB);
}

float PB7200P80::rawToTemp(int16_t raw) {
    return raw * PB7200_TEMP_LSB;
}

bool PB7200P80::isValidCellIndex(uint8_t index) {
    return (index < _cellCount);
}

bool PB7200P80::isValidTempIndex(uint8_t index) {
    return (index < PB7200_MAX_TEMPS);
}
