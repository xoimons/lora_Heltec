#ifndef CONFIG_H
#define CONFIG_H

// ============================================
// Configuracio Receptor
// Sistema LoRa punt a punt - Monitoratge boia
// ============================================

// --- Pins de sortida ---
#define OUT1_PIN  6
#define OUT2_PIN  7
#define OUT3_PIN  47
#define OUT4_PIN  48

// --- Boia diposit bomba (entrada local al receptor) ---
#define BOIA_BOMBA_PIN  5   // GPIO5, pull-down. 1=aigua detectada, 0=sense aigua

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

// --- RS485 / Modbus (Deye inversor) ---
#define RS485_TX_PIN     19
#define RS485_RX_PIN     20
#define RS485_DE_RE_PIN  3               // DE i RE del MAX485 junts
#define RS485_BAUD       9600
#define MODBUS_SLAVE_ID  1               // Adreca Modbus del Deye

// Registres Deye 6kW hybrid (Modbus holding registers)
#define DEYE_REG_SOC          184        // SOC bateria (%) - confirmat via ESPHome/documentacio Deye
#define DEYE_REG_PV1_POWER    186        // Produccio PV1 (W)
#define DEYE_REG_PV2_POWER    187        // Produccio PV2 (W)
#define DEYE_READ_INTERVAL_MS 60000      // Llegir cada 60 segons

// --- Llindars energia Deye (histeresi SOC per bomba) ---
#define DEYE_SOC_START  30   // SOC minim per arrancar bomba (%)
#define DEYE_SOC_STOP   20   // SOC per parar bomba (%), entre 20-30 mante estat

// --- Temporitzadors ---
#define RX_TIMEOUT_MS         150000            // Timeout seguretat: 150 segons (2.5 min, tolera 1 paquet perdut amb TX cada 60s)

// --- Serial ---
#define SERIAL_BAUD  115200

#endif // CONFIG_H
