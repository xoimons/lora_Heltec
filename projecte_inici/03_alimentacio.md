# Disseny d'Alimentacio - 230VAC a 3.3V/5V

## Cadena d'Alimentacio

```
230VAC (xarxa) --> Font AC/DC --> 5VDC --> Heltec V3 (regulador intern) --> 3.3V (GPIOs)
```

## Opcio 1: Font commutada encapsulada (RECOMANADA)

Modul AC/DC compacte integrat a la PCB o extern.

### Component recomanat: Hi-Link HLK-PM01 (o equivalent)

| Parametre          | Valor                |
|--------------------|----------------------|
| Entrada            | 100-240VAC 50/60Hz   |
| Sortida            | 5VDC                 |
| Corrent maxim      | 600 mA (3W)          |
| Aïllament          | 3000VAC              |
| Dimensions         | 34 x 20 x 15 mm     |
| Muntatge           | Through-hole PCB     |

**Per que 5V i no 3.3V directament?**
- El Heltec V3 te un regulador intern de 5V a 3.3V (via USB o pin Vin).
- Alimentant a 5V pel pin 5V, el modul funciona correctament amb totes les proteccions internes.
- Alternativa: HLK-PM03 (3.3V directe) si alimentem pel pin 3.3V, pero perdem les proteccions internes del Heltec.

## Opcio 2: Font externa (transformador de paret)

Transformador de paret 230VAC -> 5VDC (tipus carregador USB) amb connector a la PCB.
- Mes senzill (no hi ha 230V a la PCB)
- Menys compacte
- Connector barrel jack o USB-C a la placa

## Esquema de Proteccions (Opcio 1 - 230V a la PCB)

```
230VAC ---|FUSE 250mA|---+---|Varistor 275V|---+--- HLK-PM01 ---+--- 5VDC
          (F1)           |   (MOV1)            |                 |
                         +---[GND]             +---[GND]         +-- C1 (100uF elect.)
                                                                 +-- C2 (100nF ceramic)
                                                                 |
                                                                 +--- Pin 5V Heltec
```

### Components de proteccio (obligatoris amb 230V a la PCB)

| Component | Valor           | Funcio                          |
|-----------|-----------------|----------------------------------|
| F1        | Fusible 250mA   | Proteccio sobrecorrent            |
| MOV1      | Varistor 275VAC | Proteccio sobretensio transitoria |
| C1        | 100uF 16V elect.| Filtrat sortida                   |
| C2        | 100nF ceramic   | Desacoblament alta frequencia     |

## Consum Estimat

| Component                  | Consum tipic   | Consum maxim   |
|----------------------------|----------------|----------------|
| Heltec V3 (LoRa TX actiu) | ~120 mA        | ~250 mA        |
| Heltec V3 (LoRa RX)       | ~30 mA         | ~50 mA         |
| Heltec V3 (deep sleep)    | ~15 uA         | -              |
| Reles (si n'hi ha)         | ~70 mA cadascun| -              |
| **Total estimat (TX + 2 reles)** | **~260 mA** | **~390 mA** |

La font HLK-PM01 (600 mA) es suficient per a qualsevol configuracio.

## IMPORTANT: Seguretat amb 230VAC

- **Clearance minim** entre pistes 230V i baixa tensio: **6 mm** (IEC 61010)
- **Creepage minim**: **6 mm** sobre PCB FR4
- **Slot de separacio** recomanat a la PCB entre zona 230V i zona baixa tensio
- Zona 230V ha d'estar **clarament marcada** a la serigrafia de la PCB
- **Caixa IP65** com a minim si esta a l'exterior (boia)
- **Connexio a terra** (PE) obligatoria si caixa metalica

## Alimentacio de les Entrades (Boia)

Les entrades de la boia arriben a 3.3V. Si el senyal de la boia es a un altre voltatge:

| Voltatge boia | Solucio                                        |
|---------------|------------------------------------------------|
| 3.3V          | Connexio directa (amb resistencia de proteccio)|
| 5V            | Divisor de tensio (10k + 20k) o level shifter |
| 12V/24V       | Optoacoblador (PC817 o similar)                |
| Contacte sec  | Pull-up a 3.3V amb resistencia 10k             |

## Alimentacio de les Sortides (Receptor)

Les sortides GPIO son 3.3V / 20mA max. Per activar carregues:

| Carrega            | Solucio                                         |
|--------------------|-------------------------------------------------|
| LED indicador      | Directe amb resistencia limitadora               |
| Rele 5V            | Transistor NPN (BC547) o MOSFET (2N7000)         |
| Rele 230V          | Rele amb bobina 5V + transistor driver            |
| Contacte extern    | Rele amb contacte lliure de potencial             |
