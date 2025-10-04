/**
 * @file PB7200P80.cpp
 * @brief Implementação da biblioteca Arduino para o AFE PB7200P80
 * @author Biblioteca PB7200P80
 * @version 1.0.0
 * @date 2025-10-04
 */

#include "PB7200P80.h"

/**
 * @brief Construtor da classe
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
    
    // Inicializa arrays
    for (uint8_t i = 0; i < PB7200_MAX_CELLS; i++) {
        _cellVoltages[i] = 0.0;
    }
    for (uint8_t i = 0; i < PB7200_MAX_TEMPS; i++) {
        _temperatures[i] = 0.0;
    }
}

/**
 * @brief Inicializa a comunicação com o PB7200P80
 */
bool PB7200P80::begin(uint8_t cellCount) {
    if (cellCount == 0 || cellCount > PB7200_MAX_CELLS) {
        return false;
    }
    
    _cellCount = cellCount;
    
    // Inicializa interface de comunicação
    if (_interface == PB7200_INTERFACE_I2C) {
        _wire->begin();
        _wire->setClock(100000); // 100kHz padrão
    } else {
        // TODO: Implementar interface UART se necessário
        return false;
    }
    
    delay(100); // Aguarda estabilização
    
    // Verifica comunicação
    if (!isConnected()) {
        return false;
    }
    
    // Configuração inicial
    uint8_t ctrlValue = 0x01; // Modo normal, ADC habilitado
    if (!writeRegister(PB7200_REG_CONTROL, ctrlValue)) {
        return false;
    }
    
    delay(50);
    
    // Primeira leitura
    update();
    
    return true;
}

/**
 * @brief Verifica se o dispositivo está respondendo
 */
bool PB7200P80::isConnected() {
    uint8_t deviceID;
    if (readRegister(PB7200_REG_DEVICE_ID, deviceID)) {
        return (deviceID != 0x00 && deviceID != 0xFF);
    }
    return false;
}

/**
 * @brief Lê o ID do dispositivo
 */
uint8_t PB7200P80::getDeviceID() {
    uint8_t deviceID = 0;
    readRegister(PB7200_REG_DEVICE_ID, deviceID);
    return deviceID;
}

// ========== Leitura de Tensões ==========

/**
 * @brief Lê a tensão de uma célula específica
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
 * @brief Lê todas as tensões das células
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
 * @brief Lê dados completos de uma célula
 */
bool PB7200P80::getCellData(uint8_t cellIndex, CellData &data) {
    if (!isValidCellIndex(cellIndex)) {
        return false;
    }
    
    data.voltage = getCellVoltage(cellIndex);
    data.balancing = isBalancing(cellIndex);
    
    // Verifica proteções
    uint8_t faults = getFaultStatus();
    data.overvoltage = (faults & PB7200_STATUS_OVP) != 0;
    data.undervoltage = (faults & PB7200_STATUS_UVP) != 0;
    
    return true;
}

/**
 * @brief Obtém a tensão total do pack
 */
float PB7200P80::getTotalVoltage() {
    float total = 0.0;
    for (uint8_t i = 0; i < _cellCount; i++) {
        total += _cellVoltages[i];
    }
    return total;
}

/**
 * @brief Obtém a maior tensão entre as células
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
 * @brief Obtém a menor tensão entre as células
 */
float PB7200P80::getMinCellVoltage() {
    float minVoltage = 5.0; // Valor alto inicial
    for (uint8_t i = 0; i < _cellCount; i++) {
        if (_cellVoltages[i] < minVoltage && _cellVoltages[i] > 0.0) {
            minVoltage = _cellVoltages[i];
        }
    }
    return minVoltage;
}

/**
 * @brief Obtém a diferença entre maior e menor tensão
 */
float PB7200P80::getVoltageDelta() {
    return getMaxCellVoltage() - getMinCellVoltage();
}

// ========== Leitura de Temperatura ==========

/**
 * @brief Lê a temperatura de um sensor específico
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
 * @brief Lê todas as temperaturas
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
 * @brief Obtém a temperatura máxima
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
 * @brief Obtém a temperatura mínima
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

// ========== Leitura de Corrente ==========

/**
 * @brief Lê a corrente do pack
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
 * @brief Lê a potência do pack
 */
float PB7200P80::getPower() {
    return getTotalVoltage() * _current;
}

// ========== Status e Proteções ==========

/**
 * @brief Lê o registro de status
 */
uint8_t PB7200P80::getStatus() {
    readRegister(PB7200_REG_STATUS, _status);
    return _status;
}

/**
 * @brief Lê o registro de falhas
 */
uint8_t PB7200P80::getFaultStatus() {
    readRegister(PB7200_REG_FAULT_STATUS, _faultStatus);
    return _faultStatus;
}

/**
 * @brief Verifica se há sobretensão
 */
bool PB7200P80::isOverVoltage() {
    return (getFaultStatus() & PB7200_STATUS_OVP) != 0;
}

/**
 * @brief Verifica se há subtensão
 */
bool PB7200P80::isUnderVoltage() {
    return (getFaultStatus() & PB7200_STATUS_UVP) != 0;
}

/**
 * @brief Verifica se há sobrecorrente
 */
bool PB7200P80::isOverCurrent() {
    return (getFaultStatus() & PB7200_STATUS_OCP) != 0;
}

/**
 * @brief Verifica se há sobretemperatura
 */
bool PB7200P80::isOverTemperature() {
    return (getFaultStatus() & PB7200_STATUS_OTP) != 0;
}

/**
 * @brief Verifica se há subtemperatura
 */
bool PB7200P80::isUnderTemperature() {
    return (getFaultStatus() & PB7200_STATUS_UTP) != 0;
}

/**
 * @brief Limpa flags de falha
 */
bool PB7200P80::clearFaults() {
    return writeRegister(PB7200_REG_FAULT_STATUS, 0x00);
}

// ========== Balanceamento ==========

/**
 * @brief Habilita balanceamento de uma célula
 */
bool PB7200P80::setBalancing(uint8_t cellIndex, bool enable) {
    if (!isValidCellIndex(cellIndex)) {
        return false;
    }
    
    // Determina qual registro de controle usar (3 registros para 20 células)
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
 * @brief Habilita balanceamento automático
 */
bool PB7200P80::setAutoBalancing(bool enable, uint16_t threshold) {
    // Implementação simplificada - em chip real, consultar datasheet
    if (enable) {
        // Habilita modo automático
        uint8_t ctrlValue;
        if (!readRegister(PB7200_REG_CONTROL, ctrlValue)) {
            return false;
        }
        ctrlValue |= 0x10; // Bit de auto-balanceamento
        return writeRegister(PB7200_REG_CONTROL, ctrlValue);
    } else {
        return stopAllBalancing();
    }
}

/**
 * @brief Verifica se uma célula está sendo balanceada
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
 * @brief Desabilita balanceamento de todas as células
 */
bool PB7200P80::stopAllBalancing() {
    bool success = true;
    success &= writeRegister(PB7200_REG_BALANCE_CTRL1, 0x00);
    success &= writeRegister(PB7200_REG_BALANCE_CTRL2, 0x00);
    success &= writeRegister(PB7200_REG_BALANCE_CTRL3, 0x00);
    return success;
}

// ========== Configuração ==========

/**
 * @brief Configura proteções
 */
bool PB7200P80::setProtectionConfig(const ProtectionConfig &config) {
    bool success = true;
    
    // Converte e escreve limites de tensão
    uint16_t ovpRaw = voltageToRaw(config.overVoltageThreshold);
    uint16_t uvpRaw = voltageToRaw(config.underVoltageThreshold);
    
    success &= writeRegister(PB7200_REG_CONFIG_OVP, (ovpRaw >> 8) & 0xFF);
    success &= writeRegister(PB7200_REG_CONFIG_OVP + 1, ovpRaw & 0xFF);
    success &= writeRegister(PB7200_REG_CONFIG_UVP, (uvpRaw >> 8) & 0xFF);
    success &= writeRegister(PB7200_REG_CONFIG_UVP + 1, uvpRaw & 0xFF);
    
    // Converte e escreve limite de corrente
    int16_t ocpRaw = currentToRaw(config.overCurrentThreshold);
    success &= writeRegister(PB7200_REG_CONFIG_OCP, (ocpRaw >> 8) & 0xFF);
    success &= writeRegister(PB7200_REG_CONFIG_OCP + 1, ocpRaw & 0xFF);
    
    // Converte e escreve limites de temperatura
    int16_t otpRaw = tempToRaw(config.overTempThreshold);
    int16_t utpRaw = tempToRaw(config.underTempThreshold);
    
    success &= writeRegister(PB7200_REG_CONFIG_OTP, (otpRaw >> 8) & 0xFF);
    success &= writeRegister(PB7200_REG_CONFIG_OTP + 1, otpRaw & 0xFF);
    success &= writeRegister(PB7200_REG_CONFIG_UTP, (utpRaw >> 8) & 0xFF);
    success &= writeRegister(PB7200_REG_CONFIG_UTP + 1, utpRaw & 0xFF);
    
    return success;
}

/**
 * @brief Lê configurações de proteção
 */
bool PB7200P80::getProtectionConfig(ProtectionConfig &config) {
    uint8_t data[2];
    bool success = true;
    
    // Lê limites de tensão
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
    
    // Lê limite de corrente
    if (readRegisters(PB7200_REG_CONFIG_OCP, data, 2)) {
        int16_t raw = (data[0] << 8) | data[1];
        config.overCurrentThreshold = rawToCurrent(raw);
    } else {
        success = false;
    }
    
    // Lê limites de temperatura
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
 * @brief Define o modo de operação
 */
bool PB7200P80::setMode(PB7200_Mode mode) {
    uint8_t ctrlValue;
    if (!readRegister(PB7200_REG_CONTROL, ctrlValue)) {
        return false;
    }
    
    // Limpa bits de modo e define novo modo
    ctrlValue &= 0xFC;
    ctrlValue |= (mode & 0x03);
    
    return writeRegister(PB7200_REG_CONTROL, ctrlValue);
}

/**
 * @brief Reinicia o dispositivo
 */
bool PB7200P80::reset() {
    // Escreve comando de reset
    if (writeRegister(PB7200_REG_CONTROL, 0x80)) {
        delay(100);
        return begin(_cellCount);
    }
    return false;
}

/**
 * @brief Entra em modo sleep
 */
bool PB7200P80::sleep() {
    return setMode(PB7200_MODE_SLEEP);
}

/**
 * @brief Acorda do modo sleep
 */
bool PB7200P80::wakeup() {
    return setMode(PB7200_MODE_NORMAL);
}

/**
 * @brief Desliga o dispositivo
 */
bool PB7200P80::shutdown() {
    return writeRegister(PB7200_REG_SHUTDOWN, 0x01);
}

// ========== Estatísticas ==========

/**
 * @brief Obtém estatísticas completas do pack
 */
bool PB7200P80::getPackStats(PackStats &stats) {
    // Atualiza leituras
    if (!update()) {
        return false;
    }
    
    stats.totalVoltage = 0.0;
    stats.maxCellVoltage = 0.0;
    stats.minCellVoltage = 5.0;
    stats.avgCellVoltage = 0.0;
    stats.maxCellIndex = 0;
    stats.minCellIndex = 0;
    
    // Processa tensões
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
    
    // Processa temperaturas
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
    
    // Corrente e potência
    stats.current = _current;
    stats.power = stats.totalVoltage * _current;
    
    return true;
}

/**
 * @brief Atualiza todas as leituras (otimizado)
 */
bool PB7200P80::update() {
    bool success = true;
    
    // Lê todas as tensões
    float voltages[PB7200_MAX_CELLS];
    if (getAllCellVoltages(voltages, _cellCount)) {
        for (uint8_t i = 0; i < _cellCount; i++) {
            _cellVoltages[i] = voltages[i];
        }
    } else {
        success = false;
    }
    
    // Lê todas as temperaturas
    float temps[PB7200_MAX_TEMPS];
    if (getAllTemperatures(temps, _tempSensorCount)) {
        for (uint8_t i = 0; i < _tempSensorCount; i++) {
            _temperatures[i] = temps[i];
        }
    } else {
        success = false;
    }
    
    // Lê corrente
    getCurrent();
    
    // Lê status
    getStatus();
    getFaultStatus();
    
    _lastUpdate = millis();
    
    return success;
}

// ========== Diagnóstico ==========

/**
 * @brief Executa autoteste
 */
bool PB7200P80::selfTest() {
    // Verifica comunicação
    if (!isConnected()) {
        Serial.println(F("FALHA: Sem comunicação com PB7200P80"));
        return false;
    }
    
    Serial.println(F("OK: Comunicação estabelecida"));
    
    // Verifica ID do dispositivo
    uint8_t deviceID = getDeviceID();
    Serial.print(F("Device ID: 0x"));
    Serial.println(deviceID, HEX);
    
    // Verifica leitura de tensões
    update();
    float totalV = getTotalVoltage();
    if (totalV < 0.1) {
        Serial.println(F("AVISO: Tensão total muito baixa"));
    } else {
        Serial.print(F("OK: Tensão total = "));
        Serial.print(totalV);
        Serial.println(F(" V"));
    }
    
    return true;
}

/**
 * @brief Imprime informações de diagnóstico
 */
void PB7200P80::printDiagnostics() {
    Serial.println(F("========== Diagnóstico PB7200P80 =========="));
    Serial.print(F("Device ID: 0x"));
    Serial.println(getDeviceID(), HEX);
    Serial.print(F("Células configuradas: "));
    Serial.println(_cellCount);
    Serial.print(F("Status: 0x"));
    Serial.println(getStatus(), HEX);
    Serial.print(F("Falhas: 0x"));
    Serial.println(getFaultStatus(), HEX);
    Serial.println();
    
    printCellVoltages();
    Serial.println();
    printTemperatures();
    Serial.println();
    
    Serial.print(F("Corrente: "));
    Serial.print(_current, 3);
    Serial.println(F(" A"));
    Serial.print(F("Potência: "));
    Serial.print(getPower(), 2);
    Serial.println(F(" W"));
    Serial.println(F("=========================================="));
}

/**
 * @brief Imprime todas as tensões das células
 */
void PB7200P80::printCellVoltages() {
    Serial.println(F("Tensões das Células:"));
    for (uint8_t i = 0; i < _cellCount; i++) {
        Serial.print(F("  Célula "));
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
 * @brief Imprime todas as temperaturas
 */
void PB7200P80::printTemperatures() {
    Serial.println(F("Temperaturas:"));
    for (uint8_t i = 0; i < _tempSensorCount; i++) {
        Serial.print(F("  Sensor "));
        Serial.print(i + 1);
        Serial.print(F(": "));
        Serial.print(_temperatures[i], 1);
        Serial.println(F(" °C"));
    }
}

/**
 * @brief Imprime status completo
 */
void PB7200P80::printStatus() {
    uint8_t status = getStatus();
    uint8_t faults = getFaultStatus();
    
    Serial.println(F("Status do Sistema:"));
    Serial.print(F("  Sobretensão: "));
    Serial.println((faults & PB7200_STATUS_OVP) ? F("SIM") : F("NÃO"));
    Serial.print(F("  Subtensão: "));
    Serial.println((faults & PB7200_STATUS_UVP) ? F("SIM") : F("NÃO"));
    Serial.print(F("  Sobrecorrente: "));
    Serial.println((faults & PB7200_STATUS_OCP) ? F("SIM") : F("NÃO"));
    Serial.print(F("  Sobretemperatura: "));
    Serial.println((faults & PB7200_STATUS_OTP) ? F("SIM") : F("NÃO"));
    Serial.print(F("  Balanceamento: "));
    Serial.println((status & PB7200_STATUS_BALANCING) ? F("ATIVO") : F("INATIVO"));
    Serial.print(F("  Carregando: "));
    Serial.println((status & PB7200_STATUS_CHARGING) ? F("SIM") : F("NÃO"));
}

// ========== Métodos Privados de Comunicação ==========

/**
 * @brief Escreve um registro
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
 * @brief Escreve múltiplos registros
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
 * @brief Lê um registro
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
 * @brief Lê múltiplos registros
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

// ========== Métodos Auxiliares ==========

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
