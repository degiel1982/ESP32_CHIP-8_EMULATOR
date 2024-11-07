# CHIP-8 Emulator Library for Arduino

This library provides a high-level interface for managing a CHIP-8 emulator on Arduino-based platforms. It integrates the core CHIP-8 emulator logic with optional OLED display support, allowing you to load and run CHIP-8 ROMs with ease.

## Table of Contents

- [Introduction](#introduction)
- [Features](#features)
- [Requirements](#requirements)
- [Installation](#installation)
- [Getting Started](#getting-started)
- [Class Reference](#class-reference)
  - [`chip8` Class](#chip8-class)
    - [`setup()` Method](#setup-method)
    - [`play_game()` Method](#play_game-method)
    - [`get_display()` Method](#get_display-method)
- [Example Usage](#example-usage)
- [Additional Notes](#additional-notes)
- [License](#license)

## Introduction

The `chip8` library simplifies the process of running CHIP-8 games on Arduino-compatible boards. It handles ROM loading, emulator initialization, and the main execution loop. With optional OLED display support, you can also render the emulator's output on an SSD1306 OLED screen.

## Features

- Easy-to-use API for running CHIP-8 games
- Optional SSD1306 OLED display support
- Support for hardware timers on ESP32
- Callback mechanism for extending emulator functionality

## Requirements

- Arduino IDE or compatible development environment
- Supported Arduino board (e.g., ESP32)
- Optional: SSD1306 OLED display (if using display features)

## Installation

1. **Clone or Download the Library**: Obtain the `chip8` library files and include them in your Arduino project.

2. **Include Dependencies**: Make sure to include any required libraries such as `Wire.h` and `Adafruit_SSD1306.h` if you're using the OLED display.

## Getting Started

To start using the `chip8` library, follow these steps:

1. **Include the Library**: Include the `chip8.h` header file in your sketch.

   ```cpp
   #include "chip8.h"
   ```

2. **Instantiate the Emulator**: Create an instance of the `chip8` class.

   ```cpp
   chip8 ch8;
   ```

3. **Initialize the Emulator**: Call the `setup()` method to initialize hardware and peripherals.

   ```cpp
   void setup() {
       ch8.setup();
   }
   ```

4. **Load and Run a ROM**: Use the `play_game()` method to load a ROM and run the emulator loop.

   ```cpp
   void loop() {
       const uint8_t* rom = /* Your ROM data */;
       size_t rom_size = /* Size of your ROM */;
       bool enable_hardware_timers = true;

       while (ch8.play_game(rom, rom_size, loop_extended, enable_hardware_timers)) {
           // Emulator is running
       }
   }
   ```

5. **Extend Functionality (Optional)**: Implement a callback function to execute custom logic during each emulator loop.

   ```cpp
   void loop_extended() {
       // Your custom code here
   }
   ```

## Class Reference

### `chip8` Class

The `chip8` class provides methods to manage the emulator's lifecycle, load ROMs, and integrate with optional hardware features like an OLED display.

#### **Public Methods**

- `void setup()`
- `bool play_game(const uint8_t* rom, size_t rom_size, emulator_loop_callback loop_callback = nullptr, bool enable_hwt = false)`
- `ssd1306oled& get_display()` (Only if SSD1306 OLED support is enabled)

#### **Typedefs**

- `typedef void (*emulator_loop_callback)()`: Defines a callback type for executing custom logic during each emulator loop.

---

#### `setup()` Method

```cpp
void setup();
```

**Description**: Initializes necessary hardware or peripherals required by the emulator. If SSD1306 OLED support is enabled, this function initializes the OLED display.

**Parameters**: None

**Returns**: `void`

**Usage Example**:

```cpp
void setup() {
    ch8.setup();
}
```

---

#### `play_game()` Method

```cpp
bool play_game(const uint8_t* rom, size_t rom_size, emulator_loop_callback loop_callback = nullptr, bool enable_hwt = false);
```

**Description**: Loads a ROM and runs the CHIP-8 emulator loop. This function handles ROM loading, initializes the emulator if needed, and continuously runs the main loop of the emulator.

**Parameters**:

- `rom`: Pointer to the ROM data.
- `rom_size`: Size of the ROM data in bytes.
- `loop_callback` (Optional): User-defined callback function to be executed during each emulator loop. Default is `nullptr`.
- `enable_hwt` (Optional): Set to `true` to enable hardware timers (uses 2 of the 4 ESP32 timers). Default is `false`.

**Returns**: `bool`

- `true` if the emulator is running successfully.
- `false` if the emulator failed to start or is not running.

**Usage Example**:

```cpp
void loop() {
    const uint8_t* rom = /* Your ROM data */;
    size_t rom_size = /* Size of your ROM */;
    bool enable_hardware_timers = true;

    while (ch8.play_game(rom, rom_size, loop_extended, enable_hardware_timers)) {
        // Emulator is running
    }
}
```

---

#### `get_display()` Method

```cpp
ssd1306oled& get_display();
```

**Description**: Retrieves the OLED display instance for external use. This function provides access to the OLED instance so that external code can directly interact with the display.

**Parameters**: None

**Returns**: Reference to the `ssd1306oled` instance.

**Usage Example**:

```cpp
#ifdef SSD1306OLED
Adafruit_SSD1306& oled_display = ch8.get_display().get_display();
oled_display.clearDisplay();
oled_display.setCursor(0, 0);
oled_display.print("Hello, OLED!");
oled_display.display();
#endif
```

---

## Example Usage

Below is an example of how to use the `chip8` library in an Arduino sketch. This example includes ROM selection and handling user input with buttons.

```cpp
#include <Arduino.h>
#include "chip8.h"
#include "roms.h" // Include your ROM data here

// Instantiate the CHIP-8 emulator
chip8 ch8;

// List of ROM names and data
const char* rom_names[] = {
    "Space Invaders",
    "Glitch Ghost",
};
const uint8_t* rom_data[] = {
    space_invaders,
    glitch_ghost,
};
const size_t rom_sizes[] = {
    sizeof(space_invaders),
    sizeof(glitch_ghost),
};

// Variables for ROM selection
size_t selected_rom_index = 0;
const size_t num_roms = sizeof(rom_names) / sizeof(rom_names[0]);

// GPIO pins for buttons
const int button_left_pin = 14;    // Left navigation button
const int button_right_pin = 12;   // Right navigation button
const int button_select_pin = 13;  // Select button

void setup() {
    // Initialize serial communication
    Serial.begin(115200);

    // Initialize the CHIP-8 emulator
    ch8.setup();

    // Configure button pins
    pinMode(button_left_pin, INPUT_PULLUP);
    pinMode(button_right_pin, INPUT_PULLUP);
    pinMode(button_select_pin, INPUT_PULLUP);

    // Display the initial ROM selection (if using OLED)
    display_rom();
}

void loop() {
    // Handle button presses for ROM selection
    if (digitalRead(button_left_pin) == LOW) {
        if (selected_rom_index > 0) {
            selected_rom_index--;
            delay(200); // Debounce delay
        }
    }
    if (digitalRead(button_right_pin) == LOW) {
        if (selected_rom_index < num_roms - 1) {
            selected_rom_index++;
            delay(200); // Debounce delay
        }
    }
    if (digitalRead(button_select_pin) == LOW) {
        delay(200); // Debounce delay

        // Load and run the selected ROM
        size_t rom_size = rom_sizes[selected_rom_index];
        const uint8_t* rom = rom_data[selected_rom_index];
        bool enable_hardware_timers = true;

        while (ch8.play_game(rom, rom_size, loop_extended, enable_hardware_timers)) {
            // Emulator is running
        }

        // Return to ROM selection after the game ends
        delay(500); // Prevent immediate re-selection
    }

    // Continuously display the current ROM selection (if using OLED)
    display_rom();
}

// Optional extended loop function
void loop_extended() {
    // Custom functionality during emulator execution
}

// Function to display ROM selection on OLED
#ifdef SSD1306OLED
void display_rom() {
    Adafruit_SSD1306& oled = ch8.get_display().get_display();
    oled.clearDisplay();
    oled.setTextSize(2);
    oled.setTextColor(SSD1306_WHITE);

    String rom_name = rom_names[selected_rom_index];
    oled.setCursor(0, 0);
    oled.print(rom_name);

    oled.display();
}
#endif
```

## Additional Notes

- **Hardware Timers**: Enabling hardware timers can improve emulator performance but uses additional hardware resources (e.g., 2 of the 4 timers on an ESP32).

- **OLED Display**: The `get_display()` method is only available if `SSD1306OLED` support is enabled. Make sure to define `SSD1306OLED` with the correct I2C address before including `chip8.h`.

  ```cpp
  #define SSD1306OLED 0x3C // Replace with your OLED's I2C address
  #include "chip8.h"
  ```

- **ROM Data**: You need to provide your own CHIP-8 ROM data. This can be done by including binary data in your sketch or reading from external storage.

- **Button Debouncing**: The example includes simple software debouncing using `delay()`. For more reliable input handling, consider implementing a proper debouncing algorithm.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
