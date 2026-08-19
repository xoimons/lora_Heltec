// ============================================
// TEST MQTT - Connexio HiveMQ Cloud
// ESP32 generic (qualsevol placa amb WiFi)
// Envia valors aleatoris als topics boia/*
//
// Replica la logica de connexio del receptor real
// (receptor_mqtt.ino) per validar:
//   - Connexio WiFi amb reconnexio automatica
//   - Sincronitzacio NTP amb timeout
//   - Connexio MQTT TLS amb certificat CA
//   - Publicacio JSON a tots els topics
//   - Reconnexio automatica WiFi i MQTT
//
// Canviar WIFI_SSID, WIFI_PASSWORD, MQTT_SERVER,
// MQTT_USER, MQTT_PASSWORD abans de compilar.
// ============================================

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

// --- WiFi (modem 4G o xarxa local) ---
#define WIFI_SSID              "icon"
#define WIFI_PASSWORD          "rosroca22"
#define WIFI_TIMEOUT_MS        15000     // Temps maxim per connectar WiFi (ms)
#define WIFI_RECONNECT_MS      30000     // Interval entre intents de reconnexio WiFi (ms)

// --- MQTT (HiveMQ Cloud - TLS obligatori) ---
#define MQTT_SERVER            "e4382cc71099477ba76ea212327f55d3.s1.eu.hivemq.cloud"
#define MQTT_PORT              8883
#define MQTT_USER              "Ossera"
#define MQTT_PASSWORD          "Ossera_26"
#define MQTT_CLIENT_ID         "test_esp32_boia"
#define MQTT_RECONNECT_MS      5000      // Interval entre intents reconnexio MQTT (ms)

// Topics MQTT (publicacio) - canviar prefix per adaptar a client
#define MQTT_TOPIC_PREFIX      "boia/"
#define MQTT_TOPIC_STATUS      MQTT_TOPIC_PREFIX "status"
#define MQTT_TOPIC_OUTPUTS     MQTT_TOPIC_PREFIX "outputs"
#define MQTT_TOPIC_INPUTS      MQTT_TOPIC_PREFIX "inputs"
#define MQTT_TOPIC_DEYE        MQTT_TOPIC_PREFIX "deye"
#define MQTT_TOPIC_LORA        MQTT_TOPIC_PREFIX "lora"
#define MQTT_TOPIC_LOCALS      MQTT_TOPIC_PREFIX "locals"
#define MQTT_PUBLISH_INTERVAL_MS  30000  // Cada 30 segons (mes rapid que receptor per testejar)

// Certificat root ISRG Root X1 (Let's Encrypt) - utilitzat per HiveMQ Cloud
// Valid fins 2035-06-04
static const char *root_ca PROGMEM = R"EOF(
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

// WiFi + MQTT (TLS)
WiFiClientSecure secureClient;
PubSubClient mqtt(secureClient);
unsigned long lastWifiAttempt = 0;
unsigned long lastMqttAttempt = 0;
unsigned long lastMqttPublish = 0;
bool wifiConnected = false;
bool mqttConnected = false;
bool ntpSynced = false;
uint32_t publishCount = 0;

// ============================================
// WiFi + NTP (identic al receptor real)
// ============================================
void syncNTP() {
  if (ntpSynced) return;
  Serial.println("NTP: sincronitzant rellotge...");
  configTime(3600, 3600, "pool.ntp.org", "time.nist.gov");
  unsigned long start = millis();
  while (time(nullptr) < 100000 && (millis() - start < 10000)) {
    delay(250);
  }
  time_t now = time(nullptr);
  if (now > 100000) {
    ntpSynced = true;
    Serial.printf("NTP: sincronitzat! %s", ctime(&now));
  } else {
    Serial.println("NTP: error sincronitzacio (reintentara)");
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
// MQTT (identic al receptor real)
// ============================================
void mqttSetup() {
  secureClient.setCACert(root_ca);
  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
  mqtt.setBufferSize(512);
}

void mqttReconnect() {
  if (!wifiConnected || !ntpSynced) {
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
    Serial.println("  -2=connexio fallida (revisa URL/port/certificat)");
    Serial.println("  -1=connexio perduda");
    Serial.println("   4=credencials incorrectes (revisa user/password)");
    Serial.println("   5=no autoritzat");
  }
}

// ============================================
// Publicacio dades simulades
// Format JSON identic al receptor real
// ============================================
void mqttPublishAll() {
  if (!mqttConnected) return;

  char buf[256];
  publishCount++;

  Serial.printf("\n--- Publicacio #%lu ---\r\n", publishCount);

  // Sortides (valors aleatoris 0/1)
  int out1 = random(0, 2);
  int out2 = random(0, 2);
  int out3 = random(0, 2);
  int out4 = random(0, 2);
  float out1Min = out1 ? random(0, 1800) / 10.0 : 0;
  snprintf(buf, sizeof(buf),
    "{\"out1\":%d,\"out2\":%d,\"out3\":%d,\"out4\":%d,\"out1_min\":%.1f}",
    out1, out2, out3, out4, out1Min);
  mqtt.publish(MQTT_TOPIC_OUTPUTS, buf);
  Serial.printf("  %s: %s\r\n", MQTT_TOPIC_OUTPUTS, buf);

  // Entrades LoRa (valors aleatoris 0/1)
  snprintf(buf, sizeof(buf),
    "{\"in1\":%d,\"in2\":%d,\"in3\":%d,\"in4\":%d}",
    random(0, 2), random(0, 2), random(0, 2), random(0, 2));
  mqtt.publish(MQTT_TOPIC_INPUTS, buf);
  Serial.printf("  %s: %s\r\n", MQTT_TOPIC_INPUTS, buf);

  // Deye (SOC 20-100%, PV 0-6000W)
  int soc = random(20, 101);
  int pvPower = random(0, 6001);
  snprintf(buf, sizeof(buf),
    "{\"soc\":%d,\"pv_power\":%d}",
    soc, pvPower);
  mqtt.publish(MQTT_TOPIC_DEYE, buf);
  Serial.printf("  %s: %s\r\n", MQTT_TOPIC_DEYE, buf);

  // LoRa (RSSI -120 a -20 dBm)
  int rssi = random(-120, -19);
  snprintf(buf, sizeof(buf),
    "{\"connected\":%s,\"rssi\":%d,\"rx_ok\":%lu,\"rx_err\":%d}",
    random(0, 10) > 1 ? "true" : "false",
    rssi, publishCount * 30, random(0, 5));
  mqtt.publish(MQTT_TOPIC_LORA, buf);
  Serial.printf("  %s: %s\r\n", MQTT_TOPIC_LORA, buf);

  // Entrades locals
  snprintf(buf, sizeof(buf),
    "{\"boia_bomba\":%d,\"switch_mc\":%d,\"pot_max_min\":%d}",
    random(0, 2), random(0, 2), random(30, 241));
  mqtt.publish(MQTT_TOPIC_LOCALS, buf);
  Serial.printf("  %s: %s\r\n", MQTT_TOPIC_LOCALS, buf);

  lastMqttPublish = millis();

  Serial.println("  PUBLICAT OK!");
}

// ============================================
// SETUP
// ============================================
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n========================================");
  Serial.println("  TEST MQTT - HiveMQ Cloud");
  Serial.println("  Replica connexio del receptor real");
  Serial.println("  Envia dades simulades als topics");
  Serial.printf("  Prefix topics: %s\n", MQTT_TOPIC_PREFIX);
  Serial.println("========================================\n");

  // Seed random amb soroll analogic
  randomSeed(analogRead(0) + millis());

  // WiFi (mateixa logica que receptor)
  wifiSetup();

  // MQTT (mateixa logica que receptor)
  mqttSetup();

  Serial.println("\nSetup complet.\n");
}

// ============================================
// LOOP (mateixa estructura que receptor)
// ============================================
void loop() {
  // --- WiFi reconnexio ---
  wifiReconnect();

  // --- MQTT reconnexio i loop ---
  mqttReconnect();
  if (mqttConnected) {
    mqtt.loop();
  }

  // --- Publicar cada MQTT_PUBLISH_INTERVAL_MS ---
  if (mqttConnected && (millis() - lastMqttPublish >= MQTT_PUBLISH_INTERVAL_MS)) {
    mqttPublishAll();
  }

  delay(100);
}
