/**
 * BasicMonitoring.ino
 * 
 * Exemplo básico de monitoramento do PB7200P80
 * 
 * Este exemplo demonstra:
 * - Inicialização do PB7200P80
 * - Leitura de tensões de células
 * - Leitura de temperaturas
 * - Leitura de corrente
 * - Exibição de dados no Serial Monitor
 * 
 * Hardware:
 * - Arduino (qualquer placa)
 * - PB7200P80 conectado via I2C
 *   - SDA -> A4 (Uno/Nano) ou SDA (outras placas)
 *   - SCL -> A5 (Uno/Nano) ou SCL (outras placas)
 *   - VCC -> 3.3V ou 5V
 *   - GND -> GND
 */

#include <PB7200P80.h>

// Cria objeto PB7200P80 com interface I2C no endereço padrão 0x55
PB7200P80 bms(PB7200_INTERFACE_I2C, 0x55);

// Configuração do número de células
const uint8_t NUM_CELLS = 4;  // Altere conforme seu pack

void setup() {
  // Inicializa Serial
  Serial.begin(115200);
  while (!Serial) delay(10);
  
  Serial.println(F("==========================================="));
  Serial.println(F("    PB7200P80 - Monitoramento Básico       "));
  Serial.println(F("==========================================="));
  Serial.println();
  
  // Inicializa PB7200P80
  Serial.print(F("Inicializando PB7200P80 com "));
  Serial.print(NUM_CELLS);
  Serial.println(F(" células..."));
  
  if (!bms.begin(NUM_CELLS)) {
    Serial.println(F("ERRO: Falha ao inicializar PB7200P80!"));
    Serial.println(F("Verifique as conexões:"));
    Serial.println(F("  - SDA/SCL conectados corretamente"));
    Serial.println(F("  - Alimentação do PB7200P80"));
    Serial.println(F("  - Endereço I2C correto"));
    while (1) delay(1000);
  }
  
  Serial.println(F("PB7200P80 inicializado com sucesso!"));
  Serial.println();
  
  // Executa autoteste
  bms.selfTest();
  Serial.println();
  
  delay(1000);
}

void loop() {
  // Atualiza todas as leituras
  bms.update();
  
  // Cabeçalho
  Serial.println(F("========================================"));
  Serial.println(F("          LEITURAS DO BMS               "));
  Serial.println(F("========================================"));
  
  // Tensões das células
  Serial.println(F("\nTensões das Células:"));
  Serial.println(F("----------------------------------------"));
  for (uint8_t i = 0; i < NUM_CELLS; i++) {
    float voltage = bms.getCellVoltage(i);
    Serial.print(F("Célula "));
    Serial.print(i + 1);
    Serial.print(F(": "));
    Serial.print(voltage, 3);
    Serial.println(F(" V"));
  }
  
  // Estatísticas de tensão
  Serial.println(F("\nEstatísticas de Tensão:"));
  Serial.println(F("----------------------------------------"));
  Serial.print(F("Tensão Total:   "));
  Serial.print(bms.getTotalVoltage(), 3);
  Serial.println(F(" V"));
  
  Serial.print(F("Tensão Máxima:  "));
  Serial.print(bms.getMaxCellVoltage(), 3);
  Serial.println(F(" V"));
  
  Serial.print(F("Tensão Mínima:  "));
  Serial.print(bms.getMinCellVoltage(), 3);
  Serial.println(F(" V"));
  
  Serial.print(F("Delta:          "));
  Serial.print(bms.getVoltageDelta() * 1000, 1);
  Serial.println(F(" mV"));
  
  // Temperaturas
  Serial.println(F("\nTemperaturas:"));
  Serial.println(F("----------------------------------------"));
  for (uint8_t i = 0; i < 4; i++) {  // Mostra 4 sensores
    float temp = bms.getTemperature(i);
    Serial.print(F("Sensor "));
    Serial.print(i + 1);
    Serial.print(F(": "));
    Serial.print(temp, 1);
    Serial.println(F(" °C"));
  }
  
  Serial.print(F("Temp. Máxima:   "));
  Serial.print(bms.getMaxTemperature(), 1);
  Serial.println(F(" °C"));
  
  Serial.print(F("Temp. Mínima:   "));
  Serial.print(bms.getMinTemperature(), 1);
  Serial.println(F(" °C"));
  
  // Corrente e Potência
  Serial.println(F("\nCorrente e Potência:"));
  Serial.println(F("----------------------------------------"));
  float current = bms.getCurrent();
  Serial.print(F("Corrente:       "));
  Serial.print(current, 3);
  Serial.println(F(" A"));
  
  float power = bms.getPower();
  Serial.print(F("Potência:       "));
  Serial.print(power, 2);
  Serial.println(F(" W"));
  
  if (current > 0) {
    Serial.println(F("Status:         CARREGANDO"));
  } else if (current < -0.1) {
    Serial.println(F("Status:         DESCARREGANDO"));
  } else {
    Serial.println(F("Status:         REPOUSO"));
  }
  
  // Status e Proteções
  Serial.println(F("\nStatus do Sistema:"));
  Serial.println(F("----------------------------------------"));
  
  Serial.print(F("Sobretensão:    "));
  Serial.println(bms.isOverVoltage() ? F("ALERTA!") : F("OK"));
  
  Serial.print(F("Subtensão:      "));
  Serial.println(bms.isUnderVoltage() ? F("ALERTA!") : F("OK"));
  
  Serial.print(F("Sobrecorrente:  "));
  Serial.println(bms.isOverCurrent() ? F("ALERTA!") : F("OK"));
  
  Serial.print(F("Sobretemperatura: "));
  Serial.println(bms.isOverTemperature() ? F("ALERTA!") : F("OK"));
  
  Serial.println(F("\n========================================\n"));
  
  // Aguarda 2 segundos antes da próxima leitura
  delay(2000);
}
