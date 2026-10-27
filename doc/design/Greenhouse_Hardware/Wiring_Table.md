# Greenhouse wiring table

Reviewed **26 October 2026**, using the current KiCad schematic/netlist, firmware and parts inventory, including uncommitted project changes. The existing slide-sized version is [Wiring_Table.drawio](Wiring_Table.drawio); it has not been updated during this review.

**GPIO numbers are signal names. U1/Jx pin numbers below belong to the KiCad symbols, not a verified physical header numbering scheme.** Match the actual board and module silkscreen before using these numbers to assemble hardware. This is a documentation comparison; installed wiring, voltages and power delivery were not measured.

## Component wiring overview

| Component | Power connection | Ground connection | Signal connection(s) to ESP32 | Review result |
|---|---|---|---|---|
| ESP32-DevKitC V4 / ESP32-WROOM-32D (U1, per parts list) | Micro-USB power/programming cable listed in inventory | Common GND with all modules | Controller for the connections below | Installed power arrangement unverified |
| M5Stack Watering Unit U101: soil sensor and integrated pump (J1) | 5V → J1 pin 2 / red wire | GND → J1 pin 1 / black wire | Analog output, J1 pin 4 / white → GPIO33; PUMP_EN, J1 pin 3 / yellow → GPIO16 | Soil signal connected; pump-enable net missing |
| DHT11 temperature/humidity module (J3) | 3V3 → J3 pin 1 | GND → J3 pin 3 | DATA, J3 pin 2 ↔ GPIO17 | Connected |
| Reservoir water-level sensor (J4) | 3V3 → J4 pin 2 | GND → J4 pin 1 | Analog signal, J4 pin 3 → GPIO35 | Connected; schematic says K-0135, inventory says Solu SL067; confirm actual module |
| Photoresistor + LM393 light module (inventory) | Supply not specified in schematic; confirm module rating | Common GND required; absent from schematic | AO → GPIO34; DO is unused by firmware | Entire module missing from schematic; physical pin order unverified |
| 0.96-inch SSD1306 I2C OLED, 128 × 64 (J2) | 3V3 → J2 pin 1 | GND → J2 pin 2 | SDA, J2 pin 4 ↔ GPIO21; SCL, J2 pin 3 ← GPIO22 | Connected; no separate reset wire used by firmware |
| Regulated 5V 2.5A pump supply and barrel-jack terminal adapter (inventory) | Intended for the watering unit's 5V input | Common GND with watering unit and ESP32 | No GPIO connection | Supply/adapter not represented explicitly in schematic |

The inventory identifies the light module but does not establish its exact board variant or supply/output voltage. GPIO34 is used as an analog input, so connect **AO**, not the LM393 module's digital **DO** output. The OLED initialization passes `-1` for reset; `CONFIG_RESET_GPIO=15` in `sdkconfig` does not add a GPIO15 connection to this application.

Tubing, reservoir, plant, pot, tray and soil have no electrical wiring. The breadboard and jumper wires distribute the connections above.

## Signal connections

| Function / component | ESP32 signal | U1 symbol pin | Module connector | Direction / interface | Schematic status |
|---|---|---:|---|---|---|
| Soil moisture, M5Stack U101 (J1) | GPIO33 | 8 | J1 pin 4, analog output (white Grove wire) | Sensor to ESP32; ADC1 channel 5 | Connected |
| Pump enable, M5Stack U101 (J1) | GPIO16 | 27 | J1 pin 3, PUMP_EN (yellow Grove wire) | ESP32 to unit; HIGH = ON, LOW = OFF | **Missing:** J1 pin 3 and U1 pin 27 are separate unconnected nets |
| DHT11 data (J3) | GPIO17 | 28 | J3 pin 2 | Digital DHT data | Connected |
| K-0135 reservoir sensor (J4) | GPIO35 | 6 | J4 pin 3, analog signal | Sensor to ESP32; ADC1 channel 7 | Connected |
| Light sensor | GPIO34 | 5 | Analog output; module connector unspecified | Sensor to ESP32; ADC1 channel 6 | **Missing:** module absent; U1 pin 5 unconnected |
| SSD1306 OLED SDA (J2) | GPIO21 | 33 | J2 pin 4 | I2C data | Connected |
| SSD1306 OLED SCL (J2) | GPIO22 | 36 | J2 pin 3 | I2C clock | Connected |

The ADC channels are configured in `initSensors()` for ADC1, 12-bit reads and `ADC_ATTEN_DB_12`. The firmware uses the ADC channel constants for analog acquisition; the GPIO definitions document the corresponding intended pins. The OLED is configured as 128 x 64.

## Power and ground connections

| Component | Positive supply in schematic | Ground in schematic | Notes |
|---|---|---|---|
| ESP32-DevKitC (U1) | U1 pin 1 = +3V3; U1 pin 19 = +5V | U1 pins 14, 32 and 38 | The drawing does not establish the installed power-source arrangement. |
| M5Stack U101 (J1) | J1 pin 2 to +5V (red Grove wire) | J1 pin 1 to GND (black Grove wire) | The +5V net also reaches U1 pin 19. Pump supply and GPIO16 enable are different connections. |
| SSD1306 OLED (J2) | J2 pin 1 to +3V3 | J2 pin 2 to GND | Check physical module pin order against its markings. |
| DHT11 module (J3) | J3 pin 1 to +3V3 | J3 pin 3 to GND | J3 pin 2 carries data. |
| K-0135 reservoir sensor (J4) | J4 pin 2 to +3V3 | J4 pin 1 to GND | J4 pin 3 carries the analog signal. |
| Light sensor | Not represented | Not represented | Module identity, supply and connector pins remain undocumented. |

J1-J4 share the same GND net with U1. The schematic's shared +5V net does not by itself establish how the physical pump is powered or whether its supply can support the load. Reconcile the intended pump supply with the assembled circuit before marking the hardware design complete.

## Sources and remaining schematic work

- [Current KiCad schematic](Greenhouse_Hardware.kicad_sch): exported with `kicad-cli sch export netlist` during this review; schematic left unchanged.
- [Firmware](../../../greenhouse/main/greenhouse.c): pin/channel definitions, `initSensors()`, `initPump()` and `initOLED()`.
- [Parts inventory](../../analysis/parts.ods): controller variant, light module, water-level sensor name, USB cable and external pump supply.
- [M5Stack Unit Watering official PinMap](https://docs.m5stack.com/en/unit/Unit_Watering): black = GND, red = 5V, yellow = PUMP_EN, white = analog output. Wire colors apply to the specified Grove cable; verify the actual assembly.

Complete the missing **GPIO16 to J1 PUMP_EN** net, represent the **light sensor and its GPIO34/supply/ground connections**, resolve **K-0135 versus Solu SL067**, and confirm the installed power arrangement. In particular, the schematic ties J1's 5V input to the ESP32's 5V pin, while the inventory lists both USB power and a separate pump supply; it does not document how those sources are arranged. These are documented gaps, not changes made to the schematic in this task.
