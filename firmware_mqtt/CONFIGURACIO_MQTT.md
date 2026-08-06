# Configuracio MQTT - HiveMQ Cloud

## 1. Crear compte HiveMQ Cloud

1. Ves a https://www.hivemq.com i clica **"Start Free"**
2. Registra't amb GitHub, Google, LinkedIn o email
3. Un cop dins, clica **"Create Serverless Cluster"** (gratis, 10 GB/mes, 100 connexions)

## 2. Obtenir dades de connexio

A la pestanya **Overview** del cluster:
- **URL del broker**: algo com `abc123def.s1.eu.hivemq.cloud` (apunta-la)

## 3. Crear credencials

A la pestanya **Access Management**:
1. Clica **Edit** > **Add Credentials**
2. Crea un **username** i **password** (apunta'ls)
3. Assigna el rol **"Allow All"**
4. Clica **Save**

## 4. Configurar el firmware

### Test (qualsevol ESP32 amb WiFi)

Fitxer: `firmware_mqtt/tests/test_mqtt_hivemq/test_mqtt_hivemq.ino`

Canvia aquestes linies:
```c
#define WIFI_SSID      "nom_xarxa_wifi"
#define WIFI_PASSWORD  "password_wifi"
#define MQTT_SERVER    "abc123def.s1.eu.hivemq.cloud"
#define MQTT_USER      "el_teu_usuari_hivemq"
#define MQTT_PASSWORD  "la_teva_password_hivemq"
```

### Receptor real (Heltec WiFi LoRa 32 V3)

Fitxer: `firmware_mqtt/receptor_mqtt/config.h`

Canvia aquestes linies:
```c
#define WIFI_SSID      "nom_xarxa_wifi_modem_4g"
#define WIFI_PASSWORD  "password_wifi_modem_4g"
#define MQTT_SERVER    "abc123def.s1.eu.hivemq.cloud"
#define MQTT_USER      "el_teu_usuari_hivemq"
#define MQTT_PASSWORD  "la_teva_password_hivemq"
```

## 5. Llibreria necessaria (Arduino IDE)

Instal·lar des de **Sketch > Include Library > Manage Libraries**:
- **PubSubClient** (autor: Nick O'Leary)

## 6. Compilar i pujar

1. Obre el `.ino` amb Arduino IDE
2. Selecciona la placa:
   - Test: **ESP32 Dev Module**
   - Receptor real: **Heltec WiFi LoRa 32 (V3)**
3. Selecciona el port COM correcte
4. Clica **Upload**
5. Obre **Serial Monitor** a 115200 baud

## 7. Verificar que funciona

### Al monitor serie has de veure:

```
WiFi: OK! IP: 192.168.x.x
NTP: OK!
MQTT: CONNECTAT OK!
  boia/outputs: {"out1":1,"out2":0,...}
  boia/deye: {"soc":75,"pv_power":3200}
  PUBLICAT OK!
```

Si veus `MQTT: ERROR rc=-2`: revisa la URL del broker i el port (8883).
Si veus `MQTT: ERROR rc=4`: revisa username i password de HiveMQ.
Si veus `MQTT: ERROR rc=5`: revisa que les credencials tinguin el rol "Allow All".

### Al HiveMQ Web Client:

1. Ves al teu cluster a hivemq.com
2. Pestanya **"Web Client"**
3. Clica **"Connect"**
4. A **"Topic Subscription"** escriu `boia/#` i clica **"Subscribe"**
5. Veuras els missatges JSON arribar en temps real

## 8. Topics MQTT

| Topic | Contingut | Exemple |
|-------|-----------|---------|
| `boia/status` | Online/offline | `"online"` |
| `boia/outputs` | Estat sortides | `{"out1":1,"out2":0,"out3":0,"out4":0,"out1_min":12.5}` |
| `boia/inputs` | Entrades LoRa | `{"in1":0,"in2":1,"in3":0,"in4":0}` |
| `boia/deye` | Dades inversor | `{"soc":85,"pv_power":2400}` |
| `boia/lora` | Estat LoRa | `{"connected":true,"rssi":-45,"rx_ok":120,"rx_err":2}` |
| `boia/locals` | Entrades locals | `{"boia_bomba":1,"switch_mc":0,"pot_max_min":120}` |

## 9. Limits del pla gratis

- **10 GB/mes** de trafic (el sistema consumeix ~1 GB/mes publicant cada 2 min)
- **100 connexions** simultanies
- Sense targeta de credit
- Si superes 10 GB, el cluster es desactiva fins al mes seguent

## 10. Connexio TLS

HiveMQ Cloud nomes accepta connexions segures:
- **Port**: 8883 (no 1883)
- **Certificat**: ISRG Root X1 (Let's Encrypt), ja inclus al firmware
- **NTP**: el firmware sincronitza el rellotge automaticament (necessari per validar el certificat)
- **Android/iOS**: ja confien en aquest certificat, no cal instal·lar res al mobil

## 11. Estructura fitxers

```
firmware_mqtt/
  receptor_mqtt/              <-- Firmware receptor real (Heltec V3)
    config.h                  <-- Canviar SSID, password, broker, user
    receptor_mqtt.ino
  tests/
    test_mqtt_hivemq/         <-- Test amb qualsevol ESP32
      test_mqtt_hivemq.ino    <-- Canviar SSID, password, broker, user
```
