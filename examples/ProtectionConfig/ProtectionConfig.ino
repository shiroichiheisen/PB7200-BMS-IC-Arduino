/**
 * ProtectionConfig.ino
 * 
 * Exemplo de configuração de proteções do PB7200P80
 * 
 * Este exemplo demonstra:
 * - Configuração de limites de proteção
 * - Leitura de configurações atuais
 * - Monitoramento de eventos de proteção
 * 
 * AVISO: Ajuste os valores conforme as especificações
 *        do seu pack de baterias!
 */

#include <PB7200P80.h>

PB7200P80 bms(PB7200_INTERFACE_I2C, 0x55);

const uint8_t NUM_CELLS = 4;

// Configurações de proteção para células Li-ion típicas
// AJUSTE CONFORME SEU TIPO DE BATERIA!
ProtectionConfig protection = {
  .overVoltageThreshold = 4.25,      // Sobretensão em 4.25V
  .underVoltageThreshold = 2.80,     // Subtensão em 2.80V
  .overCurrentThreshold = 10.0,      // Sobrecorrente em 10A
  .overTempThreshold = 60.0,         // Sobretemperatura em 60°C
  .underTempThreshold = -10.0,       // Subtemperatura em -10°C
  .overVoltageDelay = 100,           // Atraso de 100ms
  .underVoltageDelay = 100,          // Atraso de 100ms
  .overCurrentDelay = 50             // Atraso de 50ms
};

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  
  Serial.println(F("==========================================="));
  Serial.println(F("  PB7200P80 - Configuração de Proteções   "));
  Serial.println(F("==========================================="));
  Serial.println();
  
  // Inicializa BMS
  if (!bms.begin(NUM_CELLS)) {
    Serial.println(F("ERRO: Falha ao inicializar PB7200P80!"));
    while (1) delay(1000);
  }
  
  Serial.println(F("PB7200P80 inicializado!"));
  Serial.println();
  
  // Lê configuração atual
  Serial.println(F("Configuração ATUAL de Proteções:"));
  Serial.println(F("----------------------------------------"));
  displayProtectionConfig();
  Serial.println();
  
  // Pergunta se deseja aplicar nova configuração
  Serial.println(F("Deseja aplicar a NOVA configuração? (S/N)"));
  Serial.println(F("(Digite 'S' nos próximos 10 segundos)"));
  
  unsigned long startTime = millis();
  bool applyConfig = false;
  
  while (millis() - startTime < 10000) {
    if (Serial.available()) {
      char response = Serial.read();
      if (response == 'S' || response == 's') {
        applyConfig = true;
        break;
      } else if (response == 'N' || response == 'n') {
        break;
      }
    }
    delay(100);
  }
  
  if (applyConfig) {
    Serial.println(F("\nAplicando nova configuração..."));
    
    if (bms.setProtectionConfig(protection)) {
      Serial.println(F("✓ Configuração aplicada com sucesso!"));
      delay(1000);
      
      Serial.println(F("\nConfiguração NOVA:"));
      Serial.println(F("----------------------------------------"));
      displayProtectionConfig();
    } else {
      Serial.println(F("✗ ERRO ao aplicar configuração!"));
    }
  } else {
    Serial.println(F("\nMantendo configuração atual."));
  }
  
  Serial.println();
  Serial.println(F("Iniciando monitoramento de proteções..."));
  Serial.println();
  delay(2000);
}

void loop() {
  // Atualiza leituras
  bms.update();
  
  // Verifica proteções
  static bool lastOVP = false;
  static bool lastUVP = false;
  static bool lastOCP = false;
  static bool lastOTP = false;
  
  bool ovp = bms.isOverVoltage();
  bool uvp = bms.isUnderVoltage();
  bool ocp = bms.isOverCurrent();
  bool otp = bms.isOverTemperature();
  
  // Detecta mudanças e alerta
  if (ovp && !lastOVP) {
    Serial.println(F("\n⚠⚠⚠ ALERTA: SOBRETENSÃO DETECTADA! ⚠⚠⚠"));
    showProtectionDetails();
  }
  if (uvp && !lastUVP) {
    Serial.println(F("\n⚠⚠⚠ ALERTA: SUBTENSÃO DETECTADA! ⚠⚠⚠"));
    showProtectionDetails();
  }
  if (ocp && !lastOCP) {
    Serial.println(F("\n⚠⚠⚠ ALERTA: SOBRECORRENTE DETECTADA! ⚠⚠⚠"));
    showProtectionDetails();
  }
  if (otp && !lastOTP) {
    Serial.println(F("\n⚠⚠⚠ ALERTA: SOBRETEMPERATURA DETECTADA! ⚠⚠⚠"));
    showProtectionDetails();
  }
  
  lastOVP = ovp;
  lastUVP = uvp;
  lastOCP = ocp;
  lastOTP = otp;
  
  // Display periódico
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
    Serial.print(F("Sobretensão:      "));
    Serial.print(config.overVoltageThreshold, 2);
    Serial.println(F(" V"));
    
    Serial.print(F("Subtensão:        "));
    Serial.print(config.underVoltageThreshold, 2);
    Serial.println(F(" V"));
    
    Serial.print(F("Sobrecorrente:    "));
    Serial.print(config.overCurrentThreshold, 1);
    Serial.println(F(" A"));
    
    Serial.print(F("Sobretemperatura: "));
    Serial.print(config.overTempThreshold, 1);
    Serial.println(F(" °C"));
    
    Serial.print(F("Subtemperatura:   "));
    Serial.print(config.underTempThreshold, 1);
    Serial.println(F(" °C"));
  } else {
    Serial.println(F("ERRO ao ler configuração!"));
  }
}

void displayMonitoring() {
  Serial.println(F("========================================"));
  Serial.println(F("    MONITORAMENTO DE PROTEÇÕES          "));
  Serial.println(F("========================================"));
  
  // Status das proteções
  Serial.println(F("\nStatus das Proteções:"));
  Serial.print(F("  Sobretensão:      "));
  Serial.println(bms.isOverVoltage() ? F("⚠ ATIVO") : F("✓ OK"));
  
  Serial.print(F("  Subtensão:        "));
  Serial.println(bms.isUnderVoltage() ? F("⚠ ATIVO") : F("✓ OK"));
  
  Serial.print(F("  Sobrecorrente:    "));
  Serial.println(bms.isOverCurrent() ? F("⚠ ATIVO") : F("✓ OK"));
  
  Serial.print(F("  Sobretemperatura: "));
  Serial.println(bms.isOverTemperature() ? F("⚠ ATIVO") : F("✓ OK"));
  
  Serial.print(F("  Subtemperatura:   "));
  Serial.println(bms.isUnderTemperature() ? F("⚠ ATIVO") : F("✓ OK"));
  
  // Valores atuais
  Serial.println(F("\nValores Atuais:"));
  Serial.print(F("  Tensão Máx. Célula: "));
  Serial.print(bms.getMaxCellVoltage(), 3);
  Serial.println(F(" V"));
  
  Serial.print(F("  Tensão Mín. Célula: "));
  Serial.print(bms.getMinCellVoltage(), 3);
  Serial.println(F(" V"));
  
  Serial.print(F("  Corrente:           "));
  Serial.print(bms.getCurrent(), 3);
  Serial.println(F(" A"));
  
  Serial.print(F("  Temperatura Máx.:   "));
  Serial.print(bms.getMaxTemperature(), 1);
  Serial.println(F(" °C"));
  
  Serial.println(F("========================================\n"));
}

void showProtectionDetails() {
  Serial.println(F("\nDetalhes da Proteção:"));
  Serial.println(F("----------------------------------------"));
  
  for (uint8_t i = 0; i < NUM_CELLS; i++) {
    Serial.print(F("Célula "));
    Serial.print(i + 1);
    Serial.print(F(": "));
    Serial.print(bms.getCellVoltage(i), 3);
    Serial.println(F(" V"));
  }
  
  Serial.print(F("Corrente: "));
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
