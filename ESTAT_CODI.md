# Estat del codi - Sistema LoRa Monitoratge Boia

## 2026-08-10 - Versio actual (fail-safe NC/NO)

---

## EMISSOR (`firmware/emissor/`)

### Entrades
| Entrada | GPIO | Debounce | Funcio |
|---------|------|----------|--------|
| IN1 | 5 | 3 min | Boia diposit desti (principal) |
| IN2 | 1 | 3 min | Reservada (opcional) |
| IN3 | 4 | Immediat | Opcional, replicada via LoRa |
| IN4 | 2 | Immediat | Opcional, replicada via LoRa |

### Debounce
- **IN1/IN2**: valor cru estable 3 minuts per confirmar canvi. Mentre espera, s'envia l'estat confirmat anterior. Si torna al valor original, es cancel·la.
- **IN3/IN4**: canvi immediat.
- **Arrencada**: primer valor confirmat directament.

### Transmissio LoRa
- **Payload**: 1 byte (bits 0-3 = IN1-IN4 confirmats)
- **TX periodic**: cada 60s
- **TX per canvi**: immediat quan un estat confirmat canvia
- **868 MHz, SF7, BW 125kHz, CR 4/5, 14 dBm**

### Display OLED
- Linia 1: Estat TX
- Linia 2: Valors confirmats + `*` si canvi pendent
- Linia 3: Comptadors enviats/errors
- Linia 4: Temps des de l'ultim TX

---

## RECEPTOR (`firmware/receptor/`)

### Entrades locals
| Entrada | GPIO | Funcio |
|---------|------|--------|
| BOIA_BOMBA | 5 | Boia diposit bomba, contacte NO (1=aigua, 0=sec/desconnectada, fail-safe) |
| POT | 2 | Potenciometre durada maxima OUT1 (0-240 min), cache cada 2s |

### Sortides
| Sortida | GPIO | Logica |
|---------|------|--------|
| OUT1 | 6 | Bomba - logica condicional fail-safe |
| OUT2 | 7 | Replica invertida IN2 LoRa (NC fail-safe) |
| OUT3 | 47 | Replica invertida IN3 LoRa (NC fail-safe) |
| OUT4 | 48 | Replica directa IN4 LoRa |

### Cablejat fail-safe boies
| Boia | Contacte | Desconnectada = | Fail-safe |
|------|----------|-----------------|-----------|
| IN1/IN2/IN3 (emissor) | NC | 0 = bomba/sortida OFF | Si |
| BOIA_BOMBA (receptor) | NO | 0 = para bomba | Si |

### Logica OUT1 (bomba) - FAIL-SAFE

**Arrencada** (totes certes):
- `IN1 = 1` via LoRa (falta aigua al diposit desti, NC tancat)
- `boiaBomba = 1` (diposit bomba te aigua, NO tancat)
- `SOC >= 30%` (Deye disponible i amb prou carrega)

**Parada** (qualsevol):
- `IN1 = 0` via LoRa (diposit desti te aigua O boia desconnectada, fail-safe)
- `boiaBomba = 0` (proteccio marxa en sec O boia desconnectada, fail-safe)
- `SOC <= 20%` (bateria baixa)
- `SOC = -1` (sense comunicacio Deye - bloqueja per seguretat)
- `switchMitjaCarrega = 1 I IN2 = 0` (nivell intermig assolit O boia desconnectada)
- Potenciometre durada maxima assolida (nomes para OUT1, no afecta OUT2/3/4)
- Timeout RX 150s sense comunicacio LoRa (safety shutdown, para totes les sortides)

**Histeresi SOC**:
- Arrenca a >= 30%, para a <= 20%, entre 20-30% mante estat actual

### Comunicacio
- **LoRa RX**: continu, 868 MHz, SF7
- **RX Timeout**: 150s, safety shutdown de totes les sortides
- **Modbus RS485**: lectura Deye cada 60s (SOC + produccio PV). `Radio.IrqProcess()` entre lectures per no perdre paquets LoRa

### Display OLED
- Linia 1: Estat connexio (RECEPTOR OK / SENSE SENYAL / REBENT)
- Linia 2: Estat OUT1 + temps actiu/max + boia bomba (`BB:0`/`BB:1`)
- Linia 3: SOC bateria (%), produccio PV (W), estat sortides receptor OUT2/OUT3/OUT4
- Linia 4: Valors IN1-IN4 rebuts via LoRa

---

## Parametres clau
| Parametre | Valor | Fitxer |
|-----------|-------|--------|
| TX_INTERVAL_MS | 60000 (1 min) | emissor/config.h |
| DEBOUNCE_CONFIRM_MS | 180000 (3 min) | emissor/config.h |
| RX_TIMEOUT_MS | 150000 (2.5 min) | receptor/config.h |
| DEYE_SOC_START | 30% | receptor/config.h |
| DEYE_SOC_STOP | 20% | receptor/config.h |
| DEYE_READ_INTERVAL_MS | 60000 (1 min) | receptor/config.h |
| POT_MAX_MINUTES | 240 (4h) | receptor/config.h |
| POT_READ_INTERVAL_MS | 2000 (2s) | receptor/receptor.ino |

---

## RECEPTOR MQTT (`firmware_mqtt/receptor_mqtt/`)

Basat en `firmware/receptor/receptor.ino` amb WiFi i MQTT afegits. La logica LoRa, sortides, Modbus i display es identica al receptor original.

### Broker: HiveMQ Cloud (pla Serverless gratis)
- **TLS obligatori** (port 8883, certificat ISRG Root X1 inclus al firmware)
- **Sincronitzacio NTP** automatica (necessaria per validar certificat TLS)
- **10 GB/mes** de trafic (consum estimat ~1 GB/mes publicant cada 2 min)
- **100 connexions** simultanies
- Sense targeta de credit
- Veure `firmware_mqtt/CONFIGURACIO_MQTT.md` per guia pas a pas

### WiFi
- Connexio a xarxa WiFi d'un modem 4G
- Reconnexio automatica (`WiFi.setAutoReconnect(true)`)
- `Radio.IrqProcess()` durant connexio WiFi per no perdre paquets LoRa
- SSID i password configurables a `config.h` (pendents de definir)

### MQTT (PubSubClient + WiFiClientSecure)
- Connexio TLS al port 8883 amb certificat ISRG Root X1 (Let's Encrypt)
- Autenticacio per username/password (creats a HiveMQ Access Management)
- Reconnexio automatica al broker
- Last Will Testament: publica `"offline"` a `boia/status` si es perd la connexio
- Publica JSON cada 2 minuts o quan hi ha canvis d'estat

| Topic | Contingut |
|-------|-----------|
| `boia/status` | `"online"` / `"offline"` (LWT, retained) |
| `boia/outputs` | `{"out1":0,"out2":1,"out3":0,"out4":0,"out1_min":12.5}` |
| `boia/inputs` | `{"in1":0,"in2":1,"in3":0,"in4":0}` |
| `boia/deye` | `{"soc":85,"pv_power":2400}` |
| `boia/lora` | `{"connected":true,"rssi":-45,"rx_ok":120,"rx_err":2}` |
| `boia/locals` | `{"boia_bomba":1,"switch_mc":0,"pot_max_min":120}` |

### Display OLED
- Linies 1-4: iguals al receptor original
- Linia 5: Estat WiFi i MQTT (`W:OK M:OK` / `W:-- M:--`)

### Dependencia addicional
- Llibreria **PubSubClient** (Nick O'Leary) - instal·lar des del Library Manager

### Parametres pendents de configurar
| Parametre | Valor actual | Fitxer |
|-----------|-------------|--------|
| WIFI_SSID | `"CANVIAR_SSID"` | firmware_mqtt/receptor_mqtt/config.h |
| WIFI_PASSWORD | `"CANVIAR_PASSWORD"` | firmware_mqtt/receptor_mqtt/config.h |
| MQTT_SERVER | `"CANVIAR.s1.eu.hivemq.cloud"` | firmware_mqtt/receptor_mqtt/config.h |
| MQTT_USER | `"CANVIAR_USUARI"` | firmware_mqtt/receptor_mqtt/config.h |
| MQTT_PASSWORD | `"CANVIAR_PASSWORD"` | firmware_mqtt/receptor_mqtt/config.h |
| MQTT_PORT | 8883 (TLS) | firmware_mqtt/receptor_mqtt/config.h |
| MQTT_PUBLISH_INTERVAL_MS | 120000 (2 min) | firmware_mqtt/receptor_mqtt/config.h |

### Test de connexio MQTT
- Fitxer: `firmware_mqtt/tests/test_mqtt_hivemq/test_mqtt_hivemq.ino`
- Funciona amb qualsevol ESP32 amb WiFi (no necessita hardware Heltec)
- Envia valors aleatoris als topics `boia/*` cada 10 segons
- Per verificar: subscriure's a `boia/#` al HiveMQ Web Client

### Estructura fitxers
```
firmware_mqtt/
  CONFIGURACIO_MQTT.md          <-- Guia pas a pas configuracio
  receptor_mqtt/
    config.h                    <-- Configuracio (WiFi, MQTT, pins, LoRa)
    receptor_mqtt.ino           <-- Firmware receptor complet
  tests/
    test_mqtt_hivemq/
      test_mqtt_hivemq.ino      <-- Test connexio amb dades aleatories
```

---

## Comunicacio Deye (RS485 / Modbus)

### Hardware RS485
- **Xip**: MAX485ED alimentat a **5V** (dins especificacions del xip)
- **Divisor de tensio**: al pin RO (pin 1 del MAX485, sortida cap a GPIO20) per baixar de 5V a nivell segur per l'ESP32 (<=3.3V)
- **DE/RE**: GPIO3 controla direccio del bus (HIGH=TX, LOW=RX)
- **Pins**: TX=GPIO19 (DI), RX=GPIO20 (RO)
- **Bus**: 9600 baud, 8N1, Slave ID 1

### Registres Modbus llegits
| Registre | Funcio | Unitat |
|----------|--------|--------|
| 190 | SOC bateria | % |
| 154 | Potencia PV total | W |

### Precaucions de timing al firmware
- `delayMicroseconds(200)` al pre/post transmission per donar temps al MAX485 a commutar DE/RE
- `Radio.IrqProcess()` entre les dues lectures Modbus (SOC i Power) per no perdre paquets LoRa durant el bloqueig
- `delay(100)` entre lectures Modbus per no saturar el bus
- Lectures cada 60s (`DEYE_READ_INTERVAL_MS`) per evitar col·lapsar el bus
- La llibreria `ModbusMaster` es sincrona (bloqueja fins resposta o timeout)

### Problemes coneguts i estat
| Problema | Estat | Notes |
|----------|-------|-------|
| MAX485ED (5V) amb ESP32 (3.3V) | Resolt | Divisor tensio al pin RO |
| Timeout 0xE4 (sense resposta) | Pendent test | Verificar config Deye: mode Modbus RTU, no METER |
| Cablejat A/B | Pendent test | Si timeout, provar invertir cables A i B |
| GND comu ESP32-Deye | Pendent test | Necessari per comunicacio estable |
| Registre 154 vs 186 (Power) | Pendent test | config.h compartit te 186, receptor te 154. Verificar model Deye |

### Checklist test Deye
- [ ] Deye alimentat i funcionant
- [ ] Port RS485 del Deye en mode **Modbus RTU** (no METER ni CAN)
- [ ] Velocitat Deye: 9600 baud
- [ ] Slave ID Deye: 1
- [ ] Cables A i B correctes (provar invertir si timeout)
- [ ] GND comu entre ESP32 i Deye connectat
- [ ] Voltatge al GPIO20 entre 2.5V i 3.3V (verificar amb multimetre)
- [ ] Provar primer amb `test_rs485_debug.ino` abans del firmware complet
- [ ] Si falla: provar amb conversor USB-RS485 + PC (QModMaster/ModbusPoll) per aislar problema
