// ============================================
// RECEPTOR - Replica d'entrades via LoRa
// Heltec WiFi LoRa 32 (V3)
//
// Cada sortida replica directament l'entrada
// LoRa corresponent: OUT1=IN1, OUT2=IN2, etc.
// ============================================

#include "LoRaWan_APP.h"
#include "HT_SSD1306Wire.h"
#include "config.h"

extern SSD1306Wire display;

// --- LoRa ---
static RadioEvents_t RadioEvents;

// --- Estat recepcio ---
uint8_t lastStateReceived = 0x00;
int16_t lastRssi = 0;
uint32_t rxOkCount = 0;
uint32_t rxErrCount = 0;
bool rxJustReceived = false;
unsigned long rxFlashTime = 0;
bool everReceived = false;

// --- Estat sortides ---
bool outState[4] = { false, false, false, false };
const uint8_t outPins[4] = { OUT1_PIN, OUT2_PIN, OUT3_PIN, OUT4_PIN };

// ============================================
// Actualitza sortides (replica directa)
// ============================================
void updateOutputs(uint8_t loraState) {
  for (int i = 0; i < 4; i++) {
    outState[i] = (loraState >> i) & 1;
    digitalWrite(outPins[i], outState[i] ? HIGH : LOW);
  }

  Serial.printf("OUT: %d %d %d %d\r\n",
                outState[0], outState[1], outState[2], outState[3]);
}

// ============================================
// Display OLED
// ============================================
void updateDisplay() {
  display.clear();

  if (rxJustReceived && (millis() - rxFlashTime > 500)) {
    rxJustReceived = false;
  }

  // Linia 1: estat connexio
  display.setFont(ArialMT_Plain_16);
  if (rxJustReceived) {
    display.drawString(0, 0, "REBUT!");
  } else if (!everReceived) {
    display.drawString(0, 0, "ESPERANT...");
  } else {
    display.drawString(0, 0, "RECEPTOR OK");
  }

  display.setFont(ArialMT_Plain_10);
  char buf[40];

  // Linia 2: estat sortides
  snprintf(buf, sizeof(buf), "OUT: %d  %d  %d  %d",
           outState[0], outState[1], outState[2], outState[3]);
  display.drawString(0, 20, buf);

  // Linia 3: entrades rebudes per LoRa
  snprintf(buf, sizeof(buf), "IN:  %d  %d  %d  %d",
           (lastStateReceived >> 0) & 1,
           (lastStateReceived >> 1) & 1,
           (lastStateReceived >> 2) & 1,
           (lastStateReceived >> 3) & 1);
  display.drawString(0, 34, buf);

  // Linia 4: comptador paquets + RSSI
  snprintf(buf, sizeof(buf), "Pkt:%lu Err:%lu RSSI:%d",
           rxOkCount, rxErrCount, lastRssi);
  display.drawString(0, 48, buf);

  display.display();
}

// ============================================
// Callbacks LoRa
// ============================================
void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
  if (size < BUFFER_SIZE) {
    Radio.Rx(0);
    return;
  }

  uint8_t data = payload[0];
  lastRssi = rssi;
  lastStateReceived = data;
  everReceived = true;
  rxOkCount++;
  rxJustReceived = true;
  rxFlashTime = millis();

  Serial.printf("RX: 0x%02X [IN1=%d IN2=%d IN3=%d IN4=%d] RSSI:%d Pkt#%lu\r\n",
                data,
                (data >> 0) & 1, (data >> 1) & 1,
                (data >> 2) & 1, (data >> 3) & 1,
                rssi, rxOkCount);

  updateOutputs(data);
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

// ============================================
// Setup
// ============================================
void setup() {
  Serial.begin(SERIAL_BAUD);
  Serial.println("\n=== RECEPTOR LoRa - Replica d'entrades ===");

  // Sortides inicials a LOW
  for (int i = 0; i < 4; i++) {
    pinMode(outPins[i], OUTPUT);
    digitalWrite(outPins[i], LOW);
  }

  // RS485 DE/RE a LOW (no utilitzat)
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
  display.drawString(0, 20, "RECEPTOR");
  display.drawString(0, 40, "Iniciant...");
  display.display();

  // LoRa
  RadioEvents.RxDone    = OnRxDone;
  RadioEvents.RxTimeout = OnRxTimeout;
  RadioEvents.RxError   = OnRxError;
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
    true,    // CRC activat
    0, 0,    // freq hop off
    LORA_IQ_INVERSION_ON,
    true     // rxContinuous
  );

  Serial.printf("LoRa: 868 MHz, SF%d, BW 125kHz, CR 4/5\r\n",
                LORA_SPREADING_FACTOR);
  Radio.Rx(0);
}

// ============================================
// Loop principal
// ============================================
void loop() {
  Radio.IrqProcess();
  updateDisplay();
  delay(100);
}
