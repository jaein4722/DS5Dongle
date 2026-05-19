# Overclock Profile TODO

DS5Dongle currently applies voltage and system clock at boot with compile-time values:

- Voltage: `VREG_VOLTAGE_1_20`
- Pico 2 W clock: `320 MHz`
- Pico W clock: `200 MHz`

Future tray-controlled overclock support should be implemented as a separate boot profile, not as part of the existing web config body.

## Proposed Design

- Add a separate packed flash-backed overclock profile.
- Add dedicated host reports, for example:
  - `0xFB`: read overclock profile
  - `0xFC`: write/reset/reboot overclock profile commands
- Load the overclock profile early in `main()`, before USB, Bluetooth, audio, and regular config initialization.
- Apply only whitelisted values.
- Treat changes as "apply on next reboot", not live runtime changes.

## Safety Notes

- Do not allow arbitrary voltage or frequency input.
- Keep a reset-to-default command available.
- Keep defaults conservative:
  - Pico 2 W: `1.20 V / 320 MHz`
  - Pico W: current Pico W defaults
- Tray UI should expose this under an Advanced section with a warning that invalid settings may require manual BOOTSEL recovery.
