# Greenhouse requirements and implementation review

Reviewed on 22 October 2026 against `doc/analysis/Greenhouse_Analysis_and_Advice_Revised.odt`, including the current uncommitted changes.

The project is substantially implemented, but not ready to claim that all Must Have requirements are met. Estimated core completion is **65%**. Calibration and watering safety are the main blockers; the database and dashboard are further along than the physical validation evidence.

This is a review, not a repair. Application code, existing documents, database contents, and device firmware were not changed. The existing local firmware build was rebuilt. Test harnesses and intermediate inspection files are in `/tmp/greenhouse-review/` and are temporary.

## Scope and limits

Inspected all application C sources and headers (excluding the contents of the private Wi-Fi credentials header), CMake/dependency configuration, backend JavaScript/package files, broker script, dashboard JSON/README, analysis and technology-selection documents, parts inventory, planning documents/Gantt data, design diagrams/presentation content, KiCad schematic/netlist, PCB placeholder, and pinout reference. Generated dependencies were inspected selectively where their behavior affects findings. Historical editor snapshots were not treated as the current design.

Checked the local PostgreSQL database using a read-only transaction and the existing backend configuration without printing credentials. No hardware was flashed or operated. No USB serial device was available at the usual ttyUSB/ttyACM paths. Real wiring, installed firmware version, actual water delivery, voltage levels, and live Grafana rendering remain unverified. A row marked `is_test=false` is evidence of data labeled real, not independent proof that it came from physical sensors.

## Completion calculation

Each of FR-01 through FR-10 receives equal weight. Scores are review estimates: 0 = absent, 25 = foundation with major blockers, 50 = significant partial implementation, 75 = largely implemented but important fixes or acceptance evidence remain, 100 = demonstrated acceptance. These are not measured percentages of development hours and are not a formal academic grade.

| Requirement | Score | Evidence and remaining work |
|---|---:|---|
| FR-01 Soil moisture, temperature, humidity | 50% | Raw ADC and DHT acquisition exist. Soil percentage is never assigned; DHT scheduling needs correction and real calibration evidence is missing. |
| FR-02 Low reservoir detection | 75% | Raw threshold 1200 and invalid-reading protection exist. Verify physical cutoff height, sensor disconnection behavior, and the upper validity cutoff of 1700. |
| FR-03 Automatic watering with a limit | 25% | Start/stop/cooldown state exists, but dry threshold is zero, pump wiring is absent from the schematic, and stop timing depends on the whole loop. No real watering events in the inspected database. |
| FR-04 Safe startup and invalid/low-water interlock | 50% | Pump is initialized OFF before other hardware, and failed ADC reads inhibit watering. A zero soil ADC reading is nevertheless accepted and triggers watering; disconnected/stuck sensor handling is unproven. |
| FR-05 Local display | 75% | Readings, pump state, low-water and sensor warnings are implemented. Soil is raw ADC locally and falsely zero percent remotely. Verify actual screen behavior and display-failure impact on watering. |
| FR-06 Send readings and watering events | 75% | Both MQTT payloads and topics are implemented. Startup events and events after queue exhaustion can be lost. |
| FR-07 Database storage | 75% | Both tables exist; event deduplication constraint matches the insert. Failed writes are discarded, and reproducible schema/setup files are absent. |
| FR-08 Current readings, trends, watering activity | 75% | Fourteen panels include current readings, trends, and event history. All queries execute in both modes. Incorrect soil data, default Test mode, and missing real-cycle/display evidence remain. |
| FR-09 Caretaker warnings | 75% | Dashboard has low-water, temperature, humidity, missing-sensor, and stale-data warnings; OLED has local water/fault warnings. Confirm plant-specific limits and caretaker recognition. There is no light-fault check in the dashboard alert table. |
| FR-10 Local operation and reconnection | 75% | Wi-Fi retries are asynchronous and the control loop does not wait for connection. MQTT starts after an IP is acquired and can reconnect. Prove offline boot, running disconnection, and reconnection on hardware. Network initialization failures still abort startup. |
| **Core total** | **65%** | **650 / 1000 points. No claim of full acceptance yet.** |

FR-11 (Should Have): approximately 50%. Relative light acquisition, storage, local display and history exist; chosen-range warnings are missing and calibration needs evidence.

FR-12 fan control and FR-13 remote settings: not implemented, but optional. Do not include them in the core denominator or prioritize them ahead of essential fixes.

FR-14 through FR-21: correctly excluded. Heating, humidity control, lighting control, disease detection, weather forecasting, AI, multiple zones and industrial deployment are not missing deliverables.

## Findings ordered by priority

### P1: Watering threshold is zero and ADC zero is treated as trustworthy

`greenhouse/main/greenhouse.c:62`, `:244`, `:274`, `:299`.

`MARK_MOISTURE_DRY` is 0; valid moisture readings span 0–4095, and watering requires `moisture <= 0`. Consequently, only exactly zero triggers watering. There is no dry/wet calibration in the project. A grounded signal can also read zero successfully, so driver success alone does not establish a usable sensor reading.

A host harness using the actual validation/control functions confirmed that ADC 2000 does not request watering, while ADC zero is valid and starts the pump with sufficient reservoir water. These synthetic ADC values demonstrate the code behavior; they do not establish this plant's dry threshold.

Measure dry/wet endpoints in the actual pot, establish direction and thresholds, distinguish supported fault conditions, and verify disconnection and short-to-ground behavior before unattended use. M5Stack's current Home Assistant example uses higher readings for drier soil, while another older example differs; do not copy example thresholds without measuring the installed unit. Source: https://docs.m5stack.com/en/homeassistant/components/sensor/unit_watering

### P1: Soil percentage is always zero

`greenhouse/main/greenhouse.c:85`, `:249`, `:474`, `:594`.

`moisture_pct` is zero-initialized and never assigned. Every valid soil sample publishes `0.0`, regardless of its raw reading. The backend accepts it, and the dashboard presents it as a valid percentage. Both tested raw readings, 2000 and 1000, produced `moisture_pct:0.0`. All 20 database rows marked real also have soil percentage zero.

Implement the calibrated conversion or explicitly publish a raw quantity with matching schema/UI units until calibration is complete. Do not present an invented percentage as a measurement.

### P1: Pump stop deadline depends on display and sensor work

`greenhouse/main/greenhouse.c:303`, `:508`, `:613`, `:619`, `:647`.

The 2000 ms limit is checked only when `updateWatering()` runs. After switching the pump on, the same task clears/redraws the screen, handles networking/telemetry, waits 500 ms, and reads sensors before the next check. Even normal operation can overshoot by a loop interval.

The installed SSD1306 driver clears all 128 character positions, then redraws text. Each character uses two I2C transfers, each with a 100 ms timeout. A stalled bus can therefore hold up the next watering check substantially. This is a code-level timing risk, not a measured physical overrun.

Give pump shutoff a bounded execution path independent of slow display/network work. Test the deadline with a stalled or disconnected display and record the allowed tolerance and measured maximum run duration.

### P1: Schematic omits pump control and light sensing

`doc/design/Greenhouse_Hardware/Greenhouse_Hardware.kicad_sch`.

KiCad's exported netlist shows J1 pin 3 and U1 GPIO16 on separate unconnected nets. Firmware drives GPIO16 as pump enable; the M5Stack pin map identifies pin 3 as PUMP_EN. GPIO34, used by firmware for light, is also unconnected and no light module is represented. Following this drawing cannot reproduce the intended system.

ERC reports 36 errors and zero warnings. Many are unused pins without no-connect markers or power-model declarations, so the number does not mean 36 physical hardware failures. Resolve the real missing nets, represent the actual development board correctly, and then annotate unused pins/power sources appropriately. Document the external pump supply and common ground; the drawing currently places pump 5 V and board 5 V on one net, whereas the technology document describes separate pump power and USB board power. Verify the actual power arrangement rather than assuming that the drawing proves a safe supply configuration.

Source for the unit connector mapping: https://docs.m5stack.com/en/unit/watering

### P1: Watering events can disappear before reaching storage

`greenhouse/main/greenhouse.c:337`, `:385`, `:613`, `:622`; `greenhouse/main/mqtt.c:108`, `:200`; `backend/index.js:16`, `:233`.

There are three distinct gaps:

1. The first watering check precedes MQTT initialization. A start event during that iteration is rejected because no MQTT client exists, even if Wi-Fi is already ready. Offline boot can lose complete cycles before the first connection.
2. Readings and events share a 4096-byte RAM outbox. Continued telemetry fills it during an outage; rejected events are only logged. This is bounded buffering, not a dependable watering journal.
3. A failed database insert is caught and logged as a rejected message, without retry or durable retention. The installed MQTT.js message handler emits the application event and acknowledges using its own callback; it does not await this async listener's database insert. Default clean sessions also provide no durable backend subscription while it is stopped.

Use a separate event queue available before network startup; prioritize event retention over replaceable telemetry; define overflow/restart behavior. On the backend, tie reliable processing to durable storage or a durable retry mechanism, and choose a stable persistent subscription if records during backend downtime must be delivered. Preserve existing event deduplication.

FR-10 only promises resumed transmission, not recovery of every historical measurement. Full telemetry replay is therefore not added as a new Must Have. Missing watering events nevertheless undermine FR-06/07 and the recorded plant-care cycle.

### P2: Buffered samples can appear freshly measured

`dashboard/greenhouse-overview.json:546`, current-alert SQL, and `backend/index.js:69`.

Freshness and history use database receipt time. After reconnection, old queued readings receive new receipt times, so the dashboard can show an old state as fresh. The README acknowledges receipt-time history, but the alert freshness decision still has this limitation. Telemetry carries uptime but no boot identity or absolute sample time.

Prefer discarding obsolete telemetry in favor of the newest snapshot, or preserve enough time/boot metadata to distinguish sample age from delivery age. Keep watering-event history separately.

### P2: DHT requests are scheduled too frequently

`greenhouse/main/greenhouse.c:32`, `:204`, `:647`.

Every loop calls the DHT driver with only a 500 ms explicit delay between iterations. No independent DHT interval or initial sensor warm-up is enforced. The DFRobot-hosted DHT11 datasheet specifies an average sensing period of 2 s and no instructions during the first second after power-on. Confirm the installed module's datasheet and schedule it accordingly, while keeping reservoir and pump checks fast. Do not simply slow the entire pump-control loop to two seconds.

Source: https://dfimg.dfrobot.com/enshop/image/data/DFR0067/DFR0067_DS_10_en.pdf

### P2: Missing light warning and incomplete fault coverage

The dashboard Current alerts query checks water, air, soil, pump status and freshness, but omits light validity and light-range limits. OLED reports a light sensor fault but has no chosen-range warning. This leaves FR-11 incomplete. Also verify plausible air ranges and reservoir upper range: firmware treats a reservoir ADC value above 1700 as invalid, not merely full. That may be intentional, but needs calibration evidence; the code comment 'tested' is not a recorded test.

Dashboard warnings count toward FR-09: the requirement does not explicitly demand email/push notifications. Add external notifications only if agreed with the caretaker. Existing temperature 16–27 °C and humidity 40–60% limits need a documented plant-specific rationale.

### P2: Setup and recovery are not reproducible from the repository

No SQL schema/migration files, database setup guide, broker production configuration, service startup guide, credential template, or backup/restore procedure are checked in. The local database works, but a new maintainer cannot rebuild it from documented steps. `wifi_credentials.h` is intentionally ignored but has no checked-in example. The backend relies on PostgreSQL environment variables and does not load `.env` itself; document a launch command that loads them.

The development broker script allows anonymous clients. Both clients use a hard-coded unencrypted broker address. This conflicts with the technology document's advice to restrict clients. Document the trusted local-network boundary and configure credentials/topic permissions for the intended demonstration environment. TLS becomes relevant if using an untrusted network; commercial deployment is out of scope.

### P3: Logging typo

`backend/index.js:190` uses `telemetryTopic. wateringTopic` instead of two values. It is valid JavaScript property access, so syntax checking passes, but it prints `undefined` instead of the subscription topics. It does not stop subscription. A mock connection callback reproduced this.

## Materials and handover review

- Analysis/advice: clear scope and MoSCoW priorities. Section number 5 is used twice. Light is both listed in scope and described as conditional; make the wording consistently Should Have. Define acceptance details: chosen plant, watering dose/maximum duration tolerance, fault handling, warning limits, and what 'alert' means for the caretaker.
- Technology selection: useful comparisons and sound separation of local watering from host services. Its advice on calibration, storage error handling, access control, backups and ownership has not all been realized. The provisional reservoir selection needs a recorded outcome.
- Planning: the PDF contains the Gantt chart; it is not missing just because text extraction cannot read it. The plan ends 23 October 2026. Gantt/CSV progress remains 0 throughout and predecessor fields are empty, so it is a baseline schedule rather than current progress evidence. Consolidate the multiple Gantt versions and update actual status after the absence. Do not infer completion from elapsed calendar time.
- Design: good architecture and watering flowcharts. Several diagrams still show command subscription, remote settings, or backend-mediated dashboard requests, whereas the current Grafana queries PostgreSQL directly and remote settings are optional/unimplemented. Mark optional paths and update the as-built diagrams. The simple Control Loop diagram puts display/send before actuator control, while the actual code applies watering first. Prototype placeholders for calibration, timeouts and initialization failures remain unresolved.
- Structure: `context.txt` proposes separate sensors/watering/display modules, but these live in `greenhouse.c`. This is not itself a requirement failure; update the as-built structure or refactor if it helps fix timing and testability. Avoid a broad refactor solely for appearance near submission.
- Parts list: reservoir, plant, pot/tray and soil remain marked Needed. Confirm whether they have been acquired and update the inventory. Do not claim a plant demonstration based only on synthetic data.
- Hardware: PCB file is an empty placeholder. A custom PCB is not required by the analysis, so this should not reduce scope completion; label/remove it from deliverables if unused. The pinout image is a generic ESP32 board reference, not proof of the exact development-board header arrangement.
- Testing/handover: no committed acceptance-test report was found. `npm test` is still the default failing placeholder; `mqtt-test.sh` starts a broker/subscriber but has no behavioral assertions. Missing final artifacts include calibration results, real-cycle evidence, fault/offline tests, installation guidance, assigned caretaker/maintainer responsibilities, and backup/recovery instructions. Do not fabricate historical test results or mark this review's mock tests as hardware validation.
- Academic scope: the project covers embedded systems, networking, and databases/software/dashboard work, satisfying the challenge's breadth in subject matter. Evidence explaining the analysis, design and realization in those areas still needs to be assembled; breadth alone does not guarantee assessment completion.

## Verification performed

| Check | Result |
|---|---|
| Existing ESP-IDF build: `ninja -C greenhouse/build` | Passed; generated ESP32 binary. App partition has 11% free, bootloader 8% free. ESP-IDF emitted Kconfig notes; no application compilation failure. This was not a clean-room installation build. |
| `node --check backend/index.js` | Passed. |
| `bash -n mqtt-test.sh` | Passed. Broker not launched during this review. |
| Dashboard JSON and SQL | JSON parsed; 14 panel queries executed in Test and Real modes, 28 successful checks. Grafana macros were substituted for a 15-minute range. Live browser rendering not tested. |
| Database schema | `readings` and `watering_events` exist; watering deduplication unique index exists. |
| Database contents | 155 test reading rows; 20 rows marked real, all moisture percentage zero. Four test watering events: two starts and two time-limit stops. No real watering events. |
| Firmware logic host harness | Actual extracted validation/formatting/control functions reproduced false zero percentage and zero-ADC pump activation. A delayed call illustrates the pump limit is checked on invocation, not autonomously. Hardware APIs were stubbed. |
| Backend mock smoke checks | Valid telemetry/start event reached SQL; null/array/invalid humidity/invalid test flag rejected. Subscription logging typo and discarded DB-write failure reproduced. No real inserts made. |
| KiCad ERC/netlist | 36 errors, 0 warnings; missing pump and light connections confirmed independently through exported nets. |
| Planning visuals | Inspected rendered Gantt page and standalone Gantt PDF; chart is present. |

## Ordered completion checklist

1. **Calibrate and correct soil sensing.** Record dry/wet raw values and voltage range, set the proper trigger direction and thresholds, calculate meaningful percentage, and verify supported disconnected/shorted fault cases. Confirm reservoir threshold and maximum validity range.
2. **Make watering timing safe.** Isolate bounded pump checks/shutoff from display/network delays. Retest low water and sensor faults during an active run, plus restart behavior and cooldown. Correct the pump/light schematic and verify actual wiring, signal levels and supply arrangement.
3. **Preserve watering records.** Capture events before MQTT is initialized, give them queue priority, and implement a defined retry/durability policy across broker/backend/database outages. Avoid replaying obsolete readings as current state.
4. **Finish monitoring details.** Schedule DHT reads independently, add light limits if pursuing FR-11, validate plant-specific alert thresholds, and switch the demonstration dashboard to Real. Update its README: it currently says 12 panels and a two-second publishing interval, but the JSON has 14 panels and firmware loops with a 500 ms delay plus work time.
5. **Record acceptance tests.** For each Must Have, save input conditions, expected result, actual result, date, firmware revision, and screenshots/log/database evidence. Essential scenarios: dry/wet soil; startup; low/invalid reservoir; invalid soil; water dropping during a run; duration/cooldown; OLED failure; boot without Wi-Fi; disconnect/reconnect; backend stopped; database unavailable; and real stored watering history. Verify caretaker comprehension of warnings.
6. **Complete handover materials.** Check in schema/setup examples without secrets, document startup/restore, update diagrams/parts/plan, assign refill/inspection and maintenance duties, and add a final demonstration with real data. Keep fan control and remote settings deferred until all Must Haves pass.

The shortest route to completion is a demonstrably safe real plant-care cycle, a recorded database/dashboard history of that cycle, fault/offline evidence, and reproducible handover instructions. More optional features will not compensate for these gaps.
