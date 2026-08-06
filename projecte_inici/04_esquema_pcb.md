# Esquema de Blocs i Consideracions PCB

## Diagrama de Blocs de la PCB (unica per TX i RX)

```
+============================================================================+
|                          PCB UNIVERSAL TX/RX                                |
|                                                                            |
|  +----------+     +-----------+     +---------------------------+          |
|  | 230VAC   |     | HLK-PM01  |     |     Heltec WiFi LoRa     |          |
|  | INPUT    |---->| AC/DC     |---->|     32 V3 (modul)         |          |
|  | + Fuse   |     | 5V/600mA  |     |                           |          |
|  | + MOV    |     +-----------+     |  GPIO1 <--- IN1 (J_IN)    |          |
|  +----------+          |            |  GPIO2 <--- IN2 (J_IN)    |          |
|                        |            |  GPIO4 <--- IN3 (J_IN)    |          |
|                   +----+----+       |  GPIO5 <--- IN4 (J_IN)    |          |
|                   | Regulat.|       |                           |          |
|                   |  intern |       |  GPIO6 ---> OUT1 (J_OUT)  |          |
|                   | 5V->3.3V|       |  GPIO7 ---> OUT2 (J_OUT)  |          |
|                   +---------+       |  GPIO47---> OUT3 (J_OUT)  |          |
|                                     |  GPIO48---> OUT4 (J_OUT)  |          |
|                                     |                           |          |
|                                     |  [OLED 0.96"]  [SX1262]  |          |
|                                     |  [USB-C]       [Antenna]  |          |
|                                     +---------------------------+          |
|                                                                            |
|  +------------------+          +-------------------+                       |
|  | CONNECTORS       |          | CONNECTORS        |                       |
|  | ENTRADES (J_IN)  |          | SORTIDES (J_OUT)  |                       |
|  |                  |          |                    |                       |
|  | IN1  IN2  IN3 IN4|          | OUT1 OUT2 OUT3 OUT4|                       |
|  | GND  GND  GND GND|          | GND  GND  GND  GND|                       |
|  +------------------+          +-------------------+                       |
|                                                                            |
|  +------- JP1 (jumper 1x3) -------+       +------------------+            |
|  |  [J_IN]  o--o  GPIO3  o--o  [J_RS485]  |    MAX485ED      |            |
|  +----------+------+------+-------+       |   (5V + divisor) |            |
|             |      |      |               | DI  <-- GPIO19   |            |
|          J_IN(IN5) |   DE+RE              | RO  -[DIV]-> G20 |---> J_485  |
|                  Heltec                   | DE/RE <-- GPIO3  |  (A, B)    |
|                                           | VCC = 5V (HLK)   |            |
|                                           +------------------+            |
|                                                                            |
+============================================================================+
```

## Dimensions Estimades de la PCB

| Element             | Dimensions           |
|---------------------|----------------------|
| Heltec V3 (modul)  | 50.2 x 25.5 mm       |
| HLK-PM01           | 34 x 20 x 15 mm      |
| **PCB total (est.)**| **80 x 60 mm**       |

## Zones de la PCB

La PCB s'ha de dividir en zones clarament separades:

```
+----------------------------------+
|  ZONA 230VAC    |  ZONA BAIXA    |
|  (Fuse, MOV,    |  TENSIO        |
|   HLK-PM01)     |  (Heltec,      |
|                  |   connectors,  |
|  >>> SLOT <<<    |   entrades,    |
|  separacio       |   sortides)    |
|  6mm minim       |                |
+----------------------------------+
```

## Circuit GPIO3 amb Jumpers (J_IN5 / J_RS485)

GPIO3 es un pin polivalent que es configura amb dos jumpers fisics (pin headers 2.54mm
de 3 pins). Segons el jumper posat, la PCB funciona com a emissor o receptor.

```
                          J_IN5 (jumper 2 pins)
                         +-----+
Connector J_IN (IN5) ----| o o |
                         +--+--+
                            |
                         GPIO3 (Heltec)
                            |
                         +--+--+
MAX485 DE+RE ------------| o o |
                         +-----+
                          J_RS485 (jumper 2 pins)
```

| Configuracio | J_IN5   | J_RS485 | Funcio GPIO3                |
|--------------|---------|---------|-----------------------------|
| Emissor      | POSAT   | OBERT   | Entrada digital (IN5)       |
| Receptor     | OBERT   | POSAT   | Control direccio MAX485     |
| Cap dels dos | OBERT   | OBERT   | GPIO3 desconnectat          |

**IMPORTANT:** No posar mai els dos jumpers alhora. Son mutuament excloents.

### Implementacio a la PCB

- Tres pads alineats: pad esquerre (J_IN), pad central (GPIO3), pad dret (MAX485 DE+RE)
- Jumper J_IN5: curtcircuita pad esquerre amb pad central
- Jumper J_RS485: curtcircuita pad central amb pad dret
- Serigrafia clara indicant "IN5" i "RS485" als costats

## Circuit RS485 (nomes receptor)

**CONFIGURACIÓ ACTUAL: MAX485ED a 5V + divisor de tensió al RO**

```
HLK-PM01 (5V)
    |
    +------------------------------------------+
    |                                          |
ESP32-S3 (3.3V)          MAX485ED (5V)         |    Bus RS485
                         +---------+           |
GPIO19 (TX) ------------>| DI    A |-----------|---> Terminal A (Deye)
                         |       VCC|<---------+
GPIO20 (RX) <--[DIVISOR]-| RO    B |------------> Terminal B (Deye)
                         |         |
GPIO3 (DE/RE) ---------->| DE  GND |<-- GND
                    +--->| RE      |
                    |    +---------+
                    |         |
                  Jumper    [120Ω]  Resistencia terminacio
                 J_RS485      |     (entre A i B)
                              GND

Divisor de tensio al pin RO (proteccio GPIO20):
MAX485 RO ──[ R1 ]──┬── ESP32 GPIO20
                     │
                   [ R2 ]
                     │
                    GND
Vout = 5V × R2/(R1+R2) ≈ 3.3V
```

### Components del circuit RS485

| Component    | Valor/Model     | Footprint | Nota                                      |
|--------------|-----------------|-----------|-------------------------------------------|
| U2           | **MAX485ED**    | SOIC-8    | Alimentat a 5V (directe HLK-PM01)         |
| R1 (divisor) | (verificar PCB) | 0805 SMD | Divisor tensio RO: serie                  |
| R2 (divisor) | (verificar PCB) | 0805 SMD | Divisor tensio RO: a GND                  |
| R_term       | 120 ohm         | 0805 SMD  | Terminacio bus RS485 (recomanat)           |
| C_bypass     | 100 nF          | 0805 SMD  | Desacoblament Vcc (al costat del MAX485ED) |

**Per què MAX485ED a 5V**: El MAX485ED opera dins d'especificacions a 5V, generant
el voltatge diferencial RS485 correcte (1.5V). El divisor de tensio al pin RO protegeix
el GPIO20 de l'ESP32 (max 3.6V). Els pins DI i DE/RE accepten 3.3V del ESP32 correctament
(VIH del MAX485ED = 2.0V < 3.3V).

**Alternativa per futures PCBs**: Substituir per MAX3485ESA (3.3V natiu), eliminant
la necessitat del divisor. Mateix footprint SOIC-8 i pinout identic.

Veure document `PROBLEMA_MAX485ED_SOLUCIO.md` per detalls tecnics complerts.

## Connector del Modul Heltec

El Heltec V3 es munta sobre la PCB amb **2 tires de pins femella de 18 pins** (pas 2.54mm).
Aixo permet:
- Extreure el modul per programar-lo via USB
- Substituir el modul si es fa malbé
- Testejar el modul independentment

## Circuit d'Entrades

Cada entrada segueix el mateix esquema:

```
                    R_proteccio (1k)
Connector IN ---+---[  1k  ]---+--- GPIOx (Heltec)
                |               |
                +---[R_pulldown]-+
                |   (10k a GND) |
                |               |
               GND             C_filter
                               (100nF)
                                |
                               GND
```

| Component    | Valor   | Funcio                                    |
|--------------|---------|-------------------------------------------|
| R_proteccio  | 1 kohm  | Limita corrent cap al GPIO                |
| R_pulldown   | 10 kohm | Defineix estat LOW quan no hi ha senyal    |
| C_filter     | 100 nF  | Filtra soroll / anti-rebots hardware       |

## Circuit de Sortides

Cada sortida amb driver per rele:

```
                    R_base (1k)          Rele bobina 5V
GPIOx (Heltec) ---[ 1k ]---+---B        +------+------+
                            |   |  NPN   |      |      |
                            |   E        |   COM  NC   NO ---> Carrega
                            |   |        |      |
                           GND  +--------+      |
                                |    C          |
                                +-----|<|-------+ D1 (1N4007)
                                  (flyback)
```

| Component   | Valor      | Funcio                              |
|-------------|------------|-------------------------------------|
| R_base      | 1 kohm     | Limita corrent base transistor      |
| Q (NPN)     | BC547/BC337| Driver de potencia per al rele      |
| D1          | 1N4007     | Proteccio flyback (inductiu rele)   |
| Rele        | 5V bobina  | Commutacio carrega de potencia      |

**Alternativa sense rele:** Si la sortida nomes es un senyal 3.3V cap a un altre sistema, no cal el driver. Es connecta directament amb una resistencia de proteccio de 330 ohm.

## Connectors Recomanats

| Connector   | Tipus                    | Pins | Nota                          |
|-------------|--------------------------|------|-------------------------------|
| J_AC        | Bornes cargol 5.08mm     | 2    | Entrada 230VAC (L, N)         |
| J_PE        | Borne cargol 5.08mm      | 1    | Terra proteccio (si cal)      |
| J_IN        | Bornes cargol 3.5mm      | 8    | 4 entrades + 4 GND            |
| J_OUT       | Bornes cargol 3.5mm      | 8    | 4 sortides + 4 GND            |
| J_ANT       | IPEX 1.0 (del modul)     | -    | Antena LoRa (forat a la PCB)  |
| J_USB       | USB-C (del modul)        | -    | Programacio (accesible)        |
| J_485       | Bornes cargol 3.5mm      | 3    | RS485: A, B, GND               |
| JP1         | Pin header 1x3 2.54mm    | 3    | Selector GPIO3: IN5 / RS485    |

## Consideracions de Disseny PCB

### Regles de Routing
- **Amplada pista 230V:** minim 1 mm (recomanat 1.5 mm)
- **Amplada pista 5V/3.3V:** minim 0.5 mm
- **Amplada pista senyal:** minim 0.25 mm
- **Separacio 230V / baixa tensio:** minim 6 mm (slot a la PCB)
- **Pla de massa** a la capa inferior per la zona de baixa tensio

### Antena LoRa
- Zona lliure de coure al voltant del connector d'antena
- L'antena LoRa ha de sortir per fora de la caixa (o caixa de plastic)
- No posar components metalics a prop de l'antena
- Freqüencia recomanada a Europa: **868 MHz** (banda ISM)

### Muntatge del Modul
- Headers femella de 2.54mm per al Heltec V3
- Forats de muntatge M3 a les 4 cantonades de la PCB
- Serigrafia clara amb noms de connectors i polaritat
