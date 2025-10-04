/**
 * @file PB7200P80.h
 * @brief Biblioteca Arduino para o AFE PB7200P80
 * @author Biblioteca PB7200P80
 * @version 1.0.0
 * @date 2025-10-04
 * 
 * Esta biblioteca fornece interface completa para comunicação e controle
 * do chip PB7200P80, um AFE (Analog Front End) para sistemas BMS.
 * 
 * Características principais:
 * - Suporte para até 20 células em série
 * - Leitura de tensão individual de células
 * - Monitoramento de temperatura (até 8 sensores)
 * - Medição de corrente com sensor Hall ou shunt
 * - Proteções configuráveis (sobrecarga, subcarga, sobrecorrente, etc.)
 * - Balanceamento de células
 * - Comunicação I2C ou UART
 */

#ifndef PB7200P80_H
#define PB7200P80_H

#include <Arduino.h>
#include <Wire.h>

// Versão da biblioteca
#define PB7200P80_VERSION "1.0.0"

// Endereço I2C padrão
#define PB7200P80_I2C_ADDR 0x55

// Registradores principais (baseado em AFE típico)
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

// Constantes
#define PB7200_MAX_CELLS 20
#define PB7200_MAX_TEMPS 8
#define PB7200_VOLTAGE_LSB 0.001  // 1mV por bit
#define PB7200_CURRENT_LSB 0.01   // 10mA por bit
#define PB7200_TEMP_LSB 0.1       // 0.1°C por bit

// Status bits
#define PB7200_STATUS_OVP    (1 << 0)  // Sobretensão
#define PB7200_STATUS_UVP    (1 << 1)  // Subtensão
#define PB7200_STATUS_OCP    (1 << 2)  // Sobrecorrente
#define PB7200_STATUS_OTP    (1 << 3)  // Sobretemperatura
#define PB7200_STATUS_UTP    (1 << 4)  // Subtemperatura
#define PB7200_STATUS_BALANCING (1 << 5)  // Balanceamento ativo
#define PB7200_STATUS_CHARGING  (1 << 6)  // Carregando
#define PB7200_STATUS_READY     (1 << 7)  // Pronto

// Modos de operação
enum PB7200_Mode {
    PB7200_MODE_NORMAL = 0,
    PB7200_MODE_SLEEP = 1,
    PB7200_MODE_SHUTDOWN = 2
};

// Interface de comunicação
enum PB7200_Interface {
    PB7200_INTERFACE_I2C = 0,
    PB7200_INTERFACE_UART = 1
};

/**
 * @brief Estrutura para armazenar dados de uma célula
 */
struct CellData {
    float voltage;      // Tensão em volts
    bool balancing;     // Se está sendo balanceada
    bool overvoltage;   // Sobretensão detectada
    bool undervoltage;  // Subtensão detectada
};

/**
 * @brief Estrutura para armazenar configurações de proteção
 */
struct ProtectionConfig {
    float overVoltageThreshold;     // Limite de sobretensão (V)
    float underVoltageThreshold;    // Limite de subtensão (V)
    float overCurrentThreshold;     // Limite de sobrecorrente (A)
    float overTempThreshold;        // Limite de sobretemperatura (°C)
    float underTempThreshold;       // Limite de subtemperatura (°C)
    uint16_t overVoltageDelay;      // Atraso de sobretensão (ms)
    uint16_t underVoltageDelay;     // Atraso de subtensão (ms)
    uint16_t overCurrentDelay;      // Atraso de sobrecorrente (ms)
};

/**
 * @brief Estrutura para estatísticas do pack
 */
struct PackStats {
    float totalVoltage;      // Tensão total do pack
    float maxCellVoltage;    // Maior tensão de célula
    float minCellVoltage;    // Menor tensão de célula
    float avgCellVoltage;    // Tensão média das células
    float voltageDelta;      // Diferença max-min
    uint8_t maxCellIndex;    // Índice da célula com maior tensão
    uint8_t minCellIndex;    // Índice da célula com menor tensão
    float current;           // Corrente (A)
    float power;             // Potência (W)
    float maxTemp;           // Temperatura máxima
    float minTemp;           // Temperatura mínima
    uint8_t maxTempIndex;    // Índice do sensor com maior temperatura
    uint8_t minTempIndex;    // Índice do sensor com menor temperatura
};

/**
 * @brief Classe principal da biblioteca PB7200P80
 */
class PB7200P80 {
public:
    /**
     * @brief Construtor da classe
     * @param interface Interface de comunicação (I2C ou UART)
     * @param address Endereço I2C (padrão: 0x55) ou pino RX para UART
     * @param wire Ponteiro para objeto Wire (apenas para I2C)
     */
    PB7200P80(PB7200_Interface interface = PB7200_INTERFACE_I2C, 
              uint8_t address = PB7200P80_I2C_ADDR,
              TwoWire *wire = &Wire);

    /**
     * @brief Inicializa a comunicação com o PB7200P80
     * @param cellCount Número de células conectadas (1-20)
     * @return true se inicializado com sucesso
     */
    bool begin(uint8_t cellCount = 4);

    /**
     * @brief Verifica se o dispositivo está respondendo
     * @return true se o dispositivo está conectado
     */
    bool isConnected();

    /**
     * @brief Lê o ID do dispositivo
     * @return ID do dispositivo
     */
    uint8_t getDeviceID();

    // ========== Leitura de Tensões ==========
    
    /**
     * @brief Lê a tensão de uma célula específica
     * @param cellIndex Índice da célula (0-19)
     * @return Tensão em volts (0.0 se erro)
     */
    float getCellVoltage(uint8_t cellIndex);

    /**
     * @brief Lê todas as tensões das células
     * @param voltages Array para armazenar as tensões
     * @param count Número de células a ler
     * @return true se sucesso
     */
    bool getAllCellVoltages(float *voltages, uint8_t count);

    /**
     * @brief Lê dados completos de uma célula
     * @param cellIndex Índice da célula
     * @param data Estrutura para armazenar os dados
     * @return true se sucesso
     */
    bool getCellData(uint8_t cellIndex, CellData &data);

    /**
     * @brief Obtém a tensão total do pack
     * @return Tensão total em volts
     */
    float getTotalVoltage();

    /**
     * @brief Obtém a maior tensão entre as células
     * @return Maior tensão em volts
     */
    float getMaxCellVoltage();

    /**
     * @brief Obtém a menor tensão entre as células
     * @return Menor tensão em volts
     */
    float getMinCellVoltage();

    /**
     * @brief Obtém a diferença entre maior e menor tensão
     * @return Diferença de tensão em volts
     */
    float getVoltageDelta();

    // ========== Leitura de Temperatura ==========
    
    /**
     * @brief Lê a temperatura de um sensor específico
     * @param tempIndex Índice do sensor (0-7)
     * @return Temperatura em °C
     */
    float getTemperature(uint8_t tempIndex);

    /**
     * @brief Lê todas as temperaturas
     * @param temperatures Array para armazenar as temperaturas
     * @param count Número de sensores a ler
     * @return true se sucesso
     */
    bool getAllTemperatures(float *temperatures, uint8_t count);

    /**
     * @brief Obtém a temperatura máxima
     * @return Temperatura máxima em °C
     */
    float getMaxTemperature();

    /**
     * @brief Obtém a temperatura mínima
     * @return Temperatura mínima em °C
     */
    float getMinTemperature();

    // ========== Leitura de Corrente ==========
    
    /**
     * @brief Lê a corrente do pack
     * @return Corrente em amperes (positivo = carga, negativo = descarga)
     */
    float getCurrent();

    /**
     * @brief Lê a potência do pack
     * @return Potência em watts
     */
    float getPower();

    // ========== Status e Proteções ==========
    
    /**
     * @brief Lê o registro de status
     * @return Byte de status
     */
    uint8_t getStatus();

    /**
     * @brief Lê o registro de falhas
     * @return Byte de falhas
     */
    uint8_t getFaultStatus();

    /**
     * @brief Verifica se há sobretensão
     * @return true se sobretensão detectada
     */
    bool isOverVoltage();

    /**
     * @brief Verifica se há subtensão
     * @return true se subtensão detectada
     */
    bool isUnderVoltage();

    /**
     * @brief Verifica se há sobrecorrente
     * @return true se sobrecorrente detectada
     */
    bool isOverCurrent();

    /**
     * @brief Verifica se há sobretemperatura
     * @return true se sobretemperatura detectada
     */
    bool isOverTemperature();

    /**
     * @brief Verifica se há subtemperatura
     * @return true se subtemperatura detectada
     */
    bool isUnderTemperature();

    /**
     * @brief Limpa flags de falha
     * @return true se sucesso
     */
    bool clearFaults();

    // ========== Balanceamento ==========
    
    /**
     * @brief Habilita balanceamento de uma célula
     * @param cellIndex Índice da célula (0-19)
     * @param enable true para habilitar, false para desabilitar
     * @return true se sucesso
     */
    bool setBalancing(uint8_t cellIndex, bool enable);

    /**
     * @brief Habilita balanceamento automático
     * @param enable true para habilitar
     * @param threshold Diferença de tensão para iniciar balanceamento (mV)
     * @return true se sucesso
     */
    bool setAutoBalancing(bool enable, uint16_t threshold = 50);

    /**
     * @brief Verifica se uma célula está sendo balanceada
     * @param cellIndex Índice da célula
     * @return true se está balanceando
     */
    bool isBalancing(uint8_t cellIndex);

    /**
     * @brief Desabilita balanceamento de todas as células
     * @return true se sucesso
     */
    bool stopAllBalancing();

    // ========== Configuração ==========
    
    /**
     * @brief Configura proteções
     * @param config Estrutura com configurações de proteção
     * @return true se sucesso
     */
    bool setProtectionConfig(const ProtectionConfig &config);

    /**
     * @brief Lê configurações de proteção
     * @param config Estrutura para armazenar configurações
     * @return true se sucesso
     */
    bool getProtectionConfig(ProtectionConfig &config);

    /**
     * @brief Define o modo de operação
     * @param mode Modo de operação
     * @return true se sucesso
     */
    bool setMode(PB7200_Mode mode);

    /**
     * @brief Reinicia o dispositivo
     * @return true se sucesso
     */
    bool reset();

    /**
     * @brief Entra em modo sleep
     * @return true se sucesso
     */
    bool sleep();

    /**
     * @brief Acorda do modo sleep
     * @return true se sucesso
     */
    bool wakeup();

    /**
     * @brief Desliga o dispositivo
     * @return true se sucesso
     */
    bool shutdown();

    // ========== Estatísticas ==========
    
    /**
     * @brief Obtém estatísticas completas do pack
     * @param stats Estrutura para armazenar estatísticas
     * @return true se sucesso
     */
    bool getPackStats(PackStats &stats);

    /**
     * @brief Atualiza todas as leituras (otimizado)
     * @return true se sucesso
     */
    bool update();

    // ========== Diagnóstico ==========
    
    /**
     * @brief Executa autoteste
     * @return true se passou no teste
     */
    bool selfTest();

    /**
     * @brief Imprime informações de diagnóstico
     */
    void printDiagnostics();

    /**
     * @brief Imprime todas as tensões das células
     */
    void printCellVoltages();

    /**
     * @brief Imprime todas as temperaturas
     */
    void printTemperatures();

    /**
     * @brief Imprime status completo
     */
    void printStatus();

private:
    // Configuração de hardware
    PB7200_Interface _interface;
    uint8_t _i2cAddress;
    TwoWire *_wire;
    HardwareSerial *_serial;
    
    // Configuração do pack
    uint8_t _cellCount;
    uint8_t _tempSensorCount;
    
    // Cache de dados
    float _cellVoltages[PB7200_MAX_CELLS];
    float _temperatures[PB7200_MAX_TEMPS];
    float _current;
    uint8_t _status;
    uint8_t _faultStatus;
    unsigned long _lastUpdate;
    
    // Métodos privados de comunicação
    bool writeRegister(uint8_t reg, uint8_t value);
    bool writeRegisters(uint8_t reg, uint8_t *values, uint8_t length);
    bool readRegister(uint8_t reg, uint8_t &value);
    bool readRegisters(uint8_t reg, uint8_t *values, uint8_t length);
    
    // Métodos auxiliares
    uint16_t voltageToRaw(float voltage);
    float rawToVoltage(uint16_t raw);
    int16_t currentToRaw(float current);
    float rawToCurrent(int16_t raw);
    int16_t tempToRaw(float temp);
    float rawToTemp(int16_t raw);
    
    // Validação
    bool isValidCellIndex(uint8_t index);
    bool isValidTempIndex(uint8_t index);
};

#endif // PB7200P80_H
