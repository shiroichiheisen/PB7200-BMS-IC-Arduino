/**
 * CompleteBMS.ino
 * 
 * Exemplo completo de sistema BMS com PB7200P80
 * 
 * Este exemplo demonstra:
 * - Monitoramento completo de todas as funções
 * - Estatísticas detalhadas do pack
 * - Balanceamento automático
 * - Proteções configuradas
 * - Interface serial interativa
 * 
 * Comandos disponíveis:
 * - 'v' : Mostrar tensões
 * - 't' : Mostrar temperaturas
 * - 's' : Mostrar estatísticas
 * - 'p' : Mostrar proteções
 * - 'b' : Toggle balanceamento automático
 * - 'c' : Limpar falhas
 * - 'd' : Diagnóstico completo
 */

#include <PB7200P80.h>

PB7200P80 bms(PB7200_INTERFACE_I2C, 0x55);

const uint8_t NUM_CELLS = 4;
bool autoBalanceEnabled = true;

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  
  printHeader();
  
  // Inicializa BMS
  Serial.println(F("Inicializando BMS..."));
  if (!bms.begin(NUM_CELLS)) {
    Serial.println(F("✗ ERRO: Falha ao inicializar!"));
    while (1) delay(1000);
  }
  Serial.println(F("✓ BMS inicializado!"));
  
  // Executa autoteste
  Serial.println();
  bms.selfTest();
  
  // Configura proteções (Li-ion)
  Serial.println();
  Serial.println(F("Configurando proteções..."));
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
    Serial.println(F("✓ Proteções configuradas!"));
  } else {
    Serial.println(F("✗ Erro ao configurar proteções"));
  }
  
  // Habilita balanceamento automático
  if (autoBalanceEnabled) {
    Serial.println(F("Habilitando balanceamento automático..."));
    if (bms.setAutoBalancing(true, 50)) {
      Serial.println(F("✓ Balanceamento automático ativo (limiar: 50mV)"));
    }
  }
  
  Serial.println();
  printMenu();
  Serial.println();
  
  delay(2000);
}

void loop() {
  // Atualiza leituras a cada 1 segundo
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate >= 1000) {
    lastUpdate = millis();
    bms.update();
    
    // Verifica alertas críticos
    checkCriticalAlerts();
  }
  
  // Processa comandos seriais
  if (Serial.available()) {
    char cmd = Serial.read();
    processCommand(cmd);
  }
  
  // Display automático a cada 5 segundos
  static unsigned long lastDisplay = 0;
  if (millis() - lastDisplay >= 5000) {
    lastDisplay = millis();
    showQuickStatus();
  }
  
  delay(100);
}

void printHeader() {
  Serial.println(F("\n╔═══════════════════════════════════════╗"));
  Serial.println(F("║   SISTEMA BMS COMPLETO - PB7200P80    ║"));
  Serial.println(F("║        Battery Management System       ║"));
  Serial.println(F("╚═══════════════════════════════════════╝\n"));
}

void printMenu() {
  Serial.println(F("┌───────────────────────────────────────┐"));
  Serial.println(F("│          COMANDOS DISPONÍVEIS          │"));
  Serial.println(F("├───────────────────────────────────────┤"));
  Serial.println(F("│ v - Mostrar tensões das células       │"));
  Serial.println(F("│ t - Mostrar temperaturas              │"));
  Serial.println(F("│ s - Mostrar estatísticas completas    │"));
  Serial.println(F("│ p - Mostrar status de proteções       │"));
  Serial.println(F("│ b - Toggle balanceamento automático   │"));
  Serial.println(F("│ c - Limpar falhas                     │"));
  Serial.println(F("│ d - Diagnóstico completo              │"));
  Serial.println(F("│ h - Mostrar este menu                 │"));
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
      Serial.print(F("Balanceamento automático: "));
      Serial.println(autoBalanceEnabled ? F("ATIVADO") : F("DESATIVADO"));
      break;
      
    case 'c':
    case 'C':
      if (bms.clearFaults()) {
        Serial.println(F("✓ Falhas limpas"));
      } else {
        Serial.println(F("✗ Erro ao limpar falhas"));
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
        Serial.println(F("Comando inválido. Digite 'h' para ajuda."));
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
    Serial.println(F("Erro ao obter estatísticas"));
    return;
  }
  
  Serial.println(F("╔═══════════════════════════════════════╗"));
  Serial.println(F("║      ESTATÍSTICAS COMPLETAS DO PACK    ║"));
  Serial.println(F("╚═══════════════════════════════════════╝"));
  
  Serial.println(F("\n┌─── Tensões ───────────────────────────┐"));
  Serial.print(F("│ Total:        "));
  printValue(stats.totalVoltage, 3, "V");
  Serial.print(F("│ Máxima:       "));
  printValue(stats.maxCellVoltage, 3, "V");
  Serial.print(F("│ Mínima:       "));
  printValue(stats.minCellVoltage, 3, "V");
  Serial.print(F("│ Média:        "));
  printValue(stats.avgCellVoltage, 3, "V");
  Serial.print(F("│ Delta:        "));
  printValue(stats.voltageDelta * 1000, 1, "mV");
  Serial.println(F("└───────────────────────────────────────┘"));
  
  Serial.println(F("\n┌─── Corrente e Potência ───────────────┐"));
  Serial.print(F("│ Corrente:     "));
  printValue(stats.current, 3, "A");
  Serial.print(F("│ Potência:     "));
  printValue(stats.power, 2, "W");
  Serial.print(F("│ Status:       "));
  if (stats.current > 0.1) {
    Serial.println(F("CARREGANDO         │"));
  } else if (stats.current < -0.1) {
    Serial.println(F("DESCARREGANDO      │"));
  } else {
    Serial.println(F("REPOUSO            │"));
  }
  Serial.println(F("└───────────────────────────────────────┘"));
  
  Serial.println(F("\n┌─── Temperaturas ──────────────────────┐"));
  Serial.print(F("│ Máxima:       "));
  printValue(stats.maxTemp, 1, "°C");
  Serial.print(F("│ Mínima:       "));
  printValue(stats.minTemp, 1, "°C");
  Serial.println(F("└───────────────────────────────────────┘"));
  
  Serial.println(F("\n┌─── Índices ───────────────────────────┐"));
  Serial.print(F("│ Célula V Max: "));
  Serial.print(stats.maxCellIndex + 1);
  Serial.println(F("                    │"));
  Serial.print(F("│ Célula V Min: "));
  Serial.print(stats.minCellIndex + 1);
  Serial.println(F("                    │"));
  Serial.print(F("│ Sensor T Max: "));
  Serial.print(stats.maxTempIndex + 1);
  Serial.println(F("                    │"));
  Serial.print(F("│ Sensor T Min: "));
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
    Serial.println(F("║           ⚠  ALERTA CRÍTICO  ⚠        ║"));
    Serial.println(F("╚═══════════════════════════════════════╝"));
    bms.printStatus();
    alertShown = true;
  } else if (!hasAlert && alertShown) {
    Serial.println(F("\n✓ Alertas limpos - Sistema normalizado"));
    alertShown = false;
  }
}
