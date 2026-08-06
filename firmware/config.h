#ifndef CONFIG_H
#define CONFIG_H

// ============================================
// Configuracio compartida Emissor / Receptor
// Sistema LoRa punt a punt - Monitoratge boia
// ============================================

// --- Pins d'entrada (emissor) ---
#define IN1_PIN  1
#define IN2_PIN  2
#define IN3_PIN  4
#define IN4_PIN  5

// --- Pins de sortida (receptor) ---
#define OUT1_PIN  6
#define OUT2_PIN  7
#define OUT3_PIN  47
#define OUT4_PIN  48

// --- Parametres LoRa ---
#define RF_FREQUENCY        868000000  // 868 MHz (Europa ISM)
#define TX_OUTPUT_POWER     14         // dBm
#define LORA_BANDWIDTH      0          // 0: 125 kHz
#define LORA_SPREADING_FACTOR 7        // SF7
#define LORA_CODINGRATE     1          // 4/5
#define LORA_PREAMBLE_LENGTH 8
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON false
#define LORA_SYMBOL_TIMEOUT  0
#define BUFFER_SIZE          1         // 1 byte de payload

// --- RS485 / Modbus (Deye inversor) ---
#define RS485_TX_PIN     19
#define RS485_RX_PIN     20
#define RS485_DE_RE_PIN  3        // DE i RE del MAX485 junts (GPIO3 via jumper J_RS485)
#define RS485_BAUD       9600
#define MODBUS_SLAVE_ID  1        // Adreca Modbus del Deye

// Registres Deye 6kW hybrid (Modbus holding registers)
#define DEYE_REG_SOC     190      // SOC bateria (%)
#define DEYE_REG_POWER   186      // Produccio actual (W)

#define DEYE_READ_INTERVAL_MS 60000  // Llegir cada 60 segons

// --- Temporitzadors ---
#define TX_INTERVAL_MS       15000     // Enviar cada 15 segons
#define POLL_INTERVAL_MS     50        // Polling entrades cada 50 ms
#define RX_TIMEOUT_MS        45000     // Timeout receptor: 45 segons (tolera 2 paquets perduts)

// --- Serial ---
#define SERIAL_BAUD          115200

#endif // CONFIG_H
