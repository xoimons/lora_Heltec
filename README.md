# Sistema LoRa Monitoratge Boia - Heltec WiFi LoRa 32 V3

Sistema de comunicació LoRa punt a punt per monitoratge de boies amb control remot de bomba.

---

## 📚 DOCUMENTACIÓ PRINCIPAL

### Documents de referència actius

| Document | Descripció |
|----------|------------|
| **[DOCUMENT_PROJECTE_COMPLET.md](DOCUMENT_PROJECTE_COMPLET.md)** | Document tècnic complet del projecte (hardware + software) |
| **[decisions_projecte.md](decisions_projecte.md)** | Historial de decisions tècniques i solucions implementades |

### Guies específiques

| Document | Descripció |
|----------|------------|
| **[PROBLEMA_MAX485ED_SOLUCIO.md](PROBLEMA_MAX485ED_SOLUCIO.md)** | ⚠️ **CRÍTIC**: Incompatibilitat MAX485ED (5V) amb ESP32-S3 (3.3V) i solució |
| **[GUIA_DIAGNOSTIC_RS485.md](GUIA_DIAGNOSTIC_RS485.md)** | Guia de diagnòstic per problemes de comunicació RS485/Modbus |

### Projecte inicial (carpeta `projecte_inici/`)

| Document | Descripció |
|----------|------------|
| [01_estructura_projecte.md](projecte_inici/01_estructura_projecte.md) | Estructura del projecte |
| [02_pinatge_gpio.md](projecte_inici/02_pinatge_gpio.md) | Assignació de pins GPIO |
| [03_alimentacio.md](projecte_inici/03_alimentacio.md) | Circuit d'alimentació 230VAC |
| [04_esquema_pcb.md](projecte_inici/04_esquema_pcb.md) | Esquema de blocs PCB |
| [05_bom_components.md](projecte_inici/05_bom_components.md) | BOM (Bill of Materials) actualitzada |

---

## ⚠️ AVÍS IMPORTANT - MAX485ED

**PROBLEMA CRÍTIC DETECTAT** (Juny 2026):

El xip **MAX485ED** (5V) de la PCB **NO és compatible** amb l'ESP32-S3 (3.3V) del Heltec V3.

**Solució**: Substituir per **MAX3485ESA** (3.3V) o equivalent.

👉 **Veure**: [PROBLEMA_MAX485ED_SOLUCIO.md](PROBLEMA_MAX485ED_SOLUCIO.md) per:
- Anàlisi tècnica completa
- Guia pas a pas de substitució
- Verificacions post-canvi

---

## 🔧 FIRMWARE

### Estructura

```
firmware/
├── emissor/
│   ├── emissor.ino          # Firmware emissor (camp/boia)
│   └── config.h             # Configuració emissor
├── receptor/
│   ├── receptor.ino         # Firmware receptor (sala tècnica)
│   ├── config.h             # Configuració receptor
│   └── test_rs485_debug.ino # Diagnòstic RS485/Modbus
└── config.h                 # Configuració compartida (deprecated)
```

### Compilació

**Arduino IDE**:
- Board: `WiFi LoRa 32(V3) / Wireless Shell(V3)`
- USB CDC On Boot: `Enabled`
- Upload Speed: `921600`

**Llibreries necessàries**:
- Heltec ESP32 Dev-Boards (inclou LoRaWan_APP)
- ModbusMaster by Doc Walker

---

## 🛠️ HARDWARE

### Components principals

- **MCU**: Heltec WiFi LoRa 32 V3 (ESP32-S3 + SX1262)
- **LoRa**: 868 MHz, SF7, BW 125 kHz
- **RS485**: MAX3485ESA (3.3V) — **NO MAX485ED**
- **Alimentació**: HLK-PM01 (230VAC → 5VDC)

### PCB

- PCB universal (mateix disseny per emissor i receptor)
- Diferenciació per jumper GPIO3: `J_IN5` (emissor) o `J_RS485` (receptor)
- Mides: 80 x 60 mm

---

## 🚀 QUICK START

### 1. Hardware

1. Muntar components a la PCB segons BOM
2. **CRÍTIC**: Soldar MAX3485ESA (3.3V), NO MAX485ED (5V)
3. Verificar jumpers:
   - Emissor: J_IN5 tancat, J_RS485 obert
   - Receptor: J_IN5 obert, J_RS485 tancat

### 2. Firmware

1. Obrir Arduino IDE
2. Instal·lar llibreries necessàries
3. Carregar `emissor.ino` a la placa emissor
4. Carregar `receptor.ino` a la placa receptor

### 3. Configuració Deye (només receptor)

Al menú del Deye:
- Communication Mode: **Modbus RTU** (o "485")
- Baud Rate: **9600**
- Slave Address: **1**

### 4. Test

Obrir Serial Monitor (115200 baud) i verificar:
- Emissor: Estat boies enviat cada 15s
- Receptor: Paquets rebuts + dades Deye (SOC, Power)

---

## 📋 TROUBLESHOOTING

### Problema: No es reben dades del Deye

1. Verificar xip RS485 → Ha de ser **MAX3485** (3.3V)
2. Si és MAX485ED (5V) → Seguir guia [PROBLEMA_MAX485ED_SOLUCIO.md](PROBLEMA_MAX485ED_SOLUCIO.md)
3. Carregar `test_rs485_debug.ino` per diagnòstic
4. Consultar [GUIA_DIAGNOSTIC_RS485.md](GUIA_DIAGNOSTIC_RS485.md)

### Problema: Pantalla OLED negra

1. Verificar ordre d'inicialització (Mcu.begin → Vext → display.init)
2. Veure secció "OLED" a [decisions_projecte.md](decisions_projecte.md)

### Problema: No es rep LoRa

1. Verificar freqüència 868 MHz (Europa)
2. Verificar antena connectada
3. Verificar SF7, BW 125 kHz a ambdues plaques

---

## 📞 SUPORT

Per problemes o dubtes:
1. Consultar [decisions_projecte.md](decisions_projecte.md) (històric de problemes resolts)
2. Revisar els documents específics de la carpeta `projecte_inici/`
3. Per problemes RS485: [PROBLEMA_MAX485ED_SOLUCIO.md](PROBLEMA_MAX485ED_SOLUCIO.md)

---

**Versió documentació**: rev.4 (Juny 2026)
**Última actualització**: Problema MAX485ED detectat i resolt
