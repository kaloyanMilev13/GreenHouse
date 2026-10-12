# Greenhouse dashboard

Import `greenhouse-overview.json` through Grafana's **Dashboards → New → Import** page. Select the existing PostgreSQL data source for **Greenhouse PostgreSQL**, then import. This creates **Greenhouse Overview** with UID `greenhouse-overview`; it does not replace the original **Greenhouse Dashboard**. Reimporting this file later targets the same overview UID, so review any overwrite prompt.

The PostgreSQL data-source account needs SELECT access to `public.readings`. Credentials stay in the Grafana data source and are not included in this file.

The dashboard provides six latest-value cards, reading age, latest sample details, and four history charts. It refreshes every five seconds. The **Data mode** selector defaults to **Test** (`is_test = true`); switch it to **Real** when the firmware publishes actual readings with `test: false`.

Latest-value cards use the most recent row in the selected mode, independent of the dashboard time range. Null measurements display **Unknown**. Check **Reading age** before treating a value as current. Age turns red after 30 seconds, a prototype freshness threshold for the two-second publishing interval, not a definitive device-connectivity test.

History charts default to the last 15 minutes and use database receipt time. If charts are empty, start the broker, ESP32 publisher and Node.js backend, or select a time range containing stored readings. Test values produce flat lines. Real mode remains empty until real readings exist.

Light and soil moisture are relative percentages requiring firmware calibration. Reservoir status represents low-water detection, not volume. Pump status represents the firmware's command, not confirmed motor operation or water flow.

All 12 panel SELECT queries were checked against the local database in both Test and Real modes when this template was created. Visual rendering still needs verification after import into Grafana.
