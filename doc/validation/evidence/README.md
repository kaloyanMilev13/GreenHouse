# Evidence index for the final report

Report: [Greenhouse Verification and Validation](../Greenhouse_Verification_and_Validation_Updated.docx). Evidence reconciled on 28 October 2026.

The current report combines executed software checks with the author’s completed caretaker self-test. Earlier firmware results remain historical records, not contradictory results for the current code.

## Verification traceability

| Report item | Result | Evidence | What it establishes |
|---|---|---|---|
| FR-01 / VER-01 | PASS | [Recorded results](2026-10-28/firmware-retest-results.txt) | Conversion, clamping, read-error handling and 32-sample averaging. |
| FR-02 / VER-02 | PASS | [Recorded results](2026-10-28/firmware-retest-results.txt) | Eight reservoir inputs, including current threshold boundaries. |
| FR-03 / VER-03 | PASS | [Recorded results](2026-10-28/firmware-retest-results.txt) | Five-second stop decision and ten-minute cooldown; delayed-check limitation retained. |
| FR-04 / VER-04 | PASS | [Recorded results](2026-10-28/firmware-retest-results.txt) | Startup OFF, low-water and reported-fault handling; accepted ADC rail limitation retained. |
| FR-05 / VER-05 | PASS | [Recorded results](2026-10-28/firmware-retest-results.txt) | Soil labels and Pump ON/OFF output during and after watering. |
| FR-06 / VER-06 | PASS | [Recorded results](2026-10-26/backend-results.txt) | Message validation and INSERT parameters; firmware payload checks also appear in the 28 October log. |
| FR-07 / VER-07 | PASS | [Recorded results](2026-10-26/dashboard-results.txt) | Fixture database saving and duplicate suppression; backend invalid-input checks support rejection. Fixture uniqueness is not a deployed-schema claim. |
| FR-08 / VER-08 | PASS | [Recorded results](2026-10-26/dashboard-results.txt) | 14 panel queries in both modes and three-cycle history. |
| FR-09 / VER-09 | PASS | [Recorded results](2026-10-26/dashboard-results.txt) | Air, water, null and 30-second receipt-age rules; acquisition freshness limitation retained. |
| FR-10 / VER-10 | PASS | [Recorded results](2026-10-28/firmware-retest-results.txt) | Local operation despite injected message-queue errors; physical user acceptance is VAL-05. |
| FR-11 / VER-11 | PASS monitoring; FAIL warnings | [Recorded results](2026-10-26/dashboard-results.txt) | Light storage/query handling and absent warning rules; conversion/payload coverage also in firmware/backend runs. |

## Validation traceability

VAL-01, VAL-02, VAL-03, VAL-04 and VAL-05 are PASS based on the [author’s caretaker self-test confirmation and findings](2026-10-28/caretaker-validation.md). They are not derived from the automated harness. FR-12/13 are not assessed; FR-14 through FR-21 are excluded.

## Reconciliation notes

- FR-05’s historical FAIL was superseded by the 27 October display fix and 28 October retest.
- Two-second watering and 60-second cooldown logs belong to earlier source versions. The current report correctly uses five seconds and ten minutes.
- Expected validation outcomes in historical notes were superseded by the author’s confirmation that all validation tasks passed.
- VAL-03 now assesses qualitative acceptance of routine watering, not a measured target volume. VAL-05 assesses recovery of monitoring, not acquisition-time verification.
- The raw PostgreSQL transcript and 27 October build log are absent from the supplied package. Broken links were removed; summaries were not converted into substitute raw logs.
- Retained test outputs, source fingerprints, scripts and SQL fixtures were preserved unchanged. Current source hashes were checked against the 28 October record; all five files match. No new execution was needed because the tested source is unchanged.

Historical context: [26 October](2026-10-26/README.md), [27 October](2026-10-27/README.md). Current evidence: [28 October](2026-10-28/README.md).
