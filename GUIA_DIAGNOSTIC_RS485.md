# GUIA DE DIAGNÒSTIC RS485/MODBUS
## Comunicació Heltec V3 + MAX485ED (5V) + Deye Inversor Monofàsic

---

## ÍNDEX
1. [Verificació Hardware](#1-verificació-hardware)
2. [Verificació Configuració Deye](#2-verificació-configuració-deye)
3. [Proves amb Firmware de Diagnòstic](#3-proves-amb-firmware-de-diagnòstic)
4. [Solucions als Problemes Més Comuns](#4-solucions-als-problemes-més-comuns)

---

## 1. VERIFICACIÓ HARDWARE

### 1.1 XIP MAX485ED - Pinatge i Circuit

El **MAX485ED** és el convertidor TTL ↔ RS485, alimentat a **5V** (HLK-PM01).
El pin RO passa per un **divisor de tensió** per protegir el GPIO20 de l'ESP32 (màx 3.6V).

```
┌─────────────┐         ┌──────────┐         ┌─────────┐
│  ESP32-S3   │         │ MAX485ED │         │  DEYE   │
│  (Heltec)   │         │  (5V)    │         │         │
│             │         │          │         │         │
│  GPIO19 TX──┼────────→│ DI       │         │         │
│  GPIO20 RX←─┼─[DIV]───│ RO       │         │         │
│  GPIO3  ────┼────────→│ DE       │         │         │
│             │    ┌───→│ RE       │         │         │
│             │    │    │        A ┼────────→│ A (RS+) │
│             │    │    │        B ┼────────→│ B (RS-) │
│  GND ───────┼────┼───→│ GND     │         │         │
└─────────────┘    │    │ VCC←5V  ┼    GND──│ GND     │
                   │    └──────────┘         └─────────┘
              Jumper J_RS485
              (connectat)

[DIV] = Divisor de tensió (5V → ~3.3V) per protegir GPIO20
```

**IMPORTANT**: Els pins TX i RX **NO** s'han de creuar. El MAX485ED ja fa la conversió.

### 1.2 COMPROVACIONS FÍSIQUES

#### ☑️ Checklist Hardware:

1. **Jumper J_RS485**:
   - [ ] Jumper J_RS485 està **connectat** (pins central + dret)
   - [ ] Jumper J_IN5 està **obert** (NO connectat)
   - [ ] **CRÍTIC**: Mai posar els dos jumpers alhora!

2. **Soldadures PCB**:
   - [ ] MAX485: tots els pins ben soldats
   - [ ] GPIO3, GPIO19, GPIO20: continuïtat fins al MAX485
   - [ ] Verificar amb multímetre: GPIO19 → DI, GPIO20 → RO, GPIO3 → DE/RE

3. **Cablejat RS485**:
   - [ ] Cable A del MAX485 → Terminal A del Deye
   - [ ] Cable B del MAX485 → Terminal B del Deye
   - [ ] **GND comú** entre ESP32 i Deye connectat

4. **Resistència de Terminació** (opcional però recomanat):
   - [ ] Resistència 120Ω entre A i B al final del bus
   - Nota: Si la distància és curta (<10m), pot funcionar sense ella

5. **Alimentació**:
   - [ ] Deye alimentat i funcionant
   - [ ] MAX485ED alimentat a **5V** (VCC pin 8 → 5V del HLK-PM01)
   - [ ] Verificar amb multímetre: VCC del MAX485ED = ~5V
   - [ ] Verificar voltatge al pad GPIO20 ≤ 3.6V (divisor de tensió)

---

## 2. VERIFICACIÓ CONFIGURACIÓ DEYE

### 2.1 Accés al Menú de Configuració

1. Al display del Deye, accedir a:
   ```
   MENU → ADVANCED SETTINGS → Communication Settings
   ```
   (pot requerir contrasenya, habitualment: **0000** o **1111**)

2. Verificar els següents paràmetres:

   | Paràmetre          | Valor Correcte     | Nota                        |
   |--------------------|--------------------|-----------------------------|
   | Communication Mode | **Modbus RTU**     | NO pot ser "CAN" o "OFF"    |
   | Baud Rate          | **9600**           | Ha de coincidir amb l'ESP32 |
   | Data Bits          | **8**              | Per defecte                 |
   | Parity             | **None**           | Per defecte                 |
   | Stop Bits          | **1**              | Per defecte                 |
   | Slave Address      | **1**              | Ha de coincidir amb el codi |

### 2.2 Registres Modbus del Deye (Monofàsic)

El firmware llegeix aquests registres (holding registers, function code 0x03):

| Registre | Funció          | Unitat | Tipus  | Nota                          |
|----------|-----------------|--------|--------|-------------------------------|
| **184**  | SOC bateria     | %      | U_WORD | State of Charge (0-100%)      |
| **186**  | Potència PV1    | W      | U_WORD | Producció string PV1          |
| **187**  | Potència PV2    | W      | U_WORD | Producció string PV2          |

**ATENCIÓ**: Aquests registres són per inversors Deye/Sunsynk **monofàsics**. Els models
trifàsics (3P) tenen registres completament diferents (SOC=588, PV1=672, etc.).
Veure `PROBLEMA_MAX485ED_SOLUCIO.md` secció 8 per detalls.

---

## 3. PROVES AMB FIRMWARE DE DIAGNÒSTIC

### 3.1 Instal·lar Firmware de Test

1. Carregar el fitxer `test_rs485_debug.ino` al Heltec V3
2. Obrir el Serial Monitor a **115200 baud**
3. El firmware farà proves automàtiques cada 10 segons

### 3.2 Interpretació dels Resultats

#### **RESULTAT A: SUCCESS (0x00)**
```
[TEST 1] Llegint registre 184 (SOC bateria)...
  Result code: 0x00 (SUCCESS)
  *** SOC llegit: 75% ***
```
✅ **TOT CORRECTE!** El sistema funciona. Podeu tornar al firmware normal (`receptor.ino`).

---

#### **RESULTAT B: TIMEOUT (0xE4)**
```
[TEST 1] Llegint registre 184 (SOC bateria)...
  Result code: 0xE4 (TIMEOUT - NO RESPOSTA DEL DEYE!)
```
❌ **PROBLEMA**: L'ESP32 envia la petició però no rep resposta.

**Diagnòstic addicional**:
- Si al [TEST 3] **cap byte rebut**: problema de cablejat o Deye apagat
- Si al [TEST 3] **bytes detectats**: el Deye respon però amb dades incorrectes

---

#### **RESULTAT C: INVALID_SLAVE_ID (0xE0)**
```
[TEST 1] Llegint registre 184 (SOC bateria)...
  Result code: 0xE0 (INVALID_SLAVE_ID)
```
❌ **PROBLEMA**: El Slave ID del Deye no és 1.

**Solució**: Canviar `MODBUS_SLAVE_ID` al config.h o canviar el Slave ID al Deye.

---

#### **RESULTAT D: INVALID_ADDRESS (0xE2)**
```
[TEST 1] Llegint registre 184 (SOC bateria)...
  Result code: 0xE2 (INVALID_ADDRESS)
```
❌ **PROBLEMA**: El registre 184 no existeix al vostre model de Deye.

**Solució**: Verificar el model exacte del Deye. El registre 184 = SOC és per models
**monofàsics**. Si teniu un model **trifàsic**, el SOC és al registre **588**.
Veure `PROBLEMA_MAX485ED_SOLUCIO.md` secció 8 per la comparativa de registres.
Referència comunitat: https://github.com/kellerza/sunsynk

---

## 4. SOLUCIONS ALS PROBLEMES MÉS COMUNS

### PROBLEMA 1: Timeout constant (0xE4)

#### Causa més probable: **Cables A i B invertits**

**Solució**:
1. Canviar físicament els cables A ↔ B al connector del Deye
2. Provar de nou

**Nota**: RS485 és diferencial. Si A i B estan invertits, no hi ha comunicació.

---

### PROBLEMA 2: Bytes rebuts però result code 0xE4

#### Causa: **Velocitat incorrecta (baud rate mismatch)**

**Solució**:
1. Verificar al menú del Deye que la velocitat és **9600 baud**
2. Si el Deye està configurat a una altra velocitat (4800, 19200, etc.):
   - Opció A: Canviar el Deye a 9600
   - Opció B: Canviar `RS485_BAUD` al config.h

---

### PROBLEMA 3: GPIO3 no commuta (sempre LOW o HIGH)

#### Causa: **Jumper J_RS485 no connectat**

**Solució**:
1. Verificar que el jumper J_RS485 està físicament col·locat
2. Verificar continuïtat entre el pad central i el pad dret del jumper

---

### PROBLEMA 4: MAX485ED s'escalfa molt

#### Causa: **Curtcircuit a les línies A/B o alimentació invertida**

**Solució**:
1. Apagar immediatament (desconnectar 230VAC)
2. Verificar que VCC del MAX485ED és ~5V (del HLK-PM01)
3. Verificar que A i B no estan curtcircuitades entre sí
4. Substituir el MAX485ED si està cremat

---

### PROBLEMA 5: Cap byte rebut al [TEST 3]

#### Causa: **Deye apagat, Modbus desactivat, o cable trencat**

**Solució**:
1. Verificar que el Deye està alimentat i funcionant
2. Verificar que "Modbus RTU" està activat al menú del Deye
3. Provar amb un cable RS485 diferent
4. Mesurar continuïtat dels cables A i B

---

## 5. EINES DE DIAGNÒSTIC AVANÇAT

### 5.1 Verificació amb Multímetre

1. **Voltatge A-B en repòs**: ~0V
2. **Voltatge A-B durant transmissió**: ±2-5V (oscil·lant)
3. **GPIO3 (DE/RE)**:
   - En repòs: 0V (LOW)
   - Durant transmissió: 3.3V (HIGH)

### 5.2 Verificació amb Oscil·loscopi (si disponible)

1. Canal 1: GPIO3 (DE/RE)
2. Canal 2: Línia A del RS485
3. Trigger: Rising edge a GPIO3
4. Hauríeu de veure:
   - GPIO3 puja a HIGH
   - 50-100μs després, senyal diferencial a A-B
   - GPIO3 baixa a LOW després de la transmissió

### 5.3 Test amb Conversor USB-to-RS485

Si teniu un conversor USB-to-RS485:
1. Connectar-lo en paral·lel al bus (A→A, B→B, GND→GND)
2. Usar software com **QModMaster** (Linux/Windows) o **ModbusPoll**
3. Intentar llegir registre 184 (SOC) amb Slave ID 1, 9600 baud, 8N1
4. Si funciona: problema al MAX485ED, divisor de tensió, o pins de l'ESP32
5. Si no funciona: problema al Deye o cablejat

---

## 6. CHECKLIST FINAL

Abans de demanar ajuda, verificar:

- [ ] Jumper J_RS485 connectat, J_IN5 obert
- [ ] Cables A i B ben connectats (provar invertir-los)
- [ ] GND comú entre ESP32 i Deye
- [ ] Deye alimentat i amb Modbus RTU activat
- [ ] Velocitat 9600 baud al Deye
- [ ] Slave ID = 1 al Deye
- [ ] Firmware de diagnòstic executat i resultat anotat
- [ ] Soldadures del MAX485ED verificades
- [ ] Alimentació 5V al MAX485ED correcta (VCC pin 8)
- [ ] Voltatge al GPIO20 ≤ 3.6V (divisor de tensió)

---

## 7. CONTACTE I REFERÈNCIES

**Documentació del projecte**: `DOCUMENT_PROJECTE_COMPLET.md`

**Mapa de registres Deye/Sunsynk**: https://github.com/kellerza/sunsynk

**Datasheet MAX485**: https://www.analog.com/media/en/technical-documentation/data-sheets/MAX1487-MAX491.pdf

---

**ÈXIT ESPERAT**: Després de seguir aquesta guia, hauríeu de veure al Serial Monitor:
```
[TEST 1] Llegint registre 184 (SOC bateria)...
  Result code: 0x00 (SUCCESS)
  *** SOC llegit: XX% ***

[TEST 2] Llegint registre 186 (Potència PV1)...
  Result code: 0x00 (SUCCESS)
  *** Power llegit: XXXXW ***
```

Si no funciona després d'aquestes comprovacions, revisar `PROBLEMA_MAX485ED_SOLUCIO.md`.
