================================================================================
  SISTEMA LORA PUNT A PUNT - MONITORATGE BOIA
  Document de referencia del projecte (per imprimir)
  Data: Agost 2026  (rev.5: logica fail-safe NC/NO per boies, inversio IN1/IN2/IN3)
================================================================================


================================================================================
1. VISIO GENERAL
================================================================================

Sistema de comunicacio LoRa punt a punt amb dues plaques PCB identiques:

  EMISSOR (TX)                                    RECEPTOR (RX)
  Boia / camp                                     Sala tecnica / inversor
  +------------------+                            +------------------+
  | Heltec V3 LoRa   |   ~~~~ 868 MHz ~~~~>      | Heltec V3 LoRa   |
  | 4 entrades digit. |                           | 4 sortides digit. |
  | OLED 0.96"        |                           | OLED 0.96"        |
  +------------------+                            | RS485 -> Deye     |
                                                  +------------------+

- Mateixa PCB per emissor i receptor (PCB universal)
- Nomes canvia el firmware (GPIO3 hardwired a MAX485 DE/RE, MAX485 nomes poblat al receptor)
- Protocol: 1 byte LoRa (bits 0-3 = estat de 4 canals)
- Seguretat: timeout 150s al receptor -> totes les sortides OFF


================================================================================
2. XIP PRINCIPAL - HELTEC WIFI LORA 32 V3
================================================================================

  Model:        Heltec WiFi LoRa 32 V3 (HTIT-WB32LA)
  MCU:          ESP32-S3FN8 (dual-core Xtensa LX7, 240 MHz)
  LoRa:         Semtech SX1262
  Display:      OLED SSD1306, 0.96", 128x64, I2C (0x3C)
  Vext:         GPIO36 controla alimentacio OLED (LOW = ON)
  Alimentacio:  5V pel pin Vin (regulador intern 5V -> 3.3V)
  Dimensions:   50.2 x 25.5 mm
  Muntatge:     2 tires de 18 pins femella (2.54mm) - enchufable


================================================================================
3. ASSIGNACIO DE PINS GPIO
================================================================================

3.1 PINS USATS INTERNAMENT PEL HELTEC V3 (NO TOCAR)
--------------------------------------------------------------------------------
  GPIO    Funcio
  ----    ------
  17      OLED SDA (I2C)
  18      OLED SCL (I2C)
  21      OLED RESET  <<<< NO fer servir per RS485!
  8       LoRa CS/NSS (SPI)
  9       LoRa SCK (SPI)
  10      LoRa MOSI (SPI)
  11      LoRa MISO (SPI)
  12      LoRa RST
  13      LoRa BUSY
  14      LoRa DIO1/IRQ
  35      LED intern
  36      Vext control
  0       Boot button (strapping pin)

3.2 PINS SEGURS PER US EXTERN (recomanats pel fabricant)
--------------------------------------------------------------------------------
  GPIO    Funcions                     Nota
  ----    --------                     ----
  1       Digital I/O, ADC1, Touch     Segur
  2       Digital I/O, ADC1, Touch     Segur
  3       Digital I/O, ADC1, Touch     Segur (no a la llista oficial, funcional)
  4       Digital I/O, ADC1, Touch     Segur
  5       Digital I/O, ADC1, Touch     Segur
  6       Digital I/O, ADC1, Touch     Segur
  7       Digital I/O, ADC1, Touch     Segur
  19      Digital I/O (USB D-)         Segur si no es fa servir USB
  20      Digital I/O (USB D+)         Segur si no es fa servir USB
  47      Digital I/O                  Segur, sense connexio interna
  48      Digital I/O                  Segur, sense connexio interna

3.3 PINS A EVITAR
--------------------------------------------------------------------------------
  GPIO        Motiu
  ----        -----
  0, 46       Strapping pins (boot mode)
  21          Reset OLED (usat internament)
  26          SubSPI chip-select
  33-38       SPI Flash intern
  39-42       JTAG
  43, 44      USB serial download

3.4 ASSIGNACIO DEFINITIVA DEL PROJECTE
--------------------------------------------------------------------------------

  ENTRADES (emissor):
  GPIO    Funcio          Connector    Nota
  ----    ------          ---------    ----
  1       Entrada 1       J_IN         Boia senyal 1 (nivell minim), contacte NC. 1=falta aigua, 0=te aigua/desconnectada
  2       Entrada 2       J_IN         Boia senyal 2 (nivell maxim), contacte NC. 1=falta aigua, 0=te aigua/desconnectada
  4       Entrada 3       J_IN         Boia senyal 3 (nivell intermig), contacte NC. 1=falta aigua, 0=te aigua/desconnectada
  5       Entrada 4       J_IN         Reserva

  ENTRADES (receptor):
  GPIO    Funcio                  Connector    Nota
  ----    ------                  ---------    ----
  1       Switch mitja carrega    J_IN         Pull-down. 1=mitja carrega activa (condiciona OUT1 amb IN2 LoRa)
  2       Potenciometre           J_IN         Durada maxima bomba, 0-240 min / 0-4h (ADC1, 500ohm, pull-down 4.7k)
  5       Boia diposit bomba      J_IN         Pull-down, contacte NO. 1=aigua detectada, 0=sense aigua/desconnectada (fail-safe marxa en sec)

  SORTIDES (receptor):
  GPIO    Funcio          Connector    Nota
  ----    ------          ---------    ----
  6       Sortida 1       J_OUT        Rele/actuador 1
  7       Sortida 2       J_OUT        Rele/actuador 2
  47      Sortida 3       J_OUT        Reserva
  48      Sortida 4       J_OUT        Reserva

  RS485 (receptor):
  GPIO    Funcio          Connexio         Nota
  ----    ------          --------         ----
  19      RS485 TX        MAX485 DI        Directe (no creuat)
  20      RS485 RX        MAX485 RO        Directe (no creuat)
  3       RS485 DE/RE     MAX485 DE+RE     Hardwired directe (sense jumper)

  NOTA GPIO3:
  - Receptor: hardwired a MAX485 DE/RE. Configurat OUTPUT, gestiona la direccio del bus RS485
  - Emissor:  MAX485 no poblat. GPIO3 configurat OUTPUT LOW per evitar pin flotant

  NOTA: Nivell logic de tots els GPIO = 3.3V. Max 20mA per pin recomanat.


================================================================================
4. PARAMETRES LORA
================================================================================

  Parametre                  Valor
  ---------                  -----
  Frequencia                 868 MHz (banda ISM Europa)
  Spreading Factor           SF7
  Bandwidth                  125 kHz
  Coding Rate                4/5
  Potencia TX                14 dBm
  Preambul                   8 simbols
  CRC                        Activat
  Payload                    1 byte (fix)
  IQ Inversion               No
  Mode RX                    Continu (Rx(0))


================================================================================
5. RS485 / MODBUS (nomes receptor)
================================================================================

  Parametre                  Valor
  ---------                  -----
  Velocitat                  9600 baud, 8N1
  Funcio lectura             FC 03 (Read Holding Registers)
  Slave ID Deye              1 (configurable des del display)
  Interval lectura           60 segons

  NOTA: Deye i Sunsynk comparteixen el mateix mapa de registres.
  Referencia completa: projecte "kellerza/sunsynk" a GitHub.

  Registres disponibles (Holding Registers, FC 03):
  Registre   Descripcio                  Escala   Unitat   Nota
  --------   ----------                  ------   ------   ----
  103        Frequencia xarxa            x0.01    Hz
  154        Potencia PV total           x1       W
  160        Potencia xarxa              x1       W        + export / - import
  166        Potencia consum             x1       W
  168        Potencia bateria            x1       W        + descarrega / - carrega
  170        Temperatura bateria         x0.1     C
  172        Tensio bateria              x0.01    V
  154        Potencia PV total (EN US)   x1       W
  186        Potencia PV1                x1       W
  187        Potencia PV2                x1       W
  190        SOC bateria (EN US)         x1       %

  Xip RS485:                 MAX3485ESA (3.3V) o equivalent
  IMPORTANT:                 NO usar MAX485 (5V) - incompatible amb ESP32-S3
                             Alternatives: SP3485EN-L, SN65HVD75DR
  Resistencia terminacio:    120 ohm entre A i B (recomanat)

  Connexions ESP32 -> MAX3485:
  ESP32 GPIO19 (TX) ---------> MAX3485 DI  (Data In)
  ESP32 GPIO20 (RX) <--------- MAX3485 RO  (Read Out)
  ESP32 GPIO3  (DE/RE) ------> MAX3485 DE + RE (junts, via jumper)

  MAX3485 -> Bus RS485:
  MAX3485 A ---------> Connector J_485 pin A
  MAX3485 B ---------> Connector J_485 pin B
  GND       ---------> Connector J_485 pin GND

  NOTA TECNICA: Veure document "PROBLEMA_MAX485ED_SOLUCIO.md" per detalls
  sobre la incompatibilitat del MAX485ED (5V) amb l'ESP32-S3 (3.3V).


================================================================================
6. ALIMENTACIO
================================================================================

6.1 CADENA D'ALIMENTACIO
--------------------------------------------------------------------------------

  230VAC --> Fusible 250mA --> Varistor 275V --> HLK-PM01 --> 5VDC --> Heltec V3
            (F1)              (MOV1)            (AC/DC)       |        (reg. intern)
                                                              |           |
                                                         C1 (100uF)   3.3V GPIOs
                                                         C2 (100nF)

  Font recomanada: Hi-Link HLK-PM01
  - Entrada: 100-240VAC 50/60Hz
  - Sortida: 5VDC / 600mA (3W)
  - Aillament: 3000VAC
  - Dimensions: 34 x 20 x 15 mm
  - Muntatge: Through-hole

6.2 CONSUM ESTIMAT
--------------------------------------------------------------------------------
  Component                         Tipic        Maxim
  ---------                         -----        -----
  Heltec V3 (LoRa TX actiu)        ~120 mA      ~250 mA
  Heltec V3 (LoRa RX)              ~30 mA       ~50 mA
  Reles (cadascun)                  ~70 mA       -
  TOTAL (TX + 2 reles)             ~260 mA      ~390 mA

  La font HLK-PM01 (600 mA) es suficient.

6.3 PROTECCIONS 230VAC
--------------------------------------------------------------------------------
  Component    Valor              Funcio
  ---------    -----              ------
  F1           Fusible 250mA      Proteccio sobrecorrent
  MOV1         Varistor 275VAC    Proteccio sobretensio transitoria
  C1           100uF 16V elect.   Filtrat sortida
  C2           100nF ceramic      Desacoblament alta frequencia

6.4 ADAPTACIO DE TENSIONS D'ENTRADA (boia)
--------------------------------------------------------------------------------
  Voltatge boia    Solucio
  --------------   -------
  3.3V             Connexio directa (amb R proteccio)
  5V               Divisor de tensio (10k + 20k) o level shifter
  12V / 24V        Optoacoblador (PC817 o similar)
  Contacte sec     Pull-up a 3.3V amb R 10k

6.5 CARREGUES DE SORTIDA
--------------------------------------------------------------------------------
  Carrega            Solucio
  -------            -------
  LED indicador      Directe amb R limitadora
  Rele 5V            Transistor NPN (BC547) o MOSFET (2N7000)
  Rele 230V          Rele bobina 5V + transistor driver
  Contacte extern    Rele amb contacte lliure de potencial


================================================================================
7. CIRCUITS ELECTRONICS
================================================================================

7.1 CIRCUIT D'ENTRADES (x4)
--------------------------------------------------------------------------------

  Cada entrada te el mateix esquema:

                      R_proteccio (1k)
  Connector IN ---+---[  1k  ]---+--- GPIOx (Heltec)
                  |               |
                  +---[R_pulldown]-+
                  |   (10k a GND) |
                  |               |
                 GND            C_filter
                                (100nF)
                                  |
                                 GND

  Component       Valor      Funcio
  ---------       -----      ------
  R_proteccio     1 kohm     Limita corrent cap al GPIO
  R_pulldown      10 kohm    Defineix estat LOW sense senyal
  C_filter        100 nF     Filtra soroll / anti-rebots HW

7.2 CIRCUIT DE SORTIDES (x4)
--------------------------------------------------------------------------------

  Cada sortida amb driver per rele:

                      R_base (1k)          Rele bobina 5V
  GPIOx (Heltec) ---[ 1k ]---+---B        +------+------+
                              |   |  NPN   |      |      |
                              |   E        |   COM  NC   NO ---> Carrega
                              |   |        |      |
                             GND  +--------+      |
                                  |    C          |
                                  +-----|<|-------+ D1 (1N4007)
                                    (flyback)

  Component    Valor          Funcio
  ---------    -----          ------
  R_base       1 kohm         Limita corrent base transistor
  Q (NPN)      BC547/BC337    Driver de potencia per al rele
  D1           1N4007         Proteccio flyback (inductiu rele)
  Rele         5V bobina 10A  Commutacio carrega de potencia

  ALTERNATIVA sense rele: si la sortida nomes es senyal 3.3V,
  connexio directa amb R proteccio de 330 ohm.

7.3 CONNEXIO GPIO3 -> MAX485 DE/RE (hardwired)
--------------------------------------------------------------------------------

  GPIO3 (Heltec) ---------> MAX485 DE + RE (junts)

  PCB identica per emissor i receptor. GPIO3 connectat directament al MAX485
  sense jumper. El firmware gestiona el comportament per codi:

  Firmware       pinMode       Estat        Efecte
  --------       -------       -----        ------
  Emissor        OUTPUT        LOW fix      MAX485 no poblat, GPIO3 fixat per evitar floating
  Receptor       OUTPUT        LOW/HIGH     Gestiona direccio TX/RX Modbus


================================================================================
8. INICIALITZACIO OLED (IMPORTANT)
================================================================================

La llibreria Heltec (LoRaWan_APP.cpp) defineix l'objecte `display` globalment.
NO crear una nova instancia a l'sketch, usar `extern`:

  // CORRECTE:
  extern SSD1306Wire display;

  // INCORRECTE (causa "multiple definition" al linker):
  SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);

Ordre d'inicialitzacio al setup() (OBLIGATORI per brillo correcte):

  1. Mcu.begin();                          // Inicialitza hardware Heltec
  2. pinMode(Vext, OUTPUT);                // Activar alimentacio OLED
     digitalWrite(Vext, LOW);              // LOW = ON al Heltec V3
     delay(50);                            // Esperar estabilitzacio
  3. display.init();                       // Inicialitzar OLED
  4. display.flipScreenVertically();       // Girar pantalla 180 graus
  5. display.setContrast(255, 241, 64);    // Contrast, pre-charge, VCOMH al maxim

NOTA: Si display.init() es crida ABANS de Mcu.begin(), el brillo queda baix
perque Mcu.begin() pot reinicialitzar Vext i l'OLED. Sempre inicialitzar
Mcu.begin() primer, despres Vext manual, despres display.

Arduino IDE Board: WiFi LoRa 32(V3) / Wireless Shell(V3)  (paquet Heltec ESP32)

Contingut OLED emissor (4 linies):
  Linia 1 (font 16):  Estat TX ("EMISSOR OK", "ENVIANT...", "ENVIAT OK", "ERROR TX")
  Linia 2 (font 10):  Estat entrades: "IN: 1  0  1  0"
  Linia 3 (font 10):  Comptadors: "Enviats:42  Errors:0"
  Linia 4 (font 10):  Temps ultim TX: "Ultim TX: fa 3s"

Contingut OLED receptor (4 linies):
  Linia 1 (font 16):  Estat connexio ("RECEPTOR OK", "SENSE SENYAL", "REBENT...")
  Linia 2 (font 10):  Estat bomba: "O1:ACTIU 0h12m/2.0h BB:1" o "O1:ATURAT max:2.0h BB:1"
  Linia 3 (font 10):  Dades Deye + sortides: "SOC:85% 2400W O2:1 O3:0 O4:0" o "Deye:-- O2:1 O3:0 O4:0"
  Linia 4 (font 10):  Estat boies rebudes: "IN: 1  0  1  0"


================================================================================
9. DISSENY PCB
================================================================================

9.1 DIAGRAMA DE BLOCS
--------------------------------------------------------------------------------

  +========================================================================+
  |                       PCB UNIVERSAL TX/RX                              |
  |                                                                        |
  |  +----------+    +-----------+    +---------------------------+        |
  |  | 230VAC   |    | HLK-PM01  |    |     Heltec WiFi LoRa     |        |
  |  | INPUT    |--->| AC/DC     |--->|     32 V3 (modul)         |        |
  |  | + Fuse   |    | 5V/600mA  |    |                           |        |
  |  | + MOV    |    +-----------+    |  GPIO1 <--- IN1 (J_IN)    |        |
  |  +----------+         |           |  GPIO2 <--- IN2 (J_IN)    |        |
  |                       |           |  GPIO4 <--- IN3 (J_IN)    |        |
  |                  +----+----+      |  GPIO5 <--- IN4 (J_IN)    |        |
  |                  | Regulat.|      |                           |        |
  |                  |  intern |      |  GPIO6 ---> OUT1 (J_OUT)  |        |
  |                  | 5V->3.3V|      |  GPIO7 ---> OUT2 (J_OUT)  |        |
  |                  +---------+      |  GPIO47---> OUT3 (J_OUT)  |        |
  |                                   |  GPIO48---> OUT4 (J_OUT)  |        |
  |                                   |                           |        |
  |                                   |  [OLED 0.96"]  [SX1262]  |        |
  |                                   |  [USB-C]       [Antenna]  |        |
  |                                   +---------------------------+        |
  |                                                                        |
  |  +------------------+         +-------------------+                    |
  |  | ENTRADES (J_IN)  |         | SORTIDES (J_OUT)  |                    |
  |  | IN1 IN2 IN3 IN4  |         | OUT1 OUT2 OUT3 OUT4|                   |
  |  | GND GND GND GND  |         | GND  GND  GND  GND|                   |
  |  +------------------+         +-------------------+                    |
  |                                                                        |
  |                                      +------------------+               |
  |                                      |     MAX485       |               |
  |                                      | DI  <-- GPIO19   |               |
  |                                      | RO  --> GPIO20   |---> J_485     |
  |  GPIO3 hardwired ---------------->   | DE/RE <-- GPIO3  |  (A, B, GND) |
  |                                      +------------------+               |
  +========================================================================+

9.2 DIMENSIONS
--------------------------------------------------------------------------------
  Element              Dimensions
  -------              ----------
  Heltec V3 (modul)    50.2 x 25.5 mm
  HLK-PM01             34 x 20 x 15 mm
  PCB total (estimat)  80 x 60 mm

9.3 ZONES DE LA PCB
--------------------------------------------------------------------------------

  +----------------------------------+
  |  ZONA 230VAC    |  ZONA BAIXA    |
  |  (Fuse, MOV,    |  TENSIO        |
  |   HLK-PM01)     |  (Heltec,      |
  |                  |   connectors,  |
  |  >>> SLOT <<<    |   entrades,    |
  |  separacio       |   sortides,    |
  |  6mm minim       |   MAX485)      |
  +----------------------------------+

9.4 REGLES DE ROUTING
--------------------------------------------------------------------------------
  Parametre                          Valor
  ---------                          -----
  Amplada pista 230V                 Min 1 mm (recomanat 1.5 mm)
  Amplada pista 5V / 3.3V           Min 0.5 mm
  Amplada pista senyal               Min 0.25 mm
  Separacio 230V / baixa tensio      Min 6 mm (slot a la PCB)
  Clearance 230V (IEC 61010)        Min 6 mm
  Creepage 230V sobre FR4            Min 6 mm
  Pla de massa                       Capa inferior, zona baixa tensio

9.5 ANTENA LORA
--------------------------------------------------------------------------------
  - Zona lliure de coure al voltant del connector IPEX d'antena
  - L'antena ha de sortir per fora de la caixa (o caixa plastica)
  - No posar components metalics a prop de l'antena
  - Frequencia: 868 MHz (banda ISM Europa)

9.6 MUNTATGE DEL MODUL
--------------------------------------------------------------------------------
  - Headers femella 2.54mm (2x18 pins) per al Heltec V3
  - Modul enchufable: es pot extreure per programar via USB-C
  - Forats de muntatge M3 a les 4 cantonades
  - Serigrafia clara: noms connectors, polaritat, avís 230V

9.7 SEGURETAT
--------------------------------------------------------------------------------
  - Zona 230V clarament marcada a la serigrafia
  - Slot fisic de separacio a la PCB (minim 6 mm)
  - Caixa IP65 minim si esta a l'exterior
  - Connexio a terra (PE) obligatoria si caixa metalica


================================================================================
10. CONNECTORS
================================================================================

  Ref       Tipus                     Pins   Funcio
  ---       -----                     ----   ------
  J_AC      Bornes cargol 5.08mm      2      Entrada 230VAC (L, N)
  J_PE      Borne cargol 5.08mm       1      Terra proteccio (si cal)
  J_IN      Bornes cargol 3.5mm       8      4 entrades + 4 GND
  J_OUT     Bornes cargol 3.5mm       8      4 sortides + 4 GND
  J_485     Bornes cargol 3.5mm       3      RS485: A, B, GND
  J_ANT     IPEX 1.0 (del modul)      -      Antena LoRa
  J_USB     USB-C (del modul)         -      Programacio (accessible)


================================================================================
11. BOM - LLISTA DE MATERIALS (per cada PCB, x2 total)
================================================================================

10.1 COMPONENTS PRINCIPALS
--------------------------------------------------------------------------------
  Ref    Component                  Valor/Model      Qty  Footprint
  ---    ---------                  -----------      ---  ---------
  U1     Heltec WiFi LoRa 32 V3    HTIT-WB32LA      1    2x18 pin header
  PS1    Font AC/DC                 HLK-PM01         1    Through-hole
  U2     Transceiver RS485          MAX485           1    DIP-8 / SOP-8

10.2 PROTECCIONS (zona 230VAC)
--------------------------------------------------------------------------------
  Ref    Component                  Valor            Qty  Footprint
  ---    ---------                  -----            ---  ---------
  F1     Fusible                    250mA 250V       1    5x20mm PCB
  MOV1   Varistor                   275VAC (14D431)  1    Radial 7.5mm

10.3 CONDENSADORS
--------------------------------------------------------------------------------
  Ref    Component                  Valor            Qty  Footprint
  ---    ---------                  -----            ---  ---------
  C1     Electrolitic               100uF 16V        1    Radial D6.3
  C2     Ceramic                    100nF 50V        1    0805 SMD
  C3-6   Ceramic (filtres entrades) 100nF 50V        4    0805 SMD

10.4 RESISTENCIES
--------------------------------------------------------------------------------
  Ref    Component                  Valor            Qty  Footprint
  ---    ---------                  -----            ---  ---------
  R1-4   Proteccio entrada          1k ohm           4    0805 SMD
  R5-8   Pull-down entrades         10k ohm          4    0805 SMD
  R9-12  Base transistor sortida    1k ohm           4    0805 SMD

10.5 TRANSISTORS I DIODES
--------------------------------------------------------------------------------
  Ref    Component                  Valor            Qty  Footprint
  ---    ---------                  -----            ---  ---------
  Q1-4   Transistor NPN             BC547 / BC337    4    TO-92
  D1-4   Diode rectificador         1N4007           4    DO-41

10.6 RELES (opcional)
--------------------------------------------------------------------------------
  Ref    Component                  Valor            Qty  Footprint
  ---    ---------                  -----            ---  ---------
  K1-4   Rele                       5V bob., 10A     4    SRD-05VDC-SL-C

10.6b COMPONENTS RECEPTOR (adicionals)
--------------------------------------------------------------------------------
  Ref    Component                  Valor/Model      Qty  Nota
  ---    ---------                  -----------      ---  ---------
  RV1    Potenciometre lineal        500 ohm          1    Durada max bomba (GPIO2, pull-down 4.7k)
  SW1    Switch mitja carrega        SPST             1    Parada bomba condicionada (GPIO1, pull-down)

10.7 CONNECTORS
--------------------------------------------------------------------------------
  Ref    Component                  Valor            Qty  Footprint
  ---    ---------                  -----            ---  ---------
  J_AC   Bornes cargol              2 pins 5.08mm    1    PCB THT
  J_IN   Bornes cargol              8 pins 3.5mm     1    PCB THT
  J_OUT  Bornes cargol              8 pins 3.5mm     1    PCB THT
  J_485  Bornes cargol              3 pins 3.5mm     1    PCB THT
  H1-2   Tira pins femella          1x18 pins 2.54mm 2    Through-hole

10.8 MECANICS
--------------------------------------------------------------------------------
  Component                   Valor          Qty  Nota
  ---------                   -----          ---  ----
  Forats muntatge             M3             4    Cantonades PCB
  Separadors PCB              M3 x 10mm     4    Muntatge a caixa
  Antena LoRa 868MHz          IPEX 1.0      1    Inclosa amb Heltec (verificar)

10.10 COST ESTIMAT (per PCB)
--------------------------------------------------------------------------------
  Grup                         Cost
  ----                         ----
  Heltec V3 modul              ~18 EUR
  HLK-PM01                     ~3 EUR
  MAX485                       ~1 EUR
  Components passius (R, C)    ~1 EUR
  Transistors + diodes         ~1 EUR
  Reles (x4)                   ~4 EUR
  Connectors                   ~3 EUR
  PCB fabricacio               ~5 EUR
  -----------------------------------
  TOTAL PER PLACA              ~36 EUR
  TOTAL PROJECTE (x2)         ~72 EUR


================================================================================
12. TEMPORITZADORS I COMPORTAMENT
================================================================================

  Parametre                       Valor         Nota
  ---------                       -----         ----
  Interval TX (emissor)           15 s          Enviament periodic
  Polling entrades (emissor)      50 ms         Detecta canvis rapids
  Timeout RX (receptor)           150 s         Safety shutdown si no rep (2.5 min, tolera 1 paquet perdut amb TX cada 60s)
  Interval lectura Deye           60 s          Modbus cada minut
  TX immediat per canvi d'estat   Si            Envia al detectar canvi

  Comportament de seguretat:
  - Si el receptor no rep paquets en 150 segons -> TOTES les sortides OFF
  - L'emissor envia immediatament quan detecta un canvi + periodic cada 15s
  - Si un TX falla (timeout 3s), l'emissor reintenta immediatament al proper cicle (50ms)
  - Errors CRC es compten pero no activen sortides
  - Radio.IrqProcess() es crida entre les dues lectures Modbus per evitar perdua de paquets LoRa durant el bloqueig RS485

  Logica de control bomba (OUT1) - VERSIO FAIL-SAFE:

  Cablejat boies: contacte NC (Normally Closed) per IN1/IN2/IN3 (emissor)
                  contacte NO (Normally Open) per boiaBomba (receptor local)

  Taula de senyals amb NC (boies emissor IN1/IN2/IN3):
  - Boia avall (falta aigua) = NC tancat = pin HIGH = 1
  - Boia amunt (hi ha aigua) = NC obert  = pin LOW  = 0
  - Boia desconnectada/trencada          = pin LOW  = 0 (fail-safe: bomba OFF)

  Taula de senyals amb NO (boiaBomba receptor local):
  - Boia amunt (hi ha aigua) = NO tancat = pin HIGH = 1
  - Boia avall (sec)         = NO obert  = pin LOW  = 0
  - Boia desconnectada/trencada          = pin LOW  = 0 (fail-safe: para bomba)

  Arrenca si TOTES les condicions es compleixen:
  - IN1 LoRa = 1 (falta aigua al diposit desti, NC tancat)
  - Boia bomba = 1 (diposit bomba amb aigua, NO tancat)
  - SOC >= 30% (si dades Deye disponibles)

  Para si QUALSEVOL condicio es compleix:
  - IN1 LoRa = 0 (diposit desti te aigua O boia desconnectada, fail-safe)
  - Boia bomba = 0 (proteccio marxa en sec O boia desconnectada, fail-safe)
  - SOC <= 20% (bateria baixa, histeresi 20-30%)
  - Switch mitja carrega = 1 I IN2 LoRa = 0 (nivell intermig assolit O boia desconnectada)
  - Potenciometre: temps maxim assolit (0-4h)
  - Timeout LoRa: 150s sense rebre paquet (safety shutdown total)

  Prioritat d'avaluacio (de mes a menys prioritat):
  1. Boia bomba = 0 -> para (marxa en sec / desconnectada)
  2. Switch mitja carrega + IN2 LoRa = 0 -> para
  3. IN1 = 1 + SOC OK -> arrenca (falta aigua)
  4. IN1 = 1 + SOC insuficient -> para
  5. IN1 = 0 -> para (diposit ple / boia desconnectada)

  Histeresi SOC:
  - Arrancar: SOC >= 30% (DEYE_SOC_START)
  - Parar: SOC <= 20% (DEYE_SOC_STOP)
  - Entre 20-30%: mante estat actual

  Dades Deye no disponibles (SOC = -1):
  - SOC no afecta: la bomba funciona normalment amb la resta de condicions
  - Nomes es bloqueja l'arrencada/aturada per SOC quan hi ha lectura valida

  updateOutputs() es reavalua despres de cada lectura Modbus (cada 60s) a mes de
  en cada recepcio de paquet LoRa, garantint resposta a canvis de SOC sense paquets

  Durada maxima bomba (potenciometre GPIO2 receptor):
  - Rang:    0 a 240 minuts (0 a 4 hores)
  - ADC:     12 bits (0-4095), mitjana 8 lectures
  - Valor visible al display OLED en temps real
  - Si pot = 0 min: bomba s'atura immediatament si esta activa
  - Potenciometre 500 ohm (pull-down 4.7k a PCB, error max 2.6%)

  ATENCIO - Potenciometre provisional (fins que arribi el component):
  - Sense potenciometre: GPIO2 queda a GND (pull-down 4.7k) -> ADC=0 -> bomba no funciona!
  - Solucio temporal: connectar un cable del pin POT (J_IN GPIO2) a 3.3V
  - Efecte: ADC llegeix maxim -> durada maxima 240 minuts (sense limitacio practica)


================================================================================
13. RESUM CONNEXIONS ESP32 <-> MAX485
================================================================================

  ESP32 (Heltec V3)          MAX485              Bus RS485
  ------------------         ------              ---------
  GPIO19 (TX)  ----------->  DI (pin 4)
  GPIO20 (RX)  <-----------  RO (pin 1)
  GPIO3 (hardwired) ------->  DE (pin 3) + RE (pin 2)   [junts]
                              A  (pin 6)  ----------->  J_485 A
                              B  (pin 7)  ----------->  J_485 B
  GND          ----------->  GND (pin 5)  --------->  J_485 GND
  5V           ----------->  VCC (pin 8)

  Direccio MAX485:
  - DE/RE = LOW  -> Mode recepcio (RO actiu, escolta el bus)
  - DE/RE = HIGH -> Mode transmissio (DI actiu, envia al bus)


================================================================================
14. NOTES PER AL DISSENY PCB (checklist)
================================================================================

  [ ] Slot fisic de separacio 230V / baixa tensio (min 6 mm)
  [ ] Pistes 230V: min 1 mm amplada (recomanat 1.5 mm)
  [ ] Pistes alimentacio: min 0.5 mm
  [ ] Pistes senyal: min 0.25 mm
  [ ] Pla de massa a capa inferior (zona baixa tensio)
  [ ] Zona lliure de coure al voltant del connector d'antena
  [ ] Headers femella 2x18 pins per al modul Heltec
  [ ] Forats M3 a les 4 cantonades
  [ ] Serigrafia: noms connectors, polaritat, avis 230V
  [ ] J_485 connector per RS485 (A, B, GND)
  [ ] GPIO3 hardwired a MAX485 DE/RE (sense jumper)
  [ ] MAX485 a prop dels pins GPIO19/20/3
  [ ] USB-C del Heltec accessible (forat/ranura a la caixa)
  [ ] Antena LoRa: sortida per fora de la caixa
  [ ] Verificar que GPIO21 NO esta connectat a res extern


================================================================================
  FI DEL DOCUMENT
================================================================================
