# PROPOSTA ARQUITECTONICA: CENTRALITZACIO DE DIPOSITS COMARCALS

## Sistema de Monitoratge i Control de Diposits d'Aigua

**Data:** Agost 2026
**Estat:** Fase de brainstorming / arquitectura inicial
**Abast:** Monitoratge centralitzat de tots els diposits d'aigua d'una comarca (Catalunya)

---

## 0. ANALISI DEL SISTEMA ACTUAL

### Que tenim avui

- **Hardware:** Heltec WiFi LoRa 32 V3 (ESP32-S3 + SX1262) sobre PCB universal dissenyada a mida (80x60mm)
- **Comunicacio LoRa:** Punt a punt, 868 MHz, SF7, BW125, 14 dBm, payload 1 byte (4 bits = 4 entrades)
- **Sensors:** Interruptors de flotador (boies) amb contactes NC (fail-safe), binaris (aigua si/no)
- **Control:** Logica de bomba amb multiples condicions de seguretat (boia, temporitzador, timeout LoRa 150s)
- **MQTT:** HiveMQ Cloud (pla gratuit, TLS obligatori, 10GB/mes), topics sota `boia/`
- **Dashboard:** HTML + MQTT.js amb visualitzacions SVG animades (diposit, bomba, antena LoRa, LEDs)
- **Connectivitat:** WiFi a modem 4G per la connexio MQTT del receptor

### Limitacions actuals per a escalat

1. **LoRa punt a punt:** Una parella emissor-receptor. No escalable a desenes de nodes
2. **Payload 1 byte:** Nomes 4 bits d'informacio. Insuficient per a dades continues (nivell %, temperatura)
3. **Sense identificador de node:** El protocol actual no distingeix entre emissors
4. **Dashboard per un sol diposit:** L'HTML actual mostra un unic diposit amb 4 entrades
5. **Sense comunicacio bidireccional:** L'emissor nomes envia; no pot rebre ordres
6. **HiveMQ gratuit:** 10GB/mes, 100 connexions. Amb 30+ nodes publicant cada 30s, es podria excedir
7. **Sense historics:** Les dades es mostren en temps real pero no es guarden

---

## 1. TECNOLOGIES DE COMUNICACIO

### 1.1 LoRaWAN (amb gateway)

Protocol estandard sobre LoRa amb gateways que reenvien dades a un Network Server. El SX1262 del Heltec V3 ja suporta LoRaWAN nativament.

**Opcions de Network Server:**
- **TTN (The Things Network):** Gratuit, comunitat global, Fair Use Policy (30s airtime/dia/device)
- **ChirpStack:** Open source, self-hosted (Raspberry Pi o VPS), sense limits
- **Helium/Nova Labs:** Xarxa descentralitzada, cobertura variable, cost per missatge

| Parametre | Valor |
|---|---|
| Cost per node | ~18 EUR (el Heltec V3 ja serveix, nomes cal canviar firmware) |
| Cost gateway | 150 EUR (RAK7268, interior) a 400 EUR (Kerlink, exterior IP67) |
| Abast | 5-15 km tipic, fins a 50 km en condicions ideals (altitud) |
| Consum | 30-50 mA RX, pico 120 mA TX, deep sleep <10 uA |
| Escalabilitat | Centenars de nodes per gateway |
| Complexitat | Mitjana-alta (configuracio inicial), baixa un cop en marxa |

**Pros:**
- Protocol estandarditzat (LoRa Alliance), gran ecosistema
- Un sol gateway cobreix 5-15 km en zona rural
- Gestio de dispositius integrada (join, keys, adaptive data rate)
- Comunicacio bidireccional nativa (Class A/C)
- Seguretat AES-128 integrada al protocol
- El Heltec V3 ja te el xip compatible sense canviar hardware

**Contres:**
- Cal comprar/instal.lar gateways (150-400 EUR cadascun)
- Fair Use Policy de TTN limita la frequencia d'enviament
- Mes complexitat de configuracio inicial (DevEUI, AppKey, join procedure)
- En terreny muntanyos, un sol gateway pot ser insuficient

### 1.2 LoRa punt a punt amb gateways personalitzats

Mantenir el protocol actual pero afegir un sistema de concentradors que recullen dades de multiples emissors propers i les reenvien via WiFi/4G.

```
Emissor A ---+
Emissor B ---+--LoRa--> Gateway local (Heltec V3 + WiFi/4G) --> MQTT Cloud
Emissor C ---+
```

| Parametre | Valor |
|---|---|
| Cost per node | ~18 EUR (emissor) + ~22 EUR (gateway) |
| Abast | 2-5 km (actual), escalable amb gateways intermedis |
| Consum | Identic a l'actual (~120 mA TX, ~30 mA RX) |
| Escalabilitat | Limitada (5-10 nodes per gateway sense gestio de col.lisions) |
| Complexitat | Baixa (evolucio natural del sistema actual) |

**Pros:**
- Reutilitza el firmware emissor actual amb minimes modificacions
- Senzill d'implementar, control total sobre el protocol
- Baix cost

**Contres:**
- Protocol propietari, no estandard
- Cal gestionar manualment col.lisions amb multiples emissors
- Sense frequency hopping ni adaptive data rate
- Escala limitada

### 1.3 LoRa Mesh (Meshtastic o mesh personalitzat)

Xarxa mallada on cada node pot retransmetre missatges d'altres nodes.

| Parametre | Valor |
|---|---|
| Cost per node | ~18 EUR (Heltec V3) |
| Abast | 10-50+ km amb multiples salts |
| Consum | Alt (~30 mA continu en RX, no permet deep sleep eficient) |
| Escalabilitat | Moderada (20-50 nodes, degradacio amb mes) |
| Complexitat | Baixa amb Meshtastic (instal.lar firmware), Alta si es mesh propi |

**Pros:**
- Auto-healing: si un node cau, els altres redirigeixen
- Abast extens amb multiples salts
- Meshtastic: ja funciona al Heltec V3 sense desenvolupament

**Contres:**
- Latencia creixent amb cada salt (30-60s amb 5 salts)
- Consum elevat (cada node ha d'estar sempre escoltant)
- Meshtastic: no dissenyat per SCADA/control industrial
- Throughput molt baix

### 1.4 4G/LTE per node

Cada node te el seu propi modem cellular i envia dades directament al cloud.

| Parametre | Valor |
|---|---|
| Cost per node | ~40-55 EUR (ESP32 + SIM7600 + antena) + 2-5 EUR/mes per SIM |
| Abast | Il.limitat (cobertura cellular) |
| Consum | 100-300 mA actiu, ~1-5 mA sleep |
| Escalabilitat | Il.limitada |
| Complexitat | Mitjana (gestio SIM, cobertura, firmware) |

**Pros:**
- Comunicacio bidireccional immediata
- Latencia molt baixa (<1s)
- Permet payloads grans, OTA

**Contres:**
- Cost recurrent per SIM (2-5 EUR/mes)
- Consum alt, dificil amb solar
- Zones rurals/muntanyoses poden no tenir cobertura 4G
- Dependencia d'un operador

### 1.5 NB-IoT (Narrowband IoT)

Tecnologia cellular de baixa potencia per IoT. Opera sobre xarxes d'operadors.

| Parametre | Valor |
|---|---|
| Cost per node | ~35-45 EUR + 1-3 EUR/mes SIM |
| Abast | 10-15 km des de l'antena de l'operador |
| Consum | Molt baix (comparable a LoRa) |
| Escalabilitat | Bona |
| Complexitat | Mitjana |

**Contres principals:** Cobertura NB-IoT molt limitada a zones rurals de Catalunya.

### 1.6 Sigfox

Xarxa LPWAN propietaria global. Cobertura a Espanya via Cellnex.

| Parametre | Valor |
|---|---|
| Cost per node | ~15-25 EUR + ~1-2 EUR/any subscripcio |
| Abast | 10-50 km (xarxa Cellnex) |
| Consum | Ultra baix |
| Escalabilitat | Bona |
| Complexitat | Molt baixa |

**Contres principals:** 140 missatges/dia maxim, payload 12 bytes, quasi unidireccional. Futur incert.

### 1.7 WiFi Mesh

**Descartat.** Abast 50-100m, necessita infraestructura electrica i WiFi. Inviable per diposits remots.

### 1.8 Hibrida: Clusters LoRaWAN + Backhaul 4G (RECOMANADA)

Combinar LoRa (baix cost, baix consum, abast rural) amb 4G (bidireccional, abast il.limitat).

```
Zona A (vall):                    Zona B (muntanya):
  Node 1 --+                        Node 5 --+
  Node 2 --+--LoRaWAN--> GW-A      Node 6 --+--LoRaWAN--> GW-B
  Node 3 --+     (4G backhaul)      Node 7 --+     (4G backhaul)
  Node 4 --+         |                                |
                     +-------> Cloud/Server <---------+
                              (ChirpStack + Grafana)
```

| Parametre | Valor |
|---|---|
| Cost per node emissor | ~18-25 EUR |
| Cost per gateway | 200-500 EUR (gateway LoRaWAN + modem 4G + solar) |
| Abast per cluster | 5-15 km al voltant de cada gateway |
| Consum nodes | Molt baix (deep sleep entre enviaments) |
| Escalabilitat | Excel.lent |
| Complexitat | Mitjana-alta (configuracio inicial), baixa en marxa |

**Pros:**
- Millor relacio cost/cobertura
- Nodes emissors molt barats i de baix consum (solar viable)
- Comunicacio bidireccional via LoRaWAN Class C al gateway
- Escalable: afegir nodes es trivial

### TAULA COMPARATIVA

| Tecnologia | Cost/node | Cost recurrent | Abast | Consum | Bidireccional | Escalabilitat |
|---|---|---|---|---|---|---|
| **LoRaWAN + GW** | 18 EUR + GW 200-400 | 5 EUR/mes SIM GW | 5-15 km/GW | Molt baix | Si | Excel.lent |
| LoRa P2P + GW | 18 EUR + GW 22 EUR | SIM si 4G | 2-5 km | Baix | No natiu | Limitada |
| LoRa Mesh | 18 EUR | Cap | 10-50 km | Alt | Limitada | Moderada |
| 4G per node | 40-55 EUR | 2-5 EUR/mes | Il.limitat | Alt | Si | Il.limitada |
| NB-IoT | 35-45 EUR | 1-3 EUR/mes | Variable | Molt baix | Si | Bona |
| Sigfox | 15-25 EUR | 1-2 EUR/any | 10-50 km | Ultra baix | Quasi no | Bona |
| **Hibrida LoRaWAN+4G** | 18-25 EUR | 5 EUR/mes per GW | 5-15 km/GW | Molt baix | Si | Excel.lent |

---

## 2. TECNOLOGIES DE SENSORS

### 2.1 Interruptors de flotador (boies) - ACTUAL

| Parametre | Valor |
|---|---|
| Precisio | Punt unic (nivell fix on esta la boia) |
| Cost | 3-10 EUR per boia |
| Manteniment | Baix, susceptible a calcareo/bruticia |
| Durabilitat | 5-10 anys (parts mecaniques) |
| Consum | Zero (contacte sec passiu) |
| Senyal | Digital (1 bit per boia) |

Amb 3 boies (minim, intermig, maxim) es pot aproximar el nivell per trams. Per molts diposits rurals, pot ser suficient.

### 2.2 Sensor ultrasonic (JSN-SR04T, MaxBotix MB7389)

Mesura la distancia entre el sensor (muntat al sostre del diposit) i la superficie de l'aigua.

| Parametre | Valor |
|---|---|
| Precisio | +/- 1-3 cm (JSN-SR04T), +/- 1 mm (MaxBotix) |
| Cost | 5-8 EUR (JSN-SR04T), 50-100 EUR (MaxBotix MB7389) |
| Manteniment | Baix (sense contacte amb l'aigua) |
| Durabilitat | Bona si muntat correctament (IP67 transductor) |
| Consum | 5-30 mA durant mesura (ms), 0 en sleep |
| Senyal | Analog/PWM/UART |

Rang tipic: 20 cm - 4.5 m. Afectat per condensacio, vapors, espuma.

### 2.3 Transductor de pressio hidrostatica (submergible, 4-20mA)

Sensor submergible que mesura la pressio de la columna d'aigua. Estandard industrial.

| Parametre | Valor |
|---|---|
| Precisio | +/- 0.25% a 0.5% del fons d'escala |
| Cost | 25-80 EUR (industrial barat), 150-500 EUR (alta precisio) |
| Manteniment | Neteja periodica del diafragma (cada 6-12 mesos) |
| Durabilitat | Excel.lent (acer inoxidable, 10-20 anys) |
| Consum | 20 mA (bucle 4-20mA, alimentacio 12-24V) |
| Senyal | 4-20mA (resistencia shunt 150-250 ohms per lectura ADC) |

Per connectar a ESP32: resistencia de 150-250 ohms entre fils del bucle 4-20mA, genera 0.6-5V. Divisor de tensio a 0-3.3V. Alternativa: ADS1115 (ADC 16 bits I2C).

### 2.4 Sensor de nivell capacitiu

| Parametre | Valor |
|---|---|
| Precisio | +/- 1-3% (basic), +/- 0.5% (industrial) |
| Cost | 30-150 EUR |
| Manteniment | Molt baix (sense parts mobils) |
| Durabilitat | Excel.lent |
| Consum | 10-30 mA |

Calibracio dependent del liquid, afectat per incrustacions.

### 2.5 Radar de nivell

| Parametre | Valor |
|---|---|
| Precisio | +/- 2-5 mm |
| Cost | 200-2000 EUR |
| Manteniment | Quasi zero |
| Durabilitat | 20+ anys |

Maxima fiabilitat, no afectat per temperatura/vapors/espuma. Nomes justificat per diposits critics o de gran capacitat (>100 m3).

### 2.6 Camera + IA

Interessant com a complement (inspecció visual remota) pero no com a sensor primari. Alt consum, complexitat IA, no fiable per mesura precisa.

### 2.7 ToF Laser (VL53L0X, VL53L1X)

Molt barat (3-8 EUR), precis (+/- 3-5 mm), baix consum. Pero rang maxim 2-4m i afectat per superficie especular de l'aigua.

### RECOMANACIO DE SENSORS

| Escenari | Sensor recomanat | Cost | Justificacio |
|---|---|---|---|
| **Tier 1 (minim)** | Boies NC (3 per diposit) | 10-20 EUR | Ja funciona, fail-safe, zero consum |
| **Tier 2 (professional)** | Ultrasonic (JSN-SR04T) + 1 boia seguretat | 15-25 EUR | Nivell continu + fail-safe boia |
| **Tier 3 (industrial)** | Pressio hidrostatica 4-20mA | 50-200 EUR | Precisio, durabilitat, estandard |

---

## 3. PLATAFORMES / DASHBOARD

### 3.1 HTML + MQTT.js (ACTUAL)

El dashboard actual: 587 linies HTML amb login MQTT, SVG animades, conectat a HiveMQ via WebSocket.

- Zero infraestructura (GitHub Pages gratuit)
- Totalment personalitzable
- **Limitacions a escala:** sense historics, sense alertes, sense gestio d'usuaris, sense API

Viabilitat: possible pero no recomanat per mes de 5-10 diposits.

### 3.2 Grafana + InfluxDB/TimescaleDB

```
Nodes --> MQTT Broker --> Telegraf/Node-RED --> InfluxDB --> Grafana
```

- Dashboards professionals amb poc esforc (drag & drop)
- Historics il.limitats, alertes integrades (email, Telegram, webhook)
- Grafana Cloud: pla gratuit fins a 10.000 metriques
- Cal un servidor (VPS 5-10 EUR/mes o Raspberry Pi local)
- Cal un "pont" entre MQTT i InfluxDB (Telegraf, Node-RED)
- **Cost:** 0 EUR (self-hosted) a 5-10 EUR/mes (VPS)

### 3.3 ThingsBoard (Open Source IoT Platform)

Plataforma IoT completa: gestio de dispositius + dashboards + alertes + regles.

- Tot-en-un: MQTT broker integrat, OTA, multi-tenant, API REST
- Rule engine visual per automatitzacions
- Corba d'aprenentatge pronunciada, requereix 2-4GB RAM
- **Cost:** 0 EUR (CE self-hosted) a 10-30 EUR/mes (Cloud)

### 3.4 Node-RED + Dashboard

- Molt facil d'integrar MQTT, bases de dades, APIs
- Programacio visual, dashboards rapids
- Limitacions estetiques, no escala be per molts usuaris
- **Cost:** 0 EUR

### 3.5 Home Assistant

Viable per Tier 1, no recomanat per Tier 2/3. Pensat per la llar, no per infraestructura critica.

### 3.6 Cloud IoT Platforms (AWS IoT Core, Azure IoT Hub)

Maxima escalabilitat i fiabilitat, gestio completa de dispositius. Pero cost significatiu (50-200 EUR/mes) i lock-in. Overkill per 20-50 diposits.

### 3.7 SCADA (Ignition, ScadaBR)

Estandard industrial per control d'infraestructures d'aigua. Redundancia, certificable per normativa. ScadaBR es gratuit i open source. Cost elevat per solucions comercials.

### RECOMANACIO DE PLATAFORMA

| Tier | Plataforma | Justificacio |
|---|---|---|
| **Tier 1** | Grafana Cloud (gratuit) + InfluxDB | Rapid, visual, historics, alertes basiques |
| **Tier 2** | ChirpStack + Grafana + InfluxDB (en VPS) | Gestio LoRaWAN + dashboards professionals |
| **Tier 3** | ThingsBoard o Ignition SCADA | Gestio completa, alarmes, multi-tenant |

---

## 4. PROPOSTES D'ARQUITECTURA (3 Tiers)

### TIER 1 - MINIM VIABLE (Rapid, Barat, Monitoratge)

**Objectiu:** Monitorar el nivell de tots els diposits en un dashboard centralitzat. Sense control remot.

- **Comunicacio:** LoRa punt a punt evolucionat (protocol actual + ID de node + payload expandit)
- **Sensors:** Boies NC existents (3 per diposit)
- **Plataforma:** HTML+MQTT millorat o Grafana Cloud gratuit
- **Pressupost:** 500-1.000 EUR per 20 diposits

```
Diposit 1 (Emissor) --+
Diposit 2 (Emissor) --+-- LoRa 868MHz --> Receptor/Gateway (Heltec V3 + WiFi/4G)
Diposit 3 (Emissor) --+                        |
                                                +--> HiveMQ Cloud --> Dashboard Web
```

**Canvis al firmware:**
1. Payload expandit (3-5 bytes): Byte 0 = Node ID, Byte 1 = Estat boies + flags, Byte 2 = Tensio bateria, Byte 3 = Comptador
2. Topics MQTT per node: `boia/{nodeId}/inputs`, `boia/{nodeId}/status`
3. Gateway multi-receptor que escolta multiples emissors

### TIER 2 - PROFESSIONAL (Fiable, Escalable, Control Remot)

**Objectiu:** Monitoratge continu amb nivell %, control remot de bombes, alertes, historics.

- **Comunicacio:** LoRaWAN amb ChirpStack + 2-3 gateways amb backhaul 4G
- **Sensors:** Ultrasonic (JSN-SR04T) + boia NC de seguretat
- **Plataforma:** ChirpStack + Node-RED + Grafana + InfluxDB en VPS
- **Pressupost:** 2.000-5.000 EUR + 15-25 EUR/mes

```
Zona Nord:                                  Zona Sud:
  Diposit 1 --+                               Diposit 5 --+
  Diposit 2 --+--> GW-1 (RAK7268+4G) --+      Diposit 6 --+--> GW-2 (RAK7268+4G) --+
  Diposit 3 --+    (solar + bateria)    |      Diposit 7 --+    (electricitat)       |
                                        |                                            |
                         +--------------+--------------------------------------------+
                         |
                    VPS (5-10 EUR/mes)
                    +-------------------------+
                    |  ChirpStack (LoRaWAN NS)|
                    |  MQTT Broker (Mosquitto)|
                    |  Node-RED (logica)      |
                    |  InfluxDB (historics)   |
                    |  Grafana (dashboards)   |
                    +-------------------------+
```

**Funcionalitats:**
- Nivell en % de cada diposit (ultrasonic)
- Mapa interactiu amb tots els diposits i el seu estat
- Historics de nivell (grafiques temporals)
- Alertes per Telegram/email
- Control remot: enviar ordre via LoRaWAN downlink
- Bateria i senyal de cada node al dashboard
- OTA firmware updates via ChirpStack

### TIER 3 - INDUSTRIAL (SCADA, Redundancia, Certificable)

**Objectiu:** Sistema de gestio d'aigua comarcal complet, certificable.

- **Comunicacio:** LoRaWAN amb gateways redundants + 4G fallback per nodes critics
- **Sensors:** Pressio hidrostatica 4-20mA + sensors de cabal + temperatura
- **Plataforma:** ThingsBoard PE o Ignition SCADA + PostgreSQL + Grafana
- **Pressupost:** 10.000-50.000 EUR + 100-300 EUR/mes

**Funcionalitats addicionals:**
- Redundancia de gateways (cada node arriba a minim 2 gateways)
- RTU industrials per nodes critics
- Integracio amb EPANET (simulacio hidraulica) o QGIS
- Multi-tenant: ajuntaments, operadors, tecnics amb permisos diferenciats
- Audit trail complet
- Alarmes multinivell amb escalat
- Manteniment predictiu

---

## 5. MODEL DE DADES

### 5.1 Dades per node/diposit

```json
{
  "node_id": "DIP-001",
  "name": "Diposit Font del Pi",
  "location": {
    "lat": 41.5432,
    "lon": 1.8765,
    "altitude_m": 450,
    "municipality": "Vallbona",
    "comarca": "Urgell"
  },
  "tank": {
    "capacity_liters": 50000,
    "diameter_m": 3.5,
    "height_m": 4.0,
    "material": "formigo",
    "year_built": 1995
  },
  "telemetry": {
    "timestamp": "2026-08-12T10:30:00Z",
    "level_percent": 72.5,
    "level_cm": 290,
    "volume_liters": 36250,
    "temperature_water_c": 18.3,
    "flow_rate_lpm": 0.0,
    "pump_active": false,
    "pump_runtime_min": 0,
    "buoy_min": 0,
    "buoy_mid": 0,
    "buoy_max": 1,
    "battery_voltage": 3.82,
    "solar_voltage": 5.1,
    "rssi_dbm": -67,
    "snr_db": 9.5,
    "frame_counter": 12345
  },
  "config": {
    "tx_interval_s": 300,
    "sensor_type": "ultrasonic",
    "pump_max_runtime_min": 240,
    "level_alarm_low_percent": 20,
    "level_alarm_high_percent": 95,
    "firmware_version": "2.1.0"
  }
}
```

### 5.2 Emmagatzematge i retencio

| Resolucio | Retencio | Proposit |
|---|---|---|
| Dades crues (cada 5 min) | 90 dies | Analisi detallada recent |
| Agregats horaris (mitjana, min, max) | 1 any | Tendencies |
| Agregats diaris | 5 anys | Historics llarg termini |
| Alarmes i events | Indefinit | Auditoria |

Espai estimat (InfluxDB): ~50 MB/any per 30 diposits amb mesures cada 5 min.

### 5.3 Regles d'alerta

| Alerta | Condicio | Prioritat | Accio |
|---|---|---|---|
| Nivell baix | level < 20% durant >30 min | Alta | Telegram + email |
| Nivell critic | level < 5% | Critica | Telegram + SMS + email |
| Diposit ple | level > 95% | Info | Aturar bomba |
| Desbordament | level > 100% | Critica | Aturar bomba + alarma |
| Node desconnectat | Sense dades > 30 min | Alta | Telegram |
| Bateria baixa | battery < 3.4V | Mitjana | Email |
| Bomba massa temps | pump_runtime > max | Alta | Aturar bomba |
| Senyal LoRa debil | rssi < -110 dBm | Baixa | Log |
| Temperatura anomala | temp > 35C o temp < 2C | Mitjana | Email |

---

## 6. AUTOMATITZACIO FUTURA

### 6.1 Control remot de bombes (bidireccional)

**Amb LoRaWAN:**
- **Class A downlink:** Ordre enviada en la finestra RX despres del seguent uplink. Latencia fins a TX_INTERVAL_S
- **Class C:** Escolta continua, latencia <1s, pero consum alt
- **Solucio practica:** Class A amb interval 60s. 1 minut de latencia acceptable per bombes d'aigua

**Payload de comanda (downlink):**
```
Byte 0: Comanda (0x01=bomba ON, 0x02=bomba OFF, 0x03=config update)
Byte 1: Parametre (durada maxima en minuts, o ID de configuracio)
Byte 2: Checksum/magic byte (seguretat)
```

### 6.2 Algorismes de distribucio automatica

Multiples diposits conectats per canonades amb bombes entre ells:

```
per cada parell (diposit_origen, diposit_desti):
  si diposit_desti.nivell < LLINDAR_BAIX
  i diposit_origen.nivell > LLINDAR_MINIM_ORIGEN
  i bomba_disponible
  i franja_horaria_permesa (nocturn per tarifa electrica?)
  llavors:
    activar_bomba(origen, desti, durada_maxima)
```

Grafic de prioritats: hospital > poblacio > rec. En sequera, omplir primer els de prioritat alta.

### 6.3 Prediccio de consum

- Historics de nivell per dia de la setmana i hora
- Patrons (consum maxim dilluns-divendres 7-9h i 19-21h)
- Model senzill: regressio lineal sobre la taxa de buidat dels ultims 7 dies

### 6.4 Integracio amb dades meteorologiques

- API AEMET (gratuita) o Open-Meteo
- Pluja forta prevista: no omplir diposits de recollida d'aigua pluvial
- Onada de calor: incrementar nivells minim acceptables
- Gelada: activar proteccions de canonades

### 6.5 Protocols d'emergencia

| Emergencia | Accio automatica | Notificacio |
|---|---|---|
| Desbordament | Aturar totes les bombes que omplen el diposit | SMS + alarma |
| Sequera (tots <30%) | Mode estalvi, prioritzar essencials | Reunio emergencia |
| Fallada bomba (runtime > max sense canvi nivell) | Aturar, marcar avariada | Telegram tecnic |
| Perdua comunicacio node critic | Fallback local, avís OLED | SMS immediat |

### 6.6 Control d'acces multi-usuari

| Rol | Permisos |
|---|---|
| Visor | Veure dashboards i historics |
| Operador | Visor + activar/desactivar bombes |
| Tecnic | Operador + configurar parametres |
| Administrador | Tot + gestio d'usuaris, provisionament |

---

## 7. ALIMENTACIO PER A NODES REMOTS

### 7.1 Solar + Bateria

- **Panell solar:** 6W (6V/1A) - suficient per LoRaWAN amb deep sleep
- **Bateria:** 18650 LiPo (3.7V, 3500 mAh) o LiFePO4 (3.2V, mes durable, fins a -20C)
- **Controlador:** TP4056 (barat), CN3791 (MPPT), o carregador integrat del Heltec

**Calcul de consum (LoRaWAN):**

| Estat | Corrent | Duracio |
|---|---|---|
| Deep sleep | 10 uA | ~295 s (98.3% del temps) |
| Despertar + mesura sensor | 30 mA | 200 ms |
| TX LoRa (SF7, 14dBm) | 120 mA | 50 ms |
| RX windows | 30 mA | 2000 ms |
| **Mitjana per cicle** | **~0.25 mA** | - |

Autonomia amb 3500 mAh sense solar: ~583 dies. Amb panell 6W: indefinida.
Marge per dies nublats (hivern Catalunya, 5-7 dies): ~15 dies autonomia sense sol.

### 7.2 Alimentacio de xarxa

Diposits amb caseta tecnica (230VAC per la bomba): reutilitzar PCB actual amb HLK-PM01 + UPS petit.

### 7.3 Consideracions practiques

- **Caixa:** IP65 minim, IP67 recomanat. ABS amb prensaestopes
- **Panell solar:** Sud, inclinacio 35-40 graus (latitud Catalunya)
- **Temperatura:** LiFePO4 per zones de muntanya (funcionen fins a -20C)
- **Proteccio:** Varistor + fusible + diode Schottky

---

## 8. SEGURETAT

| Capa | Mesura | Implementacio |
|---|---|---|
| LoRaWAN | Xifratge AES-128 (AppSKey + NwkSKey) | Natiu del protocol |
| MQTT | TLS 1.2+ (port 8883) | Ja implementat amb HiveMQ |
| Dashboard | HTTPS + autenticacio | Grafana: HTTPS amb Let's Encrypt |
| API | OAuth2 o API keys | ChirpStack API |

### OTA Updates

- LoRaWAN FUOTA: lent per LoRa (hores per 500KB)
- Practica: actualitzacio manual via USB cada 6-12 mesos per <100 nodes
- OTA imprescindible nomes amb >100 nodes o molt inaccessibles

### Seguretat fisica

- Caixes amb cargols de seguretat (torx)
- Senyalitzacio "Equipament de telecomunicacions"
- Registre de manteniment

---

## 9. PLA D'IMPLEMENTACIO RECOMANAT

### Fase 0: Pilot (zero cost addicional)
1. Mantenir sistema actual per al primer diposit
2. Modificar firmware emissor: afegir Node ID al payload (2 bytes extra)
3. Modificar gateway/receptor per gestionar multiples emissors
4. Desplegar 2-3 emissors addicionals en diposits propers
5. Millorar dashboard HTML per multiples diposits

### Fase 1: Infraestructura (500-1.000 EUR)
1. Desplegar 1 gateway LoRaWAN (RAK7268) en punt elevat
2. Instal.lar ChirpStack en VPS (5 EUR/mes)
3. Convertir firmwares a LoRaWAN (OTAA, Cayenne LPP)
4. Configurar Grafana + InfluxDB per historics
5. Desplegar alertes Telegram
6. Primer lot de 5-10 nodes amb boies

### Fase 2: Expansio (2.000-3.000 EUR)
1. Afegir gateways per cobrir tota la comarca
2. Migrar sensors a ultrasonic (nivell continu)
3. Implementar control remot de bombes (downlink LoRaWAN)
4. Dashboard amb mapa interactiu
5. Desplegar nodes solars en diposits sense electricitat
6. 20-30 nodes totals

### Fase 3: Automatitzacio (segons necessitat)
1. Algorismes de distribucio automatica
2. Integracio amb meteorologia
3. Prediccio de consum
4. Multi-tenant (si multiples municipis)

---

## 10. RESUM EXECUTIU

| Aspecte | Tier 1 | Tier 2 | Tier 3 |
|---|---|---|---|
| **Comunicacio** | LoRa P2P amb ID | LoRaWAN + 4G GW | LoRaWAN + 4G redundant |
| **Sensors** | Boies NC (3/diposit) | Ultrasonic + boia seguretat | Pressio 4-20mA + cabal |
| **Plataforma** | HTML+MQTT millorat | ChirpStack + Grafana | ThingsBoard/SCADA |
| **Control** | Local (actual) | Remot via dashboard | Automatitzat + manual |
| **Cost inicial** | 500-1.000 EUR | 2.000-5.000 EUR | 10.000-50.000 EUR |
| **Cost mensual** | 0-5 EUR | 15-25 EUR | 100-300 EUR |
| **Nodes** | 5-10 | 20-50 | 50-200+ |
| **Personal** | 1 tecnic (vosaltres) | 1 tecnic + suport IT | Equip dedicat |

**Recomanacio final:** Comencar amb **Fase 0** (zero cost, validacio multi-node amb hardware existent), despres **Fase 1** cap a Tier 2 amb LoRaWAN. El Heltec V3 amb SX1262 ja suporta LoRaWAN nativament - la transicio es de firmware, no de hardware. L'arquitectura hibrida LoRaWAN + 4G backhaul als gateways es l'opcio amb millor relacio qualitat-preu per a l'escenari comarcal catala.
