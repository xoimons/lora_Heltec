// ============================================
// Configuracio - ESP32 + MAX485 -> Inversor Deye
// ============================================
#ifndef CONFIG_H
#define CONFIG_H

// --- WiFi ---
#define WIFI_SSID      "Solar_Ossera"
#define WIFI_PASSWORD  "Solar_Ossera_26"
#define WIFI_TIMEOUT_MS       15000    // Temps maxim per connectar WiFi (ms)
#define WIFI_RECONNECT_MS     30000    // Interval entre intents de reconnexio WiFi (ms)

// --- RS485 / Modbus (modul MAX485 TTL) ---
// ESP32 DevKit generic: Serial2 per defecte (UART2)
#define RS485_RX_PIN      16    // RO del MAX485 -> GPIO16 (RX2)
#define RS485_TX_PIN      17    // DI del MAX485 -> GPIO17 (TX2)
#define RS485_DE_RE_PIN   4     // DE+RE units al MAX485 -> GPIO4
#define RS485_BAUD        9600
#define MODBUS_SLAVE_ID   1     // Slave ID del Deye

// --- Registres Modbus del Deye (holding registers) ---
#define DEYE_REG_SOC      184   // SOC bateria (%)
#define DEYE_REG_PV_POWER 186   // Produccio PV1 (W)

// --- Lectura periodica ---
#define DEYE_READ_INTERVAL_MS  5000    // Interval entre lectures Modbus (ms)

// --- MQTT (HiveMQ Cloud - mateix broker que el projecte boia) ---
#define MQTT_SERVER    "e4382cc71099477ba76ea212327f55d3.s1.eu.hivemq.cloud"
#define MQTT_PORT      8883                       // TLS obligatori HiveMQ Cloud
#define MQTT_USER      "Ossera"
#define MQTT_PASSWORD  "Ossera_26"
#define MQTT_CLIENT_ID "esp32_deye"                // ID unic (diferent del receptor boia!)
#define MQTT_RECONNECT_MS      5000                // Interval entre intents reconnexio MQTT (ms)
#define MQTT_PUBLISH_INTERVAL_MS  15000            // Publicar dades Deye cada 15s

// Topics MQTT (publicacio)
#define MQTT_TOPIC_PREFIX  "deye/"
#define MQTT_TOPIC_STATUS  MQTT_TOPIC_PREFIX "status"   // Online/offline (LWT)
#define MQTT_TOPIC_DATA    MQTT_TOPIC_PREFIX "data"     // SOC + potencia PV (JSON)

#endif
