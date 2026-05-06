# Build Notes

## PlatformIO Environments

Run these commands from `C:\Neurosense`:

```powershell
pio run -e esp8266_biometric_monitor
pio run -e esp32_sleep_stage_alarm
```

Environment summary:

| Environment | Board | Firmware |
| --- | --- | --- |
| `esp8266_biometric_monitor` | ESP8266 NodeMCU | `Biometric Monitor/Biometric_Monitor/Biometric_Monitor.ino` |
| `esp32_sleep_stage_alarm` | ESP32 Dev Module | `Sleep Stage Alarm/sleepstage_wake/sleepstage_wake.ino` |

The PlatformIO build uses wrapper files in `src/` so the original Arduino `.ino` files stay in place.

## Board Assumptions

- ESP8266 target: NodeMCU 1.0 / ESP-12E compatible board.
- ESP32 target: ESP32 Dev Module compatible board.
- MAX30100 library is vendored under `Biometric Monitor/lib/MAX30100_PulseOximeter/Arduino-MAX30100-master`.

## Known Warnings

- ESP8266 builds may show a Python `SyntaxWarning` from PlatformIO's ESP8266 `elf2bin.py`. This warning is from the tool package, not project firmware.
- Git may report LF-to-CRLF working-copy warnings on Windows.

## Tests

`tests/test_sleep_classifier.cpp` contains standalone classifier vectors. The current repo does not configure `pio test`.
