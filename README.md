## OpenVibe-Embedded

Small ESP32 firmware for OpenVibe devices. The firmware exposes a BLE service to receive Wi‑Fi credentials and a statistics characteristic that notifies a connected BLE client with device status JSON.

## High Level
- BLE service used to receive Wi‑Fi credentials (write) and to notify device statistics (notify/read).
- Connects to Wi‑Fi when credentials are received and reports IP, MAC, uptime and basic mock telemetry via BLE notifications.

## Tech Stack & Versions
- PlatformIO (project config: `platformio.ini`)
- Platform: Espressif 32 (platform version used during last build: 6.12.0)
- Framework: Arduino (Arduino-ESP32, release used in build: framework-arduinoespressif32 @ 3.20017.241212+sha.dcc1105b)
- Toolchain: xtensa-esp32 @ 8.4.0+2021r2-patch5
- esptool.py (tool-esptoolpy 4.9.0)
- Libraries:
  - ArduinoJson @ 7.4.2
  - WebSockets @ 2.7.1
  - ESP32 BLE Arduino (used from PlatformIO library registry; typically 2.0.0)

These versions reflect the environment used during the most recent successful build. Your local PlatformIO installation may use slightly different package versions; PlatformIO will fetch compatible packages automatically.

## Project Layout
- `platformio.ini` — PlatformIO configuration (board, libs, upload settings)
- `src/main.cpp` — application entry: BLE and Wi‑Fi wiring, main loop, stats publishing
- `src/ble/BLECallbacks.h` & `src/ble/BLECallbacks.cpp` — BLE server and characteristic handlers (Wi‑Fi credentials parsing, connect logic)
- `include/types/device_stats.h` — `DeviceStats` struct and shared `deviceConnected` declaration
- `lib/` — project library area (unchanged)

## BLE Details
- Service UUID: `ec2e0883-782d-433b-9a0c-6d5df5565410`
- Wi‑Fi config characteristic (write): `c2433dd7-137e-4e82-845e-a40f70dc4a8d`
  - Expects a JSON payload: `{"ssid":"<ssid>","password":"<password>"}`
- Stats characteristic (notify/read): `c2433dd7-137e-4e82-845e-a40f70dc4a8e`
  - Notification payload (example):
    ```json
    {
      "uptimeMillis": 123456,
      "intensity": 50,
      "battery": 100,
      "isCharging": false,
      "isBluetoothConnected": true,
      "isWifiConnected": true,
      "ipAddress": "192.168.0.244",
      "macAddress": "4C:11:AE:65:A2:E8",
      "version": "1.0.0"
    }
    ```

## Hardware required
- ESP32 development board (ESP32-D0WD core — generic "ESP32 Dev Module" works well)
- Good-quality USB data cable (power-only cables will not flash)
- If auto-reset/auto-boot doesn't work on your board, you'll need to use the BOOT (GPIO0) and EN (RESET) buttons during flashing.
- Optional: USB‑to‑UART adapter with RTS/DTR lines (for consistent auto-reset) or use the board's built-in USB if present.

## How to build and flash
1. From VS Code: use the PlatformIO extension build & upload buttons.
2. From the CLI (uses the project's PlatformIO venv if you want to match the environment used here):

```fish
# Build
~/.platformio/penv/bin/platformio run -e esp32dev

# Upload (explicit port can be overridden by platformio.ini)
~/.platformio/penv/bin/platformio run -e esp32dev -t upload --upload-port /dev/ttyUSB0

# Open serial monitor at 115200
~/.platformio/penv/bin/platformio device monitor -e esp32dev --port /dev/ttyUSB0 --baud 115200
```

If you use the PlatformIO CLI installed globally, you can replace `~/.platformio/penv/bin/platformio` with `platformio`.

## Troubleshooting
- Permission denied opening `/dev/ttyUSB0`: add your user to the appropriate group:

```fish
# On most Debian/Ubuntu: dialout
sudo usermod -a -G dialout $USER
# On Arch or systems that use uucp for serial devices
sudo usermod -a -G uucp $USER
```

Log out and log back in for group changes to take effect.

- If uploads fail with "No serial data received" or similar:
  - Try a different USB cable (must be data-capable).
  - Try another USB port (rear ports on desktops are preferable).
  - Try lowering upload speed in `platformio.ini` (this project uses 115200 by default).
  - Try manual BOOT/EN procedure: hold BOOT (GPIO0), press and release EN (reset), then release BOOT right before or when the upload starts.
  - Make sure no serial monitor or other process is holding the port (kill `platformio home` or any `minicom/screen` instance).

## Development Notes
- The code was modularized so BLE callbacks live under `src/ble/`, and the `DeviceStats` type is in `include/types/` so it can be included across files.
- There are some ArduinoJson warnings about `DynamicJsonDocument` deprecation; consider switching to `StaticJsonDocument` or `JsonDocument` with a statically-sized buffer if you want to remove warnings.

## License
This repository includes an MIT license (see `LICENSE` in the project root).