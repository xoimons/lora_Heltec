# Assignacio de Pins GPIO - Heltec WiFi LoRa 32 V3

## GPIOs Segurs (recomanats pel fabricant)

Segons la documentacio oficial de Heltec, els seguents GPIOs son segurs per us extern:

| GPIO | Funcions disponibles         | Nota                              |
|------|------------------------------|-----------------------------------|
| 1    | Digital I/O, ADC1, Touch     | Segur                             |
| 2    | Digital I/O, ADC1, Touch     | Segur                             |
| 4    | Digital I/O, ADC1, Touch     | Segur                             |
| 5    | Digital I/O, ADC1, Touch     | Segur                             |
| 6    | Digital I/O, ADC1, Touch     | Segur                             |
| 7    | Digital I/O, ADC1, Touch     | Segur                             |
| 19   | Digital I/O (USB D-)         | Segur si no es fa servir USB      |
| 20   | Digital I/O (USB D+)         | Segur si no es fa servir USB      |
| 47   | Digital I/O                  | Segur, sense connexio interna     |
| 48   | Digital I/O                  | Segur, sense connexio interna     |

## GPIOs a EVITAR

| GPIO      | Motiu                                          |
|-----------|-------------------------------------------------|
| 0, 46     | Strapping pins (boot mode)                      |
| 33-38     | SPI Flash intern                                |
| 21        | Reset OLED (usat internament)                   |
| 26        | SubSPI chip-select                              |
| 43, 44    | USB serial download                             |
| 39-42     | JTAG (no recomanat per principiants)            |

## Assignacio Proposada per la PCB

La PCB te 4 entrades i 4 sortides. El firmware determinara quines s'utilitzen.

### Entrades (INPUT amb pull-down extern o intern)

| Funcio       | GPIO | Tipus         | Nivell logic | Nota                        |
|--------------|------|---------------|--------------|-----------------------------|
| Entrada 1    | 1    | Digital INPUT | 3.3V         | Boia senyal 1 (emissor)     |
| Entrada 2    | 2    | Digital INPUT | 3.3V         | Boia senyal 2 (emissor)     |
| Entrada 3    | 4    | Digital INPUT | 3.3V         | Reserva                     |
| Entrada 4    | 5    | Digital INPUT | 3.3V         | Reserva                     |

### Sortides (OUTPUT)

| Funcio       | GPIO | Tipus          | Nivell logic | Nota                        |
|--------------|------|----------------|--------------|-----------------------------|
| Sortida 1    | 6    | Digital OUTPUT | 3.3V         | Activacio receptor          |
| Sortida 2    | 7    | Digital OUTPUT | 3.3V         | Activacio receptor          |
| Sortida 3    | 47   | Digital OUTPUT | 3.3V         | Reserva                     |
| Sortida 4    | 48   | Digital OUTPUT | 3.3V         | Reserva                     |

## Notes Importants

- **Tots els GPIOs de l'ESP32-S3 treballen a 3.3V.** No connectar mai 5V directament a un GPIO.
- **Corrent maxim per GPIO:** 40 mA (recomanat 20 mA maxim per pin).
- **Corrent total maxim** de tots els GPIOs: ~1200 mA (limitat pel regulador).
- Si les sortides han d'activar carregues superiors a 20 mA (reles, solenoide, etc.), cal un driver (transistor MOSFET o ULN2003/ULN2803).
- Les entrades de la boia han d'arribar a 3.3V. Si la boia dona un altre nivell de tensio, cal un divisor de tensio o un optoacoblador.

## Diagrama de Connexio (esquematic)

```
                    EMISSOR (TX)                              RECEPTOR (RX)
                +------------------+                      +------------------+
                |  Heltec V3 LoRa  |                      |  Heltec V3 LoRa  |
                |                  |                      |                  |
  Boia IN1 --->| GPIO1 (INPUT)    |  ~~~~ LoRa ~~~~>     | GPIO6  (OUTPUT)  |---> Rele/Actuador 1
  Boia IN2 --->| GPIO2 (INPUT)    |  ~~~~ 868MHz ~~~>    | GPIO7  (OUTPUT)  |---> Rele/Actuador 2
  (Reserva)--->| GPIO4 (INPUT)    |                      | GPIO47 (OUTPUT)  |---> (Reserva)
  (Reserva)--->| GPIO5 (INPUT)    |                      | GPIO48 (OUTPUT)  |---> (Reserva)
                |                  |                      |                  |
                +------------------+                      +------------------+
```
