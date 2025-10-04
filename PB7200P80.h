/**
 * @file PB7200P80.h
 * @brief Arduino Library for PB7200P80 AFE
 * @author PB7200P80 Library
 * @version 1.0.0
 * @date 2025-10-04
 * 
 * This library provides complete interface for communication and control
 * of the PB7200P80 chip, an AFE (Analog Front End) for BMS systems.
 * 
 * Main features:
 * - Support for up to 20 cells in series
 * - Individual cell voltage reading
 * - Temperature monitoring (up to 8 sensors)
 * - Current measurement with Hall sensor or shunt
 * - Configurable protections (overvoltage, undervoltage, overcurrent, etc.)
 * - Cell balancing
 * - I2C or UART communication
 */

#ifndef PB7200P80_H
#define PB7200P80_H

#include <Arduino.h>
#include <Wire.h>

// Library version
#define PB7200P80_VERSION "1.0.0"

// Default I2C address
#define PB7200P80_I2C_ADDR 0x55

// Main registers (based on typical AFE)
#define PB7200_REG_DEVICE_ID        0x00
#define PB7200_REG_STATUS           0x01
#define PB7200_REG_FAULT_STATUS     0x02
#define PB7200_REG_CELL_VOLTAGE_BASE 0x10  // 0x10-0x23 (20 células)
#define PB7200_REG_TEMP_BASE        0x30  // 0x30-0x37 (8 sensores)
#define PB7200_REG_CURRENT_H        0x40
#define PB7200_REG_CURRENT_L        0x41
#define PB7200_REG_BALANCE_CTRL1    0x50
#define PB7200_REG_BALANCE_CTRL2    0x51
#define PB7200_REG_BALANCE_CTRL3    0x52
#define PB7200_REG_CONFIG_OVP       0x60  // Over Voltage Protection
#define PB7200_REG_CONFIG_UVP       0x61  // Under Voltage Protection
#define PB7200_REG_CONFIG_OCP       0x62  // Over Current Protection
#define PB7200_REG_CONFIG_OTP       0x63  // Over Temperature Protection
#define PB7200_REG_CONFIG_UTP       0x64  // Under Temperature Protection
#define PB7200_REG_CONTROL          0x70
#define PB7200_REG_ADC_CTRL         0x71
#define PB7200_REG_SHUTDOWN         0x72

// Constants
#define PB7200_MAX_CELLS 20
#define PB7200_MAX_TEMPS 8
#define PB7200_VOLTAGE_LSB 0.001  // 1mV per bit
#define PB7200_CURRENT_LSB 0.01   // 10mA per bit
#define PB7200_TEMP_LSB 0.1       // 0.1°C per bit

// Status bits
#define PB7200_STATUS_OVP    (1 << 0)  // Overvoltage
#define PB7200_STATUS_UVP    (1 << 1)  // Undervoltage
#define PB7200_STATUS_OCP    (1 << 2)  // Overcurrent
#define PB7200_STATUS_OTP    (1 << 3)  // Overtemperature
#define PB7200_STATUS_UTP    (1 << 4)  // Undertemperature
#define PB7200_STATUS_BALANCING (1 << 5)  // Balancing active
#define PB7200_STATUS_CHARGING  (1 << 6)  // Charging
#define PB7200_STATUS_READY     (1 << 7)  // Ready

// Operation modes
enum PB7200_Mode {
    PB7200_MODE_NORMAL = 0,
    PB7200_MODE_SLEEP = 1,
    PB7200_MODE_SHUTDOWN = 2
};

// Communication interface
enum PB7200_Interface {
    PB7200_INTERFACE_I2C = 0,
    PB7200_INTERFACE_UART = 1
};

/**
 * @brief Structure to store cell data
 */
struct CellData {
    float voltage;      // Voltage in volts
    bool balancing;     // If being balanced
    bool overvoltage;   // Overvoltage detected
    bool undervoltage;  // Undervoltage detected
};

/**
 * @brief Structure to store protection configuration
 */
struct ProtectionConfig {
    float overVoltageThreshold;     // Overvoltage threshold (V)
    float underVoltageThreshold;    // Undervoltage threshold (V)
    float overCurrentThreshold;     // Overcurrent threshold (A)
    float overTempThreshold;        // Overtemperature threshold (°C)
    float underTempThreshold;       // Undertemperature threshold (°C)
    uint16_t overVoltageDelay;      // Overvoltage delay (ms)
    uint16_t underVoltageDelay;     // Undervoltage delay (ms)
    uint16_t overCurrentDelay;      // Overcurrent delay (ms)
};

/**
 * @brief Structure for pack statistics
 */
struct PackStats {
    float totalVoltage;      // Total pack voltage
    float maxCellVoltage;    // Maximum cell voltage
    float minCellVoltage;    // Minimum cell voltage
    float avgCellVoltage;    // Average cell voltage
    float voltageDelta;      // Max-min difference
    uint8_t maxCellIndex;    // Index of cell with highest voltage
    uint8_t minCellIndex;    // Index of cell with lowest voltage
    float current;           // Current (A)
    float power;             // Power (W)
    float maxTemp;           // Maximum temperature
    float minTemp;           // Minimum temperature
    uint8_t maxTempIndex;    // Index of sensor with highest temperature
    uint8_t minTempIndex;    // Index of sensor with lowest temperature
};

/**
 * @brief Main class of PB7200P80 library
 */
class PB7200P80 {
public:
    /**
     * @brief Class constructor
     * @param interface Communication interface (I2C or UART)
     * @param address I2C address (default: 0x55) or RX pin for UART
     * @param wire Pointer to Wire object (I2C only)
     */
    PB7200P80(PB7200_Interface interface = PB7200_INTERFACE_I2C, 
              uint8_t address = PB7200P80_I2C_ADDR,
              TwoWire *wire = &Wire);

    /**
     * @brief Initialize communication with PB7200P80
     * @param cellCount Number of connected cells (1-20)
     * @return true if successfully initialized
     */
    bool begin(uint8_t cellCount = 4);

    /**
     * @brief Check if device is responding
     * @return true if device is connected
     */
    bool isConnected();

    /**
     * @brief Read device ID
     * @return Device ID
     */
    uint8_t getDeviceID();

    // ========== Voltage Reading ==========
    
    /**
     * @brief Read voltage of a specific cell
     * @param cellIndex Cell index (0-19)
     * @return Voltage in volts (0.0 if error)
     */
    float getCellVoltage(uint8_t cellIndex);

    /**
     * @brief Read all cell voltages
     * @param voltages Array to store voltages
     * @param count Number of cells to read
     * @return true if successful
     */
    bool getAllCellVoltages(float *voltages, uint8_t count);

    /**
     * @brief Read complete cell data
     * @param cellIndex Cell index
     * @param data Structure to store data
     * @return true if successful
     */
    bool getCellData(uint8_t cellIndex, CellData &data);

    /**
     * @brief Get total pack voltage
     * @return Total voltage in volts
     */
    float getTotalVoltage();

    /**
     * @brief Get maximum cell voltage
     * @return Maximum voltage in volts
     */
    float getMaxCellVoltage();

    /**
     * @brief Get minimum cell voltage
     * @return Minimum voltage in volts
     */
    float getMinCellVoltage();

    /**
     * @brief Get difference between max and min voltage
     * @return Voltage difference in volts
     */
    float getVoltageDelta();

    // ========== Temperature Reading ==========
    
    /**
     * @brief Read temperature from specific sensor
     * @param tempIndex Sensor index (0-7)
     * @return Temperature in °C
     */
    float getTemperature(uint8_t tempIndex);

    /**
     * @brief Read all temperatures
     * @param temperatures Array to store temperatures
     * @param count Number of sensors to read
     * @return true if successful
     */
    bool getAllTemperatures(float *temperatures, uint8_t count);

    /**
     * @brief Get maximum temperature
     * @return Maximum temperature in °C
     */
    float getMaxTemperature();

    /**
     * @brief Get minimum temperature
     * @return Minimum temperature in °C
     */
    float getMinTemperature();

    // ========== Current Reading ==========
    
    /**
     * @brief Read pack current
     * @return Current in amperes (positive = charging, negative = discharging)
     */
    float getCurrent();

    /**
     * @brief Read pack power
     * @return Power in watts
     */
    float getPower();

    // ========== Status and Protections ==========
    
    /**
     * @brief Read status register
     * @return Status byte
     */
    uint8_t getStatus();

    /**
     * @brief Read fault register
     * @return Fault byte
     */
    uint8_t getFaultStatus();

    /**
     * @brief Check for overvoltage
     * @return true if overvoltage detected
     */
    bool isOverVoltage();

    /**
     * @brief Check for undervoltage
     * @return true if undervoltage detected
     */
    bool isUnderVoltage();

    /**
     * @brief Check for overcurrent
     * @return true if overcurrent detected
     */
    bool isOverCurrent();

    /**
     * @brief Check for overtemperature
     * @return true if overtemperature detected
     */
    bool isOverTemperature();

    /**
     * @brief Check for undertemperature
     * @return true if undertemperature detected
     */
    bool isUnderTemperature();

    /**
     * @brief Clear fault flags
     * @return true if successful
     */
    bool clearFaults();

    // ========== Cell Balancing ==========
    
    /**
     * @brief Enable cell balancing
     * @param cellIndex Cell index (0-19)
     * @param enable true to enable, false to disable
     * @return true if successful
     */
    bool setBalancing(uint8_t cellIndex, bool enable);

    /**
     * @brief Enable automatic balancing
     * @param enable true to enable
     * @param threshold Voltage difference to start balancing (mV)
     * @return true if successful
     */
    bool setAutoBalancing(bool enable, uint16_t threshold = 50);

    /**
     * @brief Check if a cell is being balanced
     * @param cellIndex Cell index
     * @return true if balancing
     */
    bool isBalancing(uint8_t cellIndex);

    /**
     * @brief Disable balancing for all cells
     * @return true if successful
     */
    bool stopAllBalancing();

    // ========== Configuration ==========
    
    /**
     * @brief Configure protections
     * @param config Structure with protection settings
     * @return true if successful
     */
    bool setProtectionConfig(const ProtectionConfig &config);

    /**
     * @brief Read protection configuration
     * @param config Structure to store settings
     * @return true if successful
     */
    bool getProtectionConfig(ProtectionConfig &config);

    /**
     * @brief Set operation mode
     * @param mode Operation mode
     * @return true if successful
     */
    bool setMode(PB7200_Mode mode);

    /**
     * @brief Reset device
     * @return true if successful
     */
    bool reset();

    /**
     * @brief Enter sleep mode
     * @return true if successful
     */
    bool sleep();

    /**
     * @brief Wake up from sleep mode
     * @return true if successful
     */
    bool wakeup();

    /**
     * @brief Shutdown device
     * @return true if successful
     */
    bool shutdown();

    // ========== Statistics ==========
    
    /**
     * @brief Get complete pack statistics
     * @param stats Structure to store statistics
     * @return true if successful
     */
    bool getPackStats(PackStats &stats);

    /**
     * @brief Update all readings (optimized)
     * @return true if successful
     */
    bool update();

    // ========== Diagnostics ==========
    
    /**
     * @brief Run self-test
     * @return true if passed
     */
    bool selfTest();

    /**
     * @brief Print diagnostic information
     */
    void printDiagnostics();

    /**
     * @brief Print all cell voltages
     */
    void printCellVoltages();

    /**
     * @brief Print all temperatures
     */
    void printTemperatures();

    /**
     * @brief Print complete status
     */
    void printStatus();

private:
    // Hardware configuration
    PB7200_Interface _interface;
    uint8_t _i2cAddress;
    TwoWire *_wire;
    HardwareSerial *_serial;
    
    // Pack configuration
    uint8_t _cellCount;
    uint8_t _tempSensorCount;
    
    // Data cache
    float _cellVoltages[PB7200_MAX_CELLS];
    float _temperatures[PB7200_MAX_TEMPS];
    float _current;
    uint8_t _status;
    uint8_t _faultStatus;
    unsigned long _lastUpdate;
    
    // Private communication methods
    bool writeRegister(uint8_t reg, uint8_t value);
    bool writeRegisters(uint8_t reg, uint8_t *values, uint8_t length);
    bool readRegister(uint8_t reg, uint8_t &value);
    bool readRegisters(uint8_t reg, uint8_t *values, uint8_t length);
    
    // Helper methods
    uint16_t voltageToRaw(float voltage);
    float rawToVoltage(uint16_t raw);
    int16_t currentToRaw(float current);
    float rawToCurrent(int16_t raw);
    int16_t tempToRaw(float temp);
    float rawToTemp(int16_t raw);
    
    // Validation
    bool isValidCellIndex(uint8_t index);
    bool isValidTempIndex(uint8_t index);
};

#endif // PB7200P80_H
