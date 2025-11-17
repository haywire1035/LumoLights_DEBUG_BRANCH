# LumoLights

HomeSpan-inspired, Arduino-based firmware for driving a custom RGBW LED fixture with Apple HomeKit compatibility, a serial command-line interface, and EEPROM-backed configuration persistence. The sketch targets ESP32-class boards and wraps hardware access, gradient generation, and storage utilities behind clean header-only modules.

## Project Highlights
- **Modular LED engine** – `050_HAL.h`, `100_LED_LINKER.h`, and `110_LED_CORE.h` split the hardware profiles, hardware access, and logical rendering so you can swap pinouts, driver types, and pixel counts without touching animation logic.
- **Rich gradient modes** – The core exposes linear, padded, single-color, midpoint split, and edge/center gradient strategies, including smooth interpolation options for the middle band.
- **Interactive console** – `200_CONSOLE.h` implements a newline-delimited serial interface with `HELP`, `SET`, `TOGGLE`, and `SAVE` commands for live tuning without reflashing.
- **Persistent settings** – `300_SETTINGS.h` provides versioned blob storage built on the ESP32 `Preferences` API, ensuring console and LED configuration structs survive resets while honoring `CONFIG_VERSION` compatibility checks.
- **Diagnostics-friendly logging** – `000_LOG.h` centralizes Serial logging helpers that can be toggled at compile time.

## Hardware & Software Requirements
- ESP32 development board with native USB serial.
- RGBW WS2812 or clock/data WS2801 strips controlled through compile-time defines in `050_HAL.h`. Pick exactly one of `HAL_CONFIG_SINGLE_WS2812`, `HAL_CONFIG_DUAL_WS2812`, `HAL_CONFIG_SINGLE_WS2801`, or `HAL_CONFIG_DUAL_WS2801` (defaults to single WS2812) and override the pin/count macros if your lamp wiring differs.
- Arduino framework toolchain (Arduino IDE, PlatformIO, or `arduino-cli`). The project currently expects the Adafruit NeoPixel library and ESP32 board support package to be installed.

### Selecting a hardware configuration
`050_HAL.h` exposes four small `#define` switches so you can match the firmware to your strip without editing the animation logic:

| Define | Purpose | Default pins/counts |
| --- | --- | --- |
| `HAL_CONFIG_SINGLE_WS2812` | One RGBW NeoPixel strip on a single data pin. | `HAL_SINGLE_WS2812_PIN=3`, `HAL_SINGLE_WS2812_LED_COUNT=69` |
| `HAL_CONFIG_DUAL_WS2812` | Two RGBW NeoPixel strips chained logically. | `HAL_DUAL_WS2812_PIN_ONE=3`, `HAL_DUAL_WS2812_PIN_TWO=4`, `HAL_DUAL_WS2812_COUNT_ONE=69`, `HAL_DUAL_WS2812_COUNT_TWO=69` |
| `HAL_CONFIG_SINGLE_WS2801` | One RGB WS2801 strip with data/clock pins. | `HAL_SINGLE_WS2801_DATA_PIN=2`, `HAL_SINGLE_WS2801_CLOCK_PIN=3`, `HAL_SINGLE_WS2801_LED_COUNT=69` |
| `HAL_CONFIG_DUAL_WS2801` | Two RGB WS2801 strips sharing the logical buffer. | `HAL_DUAL_WS2801_DATA_PIN_ONE=2`, `HAL_DUAL_WS2801_CLOCK_PIN_ONE=3`, `HAL_DUAL_WS2801_DATA_PIN_TWO=4`, `HAL_DUAL_WS2801_CLOCK_PIN_TWO=5`, `HAL_DUAL_WS2801_COUNT_ONE=31`, `HAL_DUAL_WS2801_COUNT_TWO=31` |

Override any of the per-config pin/count macros before including `050_HAL.h`, or pass them through your build system (e.g., PlatformIO `build_flags`). Only the functions for the selected configuration are compiled, keeping the firmware lean for each lamp variant.

## Repository Layout
| File | Purpose |
| --- | --- |
| `LumoLights_Smarthome.ino` | Main sketch: initializes Serial, console, LED system, and persistence loop. |
| `050_HAL.h` | Hardware abstraction layer: compile-time LED configurations, pin/count defines, and hardware helpers. |
| `100_LED_LINKER.h` | Hardware binding for LED strips plus the public `LED::` API. |
| `110_LED_CORE.h` | Gradient math, staging buffers, and color/pixel transforms. |
| `200_CONSOLE.h` | Serial console parsing, HELP text, and handlers for SET/TOGGLE commands. |
| `300_SETTINGS.h` | Preferences-backed persistence helpers and `SETTINGS::InitAndLoadReport()`. |
| `999_DEVICE.h`, `999_CCT.h` | Optional HomeSpan device definitions (currently commented out in the sketch). |
| `_CHANGELOG.h`, `_TODO.h`, `_KNOWN_BUGS.h` | Project docs for release tracking and maintenance.

## Building & Uploading
1. **Install dependencies**
   - Add the ESP32 board support package to the Arduino IDE or configure your PlatformIO environment.
   - Install the [Adafruit NeoPixel](https://github.com/adafruit/Adafruit_NeoPixel) library.
   - Install the [Adafruit WS2801](https://github.com/adafruit/Adafruit-WS2801-Library) library if you plan to use clocked strips.
2. **Open the sketch**
   - Launch `LumoLights_Smarthome.ino` in your IDE. Ensure `ENABLE_COMMAND_LINE_INTERFACE` and `DEBUG_SERIAL` are set as desired near the top of the file.
3. **Select board & port**
   - Choose the matching ESP32 board profile and the USB serial port.
4. **Upload**
   - Compile and flash the sketch. Open a Serial Monitor at 115200 baud to observe the banner output and interact with the CLI.

## Serial Console Quick Reference
The console reads newline-delimited commands. Type `HELP` to print the full guide.

| Command | Description |
| --- | --- |
| `SET COLOR <ONE|TWO> <R> <G> <B> <W>` | Stage gradient endpoint colors for the active gradient. |
| `SET BRIGHTNESS <0-255>` | Queue a brightness change with smoothing handled by `LED::CORE::Fade()`. |
| `SET PARAM <NAME> <VALUE>` | Adjust timing (`PROCESSING_INTERVAL`, `EFFECT_INTERVAL`), fade increments, and gradient padding fields. |
| `SET GRADIENT <MODE>` | Switch gradient behavior among `LINEAR`, `LINEAR_PADDING`, `SINGLE_COLOR`, `MIDPOINT_SPLIT`, or `EDGE_CENTER`. |
| `TOGGLE <FLAG>` | Toggle booleans such as gradient inversion, RGBW conversion, or effect enablement. |
| `SAVE` | Force an EEPROM write via `SETTINGS::SaveStructPref()`. |

## Persistence Workflow
`SETTINGS::InitAndLoadReport()` is called during `setup()` to load both console and LED configs. Every config struct maintains `changeCounter` and `lastModifiedMs` metadata; calling `SETTINGS::Update()` from `loop()` debounces writes so EEPROM endurance is preserved. When `CONFIG_VERSION` changes, previously saved blobs are ignored, ensuring incompatible layouts do not corrupt live state. Physical LED counts now come from the selected HAL configuration, so persisted settings stay portable across lamp variants.

## Development Tips
- Keep new public APIs near the top of each header per the contributor guidelines found in `_TODO.h` and code comments.
- Increment `SKETCH_VERSION` for every committed change and document updates in `_CHANGELOG.h`.
- Use `LED::CORE::MarkChangeInConfig()` whenever you modify config fields so the persistence layer knows to flush updates.

Happy hacking!
