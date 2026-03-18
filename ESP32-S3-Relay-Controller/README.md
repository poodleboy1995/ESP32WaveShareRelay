# ESP32-S3 Relay Controller (6CH)

This project is an ESP32-S3 firmware for the Waveshare 6-channel relay board, designed to control grow-room style devices (light, water, pump, spray, and two auxiliary outputs) from a built-in web interface.

It combines manual control, timed outputs, automatic light schedules, and sensor-driven safety logic for pump and water handling.

## What this project does

- Runs a local web server on the ESP32 (port 80)
- Controls 6 relay channels:
	- CH1: Light
	- CH2: Water
	- CH3: Pump
	- CH4: Spray
	- CH5: AUX1
	- CH6: AUX2
- Supports instant ON/OFF commands from a browser
- Supports timed runs for water, pump, spray, AUX1, and AUX2
- Includes automatic light modes:
	- VEG (18h ON / 6h OFF)
	- FLOWER (12h ON / 12h OFF)
	- CUSTOM (user-set ON hours per 24h)
- Includes pump auto-stop logic using a level sensor with debounce and max-runtime failsafe
- Includes water latch logic and manual hardware override inputs

## Hardware assumptions

- Board: Waveshare ESP32-S3-Relay-6CH
- Relay GPIO map:
	- CH1 = GPIO1
	- CH2 = GPIO2
	- CH3 = GPIO41
	- CH4 = GPIO42
	- CH5 = GPIO45
	- CH6 = GPIO46
- Sensor / control input pins:
	- Pump level sensor: GPIO4
	- Water OFF input: GPIO5
	- Water manual OFF override: GPIO40
	- Water manual ON override: GPIO39

## Safety and control behavior

- Pump auto-stop:
	- If low level is detected and stable (debounced), pump is stopped.
	- When low-level stop occurs, water output is latched ON.
	- A max pump runtime failsafe also stops the pump.
- Water control priority:
	1. Manual OFF input (highest priority)
	2. Manual ON input
	3. Normal timer/latch logic

## Basic setup

1. Open `config.h`.
2. Set your WiFi credentials:
	 - `ssid`
	 - `password`
3. Build and flash to your ESP32-S3 board.
4. Open Serial Monitor at 115200 baud.
5. Note the printed IP address and open it in a browser.

## HTTP endpoints (high level)

- Main UI: `/`
- State and status:
	- `/state`
	- `/status`
	- `/lightmeta`
	- `/watermeta`
	- `/sensor`
- Device controls:
	- `/light/on`, `/light/off`, `/light/veg`, `/light/flower`, `/light/custom`
	- `/water/on`, `/water/off`
	- `/pump/on`, `/pump/off`
	- `/spray/on`, `/spray/off`
	- `/aux1/on`, `/aux1/off`
	- `/aux2/on`, `/aux2/off`
- Timed run endpoint:
	- `/settimer?dev=<water|pump|spray|aux1|aux2>&sec=<1..86400>`

## Notes

- Relay active level is currently set so ON writes `HIGH`. If your board is inverted, adjust relay logic in `helpers.h` (`relayWrite` / `relayRead`).
- This firmware is intended for local network use.
