// ============================================
// TEST MQTT - Connexio HiveMQ Cloud
// ESP32 generic (qualsevol placa amb WiFi)
// Envia valors aleatoris als topics boia/*
// ============================================

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

// --- WiFi ---
#define WIFI_SSID      "icon"
#define WIFI_PASSWORD  "rosroca22"

// --- MQTT HiveMQ Cloud ---
#define MQTT_SERVER    "e4382cc71099477ba76ea212327f55d3.s1.eu.hivemq.cloud"
#define MQTT_PORT      8883
#define MQTT_USER      "root"
#define MQTT_PASSWORD  "rosroca22"
#define MQTT_CLIENT_ID "test_esp32_boia"

// --- Topics (mateixos que el receptor real) ---
#define MQTT_TOPIC_STATUS  "boia/status"
#define MQTT_TOPIC_OUTPUTS "boia/outputs"
#define MQTT_TOPIC_INPUTS  "boia/inputs"
#define MQTT_TOPIC_DEYE    "boia/deye"
#define MQTT_TOPIC_LORA    "boia/lora"
#define MQTT_TOPIC_LOCALS  "boia/locals"

// --- Interval publicacio ---
#define PUBLISH_INTERVAL_MS  60000  // Cada 10 segons (mes rapid per testejar)

// Certificat root ISRG Root X1 (Let's Encrypt) - HiveMQ Cloud
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

WiFiClientSecure secureClient;
PubSubClient mqtt(secureClient);
unsigned long lastPublish = 0;
uint32_t publishCount = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n========================================");
  Serial.println("  TEST MQTT - HiveMQ Cloud");
  Serial.println("  Envia dades aleatories als topics boia/*");
  Serial.println("========================================\n");

  // --- WiFi ---
  Serial.printf("WiFi: connectant a '%s'...\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 40) {
    delay(500);
    Serial.print(".");
    tries++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("WiFi: OK! IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("WiFi: ERROR - no s'ha pogut connectar!");
    Serial.println("Revisa WIFI_SSID i WIFI_PASSWORD al codi");
    while (1) delay(1000);
  }

  // --- NTP (necessari per validar certificat TLS) ---
  Serial.println("NTP: sincronitzant rellotge...");
  configTime(3600, 3600, "pool.ntp.org", "time.nist.gov");
  while (time(nullptr) < 100000) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();
  time_t now = time(nullptr);
  Serial.printf("NTP: OK! %s", ctime(&now));

  // --- MQTT ---
  secureClient.setInsecure();
  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
  mqtt.setKeepAlive(60);
  mqtt.setSocketTimeout(10);
  mqtt.setBufferSize(512);

  // Seed random amb soroll analogic
  randomSeed(analogRead(0) + millis());

  Serial.println("\nSetup complet. Intentant connectar a MQTT...\n");
}

bool mqttConnect() {
  if (mqtt.connected()) return true;

  Serial.printf("MQTT: connectant a %s:%d (TLS)...\n", MQTT_SERVER, MQTT_PORT);

  bool ok = mqtt.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD,
                         MQTT_TOPIC_STATUS, 1, true, "offline");

  if (ok) {
    Serial.println("MQTT: CONNECTAT OK!");
    mqtt.publish(MQTT_TOPIC_STATUS, "online (test)", true);
    return true;
  } else {
    Serial.printf("MQTT: ERROR rc=%d\n", mqtt.state());
    Serial.println("  -2=connexio fallida (revisa URL/port/certificat)");
    Serial.println("  -1=connexio perduda");
    Serial.println("   4=credencials incorrectes (revisa user/password)");
    Serial.println("   5=no autoritzat");
    return false;
  }
}

void publishTestData() {
  char buf[256];
  publishCount++;

  Serial.printf("\n--- Publicacio #%lu ---\n", publishCount);

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
  Serial.printf("  %s: %s\n", MQTT_TOPIC_OUTPUTS, buf);

  // Entrades LoRa (valors aleatoris 0/1)
  snprintf(buf, sizeof(buf),
    "{\"in1\":%d,\"in2\":%d,\"in3\":%d,\"in4\":%d}",
    random(0, 2), random(0, 2), random(0, 2), random(0, 2));
  mqtt.publish(MQTT_TOPIC_INPUTS, buf);
  Serial.printf("  %s: %s\n", MQTT_TOPIC_INPUTS, buf);

  // Deye (SOC 20-100%, PV 0-6000W)
  int soc = random(20, 101);
  int pvPower = random(0, 6001);
  snprintf(buf, sizeof(buf),
    "{\"soc\":%d,\"pv_power\":%d}",
    soc, pvPower);
  mqtt.publish(MQTT_TOPIC_DEYE, buf);
  Serial.printf("  %s: %s\n", MQTT_TOPIC_DEYE, buf);

  // LoRa (RSSI -120 a -20 dBm)
  int rssi = random(-120, -19);
  snprintf(buf, sizeof(buf),
    "{\"connected\":%s,\"rssi\":%d,\"rx_ok\":%lu,\"rx_err\":%d}",
    random(0, 10) > 1 ? "true" : "false",  // 80% connectat
    rssi, publishCount * 30, random(0, 5));
  mqtt.publish(MQTT_TOPIC_LORA, buf);
  Serial.printf("  %s: %s\n", MQTT_TOPIC_LORA, buf);

  // Entrades locals
  snprintf(buf, sizeof(buf),
    "{\"boia_bomba\":%d,\"switch_mc\":%d,\"pot_max_min\":%d}",
    random(0, 2), random(0, 2), random(30, 241));
  mqtt.publish(MQTT_TOPIC_LOCALS, buf);
  Serial.printf("  %s: %s\n", MQTT_TOPIC_LOCALS, buf);

  Serial.println("  PUBLICAT OK!");
  lastPublish = millis();
}

void loop() {
  // Reconnectar si cal
  if (!mqtt.connected()) {
    if (!mqttConnect()) {
      Serial.println("Reintentant en 5 segons...");
      delay(5000);
      return;
    }
  }

  mqtt.loop();

  // Publicar cada PUBLISH_INTERVAL_MS
  if (millis() - lastPublish >= PUBLISH_INTERVAL_MS) {
    publishTestData();
  }
}
