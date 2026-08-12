// ============================================
// RECEPTOR - Sistema LoRa monitoratge boia
// Heltec WiFi LoRa 32 (V3)
// VERSIO FAIL-SAFE: logica NC (Normally Closed)
// Boies amb contacte NC: 0=nivell OK o desconnectada, 1=falta nivell
// ============================================

#include "LoRaWan_APP.h"
#include "HT_SSD1306Wire.h"
#include <ModbusMaster.h>
#include "config.h"

// OLED display (ja definit per la llibreria Heltec a LoRaWan_APP.cpp)
extern SSD1306Wire display;

// Variables LoRa
static RadioEvents_t RadioEvents;

// RS485 / Modbus
ModbusMaster modbus;
int16_t deyeSoc = -1;          // SOC bateria (%), -1 = no llegit
int16_t deyePower = -1;        // Produccio PV total (W), -1 = no llegit
unsigned long lastModbusRead = 0;

// Estat recepcio
uint8_t lastStateReceived = 0x00;
uint8_t prevStateProcessed = 0xFF;  // Inicialitzat a 0xFF per forcar avaluacio al primer paquet
bool newLoRaData = false;           // Flag: hi ha dades noves per processar
bool firstLoRaPacket = true;        // Flag: primer paquet rebut (avaluar totes les sortides)
unsigned long lastRxTime = 0;
bool timedOut = true;
int16_t lastRssi = 0;
uint32_t rxOkCount = 0;
uint32_t rxErrCount = 0;
bool rxJustReceived = false;
unsigned long rxFlashTime = 0;

// Estat sortides
bool out1State = false;
unsigned long out1StartTime = 0;  // Moment en que OUT1 es va activar
bool out2State = false;
bool out3State = false;
bool out4State = false;
bool boiaBombaState = false;      // Estat confirmat boia bomba
bool switchMitjaCarrega = false;  // Estat confirmat switch mitja carrega

// Debounce entrades locals (3 lectures consecutives iguals per confirmar)
#define LOCAL_INPUT_DEBOUNCE 3
uint8_t boiaBombaCount = 0;       // Comptador lectures consecutives iguals
uint8_t boiaBombaRaw = 0;         // Ultim valor cru llegit
uint8_t switchMCCount = 0;
uint8_t switchMCRaw = 0;

// SOC anterior per detectar canvis reals
int16_t prevDeyeSoc = -1;

// Cache potenciometre (evitar 8 lectures ADC cada 100ms)
uint32_t cachedMaxDurationMs = 0;
unsigned long lastPotRead = 0;
#define POT_READ_INTERVAL_MS 2000  // Rellegir potenciometre cada 2s

// Llegeix el potenciometre (mitjana 8 lectures) i actualitza la cache
void updatePotCache() {
  int sum = 0;
  for (int i = 0; i < 8; i++) sum += analogRead(POT_PIN);
  int adcVal = sum / 8;
  uint32_t minutes = (uint32_t)((adcVal / 4095.0) * POT_MAX_MINUTES);
  cachedMaxDurationMs = minutes * 60UL * 1000UL;
  lastPotRead = millis();
}

// ============================================
// Logica OUT1 (bomba omplir diposit) - FAIL-SAFE
// Avalua condicions i escriu GPIO nomes si canvia
// ============================================
// LOGICA FAIL-SAFE (NC - Normally Closed):
// - Boies cablejades amb contacte NC: boia avall (falta aigua) = NC tancat = 1, boia amunt (hi ha aigua) = NC obert = 0
// - Boia desconnectada o cable tallat = pin LOW (pull-down) = 0 = bomba OFF (seguretat)
// - Arranca bomba: IN1=1 I boiaBomba=1 I SOC>=30% (si disponible)
// - Para bomba:    IN1=0 O boiaBomba=0 O SOC<=20% O (switchMitja=1 I IN2LoRa=0) O potenciometre O timeout
// NOTA: boiaBomba usa contacte NO (no NC) per ser fail-safe: desconnectada=0=para bomba
// - Histeresi SOC: entre 20-30% mante estat actual
// - Si dades Deye no disponibles (SOC=-1): SOC no afecta
void evaluateOut1() {
  uint8_t in1 = (lastStateReceived >> 0) & 1;
  uint8_t in2 = (lastStateReceived >> 1) & 1;

  bool socOk;
  if (deyeSoc < 0) {
    socOk = true;
  } else if (out1State) {
    socOk = (deyeSoc > DEYE_SOC_STOP);
  } else {
    socOk = (deyeSoc >= DEYE_SOC_START);
  }

  bool prevOut1 = out1State;

  if (!boiaBombaState) {
    // FAIL-SAFE boiaBomba: cablejat NO (Normally Open), NO NC!
    // NO: boia amunt (aigua) = NO tancat = 1 = permet bombejar
    // NO: boia avall (sec) = NO obert = 0 = para bomba
    // NO: desconnectada = pin LOW (pull-down) = 0 = para bomba (fail-safe)
    if (out1State) Serial.println("Boia bomba buida: OUT1 desactivada (marxa en sec)");
    out1State = false;
    out1StartTime = 0;
  } else if (switchMitjaCarrega && !in2) {
    // NC: in2=0 significa boia amunt (nivell intermig assolit) O boia desconnectada
    if (out1State) Serial.println("Mitja carrega: OUT1 desactivada (switch+IN2 LoRa)");
    out1State = false;
    out1StartTime = 0;
  } else if (in1 && socOk) {
    // NC: in1=1 significa boia avall (falta aigua al diposit desti) = arrancar bomba
    if (!out1State) {
      out1StartTime = millis();
    }
    out1State = true;
  } else if (in1 && !socOk) {
    // NC: in1=1 (falta aigua) pero SOC insuficient
    if (out1State) Serial.println("SOC baix: OUT1 desactivada");
    out1State = false;
    out1StartTime = 0;
  } else if (!in1) {
    // NC: in1=0 significa boia amunt (hi ha aigua) O boia desconnectada = parar bomba (fail-safe)
    out1State = false;
    out1StartTime = 0;
  }

  if (out1State != prevOut1) {
    digitalWrite(OUT1_PIN, out1State ? HIGH : LOW);
    Serial.printf("OUT1: %s\r\n", out1State ? "ACTIU" : "ATURAT");
  }
}

// ============================================
// Sortides 2, 3, 4: repliquen IN2, IN3, IN4 (invertides per fail-safe NC)
// NC: senyal 0 = boia amunt (nivell assolit) O desconnectada -> sortida OFF (fail-safe)
// NC: senyal 1 = boia avall (nivell no assolit) -> sortida ON
// Cada sortida s'avalua i escriu independentment
// ============================================
void evaluateOut2() {
  bool newState = !((lastStateReceived >> 1) & 1);
  if (newState != out2State) {
    out2State = newState;
    digitalWrite(OUT2_PIN, out2State ? HIGH : LOW);
    Serial.printf("OUT2: %d\r\n", out2State);
  }
}

void evaluateOut3() {
  bool newState = !((lastStateReceived >> 2) & 1);
  if (newState != out3State) {
    out3State = newState;
    digitalWrite(OUT3_PIN, out3State ? HIGH : LOW);
    Serial.printf("OUT3: %d\r\n", out3State);
  }
}

void evaluateOut4() {
  bool newState = (lastStateReceived >> 3) & 1;
  if (newState != out4State) {
    out4State = newState;
    digitalWrite(OUT4_PIN, out4State ? HIGH : LOW);
    Serial.printf("OUT4: %d\r\n", out4State);
  }
}

// Desactiva sortida (seguretat per timeout)
void safetyShutdown() {
  out1State = false;
  out1StartTime = 0;
  out2State = false;
  out3State = false;
  out4State = false;
  digitalWrite(OUT1_PIN, LOW);
  digitalWrite(OUT2_PIN, LOW);
  digitalWrite(OUT3_PIN, LOW);
  digitalWrite(OUT4_PIN, LOW);
  Serial.println("TIMEOUT: sortides desactivades");
}

// Actualitza el display OLED
void updateDisplay() {
  display.clear();

  // Esborrar flag "REBENT" despres de 500ms
  if (rxJustReceived && (millis() - rxFlashTime > 500)) {
    rxJustReceived = false;
  }

  // Linia 1: estat connexio (font gran)
  display.setFont(ArialMT_Plain_16);
  if (rxJustReceived) {
    display.drawString(0, 0, "REBENT...");
  } else if (timedOut) {
    display.drawString(0, 0, "SENSE SENYAL");
  } else {
    display.drawString(0, 0, "RECEPTOR OK");
  }

  display.setFont(ArialMT_Plain_10);

  // Linia 2: estat OUT1 + boia bomba + temps maxim
  char buf[48];
  uint32_t maxMin = cachedMaxDurationMs / 60000UL;
  char maxBuf[10];
  float maxH = maxMin / 60.0f;
  snprintf(maxBuf, sizeof(maxBuf), "%.1fh", maxH);
  if (out1State && out1StartTime > 0) {
    unsigned long elapsed = (millis() - out1StartTime) / 1000;
    snprintf(buf, sizeof(buf), "O1:ACTIU %luh%02lum/%s BB:%d",
             elapsed / 3600, (elapsed % 3600) / 60, maxBuf, boiaBombaState);
  } else {
    snprintf(buf, sizeof(buf), "O1:ATURAT max:%s BB:%d", maxBuf, boiaBombaState);
  }
  display.drawString(0, 20, buf);

  // Linia 3: dades Deye + estat OUT2/OUT3/OUT4
  if (deyeSoc >= 0 && deyePower >= 0) {
    snprintf(buf, sizeof(buf), "SOC:%d%% %dW O2:%d O3:%d O4:%d",
             deyeSoc, deyePower, out2State, out3State, out4State);
  } else {
    snprintf(buf, sizeof(buf), "Deye:-- O2:%d O3:%d O4:%d", out2State, out3State, out4State);
  }
  display.drawString(0, 34, buf);

  // Linia 4: estat boies rebudes per LoRa
  snprintf(buf, sizeof(buf), "IN: %d  %d  %d  %d",
           (lastStateReceived >> 0) & 1, (lastStateReceived >> 1) & 1,
           (lastStateReceived >> 2) & 1, (lastStateReceived >> 3) & 1);
  display.drawString(0, 48, buf);

  display.display();
}

// Callbacks LoRa - NOMES guardar dades, NO tocar sortides
void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
  if (size < BUFFER_SIZE) {
    Radio.Rx(0);
    return;
  }

  uint8_t data = payload[0];
  lastRssi = rssi;
  lastRxTime = millis();
  lastStateReceived = data;
  timedOut = false;
  rxOkCount++;
  rxJustReceived = true;
  rxFlashTime = millis();
  newLoRaData = true;  // Marcar per processar al loop

  Serial.printf("RX: 0x%02X [IN1=%d IN2=%d IN3=%d IN4=%d] RSSI:%d Pkt#%lu\r\n",
                data,
                (data >> 0) & 1, (data >> 1) & 1,
                (data >> 2) & 1, (data >> 3) & 1,
                rssi, rxOkCount);

  Radio.Rx(0);
}

void OnRxTimeout(void) {
  Radio.Rx(0);
}

void OnRxError(void) {
  Serial.println("RX error (CRC?)");
  rxErrCount++;
  Radio.Rx(0);
}

// RS485 callbacks per controlar direccio MAX485
void rs485PreTransmission() {
  digitalWrite(RS485_DE_RE_PIN, HIGH);
  delayMicroseconds(50);  // Temps per estabilitzar transceiver
}

void rs485PostTransmission() {
  Serial1.flush();           // Esperar que TOTS els bytes surtin del buffer TX
  delayMicroseconds(50);     // Marge per l'últim stop bit
  digitalWrite(RS485_DE_RE_PIN, LOW);
}

// Llegeix SOC i produccio del Deye via Modbus
void readDeye() {
  uint8_t result;

  // Registre 184: SOC bateria (%)
  result = modbus.readHoldingRegisters(DEYE_REG_SOC, 1);
  if (result == modbus.ku8MBSuccess) {
    deyeSoc = modbus.getResponseBuffer(0);
    Serial.printf("Deye SOC: %d%%\r\n", deyeSoc);
  } else {
    Serial.printf("Deye SOC error: 0x%02X\r\n", result);
    deyeSoc = -1;
  }

  Radio.IrqProcess();  // Processar events LoRa pendents durant el bloqueig Modbus
  delay(500);          // Pausa entre lectures Modbus (minim 500ms recomanat per Deye)

  // Registres 186-187: PV1 + PV2 power (W) - lectura de 2 registres consecutius
  result = modbus.readHoldingRegisters(DEYE_REG_PV1_POWER, 2);
  if (result == modbus.ku8MBSuccess) {
    int16_t pv1 = modbus.getResponseBuffer(0);
    int16_t pv2 = modbus.getResponseBuffer(1);
    deyePower = pv1 + pv2;
    Serial.printf("Deye PV: %dW (PV1:%d + PV2:%d)\r\n", deyePower, pv1, pv2);
  } else {
    Serial.printf("Deye PV error: 0x%02X\r\n", result);
    deyePower = -1;
  }

  lastModbusRead = millis();
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  Serial.println("\n=== RECEPTOR LoRa Boia ===");

  // Configurar sortides
  pinMode(OUT1_PIN, OUTPUT);
  pinMode(OUT2_PIN, OUTPUT);
  pinMode(OUT3_PIN, OUTPUT);
  pinMode(OUT4_PIN, OUTPUT);
  safetyShutdown();

  // Configurar boia diposit bomba (pull-down)
  pinMode(BOIA_BOMBA_PIN, INPUT_PULLDOWN);

  // Configurar switch mitja carrega (pull-down)
  pinMode(SWITCH_MITJA_CARREGA_PIN, INPUT_PULLDOWN);

  // Configurar entrada analogica potenciometre
  analogSetPinAttenuation(POT_PIN, ADC_11db);  // Rang complet 0-3.3V
  updatePotCache();  // Primera lectura

  // Inicialitzar LoRa (Mcu.begin gestiona Vext i OLED internament)
  Mcu.begin();

  // Activar Vext (alimentacio OLED) - GPIO36 LOW = ON al Heltec V3
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
  delay(50);

  // Inicialitzar OLED despres de Mcu.begin per evitar que sobreescrigui el brightness
  display.init();
  display.flipScreenVertically();
  display.setContrast(255, 241, 64);  // Contrast max, pre-charge max, VCOMH max
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 20, "RECEPTOR");
  display.drawString(0, 40, "Iniciant...");
  display.display();
  RadioEvents.RxDone = OnRxDone;
  RadioEvents.RxTimeout = OnRxTimeout;
  RadioEvents.RxError = OnRxError;
  Radio.Init(&RadioEvents);
  Radio.SetChannel(RF_FREQUENCY);
  Radio.SetRxConfig(
    MODEM_LORA,
    LORA_BANDWIDTH,
    LORA_SPREADING_FACTOR,
    LORA_CODINGRATE,
    0,
    LORA_PREAMBLE_LENGTH,
    LORA_SYMBOL_TIMEOUT,
    LORA_FIX_LENGTH_PAYLOAD_ON,
    0,
    true,   // CRC activat
    0, 0,   // freq hop off
    LORA_IQ_INVERSION_ON,
    true    // rxContinuous
  );

  Serial.println("LoRa inicialitzat a 868 MHz");
  Serial.printf("SF%d, BW 125kHz, CR 4/5\r\n", LORA_SPREADING_FACTOR);
  Radio.Rx(0);

  // Inicialitzar RS485
  pinMode(RS485_DE_RE_PIN, OUTPUT);
  digitalWrite(RS485_DE_RE_PIN, LOW);  // Mode recepcio per defecte
  Serial1.begin(RS485_BAUD, SERIAL_8N1, RS485_RX_PIN, RS485_TX_PIN);
  modbus.begin(MODBUS_SLAVE_ID, Serial1);
  modbus.preTransmission(rs485PreTransmission);
  modbus.postTransmission(rs485PostTransmission);
  Serial.println("RS485 Modbus inicialitzat");

  // Primera lectura Deye
  delay(500);
  readDeye();
}

void loop() {
  Radio.IrqProcess();

  // Llegir entrades locals amb debounce (3 lectures consecutives iguals per confirmar)
  // Si una entrada local canvia, reavaluar OUT1
  bool localChanged = false;

  uint8_t rawBB = digitalRead(BOIA_BOMBA_PIN) ? 1 : 0;
  if (rawBB == boiaBombaRaw) {
    if (boiaBombaCount < LOCAL_INPUT_DEBOUNCE) boiaBombaCount++;
    if (boiaBombaCount >= LOCAL_INPUT_DEBOUNCE && rawBB != boiaBombaState) {
      boiaBombaState = rawBB;
      localChanged = true;
      Serial.printf("Boia bomba: %d (confirmat)\r\n", boiaBombaState);
    }
  } else {
    boiaBombaRaw = rawBB;
    boiaBombaCount = 1;
  }

  uint8_t rawMC = digitalRead(SWITCH_MITJA_CARREGA_PIN) ? 1 : 0;
  if (rawMC == switchMCRaw) {
    if (switchMCCount < LOCAL_INPUT_DEBOUNCE) switchMCCount++;
    if (switchMCCount >= LOCAL_INPUT_DEBOUNCE && rawMC != switchMitjaCarrega) {
      switchMitjaCarrega = rawMC;
      localChanged = true;
      Serial.printf("Switch mitja carrega: %d (confirmat)\r\n", switchMitjaCarrega);
    }
  } else {
    switchMCRaw = rawMC;
    switchMCCount = 1;
  }

  // Reavaluar OUT1 si una entrada local ha canviat
  if (localChanged && !timedOut) {
    evaluateOut1();
  }

  // Timeout de seguretat: sense paquet durant RX_TIMEOUT_MS
  if (!timedOut && lastRxTime > 0 && (millis() - lastRxTime > RX_TIMEOUT_MS)) {
    timedOut = true;
    safetyShutdown();
  }

  // Actualitzar cache potenciometre cada POT_READ_INTERVAL_MS
  if (millis() - lastPotRead >= POT_READ_INTERVAL_MS) {
    uint32_t prevMaxDuration = cachedMaxDurationMs;
    updatePotCache();
    // Si el pot ha canviat de 0 a un valor, reavaluar OUT1 (podria arrancar)
    if (prevMaxDuration == 0 && cachedMaxDurationMs > 0 && !timedOut) {
      evaluateOut1();
    }
  }

  // Duracio maxima OUT1: forcar aturada segons el potenciometre (nomes OUT1)
  if (out1State && out1StartTime > 0) {
    if (cachedMaxDurationMs == 0 || (millis() - out1StartTime >= cachedMaxDurationMs)) {
      Serial.printf("MAX DURACIO: OUT1 desactivada (%lu min)\r\n", cachedMaxDurationMs / 60000UL);
      out1State = false;
      out1StartTime = 0;
      digitalWrite(OUT1_PIN, LOW);
    }
  }

  // Processar dades LoRa noves: actualitzar NOMES les sortides que canvien
  if (newLoRaData) {
    newLoRaData = false;

    if (firstLoRaPacket) {
      // Primer paquet: avaluar TOTES les sortides independentment de l'estat
      firstLoRaPacket = false;
      prevStateProcessed = lastStateReceived;
      Serial.println("Primer paquet LoRa: avaluant totes les sortides");
      evaluateOut1();
      evaluateOut2();
      evaluateOut3();
      evaluateOut4();
    } else {
      uint8_t changed = lastStateReceived ^ prevStateProcessed;
      prevStateProcessed = lastStateReceived;

      // OUT1: avaluar nomes si IN1 o IN2 han canviat (IN2 afecta via switch mitja carrega)
      if (changed & 0x03) evaluateOut1();

      // OUT2, OUT3, OUT4: independents, nomes si el seu IN ha canviat
      if (changed & 0x02) evaluateOut2();
      if (changed & 0x04) evaluateOut3();
      if (changed & 0x08) evaluateOut4();
    }

    Serial.printf("ESTAT: O1:%s O2:%d O3:%d O4:%d (IN1=%d IN2=%d BB=%d MC=%d SOC=%d PV=%d)\r\n",
                  out1State ? "ACTIU" : "ATURAT", out2State, out3State, out4State,
                  (lastStateReceived >> 0) & 1, (lastStateReceived >> 1) & 1,
                  boiaBombaState, switchMitjaCarrega, deyeSoc, deyePower);
  }

  // Lectura periodica Deye via RS485
  if (millis() - lastModbusRead >= DEYE_READ_INTERVAL_MS) {
    readDeye();
    // Reavaluar OUT1 nomes si el SOC ha canviat realment
    if (!timedOut && deyeSoc != prevDeyeSoc) {
      prevDeyeSoc = deyeSoc;
      evaluateOut1();
    }
  }

  updateDisplay();
  delay(100);
}
