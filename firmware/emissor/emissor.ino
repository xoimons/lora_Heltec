// ============================================
// EMISSOR - Sistema LoRa monitoratge boia
// Heltec WiFi LoRa 32 (V3)
// ============================================

#include "LoRaWan_APP.h"
#include "HT_SSD1306Wire.h"
#include "config.h"

// OLED display (ja definit per la llibreria Heltec a LoRaWan_APP.cpp)
extern SSD1306Wire display;

// Variables LoRa (requerides per la llibreria Heltec)
static RadioEvents_t RadioEvents;
bool lora_idle = true;

// Estat
uint8_t lastStateSent = 0xFF;  // Forcar primer enviament
unsigned long lastTxTime = 0;
uint32_t txCount = 0;          // Comptador de paquets enviats
uint32_t txOkCount = 0;        // Paquets enviats OK
uint32_t txFailCount = 0;      // Paquets fallits (timeout)
bool lastTxOk = false;         // Resultat ultim enviament
bool txInProgress = false;     // Enviant ara?
bool txJustFinished = false;   // Flag per mostrar resultat TX breument
unsigned long txFlashTime = 0; // Temps del flash visual

// --- Debounce per entrades (temporitzador 3 minuts) ---
// Nombre d'entrades
#define NUM_INPUTS 4
const uint8_t inputPins[NUM_INPUTS] = { IN1_PIN, IN2_PIN, IN3_PIN, IN4_PIN };

// Estat confirmat de cada entrada (nomes canvia despres de 3 min estable)
uint8_t confirmedState[NUM_INPUTS];
// Ultim valor cru llegit (per detectar si es manté estable)
uint8_t pendingState[NUM_INPUTS];
// Timestamp de quan el valor cru va comencar a diferir del confirmat
unsigned long pendingStartTime[NUM_INPUTS];
// Hi ha un canvi pendent esperant confirmacio?
bool hasPending[NUM_INPUTS];
// Flag per indicar que es la primera lectura (arrencada)
bool firstRead = true;

// Llegeix els pins crus i actualitza el debounce de cada entrada.
// Retorna un byte amb l'estat CONFIRMAT (bits 0-3).
uint8_t readInputs() {
  unsigned long now = millis();

  for (int i = 0; i < NUM_INPUTS; i++) {
    uint8_t raw = digitalRead(inputPins[i]) ? 1 : 0;

    if (firstRead) {
      // A l'arrencada: confirmar directament
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
        // Inici del canvi pendent
        hasPending[i] = true;
        pendingState[i] = raw;
        pendingStartTime[i] = now;
        Serial.printf("IN%d: canvi pendent detectat (%d -> %d), esperant %d min\r\n",
                      i + 1, confirmedState[i], raw, DEBOUNCE_CONFIRM_MS / 60000);
      } else if (raw != pendingState[i]) {
        // El valor cru ha canviat a un tercer valor (o ha tornat i ha tornat a canviar)
        pendingState[i] = raw;
        pendingStartTime[i] = now;
      }

      // Comprovar si han passat els 3 minuts
      if (hasPending[i] && (now - pendingStartTime[i] >= DEBOUNCE_CONFIRM_MS)) {
        Serial.printf("IN%d: canvi CONFIRMAT (%d -> %d) despres de %d min\r\n",
                      i + 1, confirmedState[i], pendingState[i], DEBOUNCE_CONFIRM_MS / 60000);
        confirmedState[i] = pendingState[i];
        hasPending[i] = false;
      }
    } else {
      // El valor cru coincideix amb el confirmat: cancelar qualsevol pendent
      if (hasPending[i]) {
        Serial.printf("IN%d: canvi pendent CANCELAT (tornat a %d)\r\n", i + 1, confirmedState[i]);
        hasPending[i] = false;
      }
    }
  }

  if (firstRead) firstRead = false;

  // Construir byte amb estats confirmats
  uint8_t state = 0;
  for (int i = 0; i < NUM_INPUTS; i++) {
    if (confirmedState[i]) state |= (1 << i);
  }
  return state;
}

// Envia un byte via LoRa
void sendPacket(uint8_t data) {
  if (!lora_idle) return;

  uint8_t txpacket[BUFFER_SIZE];
  txpacket[0] = data;

  Serial.printf("TX: 0x%02X [IN1=%d IN2=%d IN3=%d IN4=%d]\r\n",
                data,
                (data >> 0) & 1,
                (data >> 1) & 1,
                (data >> 2) & 1,
                (data >> 3) & 1);

  Radio.Send(txpacket, BUFFER_SIZE);
  lora_idle = false;
  txInProgress = true;
  txCount++;
  lastTxTime = millis();
  lastStateSent = data;
}

// Actualitza el display OLED
void updateDisplay(uint8_t state) {
  display.clear();

  // Esborrar flag "ENVIAT" despres de 500ms
  if (txJustFinished && (millis() - txFlashTime > 500)) {
    txJustFinished = false;
  }

  // Linia 1: Titol + estat TX
  display.setFont(ArialMT_Plain_16);
  if (txInProgress) {
    display.drawString(0, 0, "ENVIANT...");
  } else if (txJustFinished) {
    display.drawString(0, 0, lastTxOk ? "ENVIAT OK" : "ERROR TX");
  } else if (txCount == 0) {
    display.drawString(0, 0, "EMISSOR");
  } else if (lastTxOk) {
    display.drawString(0, 0, "EMISSOR  OK");
  } else {
    display.drawString(0, 0, "EMISSOR FAIL");
  }

  display.setFont(ArialMT_Plain_10);

  // Linia 2: Estat entrades (confirmades) + indicador pendent
  char buf[48];
  // Mostrar estat confirmat amb '*' si te canvi pendent
  snprintf(buf, sizeof(buf), "IN: %d%c %d%c %d%c %d%c",
           confirmedState[0], hasPending[0] ? '*' : ' ',
           confirmedState[1], hasPending[1] ? '*' : ' ',
           confirmedState[2], hasPending[2] ? '*' : ' ',
           confirmedState[3], hasPending[3] ? '*' : ' ');
  display.drawString(0, 20, buf);

  // Linia 3: Comptadors TX
  snprintf(buf, sizeof(buf), "Enviats:%lu  Errors:%lu", txOkCount, txFailCount);
  display.drawString(0, 34, buf);

  // Linia 4: Temps des de l'ultim TX
  if (lastTxTime > 0) {
    unsigned long elapsed = (millis() - lastTxTime) / 1000;
    snprintf(buf, sizeof(buf), "Ultim TX: fa %lus", elapsed);
  } else {
    snprintf(buf, sizeof(buf), "Ultim TX: --");
  }
  display.drawString(0, 48, buf);

  display.display();
}

// Callbacks LoRa
void OnTxDone(void) {
  Serial.println("TX completat");
  lora_idle = true;
  txInProgress = false;
  lastTxOk = true;
  txOkCount++;
  txJustFinished = true;
  txFlashTime = millis();
}

void OnTxTimeout(void) {
  Serial.println("TX timeout - reintentant...");
  Radio.Sleep();
  lora_idle = true;
  txInProgress = false;
  lastTxOk = false;
  txFailCount++;
  txJustFinished = true;
  txFlashTime = millis();
  lastStateSent = 0xFF;  // Força reintent immediat al proper loop
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  Serial.println("\n=== EMISSOR LoRa Boia ===");

  // Configurar entrades amb pull-down
  pinMode(IN1_PIN, INPUT_PULLDOWN);
  pinMode(IN2_PIN, INPUT_PULLDOWN);
  pinMode(IN3_PIN, INPUT_PULLDOWN);
  pinMode(IN4_PIN, INPUT_PULLDOWN);

  // GPIO3 no conectat a res a l'emissor (MAX485 no poblat): forcar LOW per evitar floating
  pinMode(RS485_DE_RE_PIN, OUTPUT);
  digitalWrite(RS485_DE_RE_PIN, LOW);

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
  display.drawString(0, 20, "EMISSOR");
  display.drawString(0, 40, "Iniciant...");
  display.display();

  RadioEvents.TxDone = OnTxDone;
  RadioEvents.TxTimeout = OnTxTimeout;
  Radio.Init(&RadioEvents);
  Radio.SetChannel(RF_FREQUENCY);
  Radio.SetTxConfig(
    MODEM_LORA,
    TX_OUTPUT_POWER,
    0,  // fdev (no usat en LoRa)
    LORA_BANDWIDTH,
    LORA_SPREADING_FACTOR,
    LORA_CODINGRATE,
    LORA_PREAMBLE_LENGTH,
    LORA_FIX_LENGTH_PAYLOAD_ON,
    true,   // CRC activat
    0, 0,   // freq hop off
    LORA_IQ_INVERSION_ON,
    3000     // TX timeout ms
  );

  Serial.println("LoRa inicialitzat a 868 MHz");
  Serial.printf("SF%d, BW 125kHz, CR 4/5\r\n", LORA_SPREADING_FACTOR);

  // Primer enviament immediat
  delay(500);
  uint8_t currentState = readInputs();
  sendPacket(currentState);
}

void loop() {
  Radio.IrqProcess();

  unsigned long now = millis();
  uint8_t currentState = readInputs();

  // Enviar si hi ha canvi d'estat confirmat (despres de 3 min estable)
  if (currentState != lastStateSent && lora_idle) {
    Serial.println("Canvi confirmat detectat!");
    sendPacket(currentState);
  }
  // Enviar periodic cada TX_INTERVAL_MS
  else if ((now - lastTxTime >= TX_INTERVAL_MS) && lora_idle) {
    Serial.println("Enviament periodic");
    sendPacket(currentState);
  }

  // Actualitzar OLED
  updateDisplay(currentState);

  delay(POLL_INTERVAL_MS);
}
