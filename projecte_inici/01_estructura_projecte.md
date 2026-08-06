# Estructura del Projecte - Sistema LoRa Boia

## Visio General

Sistema de comunicacio LoRa punt a punt compost per:
- **Emissor (TX):** Placa PCB amb Heltec WiFi LoRa 32 V3 que llegeix 2 entrades digitals (3.3V) d'una boia i les transmet via LoRa.
- **Receptor (RX):** Placa PCB identica amb Heltec WiFi LoRa 32 V3 que rep les dades LoRa i activa sortides en funcio de les entrades rebudes.

Ambdues plaques utilitzen el **mateix disseny de PCB** (4 entrades + 4 sortides), nomes canvia el firmware.

## Xip Principal

- **Model:** Heltec WiFi LoRa 32 V3 (HTIT-WB32LA)
- **MCU:** ESP32-S3FN8 (dual-core 240 MHz)
- **LoRa:** SX1262
- **Alimentacio del modul:** 3.3V (regulador intern des de 5V USB o bateria 3.7V)

## Estructura de Carpetes

```
lora_Heltec/
|
+-- heltec/                          # Datasheets i esquematics del fabricant
|   +-- HTIT-WB32LA(F)_V3.1_Schematic_Diagram.pdf
|   +-- HTIT-WB32LA_V3.2.pdf
|
+-- projecte_inici/                  # Documentacio inicial del projecte
|   +-- 01_estructura_projecte.md    # Aquest document
|   +-- 02_pinatge_gpio.md           # Assignacio de pins GPIO
|   +-- 03_alimentacio.md            # Disseny d'alimentacio 230VAC -> 3.3V
|   +-- 04_esquema_pcb.md            # Esquema de blocs i consideracions PCB
|   +-- 05_bom_components.md         # Llista de materials
|
+-- hardware/                        # (futur) Fitxers KiCad / EasyEDA
|   +-- esquematic/
|   +-- pcb/
|   +-- gerbers/
|
+-- firmware/                        # (futur) Codi Arduino/PlatformIO
|   +-- emissor/
|   +-- receptor/
```
