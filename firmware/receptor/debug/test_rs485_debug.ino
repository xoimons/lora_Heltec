// ============================================
// TEST DIAGNOSTIC RS485 / MODBUS
// Heltec WiFi LoRa 32 (V3) + MAX485 + Deye
// ============================================

#include <ModbusMaster.h>

// Configuració pins RS485
#define RS485_TX_PIN     19
#define RS485_RX_PIN     20
#define RS485_DE_RE_PIN  3
#define RS485_BAUD       9600
#define MODBUS_SLAVE_ID  1

// Registres Deye
#define DEYE_REG_SOC     184    // SOC bateria (%) - confirmat via ESPHome/documentacio Deye
#define DEYE_REG_POWER   186    // Produccio PV1 (W)

ModbusMaster modbus;

// Control direcció RS485
void rs485PreTransmission() {
  digitalWrite(RS485_DE_RE_PIN, HIGH);
  delayMicroseconds(50);  // Temps per estabilitzar transceiver
}

void rs485PostTransmission() {
  Serial1.flush();  // Esperar que TOTS els bytes surtin del buffer TX
  delayMicroseconds(50);  // Marge extra per l'últim bit + stop bit
  digitalWrite(RS485_DE_RE_PIN, LOW);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n\n");
  Serial.println("=====================================");
  Serial.println("  TEST DIAGNOSTIC RS485/MODBUS");
  Serial.println("  Heltec V3 + MAX485 + Deye");
  Serial.println("=====================================");
  Serial.println();

  // Configurar pin DE/RE
  pinMode(RS485_DE_RE_PIN, OUTPUT);
  digitalWrite(RS485_DE_RE_PIN, LOW);  // Mode recepció per defecte
  Serial.println("[OK] Pin DE/RE (GPIO3) configurat");

  // Inicialitzar Serial1 (RS485)
  Serial1.begin(RS485_BAUD, SERIAL_8N1, RS485_RX_PIN, RS485_TX_PIN);
  Serial.printf("[OK] Serial1 inicialitzat: %d baud, 8N1\n", RS485_BAUD);
  Serial.printf("[OK] Pins: TX=GPIO%d, RX=GPIO%d, DE/RE=GPIO%d\n",
                RS485_TX_PIN, RS485_RX_PIN, RS485_DE_RE_PIN);

  // Inicialitzar ModbusMaster
  modbus.begin(MODBUS_SLAVE_ID, Serial1);
  modbus.preTransmission(rs485PreTransmission);
  modbus.postTransmission(rs485PostTransmission);
  Serial.printf("[OK] ModbusMaster inicialitzat, Slave ID=%d\n", MODBUS_SLAVE_ID);

  Serial.println();
  Serial.println("-------------------------------------");
  Serial.println("COMPROVACIONS HARDWARE:");
  Serial.println("-------------------------------------");
  Serial.println("1. Jumper J_RS485 connectat? (NO J_IN5)");
  Serial.println("2. Cablejat RS485:");
  Serial.println("   - MAX485 A → Deye A");
  Serial.println("   - MAX485 B → Deye B");
  Serial.println("   - GND comú entre ESP32 i Deye");
  Serial.println("3. Alimentació Deye correcta?");
  Serial.println("4. Modbus RTU habilitat al Deye?");
  Serial.println("   (Menu → Settings → Communication)");
  Serial.println("5. Slave ID del Deye = 1?");
  Serial.println("6. Velocitat Deye = 9600 baud?");
  Serial.println("-------------------------------------");
  Serial.println();

  delay(2000);
}

void loop() {
  Serial.println("\n=== INTENT DE LECTURA MODBUS ===");

  // Test 1: Llegir registre SOC (190)
  Serial.println("\n[TEST 1] Llegint registre 184 (SOC bateria)...");
  // DE/RE es controla automàticament via preTransmission/postTransmission callbacks
  uint8_t result = modbus.readHoldingRegisters(DEYE_REG_SOC, 1);

  Serial.printf("  Result code: 0x%02X (", result);
  switch(result) {
    case 0x00: Serial.print("SUCCESS"); break;
    case 0xE0: Serial.print("INVALID_SLAVE_ID"); break;
    case 0xE1: Serial.print("INVALID_FUNCTION"); break;
    case 0xE2: Serial.print("INVALID_ADDRESS"); break;
    case 0xE3: Serial.print("INVALID_DATA"); break;
    case 0xE4: Serial.print("TIMEOUT - NO RESPOSTA DEL DEYE!"); break;
    default:   Serial.printf("ERROR_DESCONEGUT"); break;
  }
  Serial.println(")");

  if (result == modbus.ku8MBSuccess) {
    uint16_t soc = modbus.getResponseBuffer(0);
    Serial.printf("  *** SOC llegit: %d%% ***\n", soc);
  } else if (result == 0xE4) {
    Serial.println("\n  DIAGNOSI ERROR TIMEOUT:");
    Serial.println("  - El MAX485 no rep resposta del Deye");
    Serial.println("  - POSSIBLES CAUSES:");
    Serial.println("    1. Cablejat A/B invertit (prova canviar A↔B)");
    Serial.println("    2. Deye no té Modbus RTU habilitat");
    Serial.println("    3. Slave ID incorrecte al Deye");
    Serial.println("    4. Velocitat incorrecta (9600≠velocitat Deye)");
    Serial.println("    5. MAX485 defectuós o mal soldat");
    Serial.println("    6. GND no està connectat");
  }

  delay(1000);

  // Test 2: Llegir registre PV1 Power (186)
  Serial.println("\n[TEST 2] Llegint registre 186 (Potència PV1)...");

  result = modbus.readHoldingRegisters(DEYE_REG_POWER, 1);  // 186

  Serial.printf("  Result code: 0x%02X (", result);
  switch(result) {
    case 0x00: Serial.print("SUCCESS"); break;
    case 0xE0: Serial.print("INVALID_SLAVE_ID"); break;
    case 0xE1: Serial.print("INVALID_FUNCTION"); break;
    case 0xE2: Serial.print("INVALID_ADDRESS"); break;
    case 0xE3: Serial.print("INVALID_DATA"); break;
    case 0xE4: Serial.print("TIMEOUT"); break;
    default:   Serial.printf("ERROR_DESCONEGUT"); break;
  }
  Serial.println(")");

  if (result == modbus.ku8MBSuccess) {
    uint16_t power = modbus.getResponseBuffer(0);
    Serial.printf("  *** Power llegit: %dW ***\n", power);
  }

  // Test 3: Monitorització tràfic Serial1 (raw)
  Serial.println("\n[TEST 3] Monitorització tràfic RS485 (5 segons)...");
  Serial.println("  (Si veieu bytes, el cablejat funciona)");

  unsigned long startTime = millis();
  int bytesReceived = 0;

  while (millis() - startTime < 5000) {
    if (Serial1.available()) {
      uint8_t b = Serial1.read();
      Serial.printf("  RX: 0x%02X\n", b);
      bytesReceived++;
    }
  }

  if (bytesReceived == 0) {
    Serial.println("  *** CAP BYTE REBUT - Problema de cablejat o Deye apagat ***");
  } else {
    Serial.printf("  *** %d bytes rebuts - Comunicació detectada! ***\n", bytesReceived);
  }

  // Test 4: Verificar nivells de voltatge (només informatiu)
  Serial.println("\n[INFO] Comprovacions addicionals:");
  Serial.println("  - Mesurar voltatge A-B al bus RS485 (ha d'estar entre ±5V en transmissió)");
  Serial.println("  - Verificar que GPIO3 canvia de LOW a HIGH durant transmissió");
  Serial.println("  - Si teniu oscil·loscopi, verificar forma d'ona RS485");

  Serial.println("\n=== FI DEL TEST - Esperant 10 segons ===\n");
  delay(10000);
}
