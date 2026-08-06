#ifndef CONFIG_H
#define CONFIG_H

// ============================================
// Configuracio Emissor - Replica d'entrades
// Sistema LoRa punt a punt
// Heltec WiFi LoRa 32 (V3)
// ============================================

// --- Pins d'entrada ---
#define IN1_PIN  5
#define IN2_PIN  1
#define IN3_PIN  4
#define IN4_PIN  2

// --- RS485 (MAX485 no poblat a l'emissor, GPIO3 forcat LOW) ---
#define RS485_DE_RE_PIN  3

// --- Parametres LoRa ---
#define RF_FREQUENCY              868000000  // 868 MHz (Europa ISM)
#define TX_OUTPUT_POWER           14         // dBm
#define LORA_BANDWIDTH            0          // 0: 125 kHz
#define LORA_SPREADING_FACTOR     7          // SF7
#define LORA_CODINGRATE           1          // 4/5
#define LORA_PREAMBLE_LENGTH      8
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON      false
#define LORA_SYMBOL_TIMEOUT       0
#define BUFFER_SIZE               1          // 1 byte payload (bits 0-3 = IN1-IN4)

// --- Temporitzadors ---
#define TX_INTERVAL_MS       60000   // Enviar cada 60 segons (1 minut)
#define POLL_INTERVAL_MS     50      // Polling entrades cada 50 ms
#define DEBOUNCE_CONFIRM_MS  60000   // 1 minut per confirmar canvi d'estat IN1/IN2 (60000 ms)

// --- Serial ---
#define SERIAL_BAUD  115200

#endif // CONFIG_H
