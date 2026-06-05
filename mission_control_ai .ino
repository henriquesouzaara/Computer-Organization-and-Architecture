// =============================================================================
// MISSION CONTROL AI — Sistema de Monitoramento de Cápsula Espacial
// =============================================================================
// Disciplinas : Computer Organization and Architecture
//               Soluções em Energias Renováveis e Sustentáveis
// Plataforma  : Arduino Uno / Wokwi
// Versão      : 1.4 — sensor DS18B20
// =============================================================================

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// =============================================================================
// DISPLAY
// =============================================================================
LiquidCrystal_I2C lcd(0x27, 16, 2);

// =============================================================================
// PINOS
// =============================================================================
const int PINO_TEMPERATURA  = 4;   // DS18B20 — pino digital 4
const int PINO_LDR          = A1;
const int PINO_VIBRACAO     = 2;
const int PINO_LED_CRITICO  = 8;
const int PINO_LED_NORMAL   = 9;

// =============================================================================
// DS18B20
// =============================================================================
OneWire           oneWire(PINO_TEMPERATURA);
DallasTemperature sensors(&oneWire);

// =============================================================================
// THRESHOLDS
// =============================================================================
const float TEMP_WARN  = 50.0;
const float TEMP_CRIT  = 60.0;
const float TEMP_MIN   =  0.0;
const int   LUX_BAIXO  = 100;
const int   LUX_ALTO   = 800;
const int   VIB_WARN   = 2;
const int   VIB_CRIT   = 3;

// =============================================================================
// ENERGIA
// =============================================================================
const float SOLAR_MAX    = 500.0;
const float GER_MIN      =  10.0;
const float CONSUMO_BASE = 200.0;
const float CONSUMO_LED  =  68.0;

// =============================================================================
// INTERVALOS
// =============================================================================
const unsigned long CICLO_NORMAL  = 2000;
const unsigned long CICLO_ECONOMY = 5000;

// =============================================================================
// ENUMS
// =============================================================================
enum Alerta   { INFO=0, WARNING=1, CRITICAL=2, EMERGENCY=3 };
enum Modo     { NORMAL=0, ECONOMY=1, SAFE=2, EMERG=3 };
enum EnStatus { SUPERAVIT=0, EQUILIB=1, DEFICIT=2, CRITICO=3 };

// =============================================================================
// VARIÁVEIS GLOBAIS
// =============================================================================
float    temperatura  = 25.0;
int      luxADC       = 0;
int      vibEventos   = 0;

float    geracao      = 0;
float    consumo      = 0;
float    balanco      = 0;
float    eficiencia   = 0;
EnStatus enStatus     = EQUILIB;

Alerta   alertaNivel  = INFO;
char     alertaOrigem[16] = "SISTEMA";
Modo     modoOp       = NORMAL;
bool     ledVermAtivo = false;
bool     ledVerdAtivo = false;

unsigned long ultimoCiclo  = 0;
unsigned long ultimaTroca  = 0;
int           telaAtual    = 0;
int           histerese    = 0;

volatile int           isrContador = 0;
volatile unsigned long isrUltimo   = 0;

// =============================================================================
// ISR — VIBRAÇÃO
// =============================================================================
void ISR_Vibracao() {
  unsigned long agora = micros();
  if (agora - isrUltimo < 50000UL) return;
  if (agora - isrUltimo > 5000000UL) isrContador = 0;
  if (isrContador < VIB_CRIT + 1) isrContador++;
  isrUltimo = agora;
}

// =============================================================================
// SETUP
// =============================================================================
void setup() {
  Serial.begin(9600);

  pinMode(PINO_VIBRACAO,    INPUT_PULLUP);
  pinMode(PINO_LED_CRITICO, OUTPUT);
  pinMode(PINO_LED_NORMAL,  OUTPUT);
  digitalWrite(PINO_LED_CRITICO, LOW);
  digitalWrite(PINO_LED_NORMAL,  LOW);

  attachInterrupt(digitalPinToInterrupt(PINO_VIBRACAO), ISR_Vibracao, FALLING);

  // Inicia DS18B20
  sensors.begin();

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0); lcd.print("MISSION CTRL AI ");
  lcd.setCursor(0, 1); lcd.print("Iniciando...    ");
  delay(2000);
  lcd.clear();

  Serial.println("=== MISSION CONTROL AI v1.4 ===");
  Serial.println("Sistema pronto.");
}

// =============================================================================
// LOOP
// =============================================================================
void loop() {
  unsigned long agora = millis();
  unsigned long intervalo = (modoOp == ECONOMY || modoOp == EMERG)
                            ? CICLO_ECONOMY : CICLO_NORMAL;

  if (agora - ultimoCiclo >= intervalo) {
    ultimoCiclo = agora;
    lerSensores();
    calcAlertas();
    calcDecisao();
    calcLEDs();
    calcEnergia();
    enviarSerial();
  }

  atualizarDisplay();
}

// =============================================================================
// 1. LER SENSORES
// =============================================================================
void lerSensores() {
  // Temperatura DS18B20 — leitura digital estável
  sensors.requestTemperatures();
  float t = sensors.getTempCByIndex(0);
  if (t != DEVICE_DISCONNECTED_C) {
    temperatura = t;
  }

  // Luminosidade
  luxADC = analogRead(PINO_LDR);

  // Vibração
  noInterrupts();
  if (micros() - isrUltimo > 5000000UL) isrContador = 0;
  vibEventos = isrContador;
  interrupts();
}

// =============================================================================
// 2. ALERTAS
// =============================================================================
void calcAlertas() {
  alertaNivel = INFO;
  strcpy(alertaOrigem, "SISTEMA");
  int criticos = 0;

  // Temperatura
  if (temperatura > TEMP_CRIT || temperatura < TEMP_MIN) {
    alertaNivel = CRITICAL; strcpy(alertaOrigem, "TEMPERATURA"); criticos++;
  } else if (temperatura > TEMP_WARN) {
    if (alertaNivel < WARNING) { alertaNivel = WARNING; strcpy(alertaOrigem, "TEMPERATURA"); }
  }

  // Luminosidade
  if (luxADC < LUX_BAIXO) {
    if (alertaNivel < WARNING) { alertaNivel = WARNING; strcpy(alertaOrigem, "LUZ BAIXA"); }
  } else if (luxADC > LUX_ALTO) {
    if (alertaNivel < WARNING) { alertaNivel = WARNING; strcpy(alertaOrigem, "LUZ ALTA"); }
  }

  // Vibração
  if (vibEventos >= VIB_CRIT) {
    if (alertaNivel < CRITICAL) { alertaNivel = CRITICAL; strcpy(alertaOrigem, "VIBRACAO"); }
    criticos++;
  } else if (vibEventos >= VIB_WARN) {
    if (alertaNivel < WARNING) { alertaNivel = WARNING; strcpy(alertaOrigem, "VIBRACAO"); }
  }

  // Energia
  if (enStatus == CRITICO) {
    if (alertaNivel < CRITICAL) { alertaNivel = CRITICAL; strcpy(alertaOrigem, "ENERGIA"); }
  } else if (enStatus == DEFICIT) {
    if (alertaNivel < WARNING) { alertaNivel = WARNING; strcpy(alertaOrigem, "ENERGIA"); }
  }

  // Emergency
  if (criticos >= 2) { alertaNivel = EMERGENCY; strcpy(alertaOrigem, "MULTIPLOS"); }
}

// =============================================================================
// 3. DECISÃO (FSM)
// =============================================================================
void calcDecisao() {
  Modo novoModo;

  if      (alertaNivel == EMERGENCY) novoModo = EMERG;
  else if (enStatus == CRITICO)      novoModo = ECONOMY;
  else if (alertaNivel == CRITICAL)  novoModo = SAFE;
  else                               novoModo = NORMAL;

  if (novoModo == NORMAL && modoOp != NORMAL) {
    histerese++;
    if (histerese < 5) return;
    histerese = 0;
  } else if (novoModo != NORMAL) {
    histerese = 0;
  }

  modoOp = novoModo;
}

// =============================================================================
// 4. LEDs
// =============================================================================
void calcLEDs() {
  ledVermAtivo = (alertaNivel == WARNING || alertaNivel == CRITICAL || alertaNivel == EMERGENCY);
  ledVerdAtivo = (alertaNivel == INFO && modoOp == NORMAL);

  digitalWrite(PINO_LED_CRITICO, ledVermAtivo ? HIGH : LOW);
  digitalWrite(PINO_LED_NORMAL,  ledVerdAtivo ? HIGH : LOW);
}

// =============================================================================
// 5. ENERGIA
// =============================================================================
void calcEnergia() {
  geracao = (luxADC / 1023.0) * SOLAR_MAX;
  if (geracao < GER_MIN) geracao = GER_MIN;

  consumo = CONSUMO_BASE;
  if (ledVermAtivo) consumo += CONSUMO_LED;
  if (ledVerdAtivo) consumo += CONSUMO_LED;

  balanco    = geracao - consumo;
  eficiencia = min(100.0, (geracao / consumo) * 100.0);

  if      (balanco >= 100)  enStatus = SUPERAVIT;
  else if (balanco >= 0)    enStatus = EQUILIB;
  else if (balanco >= -50)  enStatus = DEFICIT;
  else                      enStatus = CRITICO;
}

// =============================================================================
// 6. DISPLAY
// =============================================================================
void lcdLinha(int linha, const char* txt) {
  char buf[17];
  snprintf(buf, sizeof(buf), "%-16s", txt);
  lcd.setCursor(0, linha);
  lcd.print(buf);
}

void atualizarDisplay() {
  unsigned long agora     = millis();
  unsigned long intervalo = (modoOp == ECONOMY || modoOp == EMERG)
                             ? CICLO_ECONOMY : CICLO_NORMAL;

  // Emergency
  if (alertaNivel == EMERGENCY || modoOp == EMERG) {
    lcdLinha(0, "!!! EMERGENCIA !");
    lcdLinha(1, "MULTIPLAS FALHAS");
    return;
  }

  // Critical
  if (alertaNivel == CRITICAL) {
    static unsigned long ultPiscada = 0;
    static bool lcdOn = true;
    if (agora - ultPiscada >= 500) {
      ultPiscada = agora;
      lcdOn = !lcdOn;
      if (lcdOn) lcd.backlight(); else lcd.backlight();
    }
    char l1[17];
    snprintf(l1, sizeof(l1), "%-16s", alertaOrigem);
    lcdLinha(0, "!! ALERTA CRIT  ");
    lcdLinha(1, l1);
    return;
  }

  lcd.backlight();

  // Rotação de telas
  if (agora - ultimaTroca >= intervalo) {
    ultimaTroca = agora;
    telaAtual   = (telaAtual + 1) % 5;

    char l0[17], l1[17];

    switch (telaAtual) {
      case 0:
        lcdLinha(0, "MISSION CTRL AI");
        snprintf(l1, sizeof(l1), "Modo:%-11s",
          modoOp == NORMAL  ? "NORMAL" :
          modoOp == ECONOMY ? "ECONOMY" :
          modoOp == SAFE    ? "SAFE" : "EMERGENCY");
        lcdLinha(1, l1);
        break;

      case 1: {
        // Temperatura sem %f — usa inteiros
        int tInt = (int)temperatura;
        int tDec = abs((int)((temperatura - tInt) * 10));
        snprintf(l0, sizeof(l0), "TEMP: %d.%d C", tInt, tDec);
        snprintf(l1, sizeof(l1), "LUZ:%4d ADC", luxADC);
        lcdLinha(0, l0);
        lcdLinha(1, l1);
        break;
      }

      case 2:
        snprintf(l0, sizeof(l0), "VIB:%d eventos", vibEventos);
        snprintf(l1, sizeof(l1), "ALERTA:%-9s",
          alertaNivel == INFO     ? "OK" :
          alertaNivel == WARNING  ? "WARN" :
          alertaNivel == CRITICAL ? "CRIT" : "EMERG");
        lcdLinha(0, l0);
        lcdLinha(1, l1);
        break;

      case 3:
        snprintf(l0, sizeof(l0), "GER: %4d mW", (int)geracao);
        snprintf(l1, sizeof(l1), "CONS:%4d mW", (int)consumo);
        lcdLinha(0, l0);
        lcdLinha(1, l1);
        break;

      case 4:
        snprintf(l0, sizeof(l0), "BAL:%+5d mW", (int)balanco);
        snprintf(l1, sizeof(l1), "EFI: %3d %%",  (int)eficiencia);
        lcdLinha(0, l0);
        lcdLinha(1, l1);
        break;
    }
  }
}

// =============================================================================
// 7. TELEMETRIA SERIAL
// =============================================================================
void enviarSerial() {
  const char* prefixo =
    alertaNivel == INFO      ? "[INFO]   " :
    alertaNivel == WARNING   ? "[WARN]   " :
    alertaNivel == CRITICAL  ? "[CRIT]   " : "[EMERG]  ";

  const char* enStr =
    enStatus == SUPERAVIT ? "SUPERAVIT" :
    enStatus == EQUILIB   ? "EQUILIB  " :
    enStatus == DEFICIT   ? "DEFICIT  " : "CRITICO  ";

  const char* modoStr =
    modoOp == NORMAL  ? "NORMAL   " :
    modoOp == ECONOMY ? "ECONOMY  " :
    modoOp == SAFE    ? "SAFE     " : "EMERGENCY";

  Serial.print(prefixo);
  Serial.print(" ");
  Serial.print(millis());
  Serial.print("ms | T:");
  Serial.print(temperatura, 1);
  Serial.print("C | L:");
  Serial.print(luxADC);
  Serial.print(" | V:");
  Serial.print(vibEventos);
  Serial.print(" | G:");
  Serial.print((int)geracao);
  Serial.print("mW | C:");
  Serial.print((int)consumo);
  Serial.print("mW | B:");
  Serial.print((int)balanco);
  Serial.print("mW | E:");
  Serial.print((int)eficiencia);
  Serial.print("% | ");
  Serial.print(enStr);
  Serial.print(" | ");
  Serial.print(modoStr);
  if (alertaNivel > INFO) {
    Serial.print(" | ");
    Serial.print(alertaOrigem);
  }
  Serial.println();
}
