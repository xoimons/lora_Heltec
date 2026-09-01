// ============================================
// ESP32 + MAX485 -> Inversor Deye (Modbus RTU)
// WiFi: Solar_Ossera
// ============================================
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ModbusMaster.h>
#include "config.h"

ModbusMaster modbus;

WiFiClientSecure secureClient;
PubSubClient mqtt(secureClient);

bool wifiConnected = false;
bool mqttConnected = false;
unsigned long lastWifiAttempt = 0;
unsigned long lastMqttAttempt = 0;
unsigned long lastMqttPublish = 0;
unsigned long lastDeyeRead = 0;

// Ultimes lectures Deye (per publicar via MQTT)
int deyeSoc = -1;
int deyePvPower = -1;

// --- Control direccio RS485 (DE/RE) ---
void rs485PreTransmission() {
  digitalWrite(RS485_DE_RE_PIN, HIGH);
  delayMicroseconds(50);
}

void rs485PostTransmission() {
  Serial2.flush();
  delayMicroseconds(50);
  digitalWrite(RS485_DE_RE_PIN, LOW);
}

// --- WiFi ---
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

// --- Lectura Deye via Modbus ---
void readDeye() {
  uint8_t result = modbus.readHoldingRegisters(DEYE_REG_SOC, 1);
  if (result == modbus.ku8MBSuccess) {
    deyeSoc = modbus.getResponseBuffer(0);
    Serial.printf("Deye SOC: %d%%\r\n", deyeSoc);
  } else {
    Serial.printf("Deye SOC: error lectura (codi 0x%02X)\r\n", result);
  }

  result = modbus.readHoldingRegisters(DEYE_REG_PV_POWER, 1);
  if (result == modbus.ku8MBSuccess) {
    deyePvPower = modbus.getResponseBuffer(0);
    Serial.printf("Deye PV Power: %d W\r\n", deyePvPower);
  } else {
    Serial.printf("Deye PV Power: error lectura (codi 0x%02X)\r\n", result);
  }
}

// --- MQTT (HiveMQ Cloud) ---
void mqttSetup() {
  secureClient.setInsecure();  // Saltar verificacio certificat
  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
  mqtt.setBufferSize(256);
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

void mqttPublishDeye() {
  if (!mqttConnected) return;
  if (deyeSoc < 0 || deyePvPower < 0) return;  // encara sense lectura valida

  char buf[128];
  snprintf(buf, sizeof(buf), "{\"soc\":%d,\"pv_power\":%d}", deyeSoc, deyePvPower);
  mqtt.publish(MQTT_TOPIC_DATA, buf);

  lastMqttPublish = millis();
  Serial.println("MQTT: publicat OK");
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=== ESP32 -> Deye (RS485/Modbus) + WiFi ===");

  // RS485
  pinMode(RS485_DE_RE_PIN, OUTPUT);
  digitalWrite(RS485_DE_RE_PIN, LOW);
  Serial2.begin(RS485_BAUD, SERIAL_8N1, RS485_RX_PIN, RS485_TX_PIN);
  modbus.begin(MODBUS_SLAVE_ID, Serial2);
  modbus.preTransmission(rs485PreTransmission);
  modbus.postTransmission(rs485PostTransmission);
  Serial.printf("RS485: %d baud, TX=GPIO%d, RX=GPIO%d, DE/RE=GPIO%d, Slave ID=%d\r\n",
                RS485_BAUD, RS485_TX_PIN, RS485_RX_PIN, RS485_DE_RE_PIN, MODBUS_SLAVE_ID);

  // WiFi
  wifiSetup();

  // MQTT
  mqttSetup();
}

void loop() {
  wifiReconnect();

  mqttReconnect();
  if (mqttConnected) {
    mqtt.loop();
  }

  if (millis() - lastDeyeRead >= DEYE_READ_INTERVAL_MS) {
    lastDeyeRead = millis();
    readDeye();
  }

  if (mqttConnected && (millis() - lastMqttPublish >= MQTT_PUBLISH_INTERVAL_MS)) {
    mqttPublishDeye();
  }
}
