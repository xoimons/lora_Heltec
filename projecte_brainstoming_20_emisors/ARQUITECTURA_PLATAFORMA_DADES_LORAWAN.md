# Plataforma de dades per als 18 dipòsits (LoRaWAN + ChirpStack + InfluxDB + Grafana)

Document de decisions de la sessió del 28/08/2026, sobre com centralitzar, guardar i mostrar al client les dades dels 18 dipòsits emissors. És un **projecte independent** del sistema LoRa punt a punt boia/bomba (Heltec V3, `firmware/emissor` i `firmware/receptor`) i també independent del disseny de PCB/pressupost/facturació tractat a `RESUM_BRAINSTORMING.md` — aquí ens centrem només en com arriben i es mostren les dades un cop generades pels 18 nodes.

> Estat: decisions d'arquitectura preses. Pendent de concretar `docker-compose.yml` i el firmware LoRaWAN dels nodes.

---

## 1. Visió general de l'arquitectura

```
18 dipòsits (nodes LoRaWAN, OTAA)
        │  RF (868 MHz)
        ▼
  Gateway RAK7268V2 (WisGate Edge Lite 2, 8 canals, SX1302, LoRaWAN 1.0.3)
  — a l'interior d'una caseta, antena a l'exterior (cable coax baixa pèrdua)
  — connectat per Ethernet a un router/mòdem 4G ja existent a la caseta
        │  Semtech UDP GWMP (port 1700), sortint cap a internet
        ▼
  VPS al núvol (Hetzner CX22, 2vCPU/4GB)
  ├─ ChirpStack (Network Server + Application Server) + Postgres + Redis + Mosquitto
  ├─ InfluxDB (emmagatzematge sèries temporals)
  └─ Grafana (dashboards) + reverse proxy (Traefik/Caddy) amb HTTPS (Let's Encrypt)
        │
        ▼
  Client final: navegador → https://domini-propi.com (Grafana, usuari Viewer)
```

## 2. Per què aquesta arquitectura i no una altra

- El sistema actual (boia/bomba) és LoRa **punt a punt** amb protocol propi (1 byte, sense identificador de node) — no escala a 18 emissors independents. Amb 18 dipòsits calen dispositius identificables i un gateway real: per això es passa a **LoRaWAN estàndard** (join OTAA, DevEUI/AppKey) en lloc de LoRa cru.
- **ChirpStack** es descarta córrer'l al propi gateway (existeix un firmware alternatiu "ChirpStack Gateway OS" oficial del projecte ChirpStack per al RAK7268v2, però no oficial de RAK — voldria dir perdre garantia i el gateway és maquinari petit, no pensat per allotjar Postgres+Redis a llarg termini). Es manté el **firmware de fàbrica WisGateOS**, en mode *Packet Forwarder*, apuntant a un ChirpStack centralitzat al servidor.
- **Servidor al núvol (VPS)** en lloc d'un ordinador local a la caseta: la caseta només té connexió **4G amb CGNAT** (sense IP pública), fet que impossibilita exposar un servidor local sense solucions addicionals (Cloudflare Tunnel o SIM M2M amb IP fixa). Muntar-ho local només compensa a llarg termini (5-9 anys per amortitzar maquinari+SAI) i afegeix manteniment propi (SO, backups, talls de corrent). El VPS surt més barat i amb menys feina els primers anys — es decideix anar per aquesta opció.

## 3. Gateway — RAK7268V2 (WisGate Edge Lite 2)

- 8 canals, concentrador Semtech **SX1302**, LoRaWAN 1.0.3, IP30 (només interior).
- **Sense LTE** (variant sense "C") — no cal, ja hi ha un router 4G extern a la caseta que fa de WAN; el gateway hi va per Ethernet.
- Ubicació: interior de la caseta, amb **antena externa** connectada per cable coaxial (recomanat baixa pèrdua tipus LMR400 si la distància és llarga) + protector de línia/parallamps a l'entrada del cable si l'antena queda exposada.
- Configuració (WisGateOS 2):
  - Work Mode: **Packet Forwarder**
  - Banda: **EU868**
  - Protocol: **Semtech UDP GWMP** (opció recomanada per ChirpStack v4; alternativa més segura però més feina: Basic Station/MQTT amb TLS)
  - Server address: domini/IP del VPS
  - Server port up/down: **1700** (port per defecte del `chirpstack-gateway-bridge`)
  - Gateway EUI: generat automàticament per WisGateOS a partir de la MAC Ethernet — cal donar-lo d'alta a ChirpStack (Tenant → Gateways → Add Gateway, mateix EUI + frequency plan EU868)
- Backhaul: Ethernet → router 4G de la caseta → internet → VPS. La connexió la inicia el gateway (outbound), així que el **CGNAT del 4G no és un problema en aquesta direcció** (no cal port-forwarding al router 4G). Trànsit LoRaWAN de 18 nodes és mínim, qualsevol tarifa de dades bàsica sobra.

## 4. Nodes emissors (18 dipòsits)

- Migració pendent: deixar el LoRa cru actual i implementar LoRaWAN complet (join OTAA amb DevEUI/AppKey, frame counters, payload decodificat amb un codec JS a ChirpStack).
- Ordre de magnitud de maquinari/cost per node: veure `RESUM_BRAINSTORMING.md` (secció 7-9) per al disseny de PCB pròpia (ESP32-C3 + SX1262/PA) i pressupost — aquell document tracta el disseny físic dels nodes, aquest document tracta només la plataforma de dades un cop les dades arriben al gateway.

## 5. Servidor (VPS al núvol)

**Stack Docker previst:**
- ChirpStack (network server + application server) + Postgres + Redis + Mosquitto
- InfluxDB (integració nativa des de ChirpStack v4, sense pont/script intermedi)
- Grafana
- Reverse proxy (Traefik o Caddy) fent HTTPS amb Let's Encrypt

**Proveïdor triat**: Hetzner Cloud, instància **CX22** (2 vCPU / 4GB RAM / 40GB disc, ~4-5 €/mes) — sobra per a 18 dispositius, ampliable sense reinstal·lar.

**Domini**: registrador independent (p.ex. OVH), ~12-15 €/any. DNS: registre **A** apuntant a la IP pública del VPS.

**Costos anuals estimats:**

| Concepte | Cost |
|---|---|
| VPS Hetzner CX22 | ~55-60 €/any |
| Backups automàtics VPS (opcional, recomanat) | ~12 €/any |
| Domini | ~12-15 €/any |
| Certificat HTTPS (Let's Encrypt) | 0 € |
| Software (ChirpStack/InfluxDB/Grafana) | 0 €, open source |
| **Total** | **~80-90 €/any** (~7 €/mes) |

Sense cap cost de maquinari inicial (a diferència de l'opció local, que hauria requerit ~350-700 € en mini-PC + SAI).

## 6. Accés del client a les dades

- **Un sol client final** (no cal multi-tenant / separació entre diversos clients).
- Grafana exposat al domini propi, sense sign-up públic ni accés anònim, amb `noindex`/`robots.txt` (no és una web pública).
- Estructura de carpetes a Grafana:
  - **"Intern"** → dashboards complets, accés admin/editor (nosaltres).
  - **"Client"** → dashboard(s) pensats per ell.
- Usuari client amb rol **Viewer**, amb permisos de carpeta restringits només a "Client" (suportat a Grafana OSS gratuït, no cal Enterprise).
- **Descàrrega de dades**: no cal desenvolupar res — cada panell de Grafana té `Inspect → Data → Download CSV` natiu. No cal portal web propi ni desenvolupament addicional per a "consultar i descarregar".

## 7. Pendent / següents passos

- [ ] Escriure `docker-compose.yml` complet (ChirpStack + Postgres + Redis + Mosquitto + InfluxDB + Grafana + reverse proxy)
- [ ] Contractar domini + VPS (Hetzner)
- [ ] Configurar DNS (registre A)
- [ ] Instal·lar Docker al VPS
- [ ] Configurar RAK7268V2 (packet forwarder cap al VPS) i donar d'alta el Gateway EUI a ChirpStack
- [ ] Migrar firmware dels 18 nodes a LoRaWAN (OTAA, codec de payload)
- [ ] Dissenyar dashboards Grafana (carpeta Intern vs Client) i crear usuari Viewer pel client

---

*Document viu — actualitzar a mesura que es prenguin noves decisions o canviïn les ja preses.*
