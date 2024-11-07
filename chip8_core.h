#ifndef CHIP8_CORE_H
#define CHIP8_CORE_H

#include <Arduino.h>
#include "flag_manager.h"

// Timer intervals for CPU and GPU operations
constexpr uint32_t CPU_TIMER_INTERVAL = 2;   // CPU cycle interval in milliseconds
constexpr uint32_t GPU_TIMER_INTERVAL = 16;  // GPU cycle interval in milliseconds

/**
 * @class chip8_core
 * @brief Core class for the CHIP-8 emulator, implementing the CPU, GPU, and memory management.
 *
 * This class handles all aspects of CHIP-8 emulation, including memory, timers, input,
 * and display management. It follows a singleton pattern to ensure only one instance
 * exists during runtime.
 */
class chip8_core {

  private:

    /**
     * @brief Private constructor for singleton implementation.
     *
     * Ensures that only one instance of the chip8_core class can be created.
     */
    chip8_core() {}

    // Delete copy constructor and assignment operator to prevent copying
    chip8_core(const chip8_core&) = delete;
    chip8_core& operator=(const chip8_core&) = delete;

    uint8_t flag_registers[16];  ///< General-purpose flag registers

    // Memory and display buffers
    uint8_t RAM[4096];  ///< Main memory (4KB)
    uint8_t DISPLAYBUFFER[(32 * 64) / 8];  ///< Monochrome display buffer for 64x32 screen
    uint8_t dirty_flags[32][8] = {0};  ///< Tracks modified regions of the display buffer

    // Timer management
    unsigned long last_CPU_cycle;  ///< Timestamp of the last CPU cycle
    unsigned long last_GPU_cycle;  ///< Timestamp of the last GPU cycle

    uint8_t key_states[16] = {0};  ///< Stores the current state of the 16 CHIP-8 keys

    /**
     * @struct STRUCT_REGISTERS
     * @brief Represents the CPU registers used in CHIP-8.
     */
    struct STRUCT_REGISTERS {
      uint16_t PC;               ///< Program Counter
      uint16_t INDEX;            ///< Index Register
      alignas(2) uint16_t STACK[16];  ///< Call stack (16 levels deep)
      uint8_t SP;                ///< Stack Pointer
      uint8_t DELAYTIMER;        ///< Delay timer (counts down at 60Hz)
      uint8_t SOUNDTIMER;        ///< Sound timer (counts down at 60Hz)
      uint8_t V[16];             ///< General-purpose registers (V0 to VF)
    };
    STRUCT_REGISTERS reg;  ///< CPU register instance

    // Flag manager instance to handle emulator state flags (ROM loaded, emulator state, etc.)
    flag_manager<uint16_t> flag;
    
    // Private methods for internal functionality
    void initialize();          ///< Initializes the emulator state
    void load_fontset();        ///< Loads the CHIP-8 font set into memory
    void gpu_cycle();           ///< Handles GPU cycles for rendering
    void cpu_cycle();           ///< Handles CPU cycles for instruction execution
    void execute();             ///< Decodes and executes CHIP-8 instructions
    int8_t get_pressed_key();   ///< Gets the currently pressed key

    // Timer utility methods
    bool cpu_timer_flag();       ///< Checks if the CPU timer interval has elapsed
    bool gpu_timer_flag();       ///< Checks if the GPU timer interval has elapsed
    void reset_cpu_timer_flag(); ///< Resets the CPU timer flag
    void reset_gpu_timer_flag(); ///< Resets the GPU timer flag

    // Hardware timer controls
    void ht_start(); ///< Starts hardware timers
    void ht_stop();  ///< Stops hardware timers

  public:

    /**
     * @brief Provides access to the singleton instance of chip8_core.
     * 
     * @return Reference to the chip8_core singleton instance.
     */
    static chip8_core& getInstance() {
      static chip8_core instance;  ///< Ensures only one instance exists
      return instance;
    }

    // Public interface for display and control
    uint8_t* get_display_buffer();  ///< Returns a pointer to the display buffer
    uint8_t (*get_dirty_flags())[8] { return dirty_flags; }  ///< Returns the dirty flags array

    // Emulator control methods
    bool start();  ///< Starts the emulator if a ROM is loaded
    bool stop();   ///< Stops the emulator and resets its state
    bool is_init_and_ready();  ///< Checks if the emulator is initialized and ready to run
    void loop();  ///< Main loop for managing CPU and GPU cycles
    bool is_running();  ///< Returns true if the emulator is currently running
    bool sound();  ///< Checks if the sound timer is active
    void reset_draw();  ///< Resets the draw flag after rendering a frame
    bool need_to_draw();  ///< Checks if a new frame needs to be drawn
    void load_rom(const uint8_t* rom, const size_t rom_size);  ///< Loads a ROM into memory
    void set_key_state(uint8_t key, bool is_pressed);  ///< Updates the state of a specific key
    bool is_key_pressed(uint8_t key);  ///< Checks if a specific key is currently pressed
    void enable_hardware_timers();  ///< Enables hardware timers for timing CPU/GPU cycles
};

#endif  // CHIP8_CORE_H
