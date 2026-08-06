# DECISIONS TÈCNIQUES DEL PROJECTE - HELTEC LORA V3
Data revisió: Juny 2026 (rev.5 - Corregit: MAX485ED a 5V + divisor tensió RO, registres monofàsic)

---

## 1. DOCUMENT DE REFERÈNCIA ACTIU

**Usar:** `DOCUMENT_PROJECTE_COMPLET.md`
**Descartar:** `projecte_alternatiu.txt` (GPIO conflictius, arquitectura incompleta)

---

## 2. INICIALITZACIÓ OLED — DECISIONS FIRMWARE (rev.3)

### Problema: `multiple definition of 'display'`
La llibreria Heltec (`LoRaWan_APP.cpp:28`) ja defineix `SSD1306Wire display` globalment.
Crear una nova instància a l'sketch causa error de linker.

**Solució:** Usar `extern SSD1306Wire display;` als dos sketches (emissor i receptor).

### Problema: brillo OLED baix
`setBrightness(255)` sol no era suficient. Calia configurar també pre-charge i VCOMH.

**Solució:** `display.setContrast(255, 241, 64);` (contrast, pre-charge, VCOMH al màxim).

### Problema: brillo seguia baix malgrat setContrast
`Mcu.begin()` reinicialitza Vext (GPIO36) i pot sobreescriure la configuració OLED.

**Solució:** Ordre d'inicialització obligatori:
1. `Mcu.begin()` — primer
2. `pinMode(Vext, OUTPUT); digitalWrite(Vext, LOW); delay(50);` — activar alimentació OLED manualment
3. `display.init()` — inicialitzar OLED
4. `display.setContrast(255, 241, 64)` — aplicar brillo

### Problema: `RS485_DE_RE_PIN` no definit a emissor config.h
L'emissor usava `RS485_DE_RE_PIN` per forçar GPIO3 LOW, però el define només existia al config.h compartit.

**Solució:** Afegit `#define RS485_DE_RE_PIN 3` al config.h local de l'emissor.

### Pantalla girada 180 graus
Afegit `display.flipScreenVertically()` als dos sketches despres de `display.init()`.
El modul Heltec V3 mostra el contingut invertit verticalment segons l'orientacio de muntatge a la PCB.

### Canvi display receptor: boies en lloc de RSSI
La línia 4 del display del receptor mostrava temps des de l'últim RX i RSSI.
Canviat per mostrar l'estat de les boies rebudes per LoRa: `IN: 1  0  1  0`
(IN1=boia baixa, IN2=boia alta, IN3=boia mig, IN4=reserva).

### Arduino IDE Board
**Board:** `WiFi LoRa 32(V3) / Wireless Shell(V3)` (paquet Heltec ESP32)
**USB CDC On Boot:** Enabled
**Upload:** Mode BOOT manual si el port no es detecta (mantenir BOOT premut + connectar USB)

---

## 3. ASSIGNACIÓ DE PINS DEFINITIVA

### Entrades (Emissor - camp)
| Senyal | GPIO | Nota |
|--------|------|------|
| IN1 (Boia 1) | GPIO1 | Nivell mínim (dipòsit buit) |
| IN2 (Boia 2) | GPIO2 | Nivell màxim (dipòsit ple) |
| IN3 (Boia 3) | GPIO4 | Nivell intermig |
| IN4 (Reserva) | GPIO5 | |

### Entrades (Receptor - sala tècnica)
| Senyal | GPIO | Nota |
|--------|------|------|
| IN2 (SEL_MID) | GPIO2 | Selector parada nivell mig: LOW=para a ple, HIGH=para a mig |
| IN4 (POT) | GPIO5 | Potenciòmetre durada màxima bomba (ADC1, 500Ω, pull-down 4.7kΩ PCB) |

### Sortides (Receptor - sala tècnica)
| Senyal | GPIO | Nota |
|--------|------|------|
| OUT1 (Relé 1) | GPIO6 | |
| OUT2 (Relé 2) | GPIO7 | |
| OUT3 (Relé 3) | GPIO47 | |
| OUT4 (Relé 4) | GPIO48 | |

### RS485 (Receptor)
| Senyal | GPIO | Nota |
|--------|------|------|
| TX (DI) | GPIO19 | Directe al MAX485ED (3.3V > VIH 2.0V, OK) |
| RX (RO) | GPIO20 | Via divisor de tensió (5V → ~3.3V) |
| DE/RE | GPIO3 | Directe al MAX485ED (3.3V > VIH 2.0V, OK). Només poblat al receptor |

**Hardware actual:** MAX485ED alimentat a **5V** (HLK-PM01) amb divisor de tensió al pin RO per protegir GPIO20.
Veure secció 8 per detalls del circuit.

### Pins descartats del document alternatiu (CONFLICTE)
| GPIO | Motiu |
|------|-------|
| GPIO38 | SPI Flash intern |
| GPIO39-42 | JTAG |
| GPIO45, GPIO46 | Strapping pins (boot mode) |

---

## 4. CIRCUIT DE SORTIDES (RELÉS)

### Driver escollit: BC337-40 (NPN) + pull-down afegida
```
GPIO ──[ 1kΩ ]──┬── Base (BC337-40)
                │        │
             [10kΩ]    Collector ──┬── Bobina relé
                │        │         │
               GND     Emitter    [1N4007] Cathode→VCC / Anode→Collector
                          │
                         GND
```

| Component | Valor | Referència LCSC |
|-----------|-------|----------------|
| Transistor NPN | BC337-40 | **C713611** |
| R base | 1 kΩ | - |
| R pull-down base | 10 kΩ | - |
| Diode flyback | 1N4007 | - |

**IMPORTANT diode:** Càtode (ratlla) → VCC / Ànode → Collector

### Alternativa superior (si PCB SMD): AO3400
```
GPIO ──────────────┬── Gate (AO3400)
                   │
                [10kΩ] pull-down
                   │
                  GND
```
| Component | Valor | Nota |
|-----------|-------|------|
| MOSFET | AO3400 | SOT-23, Vgs(th)~1V, Id 5.4A |
| R pull-down gate | 10 kΩ | Obligatòria |
| Diode flyback | 1N4007 | Càtode→VCC, Ànode→Drain |

---

## 5. CIRCUIT D'ENTRADES

```
Connector IN ──[ 1kΩ ]──┬── GPIOx
                         │
                      [10kΩ]   [100nF]
                         │        │
                        GND      GND
```

| Component | Valor |
|-----------|-------|
| R protecció | 1 kΩ (en sèrie) |
| R pull-down | 10 kΩ (a GND) |
| C filtre | 100 nF (anti-rebots HW) |

---

## 6. LÒGICA DE CONTROL BOMBA (OUT1) — rev.5 (Juny 2026)

### Entrades que afecten OUT1
| Entrada | Origen | Funció |
|---------|--------|--------|
| IN1 (LoRa, bit 0) | Emissor, boia dipòsit destí | 0=buit (arrancar), 1=te aigua (parar) |
| BOIA_BOMBA (GPIO5) | Local receptor, boia dipòsit bomba | 1=aigua (permesa arrencada), 0=sec (protecció marxa en sec) |
| SOC bateria | Deye via Modbus RS485 | Histèresi: arrenca >=30%, para <=20% |

### Condicions d'arrencada i parada
| Condició | Acció |
|----------|-------|
| IN1=0 + boiaBomba=1 + SOC>=30% | **Arrenca** bomba |
| IN1=0 + boiaBomba=1 + SOC entre 20-30% | Manté estat actual (histèresi SOC) |
| IN1=0 + SOC<20% | **Para** bomba (log "SOC baix: OUT1 desactivada") |
| IN1=0 + boiaBomba=0 | **Para** bomba (protecció marxa en sec) |
| IN1=1 | **Para** bomba (dipòsit destí té aigua) |
| Dades Deye no disponibles (SOC=-1) | **Bloqueja** arrencada per seguretat |
| Timeout LoRa (150s sense paquet) | **Para** totes les sortides (safety shutdown) |
| Potenciòmetre durada màxima assolida | **Para** només OUT1 (OUT2/3/4 segueixen) |

### Sortides OUT2, OUT3, OUT4
| Sortida | GPIO | Lògica |
|---------|------|--------|
| OUT2 | 7 | Replica directa IN2 LoRa |
| OUT3 | 47 | Replica directa IN3 LoRa |
| OUT4 | 48 | Replica directa IN4 LoRa |

No tenen condicions de SOC, potenciòmetre ni boia bomba. Es paren només per safety shutdown (timeout LoRa).

### Llindars energia amb histèresi (config.h receptor)
| Paràmetre | Valor | Comportament |
|-----------|-------|--------------|
| `DEYE_SOC_START` | 30% | Arrenca si SOC >= 30% |
| `DEYE_SOC_STOP` | 20% | Para si SOC <= 20% |
| Zona 20-30% | - | Manté estat actual (si corria segueix, si parada no arrenca) |

**Nota d'implementació:** `updateOutputs()` es crida tant en recepció de paquet LoRa com després de cada lectura Modbus (cada 60s). Això garanteix que la condició SOC s'avalua periòdicament sense dependre de l'arribada de paquets.

### Durada màxima (potenciòmetre)
| Paràmetre | Valor |
|-----------|-------|
| Pin | GPIO2 (receptor, ADC1) |
| Potenciòmetre | 500 Ω lineal (pull-down 4.7kΩ ja a PCB → error < 2.6%) |
| Rang | 0 – 240 minuts (0 – 4 hores) |
| ADC | 12 bits, mitjana 8 lectures, cache cada 2s |
| Display | Format "X.Xh" (hores amb 1 decimal) |
| Abast | Només para OUT1 (OUT2/3/4 no es veuen afectades) |

**Decisió pull-down vs potenciòmetre:** la R pull-down de 4.7kΩ ja existent a la PCB crea no-linealitat. Amb 1kΩ l'error arriba al 5%; un pot de 500Ω (ràtio 1:9.4) redueix l'error màxim a 2.6%, acceptable per a ajust de temps.

**ATENCIÓ - Potenciòmetre provisional:** sense el component, GPIO2 queda a GND (pull-down 4.7kΩ) → ADC=0 → la bomba no pot funcionar. Solució temporal: cable del pin POT a 3.3V → durada màxima 240 min.

---

## 7. MODBUS DEYE — MAPA DE REGISTRES (MONOFÀSIC)

> **IMPORTANT**: Aquests registres són per inversors Deye **monofàsics** (el nostre: 6kW hybrid).
> El document `docs/Deye 3p modbus address list.docx` és per trifàsics i té registres DIFERENTS.
> Referència comunitat: projecte **`kellerza/sunsynk`** a GitHub, configs ESPHome.

| Registre | Descripció | Escala | Unitat | Tipus | En ús |
|----------|------------|--------|--------|-------|-------|
| **184** | **SOC bateria** | ×1 | % | U_WORD | **Sí** |
| **186** | **Potència PV1** | ×1 | W | U_WORD | **Sí** |
| **187** | **Potència PV2** | ×1 | W | U_WORD | **Sí** |
| 109 | Tensió bateria | ×0,01 | V | U_WORD | |
| 110 | Corrent bateria | ×0,01 | A | S_WORD | |
| 150 | Tensió xarxa | ×0,1 | V | U_WORD | |
| 175 | Potència xarxa total | ×1 | W | S_WORD | |
| 176 | Potència consum total | ×1 | W | S_WORD | |
| 183 | Freqüència xarxa | ×0,01 | Hz | U_WORD | |
| 190 | Potència bateria | ×1 | W | S_WORD | |

Protocol: Modbus RTU, FC 03 (`readHoldingRegisters`), Slave ID 1, 9600 baud 8N1

---

## 8. REFERÈNCIES LCSC / EASYEDA

| Component | Referència LCSC | Model |
|-----------|----------------|-------|
| Terminal 2 pins 5.08mm | C8465 | WJ500V-5.08-2P (Kangnex) |
| Terminal 3 pins 5.08mm | **C72334** | WJ500V-5.08-3P (Kangnex) |
| Transistor BC337-40 | **C713611** | BC337-40 (LGE) |
| Transistor BC337-25 | C713610 | BC337-25 (LGE) |

---

## 9. PARÀMETRES LORA

| Paràmetre | Valor |
|-----------|-------|
| Freqüència | 868 MHz (ISM Europa) |
| Spreading Factor | SF7 |
| Bandwidth | 125 kHz |
| Coding Rate | 4/5 |
| Potència TX | 14 dBm |
| Payload | 1 byte (bits 0-3 = 4 canals) |
| Interval TX | 15 s (+ immediat en canvi) |
| Reintent TX | Immediat (~50 ms) si TX timeout |
| Timeout RX seguretat | 45 s → totes sortides OFF (tolera 2 paquets perduts consecutius) |
| IrqProcess durant Modbus | Radio.IrqProcess() cridat entre les dues lectures Modbus per reduir risc de perdua de paquet LoRa |

---

## 10. ALIMENTACIÓ

| Component | Valor | Funció |
|-----------|-------|--------|
| Font AC/DC | HLK-PM01 (5V/600mA) | 230VAC → 5VDC |
| F1 | Fusible 250mA | Protecció sobrecorrent |
| MOV1 | Varistor 275VAC | Protecció sobretensió |
| C1 | 100 µF 16V electrolític | Filtrat sortida |
| C2 | 100 nF ceràmic | Desacoblament HF |

---

## 11. CHECKLIST PCB (pendent verificació foto)

### Generals
- [ ] Slot físic separació 230V / baixa tensió (min 6 mm)
- [ ] Pistes 230V: min 1.5 mm
- [ ] Pistes alimentació 5V/3.3V: min 0.5 mm
- [ ] Pistes senyal: min 0.25 mm
- [ ] Pla de massa capa inferior (zona baixa tensió)

### Components
- [ ] GPIO45, GPIO46 NO usats com a sortides
- [ ] GPIO38-42 NO usats
- [ ] Pull-down 10kΩ a la base/gate de cada driver de relé
- [ ] Diode flyback orientat correctament (càtode a VCC)
- [ ] BC337-40 (no BC337-25 ni BC547)
- [ ] Condensadors 100nF a cada entrada de boia
- [ ] Condensador desacoblament MAX485 (100nF entre VCC pin8 i GND pin5)
- [ ] GPIO3 hardwired a MAX485 DE/RE (sense jumper, firmware gestiona per codi)
- [ ] GPIO21 NO connectat a res extern (reset OLED intern)

### Connectors
- [ ] J_IN: 8 pins 3.5mm (4 entrades + 4 GND)
- [ ] J_OUT: 8 pins 3.5mm (4 sortides + 4 GND)
- [ ] J_485: 3 pins 3.5mm (A, B, GND) → usar C72334
- [ ] USB-C del Heltec accessible des de fora de la caixa
- [ ] Antena LoRa 868MHz surt fora de la caixa

### Seguretat 230V
- [ ] Zona 230V marcada a serigrafia
- [ ] Connexió a terra (PE) si caixa metàl·lica
- [ ] Caixa IP65 si instal·lació exterior

---

## 8. CIRCUIT RS485: MAX485ED A 5V + DIVISOR DE TENSIÓ (Hardware real)

### Configuració actual de la PCB
El MAX485ED es manté a la PCB alimentat a **5VDC** (directe del HLK-PM01).
Per protegir el GPIO20 de l'ESP32-S3 (màxim 3.6V), la PCB incorpora un
**divisor de tensió al pin RO** que redueix els 5V a ~3.3V.

### Esquema real del circuit RS485
```
HLK-PM01 (5V) ──────────────────────────────> MAX485ED VCC (pin 8)
                                                +---------+
ESP32 GPIO19 (TX) ──────────────────────────>  | DI (4)   |
                                                |          |
ESP32 GPIO20 (RX) <──[divisor tensió]────────  | RO (1)   |   A ──> Deye A
                                                |          |
ESP32 GPIO3  (DE/RE) ──────────────────────>   | DE (3)   |   B ──> Deye B
                                          +--> | RE (2)   |
                                          |    +---------+
                                        Jumper   [120Ω]
                                       J_RS485     |
                                                  GND
```

### Divisor de tensió al pin RO
Redueix el voltatge de sortida del MAX485ED (fins a 5V) a nivells segurs per l'ESP32:

```
MAX485 RO (pin 1) ──┬──[ R1 ]──┬── ESP32 GPIO20
                     │           │
                    (cap)      [ R2 ]
                                │
                               GND

Vout = 5V × R2/(R1+R2) ≈ 3.3V
```

**Nota**: Verificar els valors exactes de R1 i R2 a la PCB amb multímetre.
Valors típics: R1=4.7kΩ + R2=10kΩ → 3.4V, o R1=1.8kΩ + R2=3.3kΩ → 3.24V.

### Per què funciona
- **DI (pin 4)**: L'ESP32 envia 3.3V. El MAX485ED a 5V té VIH=2.0V → 3.3V > 2.0V → OK
- **DE/RE (pins 2,3)**: Igual, 3.3V > 2.0V → OK
- **RO (pin 1)**: El MAX485ED pot emetre fins a 5V → el divisor ho redueix a ~3.3V → GPIO20 protegit
- **Bus RS485**: Alimentat a 5V, el MAX485ED genera 1.5V diferencial correcte → comunicació fiable

### Avantatges d'aquesta configuració
- El MAX485ED opera dins d'especificacions (5V)
- Voltatge diferencial RS485 correcte (1.5V)
- No cal canviar el xip
- El divisor de tensió protegeix l'ESP32

### Documentació relacionada
**`PROBLEMA_MAX485ED_SOLUCIO.md`** — Anàlisi completa del problema de voltatge amb totes les solucions possibles.

### Alternativa futura (per noves PCBs)
Per a futures revisions, substituir per **MAX3485ESA+** (3.3V natiu):
- Elimina la necessitat del divisor de tensió
- Alimentat directament des del regulador 3.3V del Heltec
- Mateix footprint SOIC-8 i pinout idèntic

### BOM actual (receptor)
| Component | Valor | Nota |
|-----------|-------|------|
| U2 | **MAX485ED** | Alimentat a 5V (HLK-PM01) |
| R_divider_1 | (verificar a PCB) | Divisor tensió RO → GPIO20 |
| R_divider_2 | (verificar a PCB) | Divisor tensió RO → GPIO20 |
| R_term | 120Ω (0805 SMD) | Terminació bus RS485 (entre A i B) |
| C_bypass | 100nF (0805 SMD) | Desacoblament VCC MAX485ED |

---

**Data actualització**: Juny 2026
**Versió document**: rev.5
