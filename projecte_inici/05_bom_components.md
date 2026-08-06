# BOM (Bill of Materials) - Llista de Components

## Per cada PCB (quantitat x2: una emissor, una receptor)

### Components Principals

| Ref  | Component                    | Valor/Model    | Qty | Footprint       | Nota                    |
|------|------------------------------|----------------|-----|-----------------|-------------------------|
| U1   | Heltec WiFi LoRa 32 V3      | HTIT-WB32LA    | 1   | 2x18 pin header | Modul enchufable        |
| PS1  | Font AC/DC                   | HLK-PM01       | 1   | Through-hole    | 230VAC -> 5VDC 600mA    |

### Proteccions (zona 230VAC)

| Ref  | Component                    | Valor          | Qty | Footprint       | Nota                    |
|------|------------------------------|----------------|-----|-----------------|-------------------------|
| F1   | Fusible                      | 250mA 250V     | 1   | 5x20mm PCB      | Proteccio sobrecorrent  |
| MOV1 | Varistor                     | 275VAC (14D431)| 1   | Radial 7.5mm    | Proteccio transitoria   |

### Condensadors

| Ref  | Component                    | Valor          | Qty | Footprint       | Nota                    |
|------|------------------------------|----------------|-----|-----------------|-------------------------|
| C1   | Condensador electrolitic     | 100uF 16V      | 1   | Radial D6.3     | Filtrat sortida font    |
| C2   | Condensador ceramic          | 100nF 50V      | 1   | 0805 SMD        | Desacoblament HF        |
| C3-6 | Condensador ceramic (filtres)| 100nF 50V      | 4   | 0805 SMD        | Filtre entrades         |

### Resistencies

| Ref   | Component                   | Valor          | Qty | Footprint       | Nota                    |
|-------|------------------------------|----------------|-----|-----------------|-------------------------|
| R1-4  | Resistencia proteccio entrada| 1k ohm         | 4   | 0805 SMD        | Limitador corrent GPIO  |
| R5-8  | Resistencia pull-down        | 10k ohm        | 4   | 0805 SMD        | Pull-down entrades      |
| R9-12 | Resistencia base transistor  | 1k ohm         | 4   | 0805 SMD        | Driver sortides         |

### Transistors i Diodes (drivers sortida)

| Ref   | Component                   | Valor          | Qty | Footprint       | Nota                    |
|-------|------------------------------|----------------|-----|-----------------|-------------------------|
| Q1-4  | Transistor NPN               | BC547 / BC337  | 4   | TO-92           | Driver rele             |
| D1-4  | Diode rectificador           | 1N4007         | 4   | DO-41           | Flyback rele            |

### Reles (opcional, segons aplicacio)

| Ref   | Component                   | Valor          | Qty | Footprint       | Nota                    |
|-------|------------------------------|----------------|-----|-----------------|-------------------------|
| K1-4  | Rele                         | 5V bobina, 10A | 4   | SRD-05VDC-SL-C | Nomes si calen sortides |

### RS485 (nomes receptor)

| Ref   | Component                   | Valor/Model     | Qty | Footprint       | Nota                          |
|-------|------------------------------|-----------------|-----|-----------------|-------------------------------|
| U2    | Transceiver RS485 3.3V      | **MAX3485ESA+** | 1   | SOIC-8          | **CRÍTIC: 3.3V, NO MAX485ED** |
|       | Alternativa 1                | SP3485EN-L      | 1   | SOIC-8          | Compatible 3.3V               |
|       | Alternativa 2                | SN65HVD75DR     | 1   | SOIC-8          | Compatible 3.3V               |
| R13   | Resistencia terminacio       | 120 ohm         | 1   | 0805 SMD        | Entre A i B del bus RS485     |

**⚠️ IMPORTANT**: El MAX485ED (5V) **NO és compatible** amb l'ESP32-S3 (3.3V). Utilitzar **MAX3485** o equivalent 3.3V. Veure document `PROBLEMA_MAX485ED_SOLUCIO.md` per detalls tècnics.

### Connectors

| Ref   | Component                   | Valor           | Qty | Footprint       | Nota                    |
|-------|------------------------------|-----------------|-----|-----------------|-------------------------|
| J_AC  | Bornes cargol                | 2 pins 5.08mm   | 1   | PCB THT          | Entrada 230VAC          |
| J_IN  | Bornes cargol                | 8 pins 3.5mm    | 1   | PCB THT          | 4 IN + 4 GND            |
| J_OUT | Bornes cargol                | 8 pins 3.5mm    | 1   | PCB THT          | 4 OUT + 4 GND           |
| J_485 | Bornes cargol                | 3 pins 3.5mm    | 1   | PCB THT          | RS485: A, B, GND (receptor)|
| H1-2  | Tira pins femella            | 1x18 pins 2.54mm| 2   | Through-hole     | Socket modul Heltec     |

### Jumpers configuracio GPIO3

| Ref     | Component                    | Valor           | Qty | Footprint       | Nota                         |
|---------|------------------------------|-----------------|-----|-----------------|------------------------------|
| JP1     | Pin header mascle            | 1x3 pins 2.54mm | 1  | Through-hole    | Selector GPIO3 (IN5/RS485)   |
| JP1_CAP | Jumper cap (shunt)           | 2.54mm          | 1   | -               | Per seleccionar J_IN5 o J_RS485 |

### Mecanics

| Ref   | Component                   | Valor          | Qty | Nota                         |
|-------|------------------------------|----------------|-----|------------------------------|
| -     | Forats muntatge              | M3             | 4   | Cantonades PCB               |
| -     | Separadors PCB               | M3 x 10mm     | 4   | Muntatge a caixa             |
| -     | Antena LoRa 868MHz           | IPEX 1.0       | 1   | Inclosa amb Heltec (verificar)|

## Resum de Cost Estimat (per PCB)

| Grup                    | Cost estimat |
|-------------------------|-------------|
| Heltec V3 modul         | ~18 EUR     |
| HLK-PM01                | ~3 EUR      |
| Components passius (R,C)| ~1 EUR      |
| Transistors + diodes    | ~1 EUR      |
| Reles (x4)              | ~4 EUR      |
| MAX3485 (receptor)      | ~1 EUR      |
| Connectors              | ~3 EUR      |
| PCB fabricacio           | ~5 EUR      |
| **Total per placa**     | **~36 EUR** |
| **Total projecte (x2)** | **~72 EUR** |

*Preus orientatius sense enviament. Poden variar segons proveidor (LCSC, Mouser, Digikey, AliExpress).*
