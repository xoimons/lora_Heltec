#ifndef CONFIG_H
#define CONFIG_H

// ============================================
// Configuracio Receptor - Replica d'entrades
// Sistema LoRa punt a punt
// Heltec WiFi LoRa 32 (V3)
// ============================================

// --- Pins de sortida ---
#define OUT1_PIN  6
#define OUT2_PIN  7
#define OUT3_PIN  47
#define OUT4_PIN  48

// --- RS485 (GPIO3 forcat LOW per compatibilitat PCB) ---
#define RS485_DE_RE_PIN  3

// --- Parametres LoRa ---
#define RF_FREQUENCY              868000000  // 868 MHz (Europa ISM)
#define LORA_BANDWIDTH            0          // 0: 125 kHz
#define LORA_SPREADING_FACTOR     7          // SF7
#define LORA_CODINGRATE           1          // 4/5
#define LORA_PREAMBLE_LENGTH      8
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON      false
#define LORA_SYMBOL_TIMEOUT       0
#define BUFFER_SIZE               1          // 1 byte payload (bits 0-3 = IN1-IN4)

// --- Serial ---
#define SERIAL_BAUD  115200

#endif // CONFIG_H
