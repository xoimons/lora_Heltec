# Estat del codi - Replica d'entrades via LoRa

## 2026-06-26 - Versio actual

---

## EMISSOR REPLICA (`firmware_replica/emissor/`)

### Entrades
| Entrada | GPIO | Funcio |
|---------|------|--------|
| IN1 | 5 | Entrada directa (pull-down) |
| IN2 | 1 | Entrada directa (pull-down) |
| IN3 | 4 | Entrada directa (pull-down) |
| IN4 | 2 | Entrada directa (pull-down) |

Sense debounce ni temporitzadors. Valor cru immediat.

### Transmissio LoRa
- **Payload**: 1 byte (bits 0-3 = IN1-IN4)
- **TX**: nomes per canvi d'estat (no hi ha enviament periodic)
- **Primer enviament**: immediat a l'arrencada
- **TX timeout**: si falla, forca reintent al proper cicle
- 868 MHz, SF7, BW 125kHz, CR 4/5, 14 dBm

### Display OLED
- Linia 1: Estat TX (ENVIANT / ENVIAT OK / EMISSOR OK)
- Linia 2: Valors entrades IN1-IN4
- Linia 3: Comptadors enviats/errors
- Linia 4: Temps des de l'ultim TX

### Altres
- GPIO3 (RS485 DE/RE) forcat a LOW (MAX485 soldat, mode RX permanent)

---

## RECEPTOR REPLICA (`firmware_replica/receptor/`)

### Sortides
| Sortida | GPIO | Logica |
|---------|------|--------|
| OUT1 | 6 | Replica directa IN1 LoRa |
| OUT2 | 7 | Replica directa IN2 LoRa |
| OUT3 | 47 | Replica directa IN3 LoRa |
| OUT4 | 48 | Replica directa IN4 LoRa |

Totes les sortides son replica directa de l'entrada LoRa corresponent. Sense logica condicional, sense timeouts, sense entrades locals.

### Recepcio LoRa
- **RX continu** a 868 MHz, SF7, BW 125kHz, CR 4/5
- Sortides inicials a LOW fins rebre primer paquet

### Display OLED
- Linia 1: Estat connexio (REBUT! / ESPERANT... / RECEPTOR OK)
- Linia 2: Estat sortides OUT1-OUT4
- Linia 3: Entrades IN1-IN4 rebudes via LoRa
- Linia 4: Comptador paquets + errors + RSSI

### Altres
- GPIO3 (RS485 DE/RE) forcat a LOW (MAX485 soldat, mode RX permanent)
- Sense Deye Modbus
- Sense entrades locals
- Sense potenciometre

---

## Diferencies clau respecte `firmware/`
| Aspecte | `firmware/` | `firmware_replica/` |
|---------|------------|-------------------|
| TX periodic | Cada 60s | No (nomes per canvi) |
| Debounce IN1/IN2 | 3 min | Cap |
| Logica OUT1 | Condicional (boia bomba + SOC + IN1 LoRa) | Replica directa IN1 LoRa |
| OUT2/3/4 | Replica directa | Replica directa |
| SOC condiciona OUT1 | Si (histeresi 20-30%) | No |
| RX Timeout safety | 150s, para tot | No hi ha timeout |
| Potenciometre | Si (durada max OUT1) | No |
| Boia bomba local | Si (GPIO5) | No |
| Entrades locals receptor | GPIO5 (boia bomba) + GPIO2 (pot) | Cap |
| Deye Modbus RS485 | Si (SOC + PV cada 60s) | No |

---

## Parametres clau
| Parametre | Valor | Fitxer |
|-----------|-------|--------|
| POLL_INTERVAL_MS | 50 (50ms) | emissor/config.h |
| TX_OUTPUT_POWER | 14 dBm | emissor/config.h |
| BUFFER_SIZE | 1 byte | emissor/config.h, receptor/config.h |
| RS485_DE_RE_PIN | GPIO3 (LOW permanent) | emissor/config.h, receptor/config.h |
