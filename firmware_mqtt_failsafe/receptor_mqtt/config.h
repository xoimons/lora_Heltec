#ifndef CONFIG_H
#define CONFIG_H

// ============================================
// Configuracio Receptor MQTT - VERSIO FAIL-SAFE
// Sistema LoRa + WiFi + MQTT (HiveMQ Cloud)
// Heltec WiFi LoRa 32 (V3)
// Logica NC (Normally Closed) per totes les boies
// Sense Deye/RS485 (eliminat)
// ============================================

// --- WiFi (modem 4G) ---
#define WIFI_SSID      "icon"               // Nom xarxa WiFi del modem 4G
#define WIFI_PASSWORD  "rosroca22"           // Contrasenya WiFi del modem 4G
#define WIFI_TIMEOUT_MS       15000          // Temps maxim per connectar WiFi (ms)
#define WIFI_RECONNECT_MS     30000          // Interval entre intents de reconnexio WiFi (ms)

// --- MQTT (HiveMQ Cloud - TLS obligatori) ---
#define MQTT_SERVER    "e4382cc71099477ba76ea212327f55d3.s1.eu.hivemq.cloud"  // URL del cluster HiveMQ Cloud
#define MQTT_PORT      8883                           // Port MQTT amb TLS (obligatori HiveMQ Cloud)
#define MQTT_USER      "root"                          // Usuari creat a Access Management de HiveMQ
#define MQTT_PASSWORD  "rosroca22"                     // Contrasenya creada a Access Management
#define MQTT_CLIENT_ID "receptor_lora_boia"            // ID unic del client MQTT
#define MQTT_RECONNECT_MS     5000                     // Interval entre intents reconnexio MQTT (ms)

// Topics MQTT (publicacio)
#define MQTT_TOPIC_PREFIX      "boia/"
#define MQTT_TOPIC_STATUS      MQTT_TOPIC_PREFIX "status"          // Online/offline
#define MQTT_TOPIC_OUTPUTS     MQTT_TOPIC_PREFIX "outputs"         // Estat sortides JSON
#define MQTT_TOPIC_INPUTS      MQTT_TOPIC_PREFIX "inputs"          // Estat entrades LoRa JSON
#define MQTT_TOPIC_LORA        MQTT_TOPIC_PREFIX "lora"            // Estat connexio LoRa
#define MQTT_TOPIC_LOCALS      MQTT_TOPIC_PREFIX "locals"          // Entrades locals (boia bomba, switch)
#define MQTT_PUBLISH_INTERVAL_MS  30000       // Publicar estat cada 30 segons

// --- Pins de sortida ---
#define OUT1_PIN  6
#define OUT2_PIN  47
#define OUT3_PIN  7
#define OUT4_PIN  48

// --- Boia diposit bomba (entrada local al receptor, FAIL-SAFE NO) ---
// Cablejat NO (Normally Open): boia amunt (aigua) = NO tancat = 1, boia avall (sec) = NO obert = 0
// Desconnectada = pin LOW (pull-down) = 0 = para bomba (fail-safe marxa en sec)
// NOTA: mateixa logica que firmware original (1=aigua, 0=sec/desconnectada)
#define BOIA_BOMBA_PIN  5   // GPIO5, pull-down. NO: 1=aigua detectada, 0=sense aigua o desconnectada

// --- Switch mitja carrega (entrada local al receptor) ---
#define SWITCH_MITJA_CARREGA_PIN  1   // GPIO1, pull-down. 1=mitja carrega activa

// --- Potenciometre durada maxima bomba ---
#define POT_PIN           2        // GPIO2 (ADC1), pull-down 4.7k a PCB
#define POT_MAX_MINUTES   240UL    // Valor maxim: 240 minuts (4 hores)

// --- Parametres LoRa ---
#define RF_FREQUENCY          868000000  // 868 MHz (Europa ISM)
#define LORA_BANDWIDTH        0          // 0: 125 kHz
#define LORA_SPREADING_FACTOR 7          // SF7
#define LORA_CODINGRATE       1          // 4/5
#define LORA_PREAMBLE_LENGTH  8
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON  false
#define LORA_SYMBOL_TIMEOUT   0
#define BUFFER_SIZE           1          // 1 byte de payload

// --- Temporitzadors ---
#define RX_TIMEOUT_MS         150000     // Timeout seguretat: 150 segons

// --- Serial ---
#define SERIAL_BAUD  115200

#endif // CONFIG_H
