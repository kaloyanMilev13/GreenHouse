# GreenHouse

**An ESP32 greenhouse monitor that waters plants automatically and tracks growing conditions.**

A student prototype built with C, ESP-IDF, Node.js, PostgreSQL and Grafana.

## What it does

- Measures temperature, humidity, light, soil moisture and reservoir water level.
- Waters dry soil, with a 5-second time limit and a 10-minute cooldown.
- Stops watering when the soil is wet, water is low or a soil/water sensor reading is invalid.
- Shows readings and status on an OLED display.
- Sends readings and watering events over Wi-Fi for storage and dashboard viewing.

Watering runs on the ESP32 and continues during Wi-Fi or MQTT disconnections after startup.

## How it works

```text
Sensors → ESP32 → MQTT broker → Node.js → PostgreSQL → Grafana
             ├→ Water pump
             └→ OLED display
```

## Project structure

| Folder | Contents |
| --- | --- |
| [`greenhouse/`](greenhouse/) | ESP32 firmware: sensors, watering, display, Wi-Fi and MQTT |
| [`backend/`](backend/) | Node.js service that validates MQTT messages and stores them in PostgreSQL |
| [`dashboard/`](dashboard/) | Grafana dashboard and import instructions |
| [`doc/`](doc/) | Analysis, hardware design, architecture and validation records |

## Hardware

ESP32-DevKitC, M5Stack U101 soil sensor and pump, DHT11, analog light and water-level sensors, and a 128 × 64 SSD1306 OLED.

See the [wiring table](doc/design/Greenhouse_Hardware/Wiring_Table.md) for connections and known schematic gaps. Soil moisture and light readings need calibration for your setup.

## Setup overview

You need ESP-IDF (the checked-in configuration uses **6.0.3**), Node.js with npm, an MQTT broker, PostgreSQL and Grafana.

1. Clone the repository:

   ```bash
   git clone https://github.com/kaloyanMilev13/GreenHouse.git
   cd GreenHouse
   ```

2. Create `greenhouse/main/wifi_credentials.h` (ignored by Git):

   ```c
   #pragma once
   #define HOTSPOT_SSID "your-wifi-name"
   #define HOTSPOT_PASSWORD "your-wifi-password"
   ```

3. Set the MQTT broker address in `greenhouse/main/mqtt.c` and `backend/index.js`. Both currently use `mqtt://10.42.0.1:1883`.

4. Prepare PostgreSQL with `readings` and `watering_events` tables matching the inserts in [`backend/index.js`](backend/index.js). Watering events require a unique constraint on `(device_id, boot_id, cycle_id, event)`. **Database creation and migrations are not included.** Set `PGHOST`, `PGPORT`, `PGDATABASE`, `PGUSER` and `PGPASSWORD` in your shell, then start the backend:

   ```bash
   cd backend
   npm ci
   node index.js
   ```

5. In an ESP-IDF terminal, open the repository's `greenhouse/` folder and build and flash the firmware. Replace `PORT` with your ESP32's serial port:

   ```bash
   idf.py build
   idf.py -p PORT flash monitor
   ```

6. Connect Grafana to PostgreSQL and [import the dashboard](dashboard/README.md). Select **Real** in the dashboard's **Data mode** selector to view sensor readings; it defaults to **Test**.

See the [system design](doc/design/System_Architecture/README.md) and [validation records](doc/validation/evidence/README.md) for more detail.
