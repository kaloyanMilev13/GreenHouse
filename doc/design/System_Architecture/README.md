# Greenhouse system design

**30 pages in 10 editable Draw.io files**, reviewed against the current working-tree implementation on **23 October 2026**. There are eight separate architecture topics, one navigation map and a two-page hardware wiring table. All pages use a 1600 x 900 landscape canvas for presentation use.

The organization follows the abstract-to-specific method: **L0 = purpose/responsibility**, **L1 = component interaction or major decision**, **L2 = actual function behavior, fields or queries**. Open a topic at its first page, then use the page tabs or the **Previous page / Explore next page** links to move deeper. A topic may have multiple detail pages at the same level.

These are native editable shapes, connectors and tables, not flattened screenshots. The accompanying PNGs are full-size slide previews; the `.drawio` files are the editing source. Each diagram has its source functions or files in the footer. Green represents local control; blue represents communication/presentation; purple represents data storage; amber notes identify current implementation limits. Arrow labels and the diagram title specify whether a connection is control flow, data flow or a logical relationship.

## Start here

- [Design navigation map](00_Design_Map.drawio)
- [Wiring table: editable pages](../Greenhouse_Hardware/Wiring_Table.drawio) and [Markdown table](../Greenhouse_Hardware/Wiring_Table.md)
- [Code review and implementation limits](REVIEW_NOTES.md)
- [Existing hardware schematic](../Greenhouse_Hardware/Greenhouse_Hardware.kicad_sch)

## Topic files

| File | Layering | Pages |
|---|---|---:|
| [01 System architecture](01_System_Architecture.drawio) | Purpose -> runtime deployment -> telemetry sequence | 3 |
| [02 Firmware flow](02_Firmware_Flow.drawio) | Responsibilities -> startup -> actual main-loop order | 3 |
| [03 Sensors and display](03_Sensors_and_Display.drawio) | Data path -> validation and conversion -> OLED status priority | 3 |
| [04 Watering control](04_Watering_Control.drawio) | State machine -> start decision -> running loop -> event identity | 4 |
| [05 Connectivity](05_Connectivity.drawio) | Channels -> Wi-Fi events -> MQTT initialization -> enqueue/delivery behavior | 4 |
| [06 Data and database](06_Data_and_Database.drawio) | Logical relationships -> physical schema -> ingest flow -> payload validation | 4 |
| [07 Files and modules](07_Files_and_Modules.drawio) | Source ownership -> public calls -> structures and persistent RAM state | 3 |
| [08 Dashboard](08_Dashboard.drawio) | Caretaker views -> query selection -> alert rules | 3 |

## Suggested presentation order

Begin with **01 / L0**, then **01 / L1** to establish the purpose and the components. Show the existing schematic with the wiring table. For each later topic, show its overview before its implementation details: firmware, sensors, watering, communication, stored data, modules, then the dashboard. Choose detail pages according to the explanation you want to give; keep remaining pages as discussion/appendix material. For example, explain the watering state machine before showing its start decision or inner control loop.

The level labels describe depth **within a topic**, not a single giant flowchart. A database relationship, a state transition and a function call have different meanings and are intentionally shown on separate pages.

## Current implementation facts

- The ESP32 operates locally after initialization while Wi-Fi/MQTT can reconnect in the background. Network initialization errors can still abort startup.
- Watering starts at soil raw ADC >= 1880, stops on a valid stop condition or the 2000 ms software threshold, and has a 60000 ms cooldown. Detailed pages show the validity guards and stop-reason priority.
- Grafana queries PostgreSQL directly. Remote command handling, runtime settings storage and an HTTP backend API are not implemented.
- The physical schema has `readings` and `watering_events`, with no foreign key between them. Dashed conceptual associations are not invented database constraints.
- The hardware netlist omits the pump-enable wire and the light sensor. The wiring table labels these as missing rather than treating them as completed connections.

The previous SVGs and presentation are preserved as design history. Their prototype command/settings paths are not included as implemented functionality in this set. The current uncommitted application changes were reviewed and preserved. Schema inspection was read-only; no firmware was flashed or hardware operated.

## Full-size page previews

Use these PNGs directly in slides, or edit/export the corresponding Draw.io page. The original editable files stay separate from these previews.

### 00 Design Map

- [L0: From purpose to individual functions](previews/00_Design_Map/01-map.png)

### 01 System Architecture

- [L0: Keep the plant watered and make its condition visible](previews/01_System_Architecture/01-system-purpose.png)
- [L1: Where each responsibility runs](previews/01_System_Architecture/02-system-containers.png)
- [L2: One reading travels from the device to the dashboard](previews/01_System_Architecture/03-system-sequence.png)

### 02 Firmware Flow

- [L0: One local loop owns the greenhouse](previews/02_Firmware_Flow/01-firmware-overview.png)
- [L1: Startup establishes safe outputs before normal work](previews/02_Firmware_Flow/02-firmware-startup.png)
- [L2: app_main(): the actual execution order](previews/02_Firmware_Flow/03-firmware-main.png)

### 03 Sensors and Display

- [L0: Turn sensor reads into usable local information](previews/03_Sensors_and_Display/01-sensors-path.png)
- [L1: Validation separates driver success from derived values](previews/03_Sensors_and_Display/02-sensor-validation.png)
- [L2: displayResults(): choose the first matching status](previews/03_Sensors_and_Display/03-oled-priority.png)

### 04 Watering Control

- [L0: Three logical states govern the pump](previews/04_Watering_Control/01-watering-states.png)
- [L1: updateWatering(): decide whether to start](previews/04_Watering_Control/02-watering-start.png)
- [L2: Stop promptly while the pump is running](previews/04_Watering_Control/03-watering-stop.png)
- [L2: A watering cycle creates two related events](previews/04_Watering_Control/04-watering-events.png)

### 05 Connectivity

- [L0: Reporting runs alongside local control](previews/05_Connectivity/01-connectivity-overview.png)
- [L1: Wi-Fi uses events and a one-shot retry timer](previews/05_Connectivity/02-wifi-states.png)
- [L2: mqttInit(): create and start the background client](previews/05_Connectivity/03-mqtt-start.png)
- [L2: Enqueue quickly; observe delivery through events](previews/05_Connectivity/04-mqtt-queue.png)

### 06 Data and Database

- [L0: Store measurements and watering activity separately](previews/06_Data_and_Database/01-data-concepts.png)
- [L1: The current PostgreSQL schema](previews/06_Data_and_Database/02-database-schema.png)
- [L2: Node.js: receive, validate and insert](previews/06_Data_and_Database/03-backend-ingest.png)
- [L2: Validate payloads before parameterized SQL](previews/06_Data_and_Database/04-backend-validation.png)

### 07 Files and Modules

- [L0: Source files follow the runtime responsibilities](previews/07_Files_and_Modules/01-files-ownership.png)
- [L1: Public interfaces connect three firmware source files](previews/07_Files_and_Modules/02-files-calls.png)
- [L2: Two data structures cross the local function boundaries](previews/07_Files_and_Modules/03-files-data.png)

### 08 Dashboard

- [L0: Help the caretaker see current condition and history](previews/08_Dashboard/01-dashboard-overview.png)
- [L1: Choose the right rows for each view](previews/08_Dashboard/02-dashboard-queries.png)
- [L2: Current alerts: freshness first, then field checks](previews/08_Dashboard/03-dashboard-alerts.png)

### Wiring Table

- [L1: Signal wiring: match the firmware to the schematic](../Greenhouse_Hardware/previews/Wiring_Table/01-wiring-signals.png)
- [L2: Power and ground: document the existing nets](../Greenhouse_Hardware/previews/Wiring_Table/02-wiring-power.png)

## Validation

All 30 pages were rendered with mxGraph and inspected visually. XML structure, connector endpoints, page-navigation targets, text fit and connector routing were checked. The backend passes `node --check`. A host check of the current C validation/control functions confirms the documented threshold and cooldown boundaries; see the review notes for its scope and hardware-testing limits.
