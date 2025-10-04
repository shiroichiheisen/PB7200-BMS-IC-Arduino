# Manual da Biblioteca PB7200P80 para Arduino

## Versão 1.0.0

---

## Índice

1. [Introdução](#introdução)
2. [Instalação](#instalação)
3. [Conexões de Hardware](#conexões-de-hardware)
4. [Início Rápido](#início-rápido)
5. [Referência da API](#referência-da-api)
6. [Exemplos de Uso](#exemplos-de-uso)
7. [Configuração de Proteções](#configuração-de-proteções)
8. [Balanceamento de Células](#balanceamento-de-células)
9. [Solução de Problemas](#solução-de-problemas)
10. [Perguntas Frequentes](#perguntas-frequentes)

---

## Introdução

A biblioteca **PB7200P80** fornece uma interface completa e fácil de usar para comunicação com o chip PB7200P80, um AFE (Analog Front End) para sistemas de gerenciamento de baterias (BMS).

### Características Principais

✓ **Monitoramento de Células**: Leitura precisa de até 20 células em série  
✓ **Medição de Temperatura**: Suporte para até 8 sensores de temperatura  
✓ **Medição de Corrente**: Leitura de corrente de carga/descarga  
✓ **Proteções Configuráveis**: Sobretensão, subtensão, sobrecorrente, sobretemperatura  
✓ **Balanceamento de Células**: Manual e automático  
✓ **Interface I2C**: Comunicação simples e confiável  
✓ **Funções de Diagnóstico**: Ferramentas para debug e validação  

### Compatibilidade

- **Plataformas**: Arduino Uno, Nano, Mega, ESP32, ESP8266, STM32
- **IDE**: Arduino IDE 1.8+, PlatformIO
- **Comunicação**: I2C (UART em desenvolvimento)

---

## Instalação

### Método 1: Instalação Manual

1. Baixe a biblioteca ou copie a pasta `PB7200P80` para:
   - Windows: `Documents\Arduino\libraries\`
   - Mac: `~/Documents/Arduino/libraries/`
   - Linux: `~/Arduino/libraries/`

2. Reinicie o Arduino IDE

3. Verifique em **Sketch → Include Library** se `PB7200P80` aparece na lista

### Método 2: PlatformIO

Adicione ao seu `platformio.ini`:

```ini
lib_deps = 
    Wire
    PB7200P80
```

### Método 3: Arduino Library Manager (quando publicada)

1. Abra Arduino IDE
2. Vá em **Sketch → Include Library → Manage Libraries**
3. Procure por "PB7200P80"
4. Clique em **Install**

---

## Conexões de Hardware

### Conexão I2C Básica

```
PB7200P80          Arduino Uno/Nano
---------          ----------------
VCC      ────────  3.3V ou 5V
GND      ────────  GND
SDA      ────────  A4 (SDA)
SCL      ────────  A5 (SCL)
```

### Conexão em ESP32

```
PB7200P80          ESP32
---------          -----
VCC      ────────  3.3V
GND      ────────  GND
SDA      ────────  GPIO21 (SDA)
SCL      ────────  GPIO22 (SCL)
```

### Notas Importantes

⚠ **Tensão de Operação**: Verifique se o PB7200P80 suporta 5V ou apenas 3.3V  
⚠ **Pull-ups**: A maioria das placas Arduino possui resistores pull-up internos. Se necessário, adicione resistores de 4.7kΩ entre SDA/SCL e VCC  
⚠ **Comprimento do Cabo**: Mantenha cabos I2C curtos (< 30cm) para evitar problemas de comunicação  

---

## Início Rápido

### Exemplo Mínimo

```cpp
#include <PB7200P80.h>

// Cria objeto BMS
PB7200P80 bms;

void setup() {
  Serial.begin(115200);
  
  // Inicializa BMS com 4 células
  if (!bms.begin(4)) {
    Serial.println("Erro ao inicializar BMS!");
    while(1);
  }
  
  Serial.println("BMS inicializado!");
}

void loop() {
  // Atualiza todas as leituras
  bms.update();
  
  // Exibe tensão total
  Serial.print("Tensão do Pack: ");
  Serial.print(bms.getTotalVoltage());
  Serial.println(" V");
  
  delay(1000);
}
```

### Leitura de Células Individuais

```cpp
void loop() {
  // Lê tensão de cada célula
  for (uint8_t i = 0; i < 4; i++) {
    float voltage = bms.getCellVoltage(i);
    Serial.print("Célula ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print(voltage, 3);
    Serial.println(" V");
  }
  
  delay(2000);
}
```

---

## Referência da API

### Inicialização

#### `PB7200P80()`
```cpp
PB7200P80 bms;  // Construtor padrão (I2C, endereço 0x55)
```

#### `begin()`
```cpp
bool begin(uint8_t cellCount)
```
Inicializa a comunicação com o PB7200P80.

**Parâmetros:**
- `cellCount`: Número de células conectadas (1-20)

**Retorno:** `true` se inicializado com sucesso

**Exemplo:**
```cpp
if (!bms.begin(4)) {
  Serial.println("Falha na inicialização!");
}
```

---

### Leitura de Tensões

#### `getCellVoltage()`
```cpp
float getCellVoltage(uint8_t cellIndex)
```
Lê a tensão de uma célula específica.

**Parâmetros:**
- `cellIndex`: Índice da célula (0-19)

**Retorno:** Tensão em volts

**Exemplo:**
```cpp
float v1 = bms.getCellVoltage(0);  // Primeira célula
```

#### `getAllCellVoltages()`
```cpp
bool getAllCellVoltages(float *voltages, uint8_t count)
```
Lê todas as tensões de uma vez (mais eficiente).

**Parâmetros:**
- `voltages`: Array para armazenar as tensões
- `count`: Número de células a ler

**Retorno:** `true` se sucesso

**Exemplo:**
```cpp
float voltages[4];
if (bms.getAllCellVoltages(voltages, 4)) {
  for (int i = 0; i < 4; i++) {
    Serial.println(voltages[i]);
  }
}
```

#### `getTotalVoltage()`
```cpp
float getTotalVoltage()
```
Retorna a soma de todas as tensões das células.

**Exemplo:**
```cpp
float totalV = bms.getTotalVoltage();
Serial.print("Pack: ");
Serial.print(totalV);
Serial.println(" V");
```

#### `getMaxCellVoltage()`
```cpp
float getMaxCellVoltage()
```
Retorna a maior tensão entre as células.

#### `getMinCellVoltage()`
```cpp
float getMinCellVoltage()
```
Retorna a menor tensão entre as células.

#### `getVoltageDelta()`
```cpp
float getVoltageDelta()
```
Retorna a diferença entre a maior e menor tensão.

**Exemplo:**
```cpp
float delta = bms.getVoltageDelta();
Serial.print("Delta: ");
Serial.print(delta * 1000);  // Converte para mV
Serial.println(" mV");
```

---

### Leitura de Temperaturas

#### `getTemperature()`
```cpp
float getTemperature(uint8_t tempIndex)
```
Lê a temperatura de um sensor específico.

**Parâmetros:**
- `tempIndex`: Índice do sensor (0-7)

**Retorno:** Temperatura em °C

**Exemplo:**
```cpp
float temp1 = bms.getTemperature(0);
Serial.print("Temperatura: ");
Serial.print(temp1);
Serial.println(" °C");
```

#### `getAllTemperatures()`
```cpp
bool getAllTemperatures(float *temperatures, uint8_t count)
```
Lê todas as temperaturas de uma vez.

#### `getMaxTemperature()`
```cpp
float getMaxTemperature()
```
Retorna a temperatura máxima detectada.

#### `getMinTemperature()`
```cpp
float getMinTemperature()
```
Retorna a temperatura mínima detectada.

---

### Leitura de Corrente e Potência

#### `getCurrent()`
```cpp
float getCurrent()
```
Lê a corrente do pack.

**Retorno:** Corrente em amperes (positivo = carga, negativo = descarga)

**Exemplo:**
```cpp
float current = bms.getCurrent();
if (current > 0) {
  Serial.println("Carregando");
} else if (current < 0) {
  Serial.println("Descarregando");
}
```

#### `getPower()`
```cpp
float getPower()
```
Retorna a potência calculada (tensão × corrente).

**Retorno:** Potência em watts

---

### Status e Proteções

#### `getStatus()`
```cpp
uint8_t getStatus()
```
Lê o registro de status do dispositivo.

#### `getFaultStatus()`
```cpp
uint8_t getFaultStatus()
```
Lê o registro de falhas.

#### `isOverVoltage()`
```cpp
bool isOverVoltage()
```
Verifica se há sobretensão detectada.

**Exemplo:**
```cpp
if (bms.isOverVoltage()) {
  Serial.println("ALERTA: Sobretensão!");
  // Desconectar carga/carregador
}
```

#### `isUnderVoltage()`
```cpp
bool isUnderVoltage()
```
Verifica se há subtensão detectada.

#### `isOverCurrent()`
```cpp
bool isOverCurrent()
```
Verifica se há sobrecorrente detectada.

#### `isOverTemperature()`
```cpp
bool isOverTemperature()
```
Verifica se há sobretemperatura detectada.

#### `isUnderTemperature()`
```cpp
bool isUnderTemperature()
```
Verifica se há subtemperatura detectada.

#### `clearFaults()`
```cpp
bool clearFaults()
```
Limpa todas as flags de falha.

---

### Balanceamento de Células

#### `setBalancing()`
```cpp
bool setBalancing(uint8_t cellIndex, bool enable)
```
Habilita ou desabilita balanceamento de uma célula.

**Parâmetros:**
- `cellIndex`: Índice da célula
- `enable`: `true` para habilitar, `false` para desabilitar

**Exemplo:**
```cpp
// Habilita balanceamento da célula 2
bms.setBalancing(1, true);
```

#### `setAutoBalancing()`
```cpp
bool setAutoBalancing(bool enable, uint16_t threshold)
```
Habilita balanceamento automático.

**Parâmetros:**
- `enable`: `true` para habilitar
- `threshold`: Diferença de tensão em mV para iniciar balanceamento (padrão: 50mV)

**Exemplo:**
```cpp
// Balancear automaticamente se diferença > 50mV
bms.setAutoBalancing(true, 50);
```

#### `isBalancing()`
```cpp
bool isBalancing(uint8_t cellIndex)
```
Verifica se uma célula está sendo balanceada.

#### `stopAllBalancing()`
```cpp
bool stopAllBalancing()
```
Desabilita balanceamento de todas as células.

---

### Configuração de Proteções

#### `setProtectionConfig()`
```cpp
bool setProtectionConfig(const ProtectionConfig &config)
```
Configura os limites de proteção.

**Exemplo:**
```cpp
ProtectionConfig config;
config.overVoltageThreshold = 4.25;     // 4.25V por célula
config.underVoltageThreshold = 2.80;    // 2.80V por célula
config.overCurrentThreshold = 10.0;     // 10A
config.overTempThreshold = 60.0;        // 60°C
config.underTempThreshold = -10.0;      // -10°C

if (bms.setProtectionConfig(config)) {
  Serial.println("Proteções configuradas!");
}
```

#### `getProtectionConfig()`
```cpp
bool getProtectionConfig(ProtectionConfig &config)
```
Lê as configurações atuais de proteção.

---

### Modos de Operação

#### `setMode()`
```cpp
bool setMode(PB7200_Mode mode)
```
Define o modo de operação.

**Modos disponíveis:**
- `PB7200_MODE_NORMAL`: Modo normal de operação
- `PB7200_MODE_SLEEP`: Modo de baixo consumo
- `PB7200_MODE_SHUTDOWN`: Desligamento

#### `sleep()`
```cpp
bool sleep()
```
Entra em modo sleep.

#### `wakeup()`
```cpp
bool wakeup()
```
Acorda do modo sleep.

#### `reset()`
```cpp
bool reset()
```
Reinicia o dispositivo.

---

### Estatísticas

#### `getPackStats()`
```cpp
bool getPackStats(PackStats &stats)
```
Obtém estatísticas completas do pack em uma única chamada.

**Exemplo:**
```cpp
PackStats stats;
if (bms.getPackStats(stats)) {
  Serial.print("Tensão Total: ");
  Serial.println(stats.totalVoltage);
  Serial.print("Célula mais alta: ");
  Serial.println(stats.maxCellIndex + 1);
  Serial.print("Delta: ");
  Serial.print(stats.voltageDelta * 1000);
  Serial.println(" mV");
}
```

**Campos de PackStats:**
- `totalVoltage`: Tensão total do pack
- `maxCellVoltage`: Maior tensão de célula
- `minCellVoltage`: Menor tensão de célula
- `avgCellVoltage`: Tensão média
- `voltageDelta`: Diferença max-min
- `maxCellIndex`: Índice da célula com maior tensão
- `minCellIndex`: Índice da célula com menor tensão
- `current`: Corrente atual
- `power`: Potência
- `maxTemp`: Temperatura máxima
- `minTemp`: Temperatura mínima

#### `update()`
```cpp
bool update()
```
Atualiza todas as leituras de uma vez (otimizado).

**Exemplo:**
```cpp
void loop() {
  bms.update();  // Atualiza tudo
  
  // Agora pode acessar valores cached
  float v = bms.getTotalVoltage();
  float i = bms.getCurrent();
  
  delay(1000);
}
```

---

### Funções de Diagnóstico

#### `selfTest()`
```cpp
bool selfTest()
```
Executa autoteste e imprime resultados no Serial.

#### `printDiagnostics()`
```cpp
void printDiagnostics()
```
Imprime informações completas de diagnóstico.

#### `printCellVoltages()`
```cpp
void printCellVoltages()
```
Imprime todas as tensões das células formatadas.

#### `printTemperatures()`
```cpp
void printTemperatures()
```
Imprime todas as temperaturas formatadas.

#### `printStatus()`
```cpp
void printStatus()
```
Imprime status de proteções e alertas.

---

## Exemplos de Uso

### Exemplo 1: Monitor Serial Simples

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

### Exemplo 2: Monitoramento com Alertas

```cpp
#include <PB7200P80.h>

PB7200P80 bms;

void setup() {
  Serial.begin(115200);
  if (!bms.begin(4)) {
    Serial.println("ERRO: BMS não inicializado!");
    while(1);
  }
}

void loop() {
  bms.update();
  
  // Verifica alertas críticos
  if (bms.isOverVoltage()) {
    Serial.println("⚠ ALERTA: SOBRETENSÃO!");
    // Ação: desconectar carregador
  }
  
  if (bms.isUnderVoltage()) {
    Serial.println("⚠ ALERTA: SUBTENSÃO!");
    // Ação: desconectar carga
  }
  
  if (bms.isOverTemperature()) {
    Serial.println("⚠ ALERTA: TEMPERATURA ALTA!");
    // Ação: reduzir corrente ou desligar
  }
  
  // Mostra delta de tensão
  float delta = bms.getVoltageDelta() * 1000;
  if (delta > 100) {  // > 100mV
    Serial.print("⚠ Delta alto: ");
    Serial.print(delta);
    Serial.println(" mV - Considere balancear");
  }
  
  delay(2000);
}
```

### Exemplo 3: Controle de Relés de Proteção

```cpp
#include <PB7200P80.h>

PB7200P80 bms;

const int CHARGE_RELAY_PIN = 7;
const int DISCHARGE_RELAY_PIN = 8;

void setup() {
  Serial.begin(115200);
  
  pinMode(CHARGE_RELAY_PIN, OUTPUT);
  pinMode(DISCHARGE_RELAY_PIN, OUTPUT);
  
  bms.begin(4);
  
  // Configura proteções
  ProtectionConfig config;
  config.overVoltageThreshold = 4.20;
  config.underVoltageThreshold = 3.00;
  config.overCurrentThreshold = 15.0;
  config.overTempThreshold = 55.0;
  bms.setProtectionConfig(config);
}

void loop() {
  bms.update();
  
  // Controle de relé de carga
  if (bms.isOverVoltage() || bms.isOverTemperature()) {
    digitalWrite(CHARGE_RELAY_PIN, LOW);  // Desliga carregador
    Serial.println("Carregador desligado");
  } else {
    digitalWrite(CHARGE_RELAY_PIN, HIGH);  // Permite carregar
  }
  
  // Controle de relé de descarga
  if (bms.isUnderVoltage() || bms.isOverCurrent()) {
    digitalWrite(DISCHARGE_RELAY_PIN, LOW);  // Desconecta carga
    Serial.println("Descarga bloqueada");
  } else {
    digitalWrite(DISCHARGE_RELAY_PIN, HIGH);  // Permite descarregar
  }
  
  delay(500);
}
```

---

## Configuração de Proteções

### Proteções para Baterias Li-ion (18650)

```cpp
ProtectionConfig liionConfig = {
  .overVoltageThreshold = 4.25,      // Máximo seguro
  .underVoltageThreshold = 2.80,     // Mínimo seguro
  .overCurrentThreshold = 10.0,      // Depende da célula
  .overTempThreshold = 60.0,         // Limite de temperatura
  .underTempThreshold = 0.0,         // Não carregar abaixo de 0°C
  .overVoltageDelay = 100,
  .underVoltageDelay = 100,
  .overCurrentDelay = 50
};

bms.setProtectionConfig(liionConfig);
```

### Proteções para Baterias LiFePO4

```cpp
ProtectionConfig lifepo4Config = {
  .overVoltageThreshold = 3.65,      // Máximo para LiFePO4
  .underVoltageThreshold = 2.50,     // Mínimo para LiFePO4
  .overCurrentThreshold = 20.0,      // LiFePO4 aguenta mais corrente
  .overTempThreshold = 60.0,
  .underTempThreshold = -20.0,       // LiFePO4 mais tolerante ao frio
  .overVoltageDelay = 100,
  .underVoltageDelay = 100,
  .overCurrentDelay = 50
};

bms.setProtectionConfig(lifepo4Config);
```

---

## Balanceamento de Células

### Quando Balancear?

Balanceamento é necessário quando há diferença significativa entre as tensões das células:

- **Diferença < 30mV**: Células bem balanceadas, não requer ação
- **Diferença 30-50mV**: Recomendado balanceamento durante carga
- **Diferença > 50mV**: Balanceamento necessário
- **Diferença > 100mV**: Balanceamento urgente, investigar células fracas

### Balanceamento Automático

```cpp
void setup() {
  bms.begin(4);
  
  // Ativa balanceamento automático
  // Limiar de 50mV
  bms.setAutoBalancing(true, 50);
}

void loop() {
  bms.update();
  
  // Monitora progresso
  for (uint8_t i = 0; i < 4; i++) {
    if (bms.isBalancing(i)) {
      Serial.print("Célula ");
      Serial.print(i + 1);
      Serial.println(" balanceando...");
    }
  }
  
  delay(5000);
}
```

### Balanceamento Manual

```cpp
void balanceHighestCell() {
  bms.update();
  
  // Encontra célula com maior tensão
  float maxV = 0;
  uint8_t maxIndex = 0;
  
  for (uint8_t i = 0; i < 4; i++) {
    float v = bms.getCellVoltage(i);
    if (v > maxV) {
      maxV = v;
      maxIndex = i;
    }
  }
  
  // Balanceia apenas a célula mais alta
  for (uint8_t i = 0; i < 4; i++) {
    bms.setBalancing(i, (i == maxIndex));
  }
  
  Serial.print("Balanceando célula ");
  Serial.println(maxIndex + 1);
}
```

---

## Solução de Problemas

### Problema: BMS não inicializa

**Sintomas:** `begin()` retorna `false`

**Soluções:**
1. Verifique conexões I2C (SDA/SCL)
2. Confirme alimentação do PB7200P80
3. Teste endereço I2C:
```cpp
Wire.begin();
Wire.beginTransmission(0x55);
byte error = Wire.endTransmission();
if (error == 0) {
  Serial.println("Dispositivo encontrado!");
}
```
4. Adicione resistores pull-up de 4.7kΩ em SDA e SCL
5. Reduza velocidade I2C:
```cpp
Wire.setClock(50000);  // 50kHz em vez de 100kHz
```

### Problema: Leituras de tensão erradas

**Sintomas:** Tensões 0V ou valores absurdos

**Soluções:**
1. Verifique se células estão conectadas ao PB7200P80
2. Confirme número de células no `begin()`:
```cpp
bms.begin(4);  // Deve corresponder ao número real
```
3. Execute diagnóstico:
```cpp
bms.printDiagnostics();
```
4. Verifique se célula está com falha (multímetro)

### Problema: Comunicação I2C instável

**Sintomas:** Leituras intermitentes, travamentos

**Soluções:**
1. Encurte cabos I2C (< 20cm ideal)
2. Adicione capacitor de 100nF entre VCC e GND próximo ao PB7200P80
3. Use cabos blindados para I2C
4. Evite rotas próximas a fontes de ruído (motores, PWM)
5. Adicione pequeno delay entre leituras:
```cpp
bms.update();
delay(50);
```

### Problema: Balanceamento não funciona

**Sintomas:** Células não balanceiam mesmo com diferença alta

**Soluções:**
1. Verifique se balanceamento está habilitado:
```cpp
if (!bms.isBalancing(0)) {
  bms.setBalancing(0, true);
}
```
2. Confirme que células estão carregadas (balanceamento só ocorre em tensão alta)
3. Verifique corrente de balanceamento do PB7200P80 (geralmente 50-100mA)
4. Balanceamento é lento, pode levar horas para equilibrar

### Problema: Proteções disparando incorretamente

**Sintomas:** Alertas falsos de sobretensão/subtensão

**Soluções:**
1. Revise limites de proteção:
```cpp
ProtectionConfig config;
bms.getProtectionConfig(config);
Serial.println(config.overVoltageThreshold);
```
2. Ajuste limites conforme tipo de bateria
3. Aumente delays de proteção para evitar falsos positivos
4. Verifique se sensores de temperatura estão conectados corretamente

---

## Perguntas Frequentes

### Q: Quantas células posso monitorar?

**R:** O PB7200P80 suporta até 20 células em série. A biblioteca suporta de 1 a 20 células.

### Q: Posso usar com ESP32/ESP8266?

**R:** Sim! A biblioteca é compatível. No ESP32, use os pinos I2C padrão (GPIO21/22) ou configure outros pinos:
```cpp
Wire.begin(SDA_PIN, SCL_PIN);
bms.begin(4);
```

### Q: Como calibro as leituras de tensão?

**R:** As leituras dependem da precisão do PB7200P80 e sua calibração de fábrica. Se necessário, você pode adicionar um fator de correção:
```cpp
float voltage = bms.getCellVoltage(0);
float calibrated = voltage * 1.02;  // Ajuste conforme necessário
```

### Q: O balanceamento drena as células completamente?

**R:** Não. O balanceamento apenas dissipa energia das células mais altas até igualar com as mais baixas. É um processo passivo que usa resistores.

### Q: Posso usar em sistema 24V/48V?

**R:** Sim, desde que o número de células em série seja compatível:
- 24V: ~6-7 células Li-ion (6S)
- 48V: ~13-14 células Li-ion (13S)

### Q: Preciso de proteção externa (BMS físico)?

**R:** O PB7200P80 é um AFE (Analog Front End) e monitora, mas pode não controlar MOSFETs de proteção. Consulte o datasheet do chip para confirmar se inclui controle de MOSFETs ou se você precisa adicionar circuito externo.

### Q: Como faço logging de dados?

**R:** Exemplo com SD card:
```cpp
#include <SD.h>

void loop() {
  bms.update();
  
  File dataFile = SD.open("bms_log.csv", FILE_WRITE);
  if (dataFile) {
    dataFile.print(millis());
    dataFile.print(",");
    dataFile.print(bms.getTotalVoltage());
    dataFile.print(",");
    dataFile.println(bms.getCurrent());
    dataFile.close();
  }
  
  delay(10000);  // Log a cada 10s
}
```

### Q: A biblioteca suporta UART?

**R:** Atualmente apenas I2C está implementado. Suporte UART está planejado para versões futuras.

### Q: Consumo de memória da biblioteca?

**R:** A biblioteca usa aproximadamente:
- Flash: ~8KB
- RAM: ~250 bytes

Compatível com Arduino Uno e placas similares.

---

## Changelog

### Versão 1.0.0 (2025-10-04)
- Release inicial
- Suporte I2C completo
- Monitoramento de até 20 células
- 8 sensores de temperatura
- Balanceamento manual e automático
- Proteções configuráveis
- Exemplos completos

---

## Suporte e Contribuições

### Reportar Bugs

Se encontrar um problema, por favor reporte com:
- Versão da biblioteca
- Plataforma (Arduino Uno, ESP32, etc.)
- Código mínimo para reproduzir o problema
- Mensagens de erro

### Contribuir

Contribuições são bem-vindas! Por favor:
1. Fork o repositório
2. Crie uma branch para sua feature
3. Commit suas mudanças
4. Envie um Pull Request

---

## Licença

Esta biblioteca é distribuída sob licença MIT. Veja arquivo LICENSE para detalhes.

---

## Autor

Biblioteca PB7200P80  
Versão 1.0.0  
Data: 04/10/2025

---

## Avisos Legais

⚠ **ATENÇÃO**: Baterias de lítio podem ser perigosas se mal gerenciadas. Esta biblioteca é fornecida "como está" sem garantias. O autor não se responsabiliza por danos causados pelo uso desta biblioteca.

✓ Sempre use proteções adequadas em sistemas de baterias  
✓ Monitore temperatura constantemente  
✓ Use circuitos de proteção redundantes em aplicações críticas  
✓ Consulte especialistas para aplicações comerciais  
✓ Siga regulamentos locais sobre baterias de lítio  

---

**Bom uso da biblioteca PB7200P80!** 🔋⚡
