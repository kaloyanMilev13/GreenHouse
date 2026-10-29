# Updated verification evidence — 27 October 2026

The user added the OLED update after starting the pump. The assistant did not change application code. The earlier pending FR-05 fix is now applied in the source and passes the host retest.

## Results recorded on 27 October

This is a historical retest. The [28 October retest](../2026-10-28/README.md) supersedes its firmware configuration and display checks. The [current evidence index](../README.md) maps the final report to the applicable runs.

- [Firmware retest](firmware-retest-results.txt): 15 scenarios passed their assertions, including normal watering, queue failure, an air-sensor fault, and a controlled delay of 20 ms for each OLED text write.
- A successful build was reported for this update, but `firmware-build.log` is not present in this package. This folder therefore provides no retained raw build transcript.
- [Source fingerprints](retest-source-sha256.json): exact versions reviewed for this update.
- [Retest script](test_firmware_retest.py): runs the actual firmware function bodies with controlled hardware and clock adapters.
- [Backend and dashboard evidence from 26 October](../2026-10-26/README.md): 43 backend cases and 56 SQL assertions. Backend/dashboard source fingerprints still match those tested versions, so those checks were not repeated.

FR-05 now displays `Pump: ON` during watering, and `Pump: OFF` afterwards. The normal state shows `WATERING`; an air fault keeps `SENSOR FAULT` visible while still displaying the pump state. This confirms what the program sends to the display adapter, not readability on a physical OLED.

## Remaining limits

The controlled display-delay scenario recorded a 2010 ms cycle. This is an injected-clock result, not measured physical pump duration. The timeout is checked by the program, so delayed checks can exceed exactly 2 seconds.

ADC 4095 is still accepted as soil data. Reported read errors block watering, but not every disconnected wire necessarily produces a read error. Telemetry still describes the pump after it stops; the event history records watering activity.

These are executed software checks. There was no physical sensor comparison, pump-flow measurement, real network-outage run or stakeholder session in this update. That historical update used expected stakeholder outcomes. The final report now records the author's confirmed [caretaker self-test](../2026-10-28/caretaker-validation.md). Historical logs remain unchanged.

The earlier [review and fix instructions](review-notes.md) are retained as history. Their statement that FR-05 is pending has been superseded by this retest.

The commands below describe the 27 October configuration. Use the 28 October harness for current firmware; keep the older execution log unchanged. Historical commands from the project root:

```bash
python3 doc/validation/evidence/2026-10-27/test_firmware_retest.py
ninja -C greenhouse/build
```
