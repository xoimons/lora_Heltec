# Guia de Desplegament - Sistema Boia IoT

Documentació per adaptar el sistema a un entorn de client diferent.
Tots els paràmetres configurables estan centralitzats per facilitar el canvi.

---

## Arquitectura del sistema

```
                        ┌─────────────┐
                        │  Emissor    │
                        │  ESP32 LoRa │
                        │  (4 inputs) │
                        └──────┬──────┘
                               │ LoRa 868 MHz
                        ┌──────▼──────┐       RS485 Modbus
                        │  Receptor   │◄─────────────────────┐
                        │  ESP32 LoRa │                      │
                        │  (4 outputs)│               ┌──────┴──────┐
                        └──────┬──────┘               │ Inversor    │
                               │ WiFi                 │ Deye 6kW    │
                               │                      └─────────────┘
                        ┌──────▼──────┐
                        │  HiveMQ     │
                        │  Cloud      │
                        │  (MQTT TLS) │
                        └──────┬──────┘
                               │ WSS (port 8884)
                        ┌──────▼──────┐
                        │  Dashboard  │
                        │  HTML/SVG   │
                        │ (GitHub Pages)│
                        └─────────────┘
```

---

## 1. Canviar el broker MQTT

### 1.1. Firmware del receptor ESP32

**Fitxer:** `firmware_mqtt/receptor_mqtt/config.h`

Modificar les línies 17-22:

```cpp
#define MQTT_SERVER    "NOVA_URL.hivemq.cloud"    // URL del nou broker
#define MQTT_PORT      8883                        // Port TLS (normalment 8883)
#define MQTT_USER      "NOU_USUARI"                // Usuari MQTT
#define MQTT_PASSWORD  "NOVA_PASSWORD"             // Password MQTT
#define MQTT_CLIENT_ID "receptor_lora_boia"        // ID client (canviar si cal)
```

**Si el broker NO és HiveMQ** (ex: Mosquitto, EMQX):
- El certificat TLS a `receptor_mqtt.ino` (línies 17-49) s'ha de canviar pel certificat root CA del nou broker
- Si el broker no requereix TLS, cal canviar `WiFiClientSecure` per `WiFiClient` i treure `secureClient.setCACert()`

### 1.2. Dashboard web

**Fitxer:** `firmware_mqtt/tests/test_mqtt_hivemq/dashboard.html`

Modificar la línia del broker (dins del `<script>`):

```javascript
const BROKER = 'wss://NOVA_URL:8884/mqtt';
```

**Ports habituals segons broker:**
| Broker | Port WSS |
|--------|----------|
| HiveMQ Cloud | 8884 |
| Mosquitto | 9001 (configurable) |
| EMQX Cloud | 8084 |

**Credencials del dashboard:**
Les credencials de login del dashboard són les mateixes que les del broker MQTT (l'usuari es connecta directament al broker via WebSocket).

---

## 2. Canviar l'allotjament de la pàgina

### 2.1. Opció actual: GitHub Pages

- **URL:** `https://xoimons.github.io/lora_Heltec/firmware_mqtt/tests/test_mqtt_hivemq/dashboard.html`
- **Repositori:** `https://github.com/xoimons/lora_Heltec`
- El desplegament és automàtic via GitHub Actions (`.github/workflows/pages.yml`)
- Cada `git push` a `main` redesplega la pàgina

### 2.2. Opció: Servidor web del client

El dashboard és un **fitxer HTML únic** sense dependències de servidor (tot és client-side). Es pot servir des de qualsevol servidor web:

- **Apache/Nginx:** Copiar `dashboard.html` al directori del servidor
- **IIS (Windows Server):** Copiar al directori del site
- **NAS (Synology, QNAP):** Activar Web Station i copiar el fitxer

No cal PHP, Node.js, ni cap backend. L'únic requisit és que el navegador pugui accedir al broker MQTT per WebSocket (port WSS obert).

### 2.3. Opció: Obrir localment

El fitxer `dashboard.html` es pot obrir directament al navegador (`File > Open`) sense cap servidor. Funciona perquè:
- La connexió MQTT es fa directament del navegador al broker
- L'únic recurs extern és la llibreria `mqtt.min.js` (carregada per CDN)

---

## 3. Canviar la xarxa WiFi (modem 4G)

**Fitxer:** `firmware_mqtt/receptor_mqtt/config.h`

Modificar les línies 11-12:

```cpp
#define WIFI_SSID      "NOM_XARXA_WIFI"
#define WIFI_PASSWORD  "PASSWORD_WIFI"
```

---

## 4. Topics MQTT

Tots els topics comencen amb `boia/`. Si es vol canviar el prefix:

**Fitxer:** `firmware_mqtt/receptor_mqtt/config.h`, línia 25:

```cpp
#define MQTT_TOPIC_PREFIX  "boia/"
```

**Fitxer:** `dashboard.html`, dins de `handleMessage()` i la subscripció:

```javascript
client.subscribe('boia/#', { qos: 0 });   // Canviar 'boia/' pel nou prefix
```

I tots els `case` dins de `handleMessage`:

```javascript
case 'boia/outputs':   // Canviar per 'nou_prefix/outputs'
case 'boia/inputs':    // etc.
case 'boia/deye':
case 'boia/lora':
case 'boia/locals':
```

### Topics publicats pel receptor

| Topic | Contingut | Freqüència |
|-------|-----------|------------|
| `boia/status` | `"online"` / `"offline"` | Connexió/desconnexió (LWT) |
| `boia/outputs` | `{"out1":1,"out2":0,"out3":0,"out4":0,"out1_min":12.5}` | Cada 2 min o canvi |
| `boia/inputs` | `{"in1":1,"in2":0,"in3":0,"in4":0}` | Cada 2 min o canvi |
| `boia/deye` | `{"soc":85,"pv_power":320}` | Cada 60s (lectura Modbus) |
| `boia/lora` | `{"connected":true,"rssi":-65,"rx_ok":142,"rx_err":0}` | Cada 2 min o canvi |
| `boia/locals` | `{"boia_bomba":1,"switch_mc":0,"pot_max_min":120}` | Cada 2 min o canvi |

---

## 5. Configuració hardware

### Pins GPIO (receptor ESP32 Heltec V3)

| Funció | GPIO | Notes |
|--------|------|-------|
| OUT1 (bomba) | 6 | Sortida relé |
| OUT2 | 47 | Sortida relé |
| OUT3 | 7 | Sortida relé |
| OUT4 | 48 | Sortida relé |
| Boia bomba | 5 | Entrada, pull-down. 1=aigua |
| Switch mitja càrrega | 1 | Entrada, pull-down |
| Potenciòmetre | 2 | ADC, duració màxima bomba (0-240 min) |
| RS485 TX | 19 | Cap a MAX485 |
| RS485 RX | 20 | Des de MAX485 |
| RS485 DE/RE | 3 | Control direcció MAX485 |

### Paràmetres LoRa

| Paràmetre | Valor |
|-----------|-------|
| Freqüència | 868 MHz (Europa ISM) |
| Spreading Factor | SF7 |
| Bandwidth | 125 kHz |
| Coding Rate | 4/5 |

### Paràmetres Modbus (Deye)

| Paràmetre | Valor |
|-----------|-------|
| Baud rate | 9600 |
| Slave ID | 1 |
| Registre SOC | 184 |
| Registre PV1 Power | 186 |
| Registre PV2 Power | 187 |
| Interval lectura | 60 segons |

---

## 6. Llindars d'energia (histeresi SOC)

**Fitxer:** `firmware_mqtt/receptor_mqtt/config.h`, línies 74-76:

```cpp
#define DEYE_SOC_START  30   // SOC mínim per arrancar bomba (%)
#define DEYE_SOC_STOP   20   // SOC per parar bomba (%)
```

Comportament:
- SOC >= 30%: permet arrancar la bomba
- SOC entre 20-30%: manté l'estat actual (histeresi)
- SOC < 20%: para la bomba

---

## 7. Futura implementació: Històric de dades amb Supabase

### Arquitectura proposada

```
ESP32 → MQTT (HiveMQ) → Dashboard (temps real)
     ↘ HTTP POST cada 5-10 min → Supabase (PostgreSQL) → Gràfiques Chart.js
```

### Passos d'implementació

1. **Crear projecte Supabase** (supabase.com, free tier: 500 MB)
2. **Crear taula `dades_boia`:**
   ```sql
   CREATE TABLE dades_boia (
     id         BIGSERIAL PRIMARY KEY,
     created_at TIMESTAMPTZ DEFAULT NOW(),
     soc        INTEGER,
     pv_power   INTEGER,
     out1       BOOLEAN,
     out1_min   REAL,
     boia_bomba BOOLEAN,
     rssi       INTEGER,
     rx_ok      INTEGER,
     in1        BOOLEAN,
     in2        BOOLEAN,
     in3        BOOLEAN,
     in4        BOOLEAN
   );
   ```
3. **Afegir al firmware ESP32** (`receptor_mqtt.ino`):
   - Incloure `HTTPClient.h`
   - Cada 5-10 minuts, fer HTTP POST a l'API REST de Supabase
   - URL: `https://<projecte>.supabase.co/rest/v1/dades_boia`
   - Header: `apikey: <clau anon>`, `Content-Type: application/json`
4. **Afegir gràfiques al dashboard:**
   - Incloure Chart.js via CDN
   - Al login, consultar últimes 24-48h via GET a Supabase
   - Pintar gràfiques temporals de SOC, PV Power, etc.

### Paràmetres Supabase a configurar

```cpp
// config.h - afegir
#define SUPABASE_URL       "https://XXXX.supabase.co"
#define SUPABASE_ANON_KEY  "eyJ..."
#define SUPABASE_TABLE     "dades_boia"
#define SUPABASE_INTERVAL_MS  300000   // Cada 5 minuts
```

```javascript
// dashboard.html - afegir
const SUPABASE_URL = 'https://XXXX.supabase.co';
const SUPABASE_ANON_KEY = 'eyJ...';
```

### Consideracions
- La clau `anon` de Supabase és pública (pensada per a clients)
- Activar Row Level Security (RLS) per permetre només INSERT des del firmware i SELECT des del dashboard
- Free tier: 500 MB ≈ ~100.000 registres/any a 1 registre cada 5 min (sobra de llarg)
- Si falla el POST, no afecta el funcionament del sistema (MQTT segueix independent)

---

## Resum de fitxers a modificar per adaptar a client

| Fitxer | Què canviar |
|--------|-------------|
| `firmware_mqtt/receptor_mqtt/config.h` | WiFi, MQTT broker, credencials, pins, llindars |
| `firmware_mqtt/receptor_mqtt/receptor_mqtt.ino` | Certificat TLS (si canvia broker) |
| `firmware_mqtt/tests/test_mqtt_hivemq/dashboard.html` | URL broker WSS, prefix topics |
| `.github/workflows/pages.yml` | Eliminar si no s'usa GitHub Pages |
