# Projecte brainstorming: escalat a 20 emissors LoRa

Resum de la sessió de brainstorming (24/08/2026) sobre l'evolució del sistema LoRa actual: des de guardar les dades que ja s'envien per MQTT, fins a un disseny propi de node emissor i el vessant legal/fiscal de facturar el projecte.

> **Avís de privacitat**: la secció 9 conté dades personals (salari, situació familiar) fetes servir només per fer una estimació d'IRPF. Aquest fitxer està exclòs de git via `.gitignore` — no el pengeu a cap repositori remot ni el compartiu sense revisar-ho abans.

---

## 1. Punt de partida: estimació de trànsit de dades (SIM 4G)

El sistema actual (`firmware_mqtt_failsafe`) publica per MQTT a HiveMQ Cloud cada 30 s (4 topics: `boia/outputs`, `boia/inputs`, `boia/lora`, `boia/locals`), amb un sol node connectat al router 4G.

- Xifra documentada al repo (`CONFIGURACIO_MQTT.md`): ~1 GB/mes publicant cada 2 min.
- Escalat a la versió de 30 s (4x més freqüent): **~3-4 GB/mes** estimat.
- Recomanació de SIM: **5-10 GB/mes** de marge (cobertura 4G inestable, reconnexions TLS).

## 2. Avaluació de Losant (plataforma IoT enterprise)

Losant (ara de SUSE) ofereix workflows, dashboards, digital twins i multi-tenancy per a flotes grans de dispositius. Preus no publicats (model "contact sales", cotització personalitzada).

**Conclusió**: sobredimensionat per al projecte actual (1 node). HiveMQ Cloud gratuït ja cobreix les necessitats. Té sentit només si en el futur s'escala a desenes/centenars de dipòsits amb necessitats enterprise.

## 3. Fase 1: guardar les dades que ja es publiquen

Objectiu: el sistema actual envia dades per MQTT però no les guarda enlloc. Primera fase del projecte personal: emmagatzemar-les per mostrar-les via web més endavant.

Requisit: **eines gratuïtes**, ja que és una fase de prova (PoC).

## 4. PoC amb Raspberry Pi 3

Arquitectura proposada:

```
Receptor ESP32 --MQTT/TLS--> HiveMQ Cloud <--MQTT/TLS-- Raspberry Pi 3 (subscriptor)
                                                              |
                                                       escriu a SQLite (targeta SD)
```

- La RPi es subscriu a HiveMQ Cloud com un client més (mateix rol que el dashboard actual), sense tocar el firmware del receptor.
- **Per què no Supabase (500 MB gratuïts)**: amb 4 topics cada 30 s, guardant cada missatge, 500 MB s'esgotarien en ~7-8 mesos. Amb SQLite local a la RPi, guardant **una fila per cicle** (no una per topic), el consum estimat és de ~150 MB/any — una SD de 16-32 GB dona per desenes d'anys. El límit de Supabase era un problema del servei cloud, no un problema real de volum de dades.
- Per a la fase web: Flask/FastAPI propi o Grafana (gratuït, ja fet per a sèries temporals). Accés extern sense obrir ports: Cloudflare Tunnel o Tailscale Funnel.
- **HiveMQ segueix sent necessari**: és el punt de trobada entre el receptor (darrere del router 4G) i la RPi (a una altra xarxa). Substituir-lo per un broker propi (Mosquitto) obligaria a fer la RPi accessible des de fora de la seva xarxa (port forwarding + DNS dinàmic) — més complex que mantenir HiveMQ Cloud gratuït.

## 5. PoC casolà: un ESP existent + RPi (xarxa local, sense HiveMQ)

Per fer proves ràpides a casa amb un ESP que ja es té, com que ESP i RPi comparteixen la mateixa WiFi, no cal passar per HiveMQ — es pot muntar un **Mosquitto local a la RPi**.

```
ESP (casa) --MQTT (WiFi local)--> Mosquitto a la RPi --> script Python --> SQLite
```

Es manté el protocol MQTT (no HTTP POST) per reutilitzar el firmware actual i perquè escala millor a N dispositius (LWT online/offline, un sol subscriptor amb wildcard).

**Canvi clau pensant en els 20 nodes futurs**: identificar cada dispositiu al topic des del primer dia.
- Topics: `devices/<device_id>/outputs`, `.../inputs`, etc. (en lloc de `boia/...` fix)
- BD: taula `devices(device_id, nom, ubicacio)` + taula `readings(id, device_id, topic, payload, timestamp)`
- El subscriptor es subscriu a `devices/+/#` — no cal tocar-lo quan s'afegeixen més ESP.

### 5.1 Firmware ESP — exemple de canvis (NOMÉS EXEMPLE, no aplicat a cap fitxer del repo)

`config.h`:
```c
// --- WiFi (xarxa de casa, NO el modem 4G) ---
#define WIFI_SSID      "NomWifiCasa"
#define WIFI_PASSWORD  "ContrasenyaCasa"

// --- MQTT (Mosquitto local a la Raspberry Pi, SENSE TLS) ---
#define MQTT_SERVER    "192.168.1.50"   // IP local de la Raspberry Pi
#define MQTT_PORT      1883             // Sense TLS, xarxa local de confianca
#define MQTT_CLIENT_ID "esp-01"

#define DEVICE_ID  "esp-01"
#define MQTT_TOPIC_PREFIX      "devices/" DEVICE_ID "/"
#define MQTT_TOPIC_STATUS      MQTT_TOPIC_PREFIX "status"
#define MQTT_TOPIC_OUTPUTS     MQTT_TOPIC_PREFIX "outputs"
#define MQTT_TOPIC_INPUTS      MQTT_TOPIC_PREFIX "inputs"
#define MQTT_TOPIC_LORA        MQTT_TOPIC_PREFIX "lora"
#define MQTT_TOPIC_LOCALS      MQTT_TOPIC_PREFIX "locals"
```

`receptor_mqtt.ino`:
```cpp
// Abans: WiFiClientSecure + secureClient.setInsecure()
// Ara (sense TLS, no cal WiFiClientSecure):
#include <WiFiClient.h>
#include <PubSubClient.h>
WiFiClient plainClient;
PubSubClient mqtt(plainClient);
```
La resta de `mqttReconnect()`/`mqttPublishAll()` queda igual — PubSubClient no depèn de si el `Client` de sota és amb TLS o no.

### 5.2 Script subscriptor Python (RPi)

```python
#!/usr/bin/env python3
# subscriber.py - subscriptor MQTT que guarda lectures a SQLite
import sqlite3, time
from datetime import datetime, timezone
import paho.mqtt.client as mqtt

MQTT_HOST = "localhost"
MQTT_PORT = 1883
TOPIC_FILTER = "devices/+/#"
DB_PATH = "/home/pi/lora_data/readings.db"

def init_db():
    conn = sqlite3.connect(DB_PATH)
    conn.execute("""
        CREATE TABLE IF NOT EXISTS readings (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            device_id TEXT NOT NULL,
            topic TEXT NOT NULL,
            payload TEXT NOT NULL,
            received_at TEXT NOT NULL
        )
    """)
    conn.execute("CREATE INDEX IF NOT EXISTS idx_device_time ON readings(device_id, received_at)")
    conn.commit()
    return conn

def parse_device_id(topic):
    parts = topic.split("/")
    return parts[1] if len(parts) >= 2 else "unknown"

def on_connect(client, userdata, flags, rc, properties=None):
    client.subscribe(TOPIC_FILTER)

def on_message(client, userdata, msg):
    conn = userdata
    device_id = parse_device_id(msg.topic)
    payload = msg.payload.decode("utf-8", errors="replace")
    now = datetime.now(timezone.utc).isoformat()
    conn.execute(
        "INSERT INTO readings (device_id, topic, payload, received_at) VALUES (?, ?, ?, ?)",
        (device_id, msg.topic, payload, now),
    )
    conn.commit()

def main():
    conn = init_db()
    client = mqtt.Client(userdata=conn, protocol=mqtt.MQTTv311)
    client.on_connect = on_connect
    client.on_message = on_message
    while True:
        try:
            client.connect(MQTT_HOST, MQTT_PORT, keepalive=60)
            client.loop_forever()
        except Exception as e:
            print(f"Error de connexio: {e}, reintentant en 5s...")
            time.sleep(5)

if __name__ == "__main__":
    main()
```

Servei systemd per fer-lo persistent:
```ini
# /etc/systemd/system/mqtt-subscriber.service
[Unit]
Description=MQTT to SQLite subscriber
After=network.target mosquitto.service

[Service]
ExecStart=/usr/bin/python3 /home/pi/lora_data/subscriber.py
Restart=always
RestartSec=5
User=pi

[Install]
WantedBy=multi-user.target
```

## 6. Escalat real: 20 emissors LoRa repartits en una comarca (fins a 3 km de la caseta)

Amb 20 emissors, el maquinari del receptor canvia de categoria.

**Opció recomanada — concentrador multicanal + ChirpStack**:
- HAT/mòdul concentrador basat en SX1301/SX1302/SX1303 (RAK2287, RAK5146, Seeed WM1302) — rep 8 canals en paral·lel i tots els SF alhora, evitant col·lisions que un sol canal no aguantaria bé amb 20 nodes.
- **ChirpStack** (gratuït, open-source) com a network server, corrent al mateix host. Publica les dades desxifrades a MQTT local — el mateix patró de subscriptor+SQLite ja dissenyat funciona igual, només canvien els noms dels topics.
- **Cost real**: reescriure el firmware dels 20 ESP per fer servir una pila LoRaWAN de veritat (join OTAA, claus, RadioLib) en lloc del protocol punt a punt actual (1 byte de payload).

**Alternativa lleugera**: un sol mòdul SX1262 (SPI) mantenint el protocol propi actual — més barat i sense reescriure firmware, però un sol canal amb risc de col·lisions creixent amb els nodes, i cal ampliar el payload (actualment 1 byte, sense identificador de node) per incloure un `device_id`.

**Antena i abast**: amb SF7/BW125 (config actual), 3 km és factible en línia de vista neta, però en terreny de comarca cal una **antena externa de guany, muntada fora i alta a la caseta**, amb cable coaxial de baixa pèrdua — sol pesar més en l'abast real que el mòdul en si.

**Límit legal de potència (868 MHz, ETSI EN 300 220)**: el màxim habitual per als canals típics de LoRaWAN sol ser ~+14 dBm ERP en molts casos — cal confirmar el límit exacte de la sub-banda/canal usat abans de configurar el firmware a plena potència.

## 7. Disseny de PCB pròpia per als nodes emissors (projecte nou, independent)

Concepte: ESP mínim + SX1262 amb antena de pal exterior, 4 entrades digitals + 1 analògica 0-10V, deep sleep amb despertar cada 30 min.

- **MCU**: **ESP32-C3** (mòdul MINI-1 per al primer disseny) — cost baix, deep sleep natiu, prou GPIO/SPI, mateix ecosistema (Arduino/PlatformIO) que ja es fa servir.
- **Ràdio**: mòdul SX1262 **amb amplificador (PA)** per emetre amb potència, ex. **Ebyte E22-900M30S** (fins a +30 dBm/1W). El PA és un transistor amplificador extra soldat al mòdul, no una funció del xip SX1262 en si (que sol arriba a +22 dBm). Cal també que el mòdul tingui **connector d'antena** (U.FL o SMA) per portar-hi l'antena de pal per cable coaxial — són dues característiques independents del mòdul.
- **Alimentació**: 2 piles (en paral·lel per doblar capacitat, o en sèrie si es vol més tensió — en paral·lel evita necessitar un buck converter, només un LDO de baix Iq com el MCP1700).
- **Entrades digitals (x4)**: resistència sèrie ~10 kΩ + pull-down/up ~10 kΩ + condensador debounce 100 nF + diode TVS de protecció per línia (cablejat exterior exposat).
- **Entrada analògica 0-10V**: divisor resistiu (ex. 68 kΩ + 33 kΩ) per escalar a l'ADC (0-3,3V), condensador de filtrat 100 nF-1 µF, protecció amb Zener/TVS.
- **Programació**: pads/header de 4 pins (no connector USB a cada unitat) — es programa amb un adaptador extern reutilitzable durant la producció.

## 8. Estimació de bateria (node cada 30 min, deep sleep entremig)

| Fase | Corrent aprox. | Energia/cicle |
|---|---|---|
| Deep sleep | ~10 µA | ~0,005 mAh |
| Despertar + llegir entrades | ~20-40 mA, ~50-100 ms | negligible |
| TX amb PA (pic curt) | ~300-500 mA, ~100-200 ms | ~0,015-0,03 mAh |

Amb 48 cicles/dia: **~1,5-2,5 mAh/dia → ~550-900 mAh/any**. Una sola bateria Li-ion 18650 (2000-3500 mAh) donaria **2-3 anys d'autonomia**. Punt a vigilar: el **pic de corrent** durant la TX (fins a ~0,5 A amb PA) — cal una química de bateria (Li-ion) que aguanti aquest pic, no una alcalina/coin cell.

## 9. Pressupost

Full de càlcul complet (editable) a `Pressupost_projecte_LoRa_20nodes.xlsx`, en aquesta mateixa carpeta, amb 3 fulls: **Node emissor**, **Receptor (comú)** i **Resum** (amb nombre de nodes editable i total calculat automàticament).

Ordres de magnitud:
- **Cost per node emissor**: ~14-24 €/unitat (ESP32-C3, mòdul LoRa+PA, passius, PCB, antena, caixa, 2 piles)
- **Infraestructura receptor** (fixa, no escala amb el nombre de nodes): ~215-340 € (HAT concentrador SPI + RPi + SD + alimentació + antena de pal + cable + **protector de sobretensions** (no opcional, antena exterior amb cable llarg cap a la caseta) + caixa)
- **Total aprox. per a 20 nodes**: ~500-800 €

### Concentrador receptor — opció encapsulada USB (en lloc del HAT SPI)

Si es vol un concentrador ja encapsulat i connectat per USB a la RPi/PC (en lloc del HAT muntat sobre els pins GPIO):

| Opció | Preu | Notes |
|---|---|---|
| **RAK2287 (EU868) + RAK7271/7371 WisGate Developer Base** | ~85-99 $ + 99 $ (~165-185 €) | Dongle USB-C plug-and-play, inclou cable i antena petita (2,3 dBi) de proves. Recomanat si es vol "compra i connecta" sense muntatge. |
| **Seeed WM1302 (USB, EU868)** | 29 $ | Mòdul mini-PCIe nu, sense caixa ni antena — més barat però necessita carcassa pròpia. |

## 10. Facturació del projecte: opcions legals (2 persones, ~10.000-15.000 € cadascuna, ja assalariades)

Situació de partida: actualment un intermediari factura al client i els paga una part "en B" (no declarat) — risc d'inspecció d'Hisenda (recàrrecs + sancions) i sense cotització. Es vol passar a facturar-ho de forma legal.

### 10.1 Opcions

- **Autònoms en pluriactivitat** (compatible amb ser assalariat): cadascú factura la seva part directament. Reducció de cotització del 50% els primers 18 mesos / 25% els següents 18 (no compatible amb la tarifa plana, cal triar). Devolució del 50% de l'excés si la suma de cotitzacions (nòmina + RETA) supera 17.323,68 €/any (2026).
- **SL entre els dos**: capital mínim 3.000 €, costos de constitució (notari + Registre Mercantil + gestoria), Impost de Societats (25%, o 15% els 2 primers anys amb beneficis), comptabilitat i dipòsit de comptes recurrents. **Per què probablement NO ajuda aquí**: quan els ingressos de la societat provenen del treball personal dels socis (com en aquest cas), la norma d'operacions vinculades/societats professionals obliga a atribuir la major part com a sou a preu de mercat, tributant als mateixos tipus marginals d'IRPF — l'"estalvi" teòric de repartir com a dividends (Impost de Societats + IRPF de l'estalvi, combinat ~33%) gairebé no aplica en la pràctica, i els costos fixos de la SL es mengen qualsevol marge petit que quedi. Té sentit si es reinverteixen beneficis dins la societat durant anys, no per treure diners ja en un projecte puntual.
- **Punt crític a revisar sempre**: contracte laboral actual (clàusules d'exclusivitat/plena dedicació/no competència), sobretot si el projecte està relacionat amb el sector de l'empresa actual.

### 10.2 Estimació IRPF (dades personals — veure avís de privacitat a l'inici del document)

Assumpcions: resident a Catalunya, declaració individual, casat amb una filla de 14 anys, sou 49.000 € bruts/any, sense altres deduccions. Escales usades: estatal 2026 (9,5/12/15/18,5/22,5/24,5%) i catalana 2026 (10,5/12/14/15/18,5/21,5/23,5/24,5/25,5%). Mínim personal 5.550 € + mínim per descendent 2.400 €.

**Comparativa final (8.000 € de facturació, sense deduir material, tarifa plana vs pluriactivitat, alta llarga vs curta):**

| Règim | Durada alta | Quota SS total | Rendiment net activitat | IRPF addicional | Net a la butxaca |
|---|---|---|---|---|---|
| Pluriactivitat | 12 mesos | ≈ 1.200 € | 6.800 € | ≈ 2.516 € | ≈ 4.284 € |
| Pluriactivitat | ~2 mesos | ≈ 200 € | 7.800 € | ≈ 2.886 € | ≈ 4.914 € |
| Tarifa plana | 12 mesos | ≈ 1.064 € | 6.936 € | ≈ 2.566 € | ≈ 4.370 € |
| **Tarifa plana** | **~2 mesos** | **≈ 177 €** | **7.823 €** | **≈ 2.894 €** | **≈ 4.929 €** ← millor opció |

**Mateixa configuració (tarifa plana + alta ~2 mesos) a diferents facturacions:**

| Facturació | Quota SS | IRPF addicional | Net a la butxaca | % net |
|---|---|---|---|---|
| 8.000 € | 177 € | ≈ 2.894 € | ≈ 4.929 € | ~61,6% |
| 15.000 € | 177 € | ≈ 5.643 € | ≈ 9.179 € | ~61,2% |

**Per què el % net es manté estable (~61%) independentment de l'import**: com que el sou ja omple els trams baixos de l'escala, qualsevol ingrés extra d'aquest projecte cau gairebé sencer al tram marginal alt (~37-40% d'IRPF combinat estatal+autonòmic), amb una quota SS petita gràcies a l'alta curta (2 mesos, prorratejada per dies — mecanisme legal des de la reforma RETA de 2023, vàlid fins a 3 altes/baixes per any).

**Conclusions clau**:
- La durada de l'alta (2 mesos vs 12 mesos) pesa més en el resultat final que triar tarifa plana o pluriactivitat.
- El tram de cotització RETA es calcula sobre el **rendiment net anual** declarat a Hisenda, no dividit pels mesos actius — concentrar la facturació en pocs mesos no penalitza pujant de tram.
- No hi ha una via legal que redueixi dràsticament el ~40% marginal quan ja es té un sou alt consolidat — les úniques palanques reals addicionals són aportacions a pla de pensions (fins a 1.500 €/any), deduir despeses reals de l'activitat, i tributació conjunta si la parella té ingressos baixos.

---

*Aquest document és un resum de brainstorming, no un document tècnic o fiscal definitiu. Les xifres de cost i fiscalitat són estimacions orientatives — a validar amb proveïdors reals i un gestor/assessor fiscal abans de prendre decisions.*
