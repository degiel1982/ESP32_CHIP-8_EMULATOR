#ifndef CHIP8_H
#define CHIP8_H

#include <Arduino.h>
#include "chip8_core.h"

#ifdef SSD1306OLED
    #include "ssd1306oled.h"
#endif

/**
 * @class chip8
 * @brief High-level interface for managing the CHIP-8 emulator.
 *
 * This class integrates the core CHIP-8 emulator logic with optional OLED display support.
 * It provides an easy-to-use API for initializing the emulator, loading ROMs, and running the main loop.
 */
class chip8 {
public:
    /**
     * @typedef emulator_loop_callback
     * @brief Defines a callback type for executing custom logic during each emulator loop.
     */
    typedef void (*emulator_loop_callback)();

    /**
     * @brief Initializes necessary hardware or peripherals.
     *
     * If SSD1306OLED support is enabled, this function initializes the OLED display.
     */
    void setup() {
    #ifdef SSD1306OLED
        #if SSD1306OLED != 0x3C && SSD1306OLED != NULL
            oled.setup(SSD1306OLED);  ///< Initialize OLED with custom I2C address.
        #else
            oled.setup();             ///< Initialize OLED with default settings.
        #endif
    #endif
    }

    /**
     * @brief Loads a ROM and runs the CHIP-8 emulator loop.
     *
     * This function handles ROM loading, initializes the emulator if needed,
     * and continuously runs the main loop of the emulator.
     *
     * @param rom Pointer to the ROM data.
     * @param rom_size Size of the ROM data in bytes.
     * @param loop_callback Optional user-defined callback function to be executed during each emulator loop.
     * @param enable_hwt Set to true to enable hardware timers (uses 2 of the 4 ESP32 timers).
     * @return True if the emulator is running successfully; false otherwise.
     */
    bool play_game(const uint8_t* rom, size_t rom_size, emulator_loop_callback loop_callback = nullptr, bool enable_hwt = false) {
        if (!chip8_core::getInstance().is_init_and_ready()) {
            // Load the ROM and initialize the emulator.
            chip8_core::getInstance().load_rom(rom, rom_size);

            // Enable hardware timers if requested.
            if (enable_hwt) {
                chip8_core::getInstance().enable_hardware_timers();
            }

            // Start the emulator; return false if it fails to start.
            if (!chip8_core::getInstance().start()) {
                return false;
            }
        } else {
            // If the emulator is already running, execute the main loop.
            if (chip8_core::getInstance().is_running()) {
                chip8_core::getInstance().loop();

            #ifdef SSD1306OLED
                oled.draw();  ///< Update the OLED display if enabled.
            #endif

                if (loop_callback != nullptr) {
                    loop_callback();  ///< Execute the user-defined loop callback, if provided.
                }
            } else {
                return false;  ///< Emulator is not running.
            }
        }

        return true;  ///< Emulator is running successfully.
    }

#ifdef SSD1306OLED
    /**
     * @brief Retrieves the OLED display instance for external use.
     *
     * This function provides access to the OLED instance so that external code
     * can directly interact with the display.
     *
     * @return Reference to the `ssd1306oled` instance.
     */
    ssd1306oled& get_display() {
        return oled;
    }
#endif

private:
#ifdef SSD1306OLED
    ssd1306oled oled;  ///< Instance of the OLED display handler.
#endif
};

#endif  // CHIP8_H
