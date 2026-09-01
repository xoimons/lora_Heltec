// ============================================
// RECEPTOR MQTT - Sistema LoRa + WiFi + MQTT
// Heltec WiFi LoRa 32 (V3)
// Basat en receptor.ino amb WiFi i HiveMQ Cloud
// VERSIO FAIL-SAFE: logica NC (Normally Closed)
// Boies amb contacte NC: 0=nivell OK o desconnectada, 1=falta nivell
// Sense Deye/RS485 (eliminat)
// ============================================

#include "LoRaWan_APP.h"
#include "HT_SSD1306Wire.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "config.h"

// Certificat root ISRG Root X1 (Let's Encrypt) - utilitzat per HiveMQ Cloud
// Valid fins 2035-06-04
static const char *hivemq_root_ca PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogZiUvsHurp5IuPF7TSKhQEfzMviwdg6RNvFbIvN/8HZaBr
ixO93PevL/yGaLc5oBhXUqkZXP5LOfQDJBzArlq2HseQ4eDgsS6rzmhLPfCFCRDE
Ma2FKGLTKB1VNFB/rvK0vXtjEGIaPXgFylIxeGJW3Cgz6b+F+XtNIaeasl9DuHsK
pKfOTcBQke0MRaBJVKiZL+lFiJDN6lSuBnAtJAQcUqKgGOpxCDP66Lp1aoeuYUE6
rO7ux/ECRXSMS4TCXHcb/eCFf/IS5ekKBBVUDJl+6KokMwRDIU9JOkJXRMSRgEwb
Fvor0DkfrEr1awVLkNQIGJPkHj6Jb/aFYfnuWKN2OlcFUQJLN2JzjAwnJT49HNcf
6EGVnjEeh0dHMFEKHsOar49JnYnCHAPTvBNcoeur0cZGMF8IPBn4MPaL/sMvzkCD
AQHfFJJnCi/RPOS5BWoFG/dGo5LcED9xFMOT8OBduGhLS1hDlBMRFsMSGbz0=
-----END CERTIFICATE-----
)EOF";

// OLED display
extern SSD1306Wire display;

// WiFi + MQTT (TLS)
WiFiClientSecure secureClient;
PubSubClient mqtt(secureClient);
unsigned long lastWifiAttempt = 0;
unsigned long lastMqttAttempt = 0;
unsigned long lastMqttPublish = 0;
unsigned long lastNtpAttempt = 0;
#define NTP_RETRY_MS 30000
bool wifiConnected = false;
bool mqttConnected = false;
bool ntpSynced = false;

// Variables LoRa
static RadioEvents_t RadioEvents;

// Estat recepcio
uint8_t lastStateReceived = 0x00;
uint8_t prevStateProcessed = 0xFF;
bool newLoRaData = false;
bool firstLoRaPacket = true;
unsigned long lastRxTime = 0;
bool timedOut = true;
int16_t lastRssi = 0;
uint32_t rxOkCount = 0;
uint32_t rxErrCount = 0;
bool rxJustReceived = false;
unsigned long rxFlashTime = 0;

// Estat sortides
bool out1State = false;
unsigned long out1StartTime = 0;
bool out2State = false;
bool out3State = false;
bool out4State = false;
bool boiaBombaState = false;
bool switchMitjaCarrega = false;

// Debounce entrades locals
#define LOCAL_INPUT_DEBOUNCE 3
uint8_t boiaBombaCount = 0;
uint8_t boiaBombaRaw = 0;
uint8_t switchMCCount = 0;
uint8_t switchMCRaw = 0;

// Cache potenciometre
uint32_t cachedMaxDurationMs = 0;
unsigned long lastPotRead = 0;
#define POT_READ_INTERVAL_MS 2000

// Flag per publicar MQTT quan hi ha canvis
bool mqttNeedsPublish = false;

// ============================================
// WiFi + NTP
// ============================================
void syncNTP() {
  if (ntpSynced) return;
  Serial.println("NTP: sincronitzant rellotge...");
  configTime(3600, 3600, "pool.ntp.org", "time.google.com", "time.cloudflare.com");
  unsigned long start = millis();
  while (time(nullptr) < 100000 && (millis() - start < 15000)) {
    delay(500);
    Radio.IrqProcess();
    Serial.print(".");
  }
  Serial.println();
  time_t now = time(nullptr);
  if (now > 100000) {
    ntpSynced = true;
    Serial.printf("NTP: sincronitzat! %s", ctime(&now));
  } else {
    Serial.printf("NTP: error sincronitzacio (time=%ld, reintentara)\r\n", (long)now);
  }
}

void wifiSetup() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  Serial.printf("WiFi: connectant a '%s'...\r\n", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start < WIFI_TIMEOUT_MS)) {
    delay(250);
    Radio.IrqProcess();
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.printf("WiFi: connectat! IP: %s\r\n", WiFi.localIP().toString().c_str());
    syncNTP();
  } else {
    wifiConnected = false;
    Serial.println("WiFi: no s'ha pogut connectar (reintentara)");
  }
  lastWifiAttempt = millis();
}

void wifiReconnect() {
  if (WiFi.status() == WL_CONNECTED) {
    if (!wifiConnected) {
      wifiConnected = true;
      Serial.printf("WiFi: reconnectat! IP: %s\r\n", WiFi.localIP().toString().c_str());
      if (!ntpSynced) syncNTP();
    }
    return;
  }

  wifiConnected = false;
  if (millis() - lastWifiAttempt < WIFI_RECONNECT_MS) return;

  Serial.println("WiFi: reintentant connexio...");
  WiFi.disconnect();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  lastWifiAttempt = millis();
}

// ============================================
// MQTT (HiveMQ Cloud)
// ============================================
void mqttSetup() {
  secureClient.setInsecure();  // Saltar verificacio certificat (ESP32-S3 Heltec V3)
  Serial.println("MQTT: TLS sense verificacio certificat");
  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
  mqtt.setBufferSize(512);
  mqtt.setKeepAlive(60);
}

void mqttReconnect() {
  if (!wifiConnected) {
    mqttConnected = false;
    return;
  }

  if (mqtt.connected()) {
    if (!mqttConnected) {
      mqttConnected = true;
      Serial.println("MQTT: connectat a HiveMQ Cloud!");
    }
    return;
  }

  mqttConnected = false;
  if (millis() - lastMqttAttempt < MQTT_RECONNECT_MS) return;

  Serial.printf("MQTT: connectant a %s:%d (TLS)...\r\n", MQTT_SERVER, MQTT_PORT);
  lastMqttAttempt = millis();

  bool ok = mqtt.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD,
                         MQTT_TOPIC_STATUS, 1, true, "offline");

  if (ok) {
    mqttConnected = true;
    Serial.println("MQTT: connectat a HiveMQ Cloud!");
    mqtt.publish(MQTT_TOPIC_STATUS, "online", true);
  } else {
    Serial.printf("MQTT: error connexio, rc=%d\r\n", mqtt.state());
  }
}

void mqttPublishAll() {
  if (!mqttConnected) return;

  char buf[256];

  // Estat sortides
  float out1Min = 0;
  if (out1State && out1StartTime > 0) {
    out1Min = (millis() - out1StartTime) / 60000.0;
  }
  snprintf(buf, sizeof(buf),
    "{\"out1\":%d,\"out2\":%d,\"out3\":%d,\"out4\":%d,\"out1_min\":%.1f}",
    out1State, out2State, out3State, out4State, out1Min);
  mqtt.publish(MQTT_TOPIC_OUTPUTS, buf);

  // Entrades LoRa
  snprintf(buf, sizeof(buf),
    "{\"in1\":%d,\"in2\":%d,\"in3\":%d,\"in4\":%d}",
    (lastStateReceived >> 0) & 1, (lastStateReceived >> 1) & 1,
    (lastStateReceived >> 2) & 1, (lastStateReceived >> 3) & 1);
  mqtt.publish(MQTT_TOPIC_INPUTS, buf);

  // Estat LoRa
  snprintf(buf, sizeof(buf),
    "{\"connected\":%s,\"rssi\":%d,\"rx_ok\":%lu,\"rx_err\":%lu}",
    timedOut ? "false" : "true", lastRssi, rxOkCount, rxErrCount);
  mqtt.publish(MQTT_TOPIC_LORA, buf);

  // Entrades locals
  snprintf(buf, sizeof(buf),
    "{\"boia_bomba\":%d,\"switch_mc\":%d,\"pot_max_min\":%lu}",
    boiaBombaState, switchMitjaCarrega, cachedMaxDurationMs / 60000UL);
  mqtt.publish(MQTT_TOPIC_LOCALS, buf);

  lastMqttPublish = millis();
  mqttNeedsPublish = false;

  Serial.println("MQTT: publicat OK");
}

// ============================================
// Potenciometre
// ============================================
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
// ============================================
// LOGICA FAIL-SAFE (NC - Normally Closed):
// - Boies cablejades amb contacte NC: boia avall (falta aigua) = NC tancat = 1, boia amunt (hi ha aigua) = NC obert = 0
// - Boia desconnectada o cable tallat = pin LOW (pull-down) = 0 = bomba OFF (seguretat)
// - Arranca bomba: IN1=1 I boiaBomba=1
// - Para bomba:    IN1=0 O boiaBomba=0 O (switchMitja=1 I IN2LoRa=0) O potenciometre O timeout
// NOTA: boiaBomba usa contacte NO (no NC) per ser fail-safe: desconnectada=0=para bomba
void evaluateOut1() {
  uint8_t in1 = (lastStateReceived >> 0) & 1;
  uint8_t in2 = (lastStateReceived >> 1) & 1;

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
  } else if (in1) {
    // NC: in1=1 significa boia avall (falta aigua al diposit desti) = arrancar bomba
    if (!out1State) {
      out1StartTime = millis();
    }
    out1State = true;
  } else if (!in1) {
    // NC: in1=0 significa boia amunt (hi ha aigua) O boia desconnectada = parar bomba (fail-safe)
    out1State = false;
    out1StartTime = 0;
  }

  if (out1State != prevOut1) {
    digitalWrite(OUT1_PIN, out1State ? HIGH : LOW);
    Serial.printf("OUT1: %s\r\n", out1State ? "ACTIU" : "ATURAT");
    mqttNeedsPublish = true;
  }
}

// ============================================
// Sortides 2, 3, 4 (invertides per fail-safe NC)
// NC: senyal 0 = boia amunt (nivell assolit) O desconnectada -> sortida OFF (fail-safe)
// NC: senyal 1 = boia avall (nivell no assolit) -> sortida ON
// ============================================
void evaluateOut2() {
  bool newState = (lastStateReceived >> 1) & 1;
  if (newState != out2State) {
    out2State = newState;
    digitalWrite(OUT2_PIN, out2State ? HIGH : LOW);
    Serial.printf("OUT2: %d\r\n", out2State);
    mqttNeedsPublish = true;
  }
}

void evaluateOut3() {
  bool newState = (lastStateReceived >> 2) & 1;
  if (newState != out3State) {
    out3State = newState;
    digitalWrite(OUT3_PIN, out3State ? HIGH : LOW);
    Serial.printf("OUT3: %d\r\n", out3State);
    mqttNeedsPublish = true;
  }
}

void evaluateOut4() {
  bool newState = (lastStateReceived >> 3) & 1;
  if (newState != out4State) {
    out4State = newState;
    digitalWrite(OUT4_PIN, out4State ? HIGH : LOW);
    Serial.printf("OUT4: %d\r\n", out4State);
    mqttNeedsPublish = true;
  }
}

// Desactiva sortides (seguretat)
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
  mqttNeedsPublish = true;
}

// ============================================
// Display OLED
// ============================================
void updateDisplay() {
  display.clear();

  if (rxJustReceived && (millis() - rxFlashTime > 500)) {
    rxJustReceived = false;
  }

  // Linia 1: estat connexio LoRa
  display.setFont(ArialMT_Plain_16);
  if (rxJustReceived) {
    display.drawString(0, 0, "REBENT...");
  } else if (timedOut) {
    display.drawString(0, 0, "SENSE SENYAL");
  } else {
    display.drawString(0, 0, "RECEPTOR OK");
  }

  display.setFont(ArialMT_Plain_10);

  // Linia 2: OUT1 + boia bomba + temps maxim
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
  display.drawString(0, 18, buf);

  // Linia 3: RSSI + sortides OUT2/OUT3/OUT4
  snprintf(buf, sizeof(buf), "RSSI:%d O2:%d O3:%d O4:%d",
           lastRssi, out2State, out3State, out4State);
  display.drawString(0, 30, buf);

  // Linia 4: entrades LoRa
  snprintf(buf, sizeof(buf), "IN: %d  %d  %d  %d",
           (lastStateReceived >> 0) & 1, (lastStateReceived >> 1) & 1,
           (lastStateReceived >> 2) & 1, (lastStateReceived >> 3) & 1);
  display.drawString(0, 42, buf);

  // Linia 5: estat WiFi + MQTT
  snprintf(buf, sizeof(buf), "W:%s M:%s",
           wifiConnected ? "OK" : "--",
           mqttConnected ? "OK" : "--");
  display.drawString(0, 54, buf);

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
  lastRxTime = millis();
  lastStateReceived = data;
  timedOut = false;
  rxOkCount++;
  rxJustReceived = true;
  rxFlashTime = millis();
  newLoRaData = true;

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

// ============================================
// SETUP
// ============================================
void setup() {
  Serial.begin(SERIAL_BAUD);
  Serial.println("\n=== RECEPTOR LoRa+MQTT Boia (FAIL-SAFE, sense Deye) ===");

  // Configurar sortides
  pinMode(OUT1_PIN, OUTPUT);
  pinMode(OUT2_PIN, OUTPUT);
  pinMode(OUT3_PIN, OUTPUT);
  pinMode(OUT4_PIN, OUTPUT);
  safetyShutdown();

  // Configurar entrades locals
  pinMode(BOIA_BOMBA_PIN, INPUT_PULLDOWN);
  pinMode(SWITCH_MITJA_CARREGA_PIN, INPUT_PULLDOWN);

  // Potenciometre
  analogSetPinAttenuation(POT_PIN, ADC_11db);
  updatePotCache();

  // Inicialitzar LoRa
  Mcu.begin();

  // Activar Vext (OLED)
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
  delay(50);

  // Inicialitzar OLED
  display.init();
  display.flipScreenVertically();
  display.setContrast(255, 241, 64);
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 10, "RECEPTOR MQTT");
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 35, "Iniciant...");
  display.display();

  // LoRa
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
    true,
    0, 0,
    LORA_IQ_INVERSION_ON,
    true
  );

  Serial.println("LoRa inicialitzat a 868 MHz");
  Serial.printf("SF%d, BW 125kHz, CR 4/5\r\n", LORA_SPREADING_FACTOR);
  Radio.Rx(0);

  // WiFi
  wifiSetup();

  // MQTT
  mqttSetup();
}

// ============================================
// LOOP
// ============================================
void loop() {
  Radio.IrqProcess();

  // --- WiFi reconnexio ---
  wifiReconnect();

  // --- NTP reintent periodic ---
  if (wifiConnected && !ntpSynced && (millis() - lastNtpAttempt >= NTP_RETRY_MS)) {
    lastNtpAttempt = millis();
    syncNTP();
  }

  // --- MQTT reconnexio i loop ---
  mqttReconnect();
  if (mqttConnected) {
    mqtt.loop();
  }

  // --- Entrades locals amb debounce ---
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

  if (localChanged && !timedOut) {
    evaluateOut1();
    mqttNeedsPublish = true;
  }

  // --- Timeout seguretat LoRa ---
  if (!timedOut && lastRxTime > 0 && (millis() - lastRxTime > RX_TIMEOUT_MS)) {
    timedOut = true;
    safetyShutdown();
  }

  // --- Potenciometre ---
  if (millis() - lastPotRead >= POT_READ_INTERVAL_MS) {
    uint32_t prevMaxDuration = cachedMaxDurationMs;
    updatePotCache();
    if (prevMaxDuration == 0 && cachedMaxDurationMs > 0 && !timedOut) {
      evaluateOut1();
    }
  }

  // --- Duracio maxima OUT1 ---
  if (out1State && out1StartTime > 0) {
    if (cachedMaxDurationMs == 0 || (millis() - out1StartTime >= cachedMaxDurationMs)) {
      Serial.printf("MAX DURACIO: OUT1 desactivada (%lu min)\r\n", cachedMaxDurationMs / 60000UL);
      out1State = false;
      out1StartTime = 0;
      digitalWrite(OUT1_PIN, LOW);
      mqttNeedsPublish = true;
    }
  }

  // --- Processar dades LoRa noves ---
  if (newLoRaData) {
    newLoRaData = false;

    if (firstLoRaPacket) {
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

      if (changed & 0x03) evaluateOut1();
      if (changed & 0x02) evaluateOut2();
      if (changed & 0x04) evaluateOut3();
      if (changed & 0x08) evaluateOut4();
    }

    mqttNeedsPublish = true;

    Serial.printf("ESTAT: O1:%s O2:%d O3:%d O4:%d (IN1=%d IN2=%d BB=%d MC=%d RSSI=%d)\r\n",
                  out1State ? "ACTIU" : "ATURAT", out2State, out3State, out4State,
                  (lastStateReceived >> 0) & 1, (lastStateReceived >> 1) & 1,
                  boiaBombaState, switchMitjaCarrega, lastRssi);
  }

  // --- Publicar MQTT: cada 30 segons o quan hi ha canvis ---
  if (mqttConnected) {
    if (mqttNeedsPublish || (millis() - lastMqttPublish >= MQTT_PUBLISH_INTERVAL_MS)) {
      mqttPublishAll();
    }
  }

  updateDisplay();
  delay(100);
}
