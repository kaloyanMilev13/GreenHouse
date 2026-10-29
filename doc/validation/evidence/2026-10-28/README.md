# Current verification and validation evidence

Updated 28 October 2026. This folder supports the [current report](../../Greenhouse_Verification_and_Validation_Updated.docx). See the [requirement-to-evidence index](../README.md) for the full mapping.

## Software verification

The [firmware execution log](firmware-retest-results.txt) records 15 completed scenarios against the actual application function bodies, using controlled hardware, clock and message adapters. The [harness](test_firmware_retest.py) checks the current configuration: five-second watering limit, ten-minute cooldown, reservoir threshold 200, wet calibration 1620, watering-stop threshold 1700 and DRY/OK/WET/ERROR display text.

All scenario assertions completed. GAP and LIMIT entries document the accepted ADC rail and delayed-stop behaviour; they are not physical safety or exact-timing passes. Application code was not changed by the tests. The firmware source matches the [recorded fingerprint](retest-source-sha256.json).

Backend, dashboard, network and MQTT source fingerprints also match. The retained [26 October backend and SQL evidence](../2026-10-26/README.md) remains applicable: 43 backend cases and 56 SQL assertions covering all 14 dashboard panels in both modes. Light monitoring passes; light warnings remain FAIL because no warning range is implemented.

## Caretaker validation

The author subsequently confirmed testing the greenhouse as caretaker and passing all five validation tasks. The [caretaker validation record](caretaker-validation.md) records that confirmation and the report findings. These completed, author-reported results replace the earlier expected validation outcomes. The automated firmware run itself did not perform a physical caretaker trial.

Watering acceptance is qualitative. Earlier flow estimates do not establish a measured dose and are not used as a numerical validation result. Network acceptance covers local-display use and resumed dashboard updates, not the acquisition age of queued readings.

## Reproduce the software check

From the project root, with Python 3 and GCC available:

```bash
python3 doc/validation/evidence/2026-10-28/test_firmware_retest.py
```

This script executes the current source and writes a new firmware result log. Historical 26 and 27 October harnesses belong to their recorded versions and should not overwrite historical logs with current-source results.
