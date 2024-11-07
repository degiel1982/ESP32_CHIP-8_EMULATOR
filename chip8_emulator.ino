#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

// Define SSD1306 OLED I2C address
#define SSD1306OLED 0x3C
#define MENU_ENABLED

#include "chip8.h"
#include "roms.h"

// Instantiate the CHIP-8 emulator
chip8 ch8;

// List of ROM names available for selection
const char* rom_names[] = {
    "Space Invaders",
    "Glitch Ghost",
};

// Corresponding ROM data pointers
const uint8_t* rom_data[] = {
    space_invaders,
    glitch_ghost,
};

// Sizes of the ROMs in bytes
const size_t rom_sizes[] = {
    sizeof(space_invaders),
    sizeof(glitch_ghost),
};

#ifdef MENU_ENABLED

// Total number of ROMs available
const size_t num_roms = sizeof(rom_names) / sizeof(rom_names[0]);

// Index of the currently selected ROM
size_t selected_rom_index = 0;

// GPIO pins connected to navigation buttons
const int button_left_pin = 14;    // Left navigation button
const int button_right_pin = 12;   // Right navigation button
const int button_select_pin = 13;  // Select button

#endif

/**
 * @brief Arduino setup function, runs once at startup.
 */
void setup() {
    // Initialize serial communication for debugging purposes
    Serial.begin(115200);

    // Initialize the CHIP-8 emulator
    ch8.setup();

#ifdef MENU_ENABLED
    // Configure button pins as inputs with internal pull-up resistors
    pinMode(button_left_pin, INPUT_PULLUP);
    pinMode(button_right_pin, INPUT_PULLUP);
    pinMode(button_select_pin, INPUT_PULLUP);

    // Display the initial ROM selection screen
    display_rom();
#endif
}

#ifdef MENU_ENABLED
// Variable to keep track of scroll position (unused but can be extended for scrolling functionality)
size_t scroll_offset = 0;

/**
 * @brief Displays the currently selected ROM on the OLED screen.
 */
 #ifdef SSD1306OLED
void display_rom() {

    // Reference to the OLED display object from the CHIP-8 emulator
    Adafruit_SSD1306& oled1 = ch8.get_display().get_display();

    // Clear the display buffer
    oled1.clearDisplay();
    oled1.setTextSize(2);              // Set text size to 2x
    oled1.setTextColor(SSD1306_WHITE); // Set text color to white

    // Retrieve the name of the selected ROM
    String rom_name = rom_names[selected_rom_index];

    // Variables for splitting the ROM name into multiple lines
    const int maxCharsPerLine = 8; // Maximum characters per line (excluding spaces)
    const int maxLines = 3;        // Maximum number of lines to display
    String lines[maxLines];        // Array to hold the split lines
    int currentLineIndex = 0;      // Index of the current line being processed

    String currentLine = "";       // Temporary string to build each line
    int charCount = 0;             // Character count for the current line

    // Loop through each character in the ROM name to split it into lines
    for (int i = 0; i < rom_name.length(); i++) {
        char currentChar = rom_name[i];

        // Check for space character to handle word breaks
        if (currentChar == ' ') {
            if (!currentLine.isEmpty()) {
                // Save the current line and reset counters
                lines[currentLineIndex++] = currentLine;
                currentLine = "";
                charCount = 0;

                // Break if maximum number of lines is reached
                if (currentLineIndex >= maxLines) {
                    break;
                }
            }
            continue; // Skip adding the space to the line
        }

        // Add the character to the current line
        currentLine += currentChar;
        charCount++;

        // Start a new line if character limit is reached
        if (charCount >= maxCharsPerLine) {
            lines[currentLineIndex++] = currentLine;
            currentLine = "";
            charCount = 0;

            // Break if maximum number of lines is reached
            if (currentLineIndex >= maxLines) {
                break;
            }
        }
    }

    // Add any remaining text as the last line
    if (currentLineIndex < maxLines && currentLine.length() > 0) {
        lines[currentLineIndex++] = currentLine;
    }

    // Calculate vertical starting position to vertically center the text
    int totalLines = currentLineIndex;                   // Total number of lines to display
    int startY = (64 - (totalLines * 16)) / 2;           // 64 is the display height; 16 is the line height

    // Iterate over each line to display it on the screen
    for (int i = 0; i < totalLines; i++) {
        String lineText = lines[i];

        // Calculate horizontal position to center the text
        int16_t textWidth = lineText.length() * 12;      // Approximate character width at text size 2
        int16_t startX = (128 - textWidth) / 2;          // 128 is the display width

        oled1.setCursor(startX, startY + (i * 16));      // Set cursor position for the line
        oled1.print(lineText);                           // Print the line text
    }

    // Draw navigation arrows for left and right navigation
    oled1.fillTriangle(0, 32, 6, 26, 6, 38, SSD1306_WHITE);        // Left arrow
    oled1.fillTriangle(122, 26, 128, 32, 122, 38, SSD1306_WHITE);  // Right arrow

    // Display the ROM position indicator (e.g., "1/2") at the bottom-right corner
    oled1.setTextSize(1);  // Set text size to 1x for smaller text
    String rom_position = String(selected_rom_index + 1) + "/" + String(num_roms);
    oled1.setCursor(128 - (rom_position.length() * 6), 64 - 8);  // Adjust cursor for text width and display height
    oled1.print(rom_position);

    // Update the OLED display with the new content
    oled1.display();

}
#endif
#endif

/**
 * @brief Arduino main loop function, runs repeatedly after setup().
 */
void loop() {
#ifdef MENU_ENABLED
    // Continuously display the current ROM selection
    display_rom();

    // Handle left navigation button press (active LOW)
    if (digitalRead(button_left_pin) == LOW) {
        if (selected_rom_index > 0) {
            selected_rom_index--;  // Move selection to the previous ROM
            delay(200);            // Debounce delay to prevent multiple triggers
        }
    }

    // Handle right navigation button press (active LOW)
    if (digitalRead(button_right_pin) == LOW) {
        if (selected_rom_index < num_roms - 1) {
            selected_rom_index++;  // Move selection to the next ROM
            delay(200);            // Debounce delay to prevent multiple triggers
        }
    }

    // Handle select button press to load and play the selected ROM (active LOW)
    if (digitalRead(button_select_pin) == LOW) {
        delay(200); // Debounce delay to avoid accidental re-selection

        // Retrieve the size and data pointer of the selected ROM
        size_t rom_size = rom_sizes[selected_rom_index];
        const uint8_t* rom = rom_data[selected_rom_index];

        // Enable hardware timers for the emulator
        bool enable_hardware_timers = true;

        // Run the selected ROM using the CHIP-8 emulator
        while (ch8.play_game(rom, rom_size, loop_extended, enable_hardware_timers)) {
            // The emulator is actively running the game
        }

        // After the game ends, return to the ROM selection menu
        delay(500); // Small delay to prevent immediate re-selection
    }
#else
    // If MENU_ENABLED is not defined, automatically start the emulator with a default ROM
    size_t rom_size = rom_sizes[1];     // Default to the second ROM in the list
    const uint8_t* rom = rom_data[1];   // Pointer to the default ROM data

    bool enable_hardware_timers = true; // Enable hardware timers for the emulator

    // Run the default ROM using the CHIP-8 emulator
    while (ch8.play_game(rom, rom_size, loop_extended, enable_hardware_timers)) {
        // The emulator is actively running the game
    }
#endif
}

/**
 * @brief Additional functionality to be executed during the emulator's main loop.
 *
 * This function is intended to be passed as a callback to the emulator and can
 * include code for handling tasks such as input, sound, or other features that
 * need to be updated alongside the emulator's execution.
 */
void loop_extended() {
    // Placeholder for extended functionality during emulator execution
    // This function can be expanded to include additional features
}
