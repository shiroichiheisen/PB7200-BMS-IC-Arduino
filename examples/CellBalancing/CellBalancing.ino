/**
 * CellBalancing.ino
 * 
 * Exemplo de balanceamento de células com PB7200P80
 * 
 * Este exemplo demonstra:
 * - Balanceamento manual de células
 * - Balanceamento automático
 * - Monitoramento do processo de balanceamento
 * 
 * Hardware:
 * - Arduino (qualquer placa)
 * - PB7200P80 conectado via I2C
 */

#include <PB7200P80.h>

// Cria objeto PB7200P80
PB7200P80 bms(PB7200_INTERFACE_I2C, 0x55);

const uint8_t NUM_CELLS = 4;
const float BALANCE_THRESHOLD_MV = 50.0;  // Balancear se diferença > 50mV

// Modo de operação
enum BalanceMode {
  MANUAL,
  AUTOMATIC
};

BalanceMode currentMode = AUTOMATIC;  // Altere para MANUAL se desejar

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  
  Serial.println(F("==========================================="));
  Serial.println(F("   PB7200P80 - Balanceamento de Células   "));
  Serial.println(F("==========================================="));
  Serial.println();
  
  // Inicializa BMS
  if (!bms.begin(NUM_CELLS)) {
    Serial.println(F("ERRO: Falha ao inicializar PB7200P80!"));
    while (1) delay(1000);
  }
  
  Serial.println(F("PB7200P80 inicializado!"));
  
  if (currentMode == AUTOMATIC) {
    Serial.println(F("Modo: BALANCEAMENTO AUTOMÁTICO"));
    Serial.print(F("Limiar: "));
    Serial.print(BALANCE_THRESHOLD_MV, 0);
    Serial.println(F(" mV"));
    
    // Habilita balanceamento automático
    if (bms.setAutoBalancing(true, BALANCE_THRESHOLD_MV)) {
      Serial.println(F("Balanceamento automático ativado!"));
    } else {
      Serial.println(F("ERRO ao ativar balanceamento automático"));
    }
  } else {
    Serial.println(F("Modo: BALANCEAMENTO MANUAL"));
    Serial.println(F("Digite o número da célula (1-4) para balancear"));
    Serial.println(F("ou '0' para parar todo balanceamento"));
  }
  
  Serial.println();
  delay(2000);
}

void loop() {
  // Atualiza leituras
  bms.update();
  
  // Modo manual - processa comandos seriais
  if (currentMode == MANUAL && Serial.available()) {
    char cmd = Serial.read();
    
    if (cmd >= '1' && cmd <= '4') {
      uint8_t cellIndex = cmd - '1';
      
      // Alterna balanceamento da célula
      bool currentState = bms.isBalancing(cellIndex);
      bms.setBalancing(cellIndex, !currentState);
      
      Serial.print(F("Célula "));
      Serial.print(cellIndex + 1);
      Serial.print(F(": Balanceamento "));
      Serial.println(!currentState ? F("ATIVADO") : F("DESATIVADO"));
    } else if (cmd == '0') {
      bms.stopAllBalancing();
      Serial.println(F("Todo balanceamento desativado"));
    }
  }
  
  // Exibe status a cada 3 segundos
  static unsigned long lastDisplay = 0;
  if (millis() - lastDisplay >= 3000) {
    lastDisplay = millis();
    displayBalancingStatus();
  }
  
  delay(100);
}

void displayBalancingStatus() {
  Serial.println(F("========================================"));
  Serial.println(F("     STATUS DE BALANCEAMENTO            "));
  Serial.println(F("========================================"));
  
  // Tensões e status de balanceamento
  Serial.println(F("\nCélula | Tensão  | Delta  | Balanc."));
  Serial.println(F("-------|---------|--------|----------"));
  
  float avgVoltage = 0;
  for (uint8_t i = 0; i < NUM_CELLS; i++) {
    avgVoltage += bms.getCellVoltage(i);
  }
  avgVoltage /= NUM_CELLS;
  
  for (uint8_t i = 0; i < NUM_CELLS; i++) {
    float voltage = bms.getCellVoltage(i);
    float delta = (voltage - avgVoltage) * 1000;  // Em mV
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
      Serial.println(F(">>> SIM <<<"));
    } else {
      Serial.println(F("Não"));
    }
  }
  
  // Estatísticas
  Serial.println(F("\nEstatísticas:"));
  Serial.print(F("  Tensão Média:    "));
  Serial.print(avgVoltage, 3);
  Serial.println(F(" V"));
  
  Serial.print(F("  Tensão Máxima:   "));
  Serial.print(bms.getMaxCellVoltage(), 3);
  Serial.println(F(" V"));
  
  Serial.print(F("  Tensão Mínima:   "));
  Serial.print(bms.getMinCellVoltage(), 3);
  Serial.println(F(" V"));
  
  float delta = bms.getVoltageDelta() * 1000;
  Serial.print(F("  Delta Total:     "));
  Serial.print(delta, 1);
  Serial.println(F(" mV"));
  
  // Recomendação
  Serial.println();
  if (delta < BALANCE_THRESHOLD_MV) {
    Serial.println(F("✓ Células bem balanceadas!"));
  } else {
    Serial.println(F("⚠ Balanceamento necessário"));
  }
  
  Serial.println(F("========================================\n"));
}
