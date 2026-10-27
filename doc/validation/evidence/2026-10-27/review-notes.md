# Verification review and pending display fix — 27 October 2026

The original requirements were reread from `doc/analysis/Greenhouse_Analysis_and_Advice_Revised.odt`. No application code was changed in this review. The executed evidence remains the 26 October run; this review does not claim new physical or user tests.

## Requirement scope

- **FR-03:** The requirement is to water dry soil when enough water is available and stop at the configured limit. The existing host evidence supports a **PASS for software timeout and cooldown decisions**. The earlier added criterion, physical ON time always at most 2.000 seconds, has not been demonstrated. A deliberately late host check stopped at 2007 ms. The revised criterion explicitly assesses the cooperative control check, and the timing limitation remains visible; this is not a claim of a hard physical cutoff.
- **FR-04:** The requirement is to block watering with low water or missing required readings, including startup. The existing host evidence supports a **PASS for reported missing/invalid readings and low-water handling**. It does not establish detection of every electrical disconnection: a disconnected input returning an in-range value could be accepted, and the host run showed ADC 4095 allowing watering. This limitation remains visible.
- **FR-05:** Current code does not update the OLED while watering. **FAIL remains** until the proposed fix below is applied and checked. The user asked for instructions, so the source was left unchanged.
- **FR-11:** A relative 0–100% light reading is implemented in conversion, telemetry, storage and display paths. Those software paths can be assessed separately from the absent warning. The 0–100% scale is not a plant-specific acceptable-light range. The warning subrequirement remains **FAIL**, while tested monitoring components remain **PASS**. FR-11 is a Should Have and does not block completion of the Must Haves by itself.
- **FR-12 and FR-13:** Could Haves; not assessed in this report.
- **FR-14 through FR-21:** Won't Haves; excluded from testing.

## How to fix FR-05

File: `greenhouse/main/greenhouse.c`, immediately after the outer-loop `updateWatering(&readings, &pump_on);` call (line 753 in the reviewed version), before `while (pump_on)`.

Insert the conditional display call shown below:

```c
updateWatering(&readings, &pump_on);

if (pump_on) {
    displayResults(&oled, &readings);
}

while (pump_on) {
    // Existing control loop stays here.
}
```

`updateWatering()` has just set the global pump state. The additional display call can therefore show `Pump: ON` and, with valid sensors, `WATERING`. Keep the existing display call after the watering loop to show the stopped state. Updating once on entry avoids redrawing the full display on every 50 ms control iteration. When an air/light fault is active, the existing warning priority can still show `SENSOR FAULT`, while the pump line indicates ON.

**Timing limitation:** OLED writes use I2C and take time. The next timeout check runs after this display call. This small fix does not guarantee a hard two-second physical cutoff. Check actual pump timing after applying it; a strict independent cutoff would require a separate design change.

## Checks after applying the fix

1. Extend the existing host display adapter to retain the texts written while the pump GPIO is ON. The full main-loop case should assert `Pump: ON` and `WATERING` during a normal run, and `Pump: OFF` after stopping. Add a case confirming fault-warning priority remains correct during watering.
2. Run the timeout, cooldown, low-water and missing-reading checks again. Do not delete the late-check or accepted-ADC-rail observations.
3. Build with `ninja -C greenhouse/build`.
4. With the assembled controller available, observe the OLED during a real cycle and measure pump ON time. Record that result separately from the host result.

The historical host harness currently asserts that no display write occurs while pumping. After applying the fix, that assertion must be replaced with meaningful positive display assertions. Preserve the historical log rather than overwriting it with a retest of changed code.

## Hypothetical stakeholder walkthrough

The revised Word report uses predicted stakeholder outcomes, not an invented participant. PASS* means the interface and recorded logic support an expected successful task; FAIL* marks a usability concern or insufficient evidence for acceptance. No task completion times, delivered water volumes or stakeholder feedback have been invented. Warning response and history interpretation are predicted passes; reading clarity, useful water dose and acquisition freshness after recovery remain concerns.
