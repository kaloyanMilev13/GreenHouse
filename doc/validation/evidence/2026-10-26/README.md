# Verification evidence — 26 October 2026

This folder records the 26 October run. Its backend and dashboard results support the [current report](../../Greenhouse_Verification_and_Validation_Updated.docx); its firmware results are historical and superseded by the [28 October retest](../2026-10-28/README.md). Tests were executed against the working-tree files present on 26 October, including the user's existing uncommitted changes. Application source was not changed for these tests. No controller was flashed and no pump was operated.

## Results and traceability

| Evidence | What actually ran | Report coverage |
|---|---|---|
| [Firmware results](firmware-results.txt) | 13 host scenarios using the current C function bodies, with clock, GPIO, ADC, DHT, OLED and MQTT adapters | VER-01 to VER-06, VER-08, VER-10 |
| [Backend results](backend-results.txt) | 43 cases against the unchanged JavaScript, with MQTT and database adapters | VER-06, VER-07 |
| [Dashboard results](dashboard-results.txt) | 56 assertions in PostgreSQL 18.6; all 14 actual panel queries in both modes; captured backend INSERT statements | VER-07 to VER-09, VER-11 |
| [Build result](build-results.md) | ESP-IDF build using the existing Ninja build directory; Node syntax check | Software can build and parse |
| [Source fingerprints](source-sha256.json) | SHA-256 hashes of the code, dashboard and relevant design/analysis files | Exact reviewed versions |

Passing harness assertions include assertions that reproduce a defect. Therefore, 13, 43 and 56 are software coverage counts, **not counts of accepted project requirements**. GAP and LIMIT entries remain findings to address.

## Test method and limits

- Firmware: `test_firmware.py` removes include directives and a commented-out calibration program, then compiles the actual application bodies with GCC. Each scenario runs in a fresh process. Controlled clock values test decisions at boundaries; they are not real elapsed-time measurements. The main-loop scenarios execute one complete cycle and stop at its final five-second delay. The queue-failure case tests local operation when enqueueing returns an error, not a real Wi-Fi outage or reconnection.
- Backend: `test_backend.cjs` evaluates the unchanged source in Node's VM with adapters for `mqtt` and `pg`. Inputs are passed through the actual message handler. Valid inputs reach the actual parameterized SQL; invalid inputs reach no INSERT. Database failure is injected and produces one insert attempt and an error log. The test does not establish durable delivery or a database commit.
- SQL: `test_dashboard.py` starts a disposable PostgreSQL cluster in **single-user mode**, without a network listener. It runs the dashboard SQL with only the Grafana mode and time-filter macros expanded. Tables are explicit fixtures, not a copy of the deployed database. Their event uniqueness key follows the design and 23 October schema review. Duplicate-event suppression works with that key; the deployed constraint was not rechecked. The test transaction is rolled back and the disposable cluster removed. No production data is read or changed.
- Dashboard checks cover row counts, current-value selection, ties, mode separation, temperature history values, watering-history order/duration, air threshold boundaries, low reservoir, null values, no data, and the freshness boundary. They do not inspect Grafana rendering, every plotted value, actual refresh delay or browser usability.
- PostgreSQL connection attempts to the existing database failed: TCP defaults lacked a password; the local socket reported that role `kala420` does not exist. Therefore live storage, schema, backend restart persistence and end-to-end latency remain unverified.
- No `/dev/ttyUSB*` or `/dev/ttyACM*` controller was available. Sensor accuracy, physical cutoff height, electrical disconnection, pump flow and timing, watering adequacy, Wi-Fi recovery and caretaker task performance were not measured. This describes the automated run on 26 October. The later [caretaker self-test record](../2026-10-28/caretaker-validation.md) supersedes the earlier predicted validation outcomes.

## Findings supported by the 26 October execution

The OLED finding and two-second configuration below are historical; see the 28 October retest for the corrected display, five-second limit and ten-minute cooldown. The raw PostgreSQL transcript is not present in this evidence package; the retained SQL evidence consists of the result summary, fixture SQL, captured INSERT fixtures and test script.

1. Conversion endpoints, the 32-sample rounded mean and reservoir boundary logic behaved as coded.
2. A control check at 2000 ms stops the pump command; a deliberately late check at 2007 ms records 2007 ms. Physical maximum ON time cannot be inferred from these clock-adapter tests.
3. ADC value 4095 is accepted as soil data and can start watering. This demonstrates a code-level limit, not the voltage an unplugged sensor actually produces.
4. In the complete main-loop cycle there were no OLED writes while the pump command was ON. Telemetry was formatted only after OFF.
5. The SQL warning limits include 16/27 °C and 40/60% humidity as normal. A receipt exactly 30 seconds old is fresh; 30.001 seconds is stale.
6. A newly received payload with old uptime is marked fresh. The check measures receipt age, not acquisition age.
7. Relative-light values 0, 100 and NULL produced no light alert. Plant-specific light limits are not implemented.

## Reproduce

The firmware command below belongs to the historical source fingerprint. On the current source, use `2026-10-28/test_firmware_retest.py`; do not overwrite old logs with a run of changed firmware.

From the project root, with Python 3, GCC, Node.js and PostgreSQL binaries installed:

```bash
python3 doc/validation/evidence/2026-10-26/test_firmware.py
node doc/validation/evidence/2026-10-26/test_backend.cjs
python3 doc/validation/evidence/2026-10-26/test_dashboard.py
node --check backend/index.js
ninja -C greenhouse/build
```

The backend script must run before the SQL script because it saves the actual INSERT statements and their parameters. The fixture schema deliberately does not claim to reproduce all production constraints.

## Sources for expectations

- [Analysis and advice](../../../analysis/Greenhouse_Analysis_and_Advice_Revised.odt): FR-01 to FR-10 are core requirements; FR-11 is a Should Have; local control was advised so watering does not depend on remote monitoring.
- [Current architecture review](../../../design/System_Architecture/REVIEW_NOTES.md): design/code relationships, bounded MQTT buffering, database retry limits, receipt timestamps and local-control limits.
- [Wiring review](../../../design/Greenhouse_Hardware/Wiring_Table.md): unresolved pump-enable and light connections in the drawing, plus actual supply and assembly details still to confirm.
- These sources explain the original requirements and design. The current report uses the scoped software checks and qualitative caretaker acceptance recorded in the [evidence index](../README.md). Earlier proposed timings and water-volume targets are not measurements of the caretaker self-test.
