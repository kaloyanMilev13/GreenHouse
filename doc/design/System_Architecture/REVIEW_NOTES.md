# Code and design review - 23 October 2026

This review describes the current working-tree code, including changes that were already uncommitted. Application code and the KiCad schematic were not changed. The new diagrams represent implemented behavior; amber notes identify limitations. The older review dated 22 October describes an earlier version and should not be used as the source for current thresholds or loop order.

## Current behavior verified

- Soil moisture now uses a 32-sample rounded mean and a clamped relative percentage based on dry ADC 1883 and wet ADC 1822. Watering starts at raw soil ADC **>= 1880** and can stop at **<= 1860**. These constants are present in the code; this review does not establish their physical calibration accuracy.
- The pump has a **2000 ms** software timeout and a **60000 ms** cooldown after stopping. During a run, the inner loop checks the deadline, reads ADCs without another DHT request, validates the results, checks the stop conditions and delays **50 ms** only while the pump remains on. The five-second delay and OLED redraw are outside that loop.
- MQTT initialization is attempted **before** the first sensor/control pass. It no longer waits for `networkWaitReady()`. Wi-Fi reconnection uses a three-second one-shot timer. `networkWaitReady()` is still declared and implemented but unused.
- Grafana reads PostgreSQL directly. The Node.js program consumes MQTT and inserts data; it does not provide an HTTP API or remote control. The dashboard contains 14 panels and refreshes every five seconds.
- The local database's two tables, column nullability, checks and indexes were inspected in a read-only transaction. There is no devices table and no foreign key between readings and watering events. Event identity is unique on `(device_id, boot_id, cycle_id, event)`.

## Findings that affect the presentation

### 1. The hardware drawing omits two implemented signals

The exported netlist leaves U1 GPIO16 (pin 27) and J1 pin 3 unconnected, although the firmware drives GPIO16 for pump enable. GPIO34 (pin 5) is also unconnected and no light module is represented. The table marks both gaps explicitly. The physical supply arrangement is not established by the shared +5V net.

See [the wiring table](../Greenhouse_Hardware/Wiring_Table.md), [schematic](../Greenhouse_Hardware/Greenhouse_Hardware.kicad_sch) and `greenhouse.c` pin definitions. Do not present the current schematic as a complete assembly reference until these connections are reconciled.

### 2. Successful MQTT delivery does not guarantee stored history

`mqtt.c` gives telemetry and watering events a shared **4096-byte RAM outbox**. Queue failures are logged, without an application retry or separate persistent event journal. If MQTT initialization itself fails, watering can still proceed and event enqueueing can fail. Reboot also loses this RAM state.

On the backend, database failures are caught as rejected messages without retry. The installed MQTT.js QoS 1 receive handler emits the message event and acknowledges via `handleMessage()`; it does not await the async listener's database insert. Thus a broker acknowledgement is not a database commit. The default client setup also does not establish a durable backend subscription for downtime recovery.

Watering-event deduplication is implemented; telemetry deduplication is not. A QoS 1 redelivery can create an additional readings row. The diagrams show bounded buffering and event deduplication without promising lossless history.

Sources: [mqtt.c](../../../greenhouse/main/mqtt.c), [backend/index.js](../../../backend/index.js), and the installed `backend/node_modules/mqtt/build/lib/handlers/publish.js` / `client.js` implementation.

### 3. The displayed pump state normally misses the active run

`displayResults()` can select `WATERING`, but `app_main()` calls it before starting the pump and again after the inner loop has stopped it. The pre-run display may show `Pump: checking`. `formatTelemetry()` also runs after the pump stops, so normal telemetry records report `pump_on: false` even when watering took place during that pass.

The watering event history is therefore the implemented record of an active cycle. The OLED branch and dashboard pump card should not be described as continuously displaying the motor's live state. Physical water flow is not measured at all.

Source: [greenhouse.c](../../../greenhouse/main/greenhouse.c), `displayResults()` at line 553 and `app_main()` at line 693.

### 4. Timestamps describe different things

`sampled_at_ms` and event `occurred_at_ms` are device uptime, not wall-clock timestamps. Readings do not carry `boot_id`, so uptime resets cannot be distinguished reliably in the readings table. `readSensors(..., false)` updates the shared sample timestamp while keeping the previous air values and DHT status.

The database adds `received_at`; Grafana uses it for trends and the 30-second freshness check. After an outage, an old queued measurement may receive a new database receipt time and appear fresh. The diagram notes distinguish receipt freshness from sensor acquisition age.

Sources: [greenhouse.c](../../../greenhouse/main/greenhouse.c), `readSensors()` / `formatTelemetry()`; [dashboard queries](../../../dashboard/greenhouse-overview.json).

### 5. Local control has explicit limits

The pump timeout is checked when `updateWatering()` executes. Moving display work outside the pumping loop improves the execution path, but the code still has no independent hard cutoff; scheduling and ADC work determine when a due check runs. Reset clears cooldown history. Air and light faults do not inhibit watering.

Moisture validity accepts any successful ADC result in 0..4095. A stuck-high or disconnected input that reads in that range can still be interpreted as dry soil. The host check confirmed ADC 4095 is accepted and requests watering when reservoir data is valid. It did not simulate electrical disconnection. Reservoir values above 1700 are treated as invalid even though the ADC's full range is larger.

Network initialization failure is also different from later connection loss: `ESP_ERROR_CHECK(networkInit())` can abort startup. The diagrams say local operation continues through connection loss **after setup**, rather than promising that every network initialization failure is tolerated.

Sources: [greenhouse.c](../../../greenhouse/main/greenhouse.c), `verifyReadings()`, `updateWatering()`, `app_main()`; [network.c](../../../greenhouse/main/network.c), `networkInit()`.

### 6. Small documentation and diagnostic mismatches remain

- [backend/index.js](../../../backend/index.js), line 190: `telemetryTopic. wateringTopic` accesses a property on a string instead of logging two topic variables. It prints `undefined`; the actual subscription array above it is correct.
- [dashboard/README.md](../../../dashboard/README.md) still describes an older panel count and two-second publishing interval. The JSON has 14 panels; the firmware delays five seconds **plus work and any watering time**.
- Some older SVGs show loading settings, command subscription or a backend serving the dashboard. Those paths are not implemented in the current source. The new files are the current implementation view; older artifacts remain available as design history.

## Checks performed and limits

| Check | Result |
|---|---|
| Review of current C sources/headers, backend, dashboard and build declarations | Completed; private Wi-Fi credential contents excluded |
| KiCad schematic netlist export | Completed; GPIO/channel connections and missing nets compared with code |
| PostgreSQL schema inspection | Completed in a read-only transaction; no data inserted or schema changed |
| `node --check backend/index.js` | Passed |
| Host-side C check using the current extracted validation/control/telemetry functions | Passed start threshold, 2000 ms stop boundary, 60000 ms cooldown boundary, wet-soil stop, low-water cutoff, invalid-sensor reason priority, reservoir upper validity limit, soil conversion endpoints, accepted ADC rail and JSON null handling |
| Draw.io validation | Native XML models, edge endpoints and page links checked; all pages rendered and visually inspected; text and arrow routing corrected |

The host C check stubbed the clock, GPIO, logging and event queue. It validates code decisions, not pump duration on the ESP32 or actual sensor readings. No firmware was flashed, no pump was operated, no hardware was rewired, and no live Grafana rendering was tested in this task. Full-size previews accompany the editable diagrams.
