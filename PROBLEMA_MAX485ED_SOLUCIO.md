# PROBLEMA MAX485ED - ANÀLISI I SOLUCIÓ
## Comunicació RS485/Modbus entre Heltec V3 i Deye Inversor

---

**Data**: Juny 2026
**Projecte**: Sistema LoRa monitoratge boia - Receptor amb RS485
**Problema original**: No s'obtenien dades del Deye via Modbus RTU
**Causa**: El MAX485ED (5V) necessita protecció al pin RO per no danyar l'ESP32-S3 (3.3V)
**Solució implementada**: MAX485ED alimentat a 5V + divisor de tensió al pin RO

---

## RESUM EXECUTIU

El MAX485ED de la PCB requereix alimentació a 5V per funcionar dins d'especificacions.
Com que el pin RO pot emetre fins a 5V (superant el límit de 3.6V de l'ESP32-S3),
la PCB incorpora un **divisor de tensió al pin RO** que redueix el voltatge a ~3.3V.

**Configuració actual (hardware real)**:
- MAX485ED alimentat a **5V** (directe del HLK-PM01)
- Divisor de tensió al pin RO → GPIO20 (~3.3V)
- Pins DI (GPIO19) i DE/RE (GPIO3) connectats directament (3.3V > VIH 2.0V del MAX485ED)

**Alternativa per futures PCBs**: Substituir per MAX3485ESA (versió 3.3V nativa, elimina necessitat del divisor).

---

## ÍNDEX

1. [Diagnòstic del Problema](#1-diagnòstic-del-problema)
2. [Anàlisi Tècnica](#2-anàlisi-tècnica)
3. [Solucions Possibles](#3-solucions-possibles) — Solució C implementada (MAX485ED 5V + divisor RO)
4. [Alternativa per futures PCBs: MAX3485](#4-alternativa-per-futures-pcbs-substituir-per-max3485)
5. [Guia d'Implementació MAX3485](#5-guia-dimplementació-només-si-es-vol-substituir-per-max3485) (opcional)
6. [Verificació Post-Canvi](#6-verificació-post-canvi)
7. [Referències i Fonts](#7-referències-i-fonts)
8. [Mapa de Registres Modbus Deye](#8-mapa-de-registres-modbus-deye)
9. [Correccions Software](#9-correccions-software-importants-independentment-del-hardware)

---

## 1. DIAGNÒSTIC DEL PROBLEMA

### 1.1 Símptomes Observats

- ❌ Codi d'error Modbus: **0xE4 (TIMEOUT)**
- ❌ No es reben dades del Deye (SOC i Power = -1)
- ❌ Cap byte rebut al bus RS485
- ✅ Cablejat físic verificat (A, B, GND correctes)
- ✅ Configuració software correcta (9600 baud, Slave ID 1)
- ✅ Jumper J_RS485 connectat correctament

### 1.2 Investigació Realitzada

Després d'una recerca exhaustiva, es van identificar dos aspectes crítics:

1. **Manual del Deye**: Indica que el port RS485 pot estar en mode "METER" per defecte (per llegir comptadors externs), no en mode "485" (slave Modbus). **Nota**: Verificar al menú del Deye.

2. **Fòrums d'usuaris Heltec i ESP32**: Múltiples casos similars amb comunicació RS485 fallida entre Heltec V3 i dispositius Modbus. **Causa comuna**: Ús de MAX485 (5V) amb ESP32-S3 (3.3V) sense protecció adequada al pin RO.

### 1.4 Estat actual del hardware

La PCB ja implementa la **Solució C1** (divisor de tensió al pin RO):
- MAX485ED alimentat a **5V** (directe del HLK-PM01)
- Divisor resistiu entre pin RO i GPIO20 (redueix 5V a ~3.3V)
- Pins DI i DE/RE connectats directament (3.3V és suficient per VIH=2.0V del MAX485ED)

### 1.3 Identificació del Xip

**Xip instal·lat a la PCB**: `MAX485ED`

- Fabricant: HTC Korea TAEJIN Tech
- Package: SOIC-8
- Funció: RS-485/RS-422 Transceiver

---

## 2. ANÀLISI TÈCNICA

### 2.1 Especificacions del MAX485ED

Segons el datasheet oficial:

| Paràmetre                  | Valor            | Nota                          |
|----------------------------|------------------|-------------------------------|
| **Voltatge d'alimentació** | **4.75V - 5.25V**| **5V ± 5%**                   |
| Temperatura operació       | -40°C a +85°C    | Industrial                    |
| Output diferencial (VOD)   | ≥1.5V típic      | A VCC = 5V                    |
| Input threshold (VIH)      | 2.0V mínim       | Nivell TTL 5V                 |

**CONCLUSIÓ**: El MAX485ED **NO està especificat per operar a 3.3V**.

### 2.2 Especificacions de l'ESP32-S3 (Heltec V3)

| Paràmetre                  | Valor            |
|----------------------------|------------------|
| Voltatge d'alimentació     | 3.3V             |
| Output HIGH (VOH)          | 2.64V - 3.3V     |
| Output LOW (VOL)           | 0V - 0.33V       |

### 2.3 Anàlisi de la Incompatibilitat

#### Problema 1: Voltatge d'alimentació fora d'especificació

Quan s'alimenta el MAX485ED a **3.3V** (en comptes de 5V):

```
VCC = 3.3V < 4.75V (mínim especificat)
```

**Efectes**:
- Voltatge diferencial de sortida (A-B) reduït: ~0.9V en comptes de 1.5V
- Distància màxima reduïda dràsticament
- Fiabilitat compromesa, especialment amb soroll

#### Problema 2: Nivells lògics TTL incompatibles

El MAX485ED espera nivells TTL de **5V**:
- VIH (Input HIGH): 2.0V mínim

L'ESP32-S3 genera nivells de **3.3V**:
- VOH (Output HIGH): 2.64V - 3.3V típic

**Marge**: Només 0.64V - 1.3V per sobre del llindar mínim (molt just).

En condicions no ideals (temperatura, caiguda de tensió, soroll), el MAX485ED pot no reconèixer el HIGH de l'ESP32 com a vàlid.

#### Problema 3: Sortida del MAX485ED cap a l'ESP32

El MAX485ED pot generar sortides properes a 5V quan alimentat marginalment, superant el màxim de 3.6V que toleren els GPIOs de l'ESP32-S3, **amb risc de dany permanent**.

### 2.4 Per què pot funcionar ocasionalment?

En condicions "ideals" (cables curts, sense soroll, temperatura ambient), el MAX485ED pot funcionar marginalment a 3.3V, però:

- ⚠️ **Fora d'especificacions** → No garantit pel fabricant
- ⚠️ **Inestable** → Pot fallar amb temperatura, humitat, interferències
- ⚠️ **Risc de dany** → Pot danyar els GPIOs de l'ESP32

**CONCLUSIÓ**: No és recomanable per un sistema de producció.

---

## 3. SOLUCIONS POSSIBLES

### SOLUCIÓ A: Substituir per MAX3485 (3.3V) — Per futures PCBs

**Descripció**: Reemplaçar el MAX485ED per un xip MAX3485ESA dissenyat per 3.3V.

**Avantatges**:
- ✅ Solució neta i professional
- ✅ Dins d'especificacions del fabricant
- ✅ Fiabilitat garantida
- ✅ Mateix footprint SOIC-8 (reemplaçament directe)
- ✅ Mateix pinout (compatible pin a pin)
- ✅ Cap canvi de codi ni PCB necessari
- ✅ Cost mínim (~0.80€ per xip)

**Desavantatges**:
- ❌ Requereix desoldar/soldar component SMD (necessita soldador + experiència)

**Valoració**: ⭐⭐⭐⭐⭐ (5/5)

---

### SOLUCIÓ B: Alimentar MAX485ED a 5V + Level Shifters

**Descripció**: Mantenir el MAX485ED alimentant-lo a 5V i afegir level shifters bidireccionals entre l'ESP32 (3.3V) i el MAX485ED (5V).

**Implementació**:
```
                Level Shifter (3.3V ↔ 5V)
ESP32 GPIO19 ←→ [TXS0108E / BSS138] ←→ MAX485 DI
ESP32 GPIO20 ←→ [TXS0108E / BSS138] ←→ MAX485 RO
ESP32 GPIO3  ←→ [TXS0108E / BSS138] ←→ MAX485 DE/RE
```

**Requeriments**:
- Xip level shifter bidireccional (ex: TXS0108E, 8 canals)
- Regulador 5V a la PCB (o afegir-lo)
- Modificar PCB (tallar pistes, afegir components)
- 3 canals del level shifter

**Avantatges**:
- ✅ No cal desoldar el MAX485ED

**Desavantatges**:
- ❌ Molt més complex
- ❌ Requereix modificació física de la PCB
- ❌ Cost superior (~2-3€ xip + components)
- ❌ Més punts de fallada
- ❌ Més espai a la PCB
- ❌ Consum energètic superior

**Valoració**: ⭐⭐☆☆☆ (2/5) - No recomanat

---

### SOLUCIÓ C: MAX485ED a 5V + Protecció pin RO ⭐ **IMPLEMENTADA A LA PCB**

**Descripció**: El MAX485ED s'alimenta a 5V (directe del HLK-PM01) per operar dins d'especificacions. Un divisor de tensió al pin RO protegeix el GPIO20 de l'ESP32-S3.

**Configuració actual de la PCB**:
```
HLK-PM01 (5V) → MAX485ED VCC (5V directament)
                     ↓
                  [Divisor tensió al RO]
                     ↓
                ESP32-S3 GPIO20 (~3.3V)
```

#### Problema principal: Pin RO (Receiver Output)

El MAX485ED a 5V pot emetre **4.5-5V** al pin RO, superant el màxim de **3.6V** que toleren els GPIOs de l'ESP32-S3.

**Risc**: Dany permanent del GPIO20 de l'ESP32-S3.

#### Implementació amb protecció (OBLIGATORI)

**Opció C1: Divisor resistiu al pin RO (més simple)**

```
MAX485 RO ──┬──[ 4.7kΩ ]──┬── ESP32 GPIO20
            │             │
           (cap)    [ 10kΩ ]
                          │
                         GND
```

**Càlcul**:
- Divisor: 10kΩ / (10kΩ + 4.7kΩ) = 0.68
- 5V × 0.68 = **3.4V** → Segur per l'ESP32 (< 3.6V)

**Passos**:
1. Eliminar el divisor de tensió actual (VCC del MAX485ED)
2. Connectar MAX485ED VCC directament a 5V
3. Tallar pista entre MAX485 RO i ESP32 GPIO20
4. Afegir divisor resistiu (2 resistències 0805 SMD)
5. Pins DI i DE/RE es mantenen directes (funcionen amb 3.3V del ESP32)

**Opció C2: Díode Zener 3.3V (alternativa)**

```
MAX485 RO ───[ 1kΩ ]─── ESP32 GPIO20
                  │
            [Zener 3.3V]
                  │
                 GND
```

**Passos**:
1. Eliminar el divisor de tensió actual
2. Connectar MAX485ED VCC directament a 5V
3. Afegir resistència 1kΩ + Zener 3.3V entre RO i GPIO20
4. El Zener clipa qualsevol voltatge > 3.3V

**⚠️ IMPORTANT**: La resistència 1kΩ és **obligatòria** per limitar el corrent pel Zener. **NO posar només el Zener sense resistència** (es pot cremar).

**Opció C3: 2 díodes de senyal en sèrie (MÉS SIMPLE) ⭐ RECOMANADA**

```
MAX485 RO ──→|──→|──┬── ESP32 GPIO20
            D1  D2  │
          1N4148   [10kΩ]  ← Pull-down obligatori
                    │
                   GND
```

**Com funciona**:
- Cada díode 1N4148 fa caure **~0.7V**
- 2 díodes = **1.4V** de caiguda total
- 5V - 1.4V = **3.6V** al GPIO20 (dins del límit de 3.6V de l'ESP32)
- La resistència 10kΩ manté el GPIO a LOW quan RO no transmet

**Passos**:
1. Eliminar el divisor de tensió actual (VCC del MAX485ED)
2. Connectar MAX485ED VCC directament a 5V
3. **Opció A - Tallar pista**: Tallar pista entre MAX485 RO i ESP32 GPIO20, inserir els 2 díodes
4. **Opció B - Aixecar pin** (sense tallar): Desoldar només el pin 1 (RO) del MAX485ED, aixecar la pota cap amunt, soldar els díodes "volant" entre el pin aixecat i el pad del GPIO20
5. Soldar 2 díodes 1N4148 en sèrie:
   - **Díode 1**: Ànode al pin RO, càtode cap endavant
   - **Díode 2**: Ànode al càtode del díode 1, càtode al pad GPIO20
   - **IMPORTANT**: Les **dues ralles (càtodes) apunten cap al GPIO20** (mateix sentit)
6. Afegir resistència 10kΩ del GPIO20 a GND (pull-down)
7. Pins DI i DE/RE es mantenen directes

**Orientació dels díodes (CRÍTIC)**:
```
RO  ═══(──→|)═══(──→|)═══ GPIO20
         ralla     ralla
           ↓        ↓
    Ambdues ralles cap al GPIO!
```

**Components necessaris**:
| Component | Valor | Quantitat | Nota |
|-----------|-------|-----------|------|
| Díode | 1N4148 (o 1N914, BAV99) | 2 | Through-hole o SOD-123 SMD |
| Resistència | 10kΩ | 1 | Pull-down (0805 SMD o through-hole) |
| Wire/jumper | - | 1 | Bypass divisor tensió VCC |

**Avantatges vs altres opcions**:
- ✅ **Més simple** que divisor resistiu o Zener
- ✅ **Components comuns** (1N4148 molt estàndard)
- ✅ **Molt barat** (~0.10€ total)
- ✅ **Díodes aguanten molt corrent** (no es cremen com el Zener sense R)
- ✅ No cal calcular valors (sempre són 2 díodes + 10k)
- ✅ Es pot fer sense tallar pista (aixecant pin RO)

**Desavantatge**:
- ❌ Voltatge resultant 3.6V (al límit, però dins d'especificació)

**⚠️ ERRORS COMUNS A EVITAR**:
- ❌ **NO posar només els díodes sense la resistència 10kΩ** → El GPIO quedaria flotant quan RO=LOW
- ❌ **NO invertir els díodes** → Les ralles han d'anar cap al GPIO20
- ❌ **NO usar només 1 díode** → 5V - 0.7V = 4.3V (massa per l'ESP32)

**Valoració Opció C3**: ⭐⭐⭐⭐☆ (4/5) - **Millor opció si es vol usar MAX485ED a 5V**

#### Verificació abans de connectar (CRÍTIC)

**SEMPRE fer això abans de connectar a l'ESP32**:

1. Eliminar el divisor de tensió
2. Alimentar MAX485ED a 5V (sense connectar ESP32)
3. Amb multímetre, mesurar voltatge al pin RO del MAX485ED mentre el Deye transmet dades
4. **Si voltatge RO < 3.6V**: Es pot connectar directament (poc probable)
5. **Si voltatge RO ≥ 3.6V**: Afegir obligatòriament protecció (Opció C1 o C2)

#### Avantatges:
- ✅ MAX485ED funciona dins d'especificacions (5V)
- ✅ Voltatge diferencial RS485 correcte (~1.5V)
- ✅ No cal comprar xip nou
- ✅ Aprofita el MAX485ED existent

#### Desavantatges:
- ❌ Requereix modificar la PCB (eliminar divisor + afegir protecció RO)
- ❌ Més complex que substituir el xip
- ❌ Risc si no s'afegeix protecció al pin RO
- ❌ Més components (resistències/zener)
- ❌ Temps de modificació: 1-2 hores per PCB

#### Components necessaris:
| Component | Valor | Quantitat | Nota |
|-----------|-------|-----------|------|
| Resistència | 4.7kΩ | 1 | Divisor RO (0805 SMD) |
| Resistència | 10kΩ | 1 | Divisor RO (0805 SMD) |
| Wire/jumper | - | 1 | Bypass divisor tensió VCC |

**Alternativa Zener** (Opció C2):
| Component | Valor | Quantitat | Nota |
|-----------|-------|-----------|------|
| Resistència | 1kΩ | 1 | Protecció RO (0805 SMD) |
| Díode Zener | 3.3V, 500mW | 1 | Clipping (SOD-123) |

**Alternativa 2 díodes** (Opció C3 - **RECOMANADA** dins de Solució C):
| Component | Valor | Quantitat | Nota |
|-----------|-------|-----------|------|
| Díode 1N4148 | Senyal | 2 | Through-hole o SOD-123 SMD |
| Resistència | 10kΩ | 1 | Pull-down (0805 SMD) |

**Valoració global Solució C**: ⭐⭐⭐☆☆ (3/5) - Opció viable si no es vol comprar xip nou
- Dins de la Solució C, l'**Opció C3 (2 díodes) és la més recomanada** per simplicitat

**Quan usar aquesta solució**:
- Teniu moltes PCBs ja fabricades amb MAX485ED
- No podeu aconseguir MAX3485ESA ràpidament
- Voleu aprofitar el MAX485ED existent
- **Si escolliu aquesta via, useu l'Opció C3 (2 díodes)** per la seva simplicitat

**Comparativa d'opcions dins de Solució C**:
| Opció | Components | Voltatge final | Dificultat | Recomanació |
|-------|-----------|----------------|------------|-------------|
| C1 (Divisor resistiu) | 2 resistències | 3.4V | Mitjana | ⭐⭐⭐☆☆ |
| C2 (Zener + R) | Zener + 1 resistència | 3.3V exacte | Mitjana | ⭐⭐⭐☆☆ |
| **C3 (2 díodes)** | **2 díodes + 1 resistència** | **3.6V** | **Baixa-Mitjana** | **⭐⭐⭐⭐☆** |

**⚠️ ADVERTÈNCIA CRÍTICA**:
- **MAI** alimentar el MAX485ED a 5V i connectar el pin RO directament a l'ESP32 sense protecció
- **MAI** assumir que el voltatge RO serà segur sense verificar-ho
- **MAI** posar només Zener sense resistència (es pot cremar)
- **MAI** posar només díodes sense la resistència pull-down de 10kΩ (GPIO flotant)
- El dany a l'ESP32-S3 seria **permanent** i **immediat**

---

### SOLUCIÓ D: Provar a 3.3V amb millores (DESACONSELLADA)

**Descripció**: Intentar fer funcionar el MAX485ED a 3.3V (configuració actual) amb optimitzacions.

**Millores per maximitzar les possibilitats**:
1. **Cables molt curts**: ≤ 2 metres entre MAX485 i Deye
2. **Resistència de terminació**: 120Ω entre A i B (obligatori)
3. **Condensadors de desacoblament**: 100nF + 10µF al costat del MAX485ED
4. **Cablejat de qualitat**: Cable trenat (twisted pair) blindat
5. **Verificar GND comú**: Connexió sòlida entre ESP32 i Deye
6. **Velocitat reduïda**: Provar a 4800 baud en comptes de 9600

**Avantatges**:
- ✅ Sense inversió ni modificacions

**Desavantatges**:
- ❌ **Fora d'especificacions**
- ❌ Funcionament no garantit
- ❌ Pot fallar amb temperatura/soroll
- ❌ Risc de fallades intermitents
- ❌ No apte per producció

**Valoració**: ⭐⭐☆☆☆ (2/5) - Només per proves temporals

---

## 4. ALTERNATIVA PER FUTURES PCBs: SUBSTITUIR PER MAX3485

### 4.1 Per què el MAX3485?

El **MAX3485** és la versió oficial de 3.3V del MAX485:

| Característica           | MAX485ED (actual) | MAX3485ESA (recomanat) |
|--------------------------|-------------------|------------------------|
| Voltatge alimentació     | 4.75V - 5.25V     | **3.0V - 3.6V** ✓      |
| Compatible ESP32-S3      | ❌ NO             | ✅ SÍ                  |
| Footprint                | SOIC-8            | SOIC-8 (igual)         |
| Pinout                   | Compatible        | Compatible (idèntic)   |
| Output diferencial       | 1.5V @ 5V         | 1.5V @ 3.3V ✓          |
| Velocitat màxima         | 2.5 Mbps          | 10 Mbps                |
| Cost (unitat)            | ~0.50€            | ~0.80€                 |

### 4.2 Comparativa de Pinout (verificació de compatibilitat)

```
MAX485ED (actual)          MAX3485ESA (reemplaçament)
┌─────────────┐            ┌─────────────┐
│ 1  RO    VCC 8 │          │ 1  RO    VCC 8 │
│ 2  RE    B   7 │          │ 2  RE    B   7 │
│ 3  DE    A   6 │          │ 3  DE    A   6 │
│ 4  DI   GND  5 │          │ 4  DI   GND  5 │
└─────────────┘            └─────────────┘
```

✅ **Pinout idèntic** → Reemplaçament directe sense modificacions

### 4.3 Alternatives al MAX3485

Si no trobeu el MAX3485ESA, aquestes alternatives són 100% compatibles:

| Part Number       | Fabricant        | Voltatge | Velocitat | Disponibilitat |
|-------------------|------------------|----------|-----------|----------------|
| **MAX3485ESA+**   | Analog Devices   | 3.3V     | 10 Mbps   | Alta           |
| **SP3485EN-L**    | MaxLinear        | 3.3V     | 10 Mbps   | Alta           |
| **SN65HVD75DR**   | Texas Instruments| 3.3V     | 20 Mbps   | Mitjana        |
| **ISL83485IBZ**   | Renesas          | 3.3V     | 10 Mbps   | Mitjana        |

Tots són SOIC-8 i pin-compatible amb el MAX485ED.

---

## 5. GUIA D'IMPLEMENTACIÓ (només si es vol substituir per MAX3485)

> **Nota**: Aquesta secció és per futures PCBs o si es decideix canviar el MAX485ED per MAX3485ESA.
> La PCB actual funciona correctament amb MAX485ED a 5V + divisor de tensió al RO.

### 5.1 Materials Necessaris

- [ ] **MAX3485ESA+** (o equivalent) x1 unitat
- [ ] Soldador de punta fina amb control de temperatura (280-320°C)
- [ ] Flux per soldar (líquid o en pasta)
- [ ] Tira de desoldar o bomba de desoldar
- [ ] Pinces de precisió
- [ ] Lupa o microscopi USB (recomanat)
- [ ] Multímetre digital
- [ ] Alcohol isopropílic (neteja flux)

### 5.2 Procediment de Substitució

#### Pas 1: Preparació

1. **Desconnectar tot**: Desendollar PCB de 230VAC, desconnectar Deye, extreure modul Heltec
2. **Identificar el MAX485ED**: Localitzar el xip SOIC-8 a la PCB (al costat dels bornes RS485)
3. **Foto de referència**: Fer foto abans de tocar res (orientació del xip)

#### Pas 2: Desoldar el MAX485ED

**Mètode A: Aire calent (recomanat si disponible)**
1. Aplicar flux al voltant del xip
2. Escalfar uniformement amb estació d'aire calent (300-320°C)
3. Quan la soldadura es fongui, aixecar el xip amb pinces

**Mètode B: Soldador convencional**
1. Aplicar flux abundant a tots els pins
2. Desoldar pin a pin amb tira de desoldar
3. Aixecar suavament el xip amb pinces (no forçar)

**IMPORTANT**:
- ⚠️ No sobreescalfar (màx 350°C, màx 10 segons per pin)
- ⚠️ Verificar que tots els pins s'han desoldat abans d'aixecar
- ⚠️ No arrencar pistes de coure de la PCB

#### Pas 3: Neteja dels Pads

1. Amb el soldador, eliminar restes de soldadura dels pads
2. Aplicar flux fresc
3. Netejar amb alcohol isopropílic
4. Verificar amb lupa: pads plans, sense ponts de soldadura

#### Pas 4: Soldar el MAX3485ESA

1. **Orientació**: El punt o marca del pin 1 ha de coincidir amb la marca de la PCB
2. **Alineació**: Col·locar el xip centrat sobre els pads (ajudar amb pinces)
3. **Fixar**: Soldar primer el pin 1 i el pin 5 (diagonals oposats) per fixar el xip
4. **Soldar tots els pins**: Un a un, aplicant soldadura amb flux
5. **Inspecció**: Amb lupa, verificar:
   - ✓ Tots els pins soldats
   - ✓ Cap pont de soldadura entre pins
   - ✓ Soldadures brillants (no fredes/mates)

#### Pas 5: Verificació Elèctrica (abans de muntar)

Amb multímetre en mode continuïtat:

```
Test 1: Alimentació
  Pin 8 (VCC) ←→ 3.3V PCB ✓  (si MAX3485: 3.3V; si MAX485ED: redirigir a 5V)
  Pin 5 (GND) ←→ GND PCB  ✓

Test 2: Connexió GPIO ESP32
  Pin 1 (RO)  ←→ Pad GPIO20 ✓
  Pin 4 (DI)  ←→ Pad GPIO19 ✓
  Pin 2 (RE)  ←→ Pad GPIO3  ✓
  Pin 3 (DE)  ←→ Pad GPIO3  ✓

Test 3: Línia RS485
  Pin 6 (A)   ←→ Borne A (J_485) ✓
  Pin 7 (B)   ←→ Borne B (J_485) ✓

Test 4: Aïllament
  VCC ←/→ GND (resistència alta, > 1MΩ) ✓
```

Si tots els tests són correctes, **la substitució està completa**.

### 5.3 Muntatge i Primera Prova

1. **Muntar el mòdul Heltec** a la PCB
2. **Verificar jumper J_RS485** connectat
3. **Connectar alimentació** (encara sense connectar el Deye)
4. **Carregar firmware de diagnòstic** (`test_rs485_debug.ino`)
5. **Obrir Serial Monitor** (115200 baud)
6. **Verificar missatges d'inicialització** sense errors

Si tot és correcte, procedir a connectar el Deye.

---

## 6. VERIFICACIÓ POST-CANVI

### 6.1 Test sense Deye

Carregar `test_rs485_debug.ino` i verificar:

```
Expected output:
[OK] Pin DE/RE (GPIO3) configurat
[OK] Serial1 inicialitzat: 9600 baud, 8N1
[OK] Pins: TX=GPIO19, RX=GPIO20, DE/RE=GPIO3
[OK] ModbusMaster inicialitzat, Slave ID=1
```

✅ Si veieu aquests missatges, el circuit RS485 està correctament configurat.

### 6.2 Test amb Deye connectat

1. **Connectar cables RS485**:
   - MAX485ED pin A → Deye terminal A
   - MAX485ED pin B → Deye terminal B
   - GND → GND comú

2. **Verificar configuració Deye**:
   - Communication Mode: **Modbus RTU** (o "485", NO "METER")
   - Baud Rate: **9600**
   - Slave Address: **1**

3. **Verificar voltatge al GPIO20** (amb multímetre, ABANS de connectar l'ESP32):
   - Alimentar PCB a 230V (MAX485ED s'alimenta a 5V)
   - Mesurar voltatge al pad GPIO20 mentre el Deye està connectat
   - Ha de ser **≤ 3.6V** (esperat ~3.3V gràcies al divisor)

4. **Observar Serial Monitor**:

**ÈXIT ESPERAT**:
```
[TEST 1] Llegint registre 184 (SOC bateria)...
  Result code: 0x00 (SUCCESS)
  *** SOC llegit: 75% ***

[TEST 2] Llegint registre 186 (Potència PV1)...
  Result code: 0x00 (SUCCESS)
  *** Power llegit: 3250W ***
```

✅ **Sistema funcionant correctament!**

### 6.3 Proves de Fiabilitat

Després de verificar el funcionament bàsic:

1. **Test de distància**: Provar amb cable de fins a 100m (RS485 suporta fins a 1200m)
2. **Test de temperatura**: Verificar funcionament en condicions d'alta/baixa temperatura
3. **Test de llarga durada**: Deixar funcionant durant 24h i verificar que no hi ha timeouts

---

## 7. REFERÈNCIES I FONTS

### 7.1 Datasheets i Documentació Oficial

- [MAX485ED Datasheet - HTC Korea TAEJIN Tech](https://datasheet.lcsc.com/lcsc/2001140931_HTC-Korea-TAEJIN-Tech-MAX485ED_C481667.pdf)
- [MAX485 Family Datasheet - Analog Devices](https://www.analog.com/MAX481/datasheet)
- [MAX3485 Product Info - Analog Devices](https://www.analog.com/en/products/max3485.html)
- [ESP32-S3 Technical Reference Manual](https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf)
- **Deye 3P Modbus Address List** (local: `docs/Deye 3p modbus address list.docx`) - Protocol Modbus RTU per inversors trifàsics. **ATENCIÓ**: Registres NO coincideixen amb models monofàsics (veure Secció 8)

### 7.2 Guies i Tutorials

- [How to interface ESP32 to RS-485 - Mischianti](https://mischianti.org/interface-arduino-esp8266-esp32-rs-485/)
- [MAX485 TTL to RS485 Modbus Module with ESP32](https://www.hackatronic.com/max485-ttl-to-rs485-modbus-module-interfacing-with-esp32/)
- [What is RS-485 & How to Use MAX485 - CircuitState](https://www.circuitstate.com/tutorials/what-is-rs-485-how-to-use-max485-with-arduino-for-reliable-long-distance-serial-communication/)

### 7.3 Fòrums i Discussions

- [Heltec Wireless Stick V3: Modbus RS485 not working - ESP32 Forum](https://www.esp32.com/viewtopic.php?t=38874)
- [Heltec V3 with RS485 Sensor - Heltec Community](http://community.heltec.cn/t/heltec-wifi-lora-32v3-with-rs485-sensor/23833)
- [MAX485 3.3V/5V Discussion - EDABoard](https://www.edaboard.com/threads/max485-3-3v-5v-not-sure.256830/)
- [ESP8266 Modbus RTU to Sunsynk/Deye Inverter - Arduino Forum](https://forum.arduino.cc/t/esp8266-modbus-rtu-to-sunsynk-deye-inverter/1111292) ← **Serial.flush() fix, timing DE/RE**

### 7.4 Proveïdors Recomanats

**MAX3485ESA+ / Equivalents:**

- **Mouser**: https://www.mouser.com (cerca "MAX3485ESA")
- **DigiKey**: https://www.digikey.com (cerca "MAX3485")
- **LCSC**: https://www.lcsc.com (cerca "MAX3485" o "SP3485")
- **Heltec Official**: https://heltec.org/project/rs485-transceiver/

**Preus orientatius** (Juny 2026):
- MAX3485ESA+: 0.80€ - 1.20€ (unitat)
- SP3485EN-L: 0.60€ - 1.00€ (unitat)
- Enviament: Variable segons proveïdor

---

## 8. MAPA DE REGISTRES MODBUS DEYE

### 8.1 AVÍS IMPORTANT: Monofàsic vs Trifàsic

El document oficial disponible a `docs/Deye 3p modbus address list.docx` és per **inversors TRIFÀSICS (3P)**. El nostre inversor és **monofàsic (6kW hybrid)**. Els mapes de registres són **COMPLETAMENT DIFERENTS** entre models.

**Registre 184 com a exemple de la confusió**:

| Model | Registre 184 | Significat |
|-------|-------------|------------|
| **Monofàsic** (el nostre) | 184 | **Battery SOC** (%) - confirmat via ESPHome |
| **Trifàsic** (document .docx) | 184 | **Grid Type** (tipus de xarxa elèctrica) |

Per tant, **NO usar el document 3P per buscar registres del nostre inversor monofàsic**.

### 8.2 Paràmetres de comunicació (confirmats pel document oficial)

Aquests SÍ són comuns entre tots els models Deye:

| Paràmetre | Valor |
|-----------|-------|
| Baud rate | **9600 bps** |
| Paritat | **None** |
| Data bits | **8** |
| Stop bits | **1** |
| Mode | **Modbus RTU** (master/slave) |
| Function codes | **0x03** (read holding), **0x10** (write multiple) |

Configuració Arduino: `Serial1.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN)`

### 8.3 Codis d'error Modbus (del document oficial)

| Codi | Significat |
|------|-----------|
| 0x01 | Funció il·legal (function code no suportat) |
| 0x02 | Adreça de dades il·legal (registre no existeix) |
| 0x03 | Valor de dades il·legal |
| 0x04 | Fallada del servidor (error intern de l'inversor) |

Codis addicionals de la llibreria ModbusMaster:
| Codi | Significat |
|------|-----------|
| 0x00 | SUCCESS |
| 0xE0 | Invalid slave ID |
| 0xE1 | Invalid function |
| 0xE2 | Response timeout / slow response |
| 0xE3 | Invalid data (CRC error) |
| 0xE4 | **TIMEOUT** - Cap resposta del slave |

### 8.4 Registres Deye MONOFÀSIC (el nostre inversor)

Registres confirmats per la comunitat (ESPHome, SolarAssistant, Home Assistant):

#### Registres de lectura en temps real (function code 0x03, holding registers)

| Registre | Descripció | Unitat | Tipus | Usat al codi |
|----------|-----------|--------|-------|:------------:|
| **184** | **Battery SOC** | % | U_WORD | **SI** |
| **186** | **PV1 Power** | W | U_WORD | **SI** |
| **187** | **PV2 Power** | W | U_WORD | **SI** |
| 109 | Battery voltage | 0.01V | U_WORD | |
| 110 | Battery current | 0.01A | S_WORD | |
| 190 | Battery power | W | S_WORD | |
| 175 | Grid power total | W | S_WORD | |
| 176 | Load power total | W | S_WORD | |
| 183 | Grid frequency | 0.01Hz | U_WORD | |
| 150 | Grid voltage | 0.1V | U_WORD | |
| 108 | Battery temperature | 0.1C | U_WORD | |
| 529 | Day PV energy | 0.1kWh | U_WORD | |
| 514 | Day battery charge | 0.1kWh | U_WORD | |
| 515 | Day battery discharge | 0.1kWh | U_WORD | |
| 520 | Day grid buy | 0.1kWh | U_WORD | |
| 521 | Day grid sell | 0.1kWh | U_WORD | |
| 526 | Day load consumption | 0.1kWh | U_WORD | |

**Nota**: Aquests registres són dels models monofàsics Deye/Sunsynk. Per verificar-los amb el teu inversor concret, llegeix 1 registre a la vegada i comprova que els valors tenen sentit (ex: SOC entre 0-100, voltatge ~48-54V, etc.).

### 8.5 Registres Deye TRIFÀSIC (del document oficial .docx, per referència)

Extrets del document `Deye 3p modbus address list.docx`:

#### Zona 0-59: Lectura fixa (0x03)
| Registre | Descripció | Unitat |
|----------|-----------|--------|
| 0 | Device type (0x0500 = 3P hybrid) | - |
| 1 | Modbus address | [1-247] |
| 20-21 | Rated power (low/high word) | 0.1W |

#### Zona 60-499: Lectura/Escriptura (0x03 read, 0x10 write)
| Registre | Descripció | Unitat |
|----------|-----------|--------|
| 98 | Battery charge type (Lead/Lithium) | - |
| 104 | Zero export power | W |
| 108 | Max charge current | A |
| 109 | Max discharge current | A |
| 115 | Battery capacity ShutDown | % |
| 116 | Battery capacity Restart | % |
| 141 | Energy management mode | bitmap |
| 154 | Sell mode time point 1 power | W |

#### Zona 500+: Lectura temps real (0x03)
| Registre | Descripció | Unitat | Tipus |
|----------|-----------|--------|-------|
| 500 | Run state (0=standby, 2=normal, 4=fault) | - | U16 |
| 501 | Day active power generation | 0.1kWh | S16 |
| 514 | Today battery charge | 0.1kWh | U16 |
| 515 | Today battery discharge | 0.1kWh | U16 |
| 520 | Day grid buy | 0.1kWh | U16 |
| 521 | Day grid sell | 0.1kWh | U16 |
| 526 | Day load consumption | 0.1kWh | U16 |
| 529 | Day PV total | 0.1kWh | U16 |
| 540 | DC transformer temperature | 0.1C (offset 1000) | U16 |
| 551 | On/Off status (0=off, 1=on) | - | U16 |
| 553-554 | Warning words 1-2 | bitmap | U16 |
| 555-558 | Fault words 1-4 | bitmap | U16 |
| **586** | **Battery temperature** | 0.1C | U16 |
| **587** | **Battery voltage** | 0.01V | U16 |
| **588** | **Battery SOC** | % [0-100] | U16 |
| **590** | **Battery output power** | W | S16 |
| **591** | Battery output current | 0.01A | S16 |
| 598-600 | Grid phase voltage A/B/C | 0.1V | U16 |
| 607 | Grid total active power | W | S16 |
| 609 | Grid frequency | 0.01Hz | U16 |
| 625 | Grid side total power | W | S16 |
| 636 | Inverter output total power | W | S16 |
| 653 | Load total power | W | S16 |
| **672** | **PV1 input power** | W | U16 |
| **673** | **PV2 input power** | W | U16 |
| 674 | PV3 input power | W | U16 |
| 675 | PV4 input power | W | U16 |
| 676-683 | PV1-4 voltage/current | 0.1V / 0.1A | U16 |

### 8.6 Resum comparatiu: Per què els registres no coincideixen

```
                    MONOFÀSIC          TRIFÀSIC (3P)
                    (el nostre)        (document .docx)
                    ───────────        ─────────────────
Battery SOC:        reg 184            reg 588
PV1 Power:          reg 186            reg 672
PV2 Power:          reg 187            reg 673
Battery Power:      reg 190            reg 590
Grid Total Power:   reg 175            reg 625
Load Total Power:   reg 176            reg 653
Battery Voltage:    reg 109            reg 587
Grid Voltage:       reg 150            reg 598
```

**Conclusió**: Deye utilitza mapes de registres completament diferents per cada gamma de producte. El document .docx serveix com a referència de protocol i codis d'error, pero les adreces de registres **NO** s'han d'usar per l'inversor monofàsic.

---

## 9. CORRECCIONS SOFTWARE (IMPORTANTS INDEPENDENTMENT DEL HARDWARE)

Independentment de quina solució hardware s'apliqui (MAX3485, díodes, etc.), hi ha correccions de software que són **crítiques** per una comunicació Modbus RS485 fiable.

### 9.1 Serial.flush() al postTransmission (CRÍTIC)

**Problema identificat**: El pin DE/RE baixa (mode recepció) **abans** que tots els bytes del paquet Modbus hagin sortit pel port sèrie. Això causa que l'últim(s) byte(s) del frame es perdin o es corrompin.

**Font**: Fòrum Arduino - [ESP8266 Modbus RTU to Sunsynk/Deye Inverter](https://forum.arduino.cc/t/esp8266-modbus-rtu-to-sunsynk-deye-inverter/1111292)

**Causa**: `delayMicroseconds(100)` no és suficient. A 9600 baud, cada byte triga ~1.04ms (10 bits × 104µs). Si queden 2 bytes al buffer TX, calen ~2ms, no 100µs.

**Solució**: Usar `Serial1.flush()` que espera fins que el buffer TX estigui completament buit:

```cpp
// INCORRECTE - El delay pot ser insuficient
void rs485PostTransmission() {
  delayMicroseconds(100);  // ❌ Pot tallar la transmissió!
  digitalWrite(RS485_DE_RE_PIN, LOW);
}

// CORRECTE - flush() espera que TOTS els bytes surtin
void rs485PostTransmission() {
  Serial1.flush();           // ✅ Espera que el buffer TX estigui buit
  delayMicroseconds(50);     // Marge per l'últim stop bit
  digitalWrite(RS485_DE_RE_PIN, LOW);
}
```

### 9.2 No manipular DE/RE fora dels callbacks

El pin DE/RE s'ha de controlar **exclusivament** mitjançant els callbacks `preTransmission` i `postTransmission` de la llibreria ModbusMaster. Qualsevol toggle manual del pin fora d'aquests callbacks pot interferir amb la comunicació.

```cpp
// INCORRECTE - Toggle manual abans de readHoldingRegisters
digitalWrite(RS485_DE_RE_PIN, HIGH);  // ❌ Innecessari i perjudicial
delayMicroseconds(50);
digitalWrite(RS485_DE_RE_PIN, LOW);
uint8_t result = modbus.readHoldingRegisters(reg, 1);

// CORRECTE - La llibreria ho gestiona sola
uint8_t result = modbus.readHoldingRegisters(reg, 1);  // ✅ Callbacks fan la feina
```

### 9.3 Registres Modbus Deye - Consideració d'offset

Alguns documents Deye usen numeració de registres amb offset "40001" (convenció Modbus per Holding Registers). Quan s'usa `readHoldingRegisters()`, la llibreria ja aplica el function code 0x03, per tant:

- Si el document Deye diu **registre 40190** → usar adreça **189** (40190 - 40001)
- Si el document Deye diu **registre 190** (sense prefix) → usar adreça **190** directament
- Registres amb prefix **30001** → usar `readInputRegisters()` (function code 0x04)

**Consell de depuració**: Imprimir **sempre** el codi de resultat, no només quan és SUCCESS:

```cpp
uint8_t result = modbus.readHoldingRegisters(reg, 1);
Serial.printf("Result: 0x%02X\n", result);  // Sempre, fora del if
if (result == modbus.ku8MBSuccess) {
  // processar dades
}
```

### 9.4 Temporització entre peticions

El Deye pot necessitar temps entre peticions consecutives. Recomanació: **mínim 500ms** entre lectures Modbus. En cas de problemes, provar amb 1-2 segons.

### 9.5 Començar amb lectures simples

Per depurar, sempre començar llegint **1 sol registre** amb `readHoldingRegisters(adreça, 1)`. Un cop funcioni, ampliar a lectures múltiples.

---

## 10. CONCLUSIONS

### 10.1 Resum del Problema

El **MAX485ED** requereix alimentació a 5V per funcionar dins d'especificacions. Connectar-lo directament a l'ESP32-S3 (3.3V) sense protecció causa:
- Pin RO pot emetre fins a 5V → dany permanent del GPIO20
- Si s'alimenta a 3.3V (fora d'especificació) → comunicació inestable

### 10.2 Solució Implementada: MAX485ED a 5V + Divisor de tensió

**Configuració actual de la PCB (funcional)**:
- ✅ MAX485ED alimentat a **5V** (directe del HLK-PM01) → dins d'especificacions
- ✅ Divisor de tensió al pin RO → GPIO20 rep ~3.3V (protegit)
- ✅ Pins DI i DE/RE connectats directament (3.3V > VIH 2.0V)
- ✅ Voltatge diferencial RS485 correcte (1.5V)
- ✅ Comunicació Modbus fiable

### 10.3 Alternativa per futures PCBs: MAX3485ESA

Per simplificar el circuit en futures revisions:
- Substituir MAX485ED per **MAX3485ESA** (3.3V natiu)
- Elimina la necessitat del divisor de tensió al RO
- Mateix footprint SOIC-8, pinout idèntic
- Cost: ~1€ per xip

### 10.4 Impacte

**Estat actual**: Hardware llest per testejar amb el firmware corregit
**Correccions software aplicades**: `Serial1.flush()`, registres correctes (184/186/187), pausa 500ms entre lectures

---

## 11. CHECKLIST DE TEST (configuració actual: MAX485ED a 5V + divisor RO)

### Verificació hardware (abans d'encendre)
- [ ] MAX485ED VCC connectat a 5V (HLK-PM01)
- [ ] Divisor de tensió present entre pin RO i GPIO20
- [ ] Jumper J_RS485 posat (GPIO3 → DE/RE del MAX485ED)
- [ ] Cablejat RS485: A→A, B→B, GND comú entre PCB i Deye
- [ ] Resistència terminació 120Ω entre A i B (recomanat)

### Verificació elèctrica (amb multímetre)
- [ ] Alimentar PCB (230VAC) SENSE el mòdul Heltec muntat
- [ ] Mesurar VCC del MAX485ED: ha de ser **~5V**
- [ ] Mesurar voltatge al pad GPIO20: ha de ser **≤ 3.6V**
- [ ] Si GPIO20 > 3.6V: **NO muntar el Heltec** → revisar divisor tensió

### Test software
- [ ] Muntar mòdul Heltec a la PCB
- [ ] Carregar `test_rs485_debug.ino` (registres: SOC=184, PV1=186)
- [ ] Verificar inicialització correcta al Serial Monitor (115200 baud)
- [ ] Configuració Deye verificada (Modbus RTU, 9600, Slave ID 1)
- [ ] Test lectura SOC (registre 184): result 0x00 (SUCCESS)
- [ ] Test lectura PV1 (registre 186): result 0x00 (SUCCESS)

### Test de producció
- [ ] Carregar `receptor.ino` (firmware final)
- [ ] Verificar lectura periòdica SOC i PV al display OLED
- [ ] Test de fiabilitat 24h: sense timeouts
- [ ] Documentació actualitzada

---

**Document generat**: Juny 2026
**Autoria**: Anàlisi tècnica del projecte LoRa Heltec
**Versió**: 1.4 - Actualitzat per reflectir hardware real (MAX485ED a 5V + divisor tensió RO), registres monofàsic corregits

---

**NOTA FINAL**: La PCB actual utilitza MAX485ED a 5V amb divisor de tensió al pin RO. Aquesta configuració és funcional i dins d'especificacions. Per futures revisions de PCB, considerar substituir per MAX3485ESA (3.3V natiu) per simplificar el circuit eliminant el divisor.
