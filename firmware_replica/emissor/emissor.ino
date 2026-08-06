// ============================================
// EMISSOR - Replica d'entrades via LoRa
// Heltec WiFi LoRa 32 (V3)
//
// Llegeix 4 entrades i envia l'estat per LoRa.
// IN1 i IN2: temporitzador 3 minuts (canvi confirmat)
// IN3 i IN4: canvi immediat
// ============================================

#include "LoRaWan_APP.h"
#include "HT_SSD1306Wire.h"
#include "config.h"

extern SSD1306Wire display;

// --- LoRa ---
static RadioEvents_t RadioEvents;
bool txDone = true;
uint32_t txCount = 0;
uint32_t txErrors = 0;
unsigned long lastTxTime = 0;

// --- Estat entrades ---
#define NUM_INPUTS 4
const uint8_t inputPins[NUM_INPUTS] = { IN1_PIN, IN2_PIN, IN3_PIN, IN4_PIN };

// Debounce 3 minuts per IN1 i IN2
uint8_t confirmedState[NUM_INPUTS];
uint8_t pendingState[NUM_INPUTS];
unsigned long pendingStartTime[NUM_INPUTS];
bool hasPending[NUM_INPUTS];
bool firstRead = true;

uint8_t lastStateSent = 0xFF;  // Forcar primer enviament

// --- Display ---
bool txJustSent = false;
unsigned long txFlashTime = 0;

// ============================================
// Llegeix les 4 entrades amb debounce
// IN1 i IN2: temporitzador 3 minuts
// IN3 i IN4: canvi immediat
// ============================================
uint8_t readInputs() {
  unsigned long now = millis();

  for (int i = 0; i < NUM_INPUTS; i++) {
    uint8_t raw = digitalRead(inputPins[i]) ? 1 : 0;

    if (firstRead) {
      confirmedState[i] = raw;
      hasPending[i] = false;
      continue;
    }

    if (i >= 2) {
      // IN3 i IN4: canvi immediat, sense temporitzador
      if (raw != confirmedState[i]) {
        Serial.printf("IN%d: canvi immediat (%d -> %d)\r\n", i + 1, confirmedState[i], raw);
        confirmedState[i] = raw;
      }
    } else if (raw != confirmedState[i]) {
      // IN1 i IN2: temporitzador de 3 minuts
      if (!hasPending[i]) {
        hasPending[i] = true;
        pendingState[i] = raw;
        pendingStartTime[i] = now;
        Serial.printf("IN%d: canvi pendent detectat (%d -> %d), esperant %d min\r\n",
                      i + 1, confirmedState[i], raw, DEBOUNCE_CONFIRM_MS / 60000);
      } else if (raw != pendingState[i]) {
        pendingState[i] = raw;
        pendingStartTime[i] = now;
      }

      if (hasPending[i] && (now - pendingStartTime[i] >= DEBOUNCE_CONFIRM_MS)) {
        Serial.printf("IN%d: canvi CONFIRMAT (%d -> %d) despres de %d min\r\n",
                      i + 1, confirmedState[i], pendingState[i], DEBOUNCE_CONFIRM_MS / 60000);
        confirmedState[i] = pendingState[i];
        hasPending[i] = false;
      }
    } else {
      if (hasPending[i]) {
        Serial.printf("IN%d: canvi pendent CANCELAT (tornat a %d)\r\n", i + 1, confirmedState[i]);
        hasPending[i] = false;
      }
    }
  }

  if (firstRead) firstRead = false;

  uint8_t state = 0;
  for (int i = 0; i < NUM_INPUTS; i++) {
    if (confirmedState[i]) state |= (1 << i);
  }
  return state;
}

// ============================================
// Envia 1 byte per LoRa
// ============================================
void sendState(uint8_t state) {
  if (!txDone) return;

  txDone = false;
  uint8_t buf[BUFFER_SIZE];
  buf[0] = state;
  Radio.Send(buf, BUFFER_SIZE);

  lastStateSent = state;
  lastTxTime = millis();

  Serial.printf("TX: 0x%02X [IN1=%d IN2=%d IN3=%d IN4=%d]\r\n",
                state,
                (state >> 0) & 1, (state >> 1) & 1,
                (state >> 2) & 1, (state >> 3) & 1);
}

// ============================================
// Display OLED
// ============================================
void updateDisplay(uint8_t state) {
  display.clear();

  if (txJustSent && (millis() - txFlashTime > 500)) {
    txJustSent = false;
  }

  // Linia 1: estat TX
  display.setFont(ArialMT_Plain_16);
  if (!txDone) {
    display.drawString(0, 0, "ENVIANT...");
  } else if (txJustSent) {
    display.drawString(0, 0, "ENVIAT OK");
  } else {
    display.drawString(0, 0, "EMISSOR OK");
  }

  display.setFont(ArialMT_Plain_10);
  char buf[48];

  // Linia 2: estat entrades (confirmades) + indicador pendent
  snprintf(buf, sizeof(buf), "IN: %d%c %d%c %d%c %d%c",
           confirmedState[0], hasPending[0] ? '*' : ' ',
           confirmedState[1], hasPending[1] ? '*' : ' ',
           confirmedState[2], hasPending[2] ? '*' : ' ',
           confirmedState[3], hasPending[3] ? '*' : ' ');
  display.drawString(0, 20, buf);

  // Linia 3: comptadors
  snprintf(buf, sizeof(buf), "Enviats:%lu Errors:%lu", txCount, txErrors);
  display.drawString(0, 34, buf);

  // Linia 4: temps des de l'ultim TX
  if (lastTxTime > 0) {
    unsigned long elapsed = (millis() - lastTxTime) / 1000;
    snprintf(buf, sizeof(buf), "Ultim TX: fa %lus", elapsed);
  } else {
    snprintf(buf, sizeof(buf), "Ultim TX: --");
  }
  display.drawString(0, 48, buf);

  display.display();
}

// ============================================
// Callbacks LoRa
// ============================================
void OnTxDone(void) {
  txDone = true;
  txCount++;
  txJustSent = true;
  txFlashTime = millis();
  Serial.printf("TX OK #%lu\r\n", txCount);
}

void OnTxTimeout(void) {
  txDone = true;
  txErrors++;
  lastStateSent = 0xFF;  // Forcar reintent al proper cicle
  txJustSent = true;
  txFlashTime = millis();
  Serial.println("TX timeout");
}

// ============================================
// Setup
// ============================================
void setup() {
  Serial.begin(SERIAL_BAUD);
  Serial.println("\n=== EMISSOR LoRa - Replica d'entrades ===");

  // Entrades amb pull-down
  for (int i = 0; i < NUM_INPUTS; i++) {
    pinMode(inputPins[i], INPUT_PULLDOWN);
  }

  // RS485 DE/RE a LOW (no utilitzat a l'emissor)
  pinMode(RS485_DE_RE_PIN, OUTPUT);
  digitalWrite(RS485_DE_RE_PIN, LOW);

  Mcu.begin();

  // OLED
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
  delay(50);

  display.init();
  display.flipScreenVertically();
  display.setContrast(255, 241, 64);
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 20, "EMISSOR");
  display.drawString(0, 40, "Iniciant...");
  display.display();

  // LoRa
  RadioEvents.TxDone    = OnTxDone;
  RadioEvents.TxTimeout = OnTxTimeout;
  Radio.Init(&RadioEvents);
  Radio.SetChannel(RF_FREQUENCY);
  Radio.SetTxConfig(
    MODEM_LORA,
    TX_OUTPUT_POWER,
    0,
    LORA_BANDWIDTH,
    LORA_SPREADING_FACTOR,
    LORA_CODINGRATE,
    LORA_PREAMBLE_LENGTH,
    LORA_FIX_LENGTH_PAYLOAD_ON,
    true,    // CRC activat
    0, 0,    // freq hop off
    LORA_IQ_INVERSION_ON,
    3000     // TX timeout (ms)
  );

  Serial.printf("LoRa: 868 MHz, SF%d, BW 125kHz, CR 4/5, %d dBm\r\n",
                LORA_SPREADING_FACTOR, TX_OUTPUT_POWER);

  // Primer enviament
  delay(500);
  uint8_t state = readInputs();
  lastStateSent = state;
  sendState(state);
}

// ============================================
// Loop principal
// ============================================
void loop() {
  Radio.IrqProcess();

  unsigned long now = millis();
  uint8_t state = readInputs();

  // Enviar si hi ha canvi d'estat confirmat
  if (state != lastStateSent && txDone) {
    Serial.printf("Canvi: 0x%02X -> 0x%02X\r\n", lastStateSent, state);
    sendState(state);
  }
  // Enviar periodic cada TX_INTERVAL_MS
  else if ((now - lastTxTime >= TX_INTERVAL_MS) && txDone) {
    Serial.println("Enviament periodic");
    sendState(state);
  }

  updateDisplay(state);
  delay(POLL_INTERVAL_MS);
}
