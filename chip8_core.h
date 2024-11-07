#ifndef CHIP8_CORE_H
#define CHIP8_CORE_H

#include <Arduino.h>
// Include the flag manager library for managing emulator state flags
#include "flag_manager.h"


// Defines the CPU and GPU timer intervals
constexpr uint32_t CPU_TIMER_INTERVAL = 1;  // CPU cycle interval in milliseconds
constexpr uint32_t GPU_TIMER_INTERVAL = 16;  // GPU cycle interval in milliseconds
hw_timer_t* timer0 = NULL;
hw_timer_t* timer1 = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

std::atomic<bool> timer0_flag = false;
std::atomic<bool> timer1_flag = false;
// Create the main class for the CHIP-8 emulator core
class chip8_core {

  private:

    // Singleton pattern to ensure only one instance of chip8_core exists
    chip8_core() {}  // Private constructor to restrict instantiation
    chip8_core(const chip8_core&) = delete;  // Delete copy constructor to prevent copying
    chip8_core& operator=(const chip8_core&) = delete;  // Delete assignment operator

    // Flag manager instance to handle emulator state flags (ROM loaded, emulator state, etc.)
    flag_manager<uint8_t> flag;

    // Memory for the CHIP-8 emulator
    uint8_t RAM[4096];  // Main memory (4KB)
    uint8_t DISPLAYBUFFER[(32*64)/8]; 
    uint8_t dirty_flags[32][8] = {0};
    



    uint8_t key_states[16] = {0};
     
    // Stores the last cycle times for both CPU and GPU
    unsigned long last_CPU_cycle;  // Stores last CPU cycle timestamp
    unsigned long last_GPU_cycle;  // Stores last GPU cycle timestamp

    // Registers structure representing the main CHIP-8 registers
    struct STRUCT_REGISTERS {
      uint16_t PC;               // Program Counter
      uint16_t INDEX;            // Index Register
      alignas(2) uint16_t STACK[16];  // Stack for function calls (16 levels deep)
      uint8_t SP;                // Stack Pointer
      uint8_t DELAYTIMER;        // Delay timer (counts down at 60Hz)
      uint8_t SOUNDTIMER;        // Sound timer (counts down at 60Hz)
      uint8_t V[16];         // General purpose registers V0 to VF
    };
    STRUCT_REGISTERS reg;  // Create an instance of the registers structure
    
    // Private methods for internal functionality
    void initialize();       // Initializes emulator to default state
    void load_fontset();     // Loads the CHIP-8 font set into memory
    void gpu_cycle();        // Handles GPU cycles for drawing and updates
    void cpu_cycle();        // Handles CPU cycles for instruction execution
    void execute();
    int8_t get_pressed_key();
    
  public:
    // Singleton pattern to access the single instance of chip8_core
    static chip8_core& getInstance() {
      static chip8_core instance;  // Static local variable to ensure single instance
      return instance;
    }
    
    // Display buffer for the CHIP-8's 64x32 monochrome screen
     // Buffer for storing the display output
    uint8_t* get_display_buffer();
    uint8_t (*get_dirty_flags())[8] { return dirty_flags; }

    // Public methods to control the emulator
    bool start();              // Starts the emulator if a ROM is loaded
    bool stop();
    bool is_init_and_ready();  // Stops the emulator and clears its state            // Resets the emulator to the initial state
    void loop();               // Main loop that handles CPU and GPU cycles
    bool is_running(); 
    bool sound();        // Checks if the emulator is currently running
    void reset_draw();         // Resets the draw flag once a frame is rendered
    bool need_to_draw();       // Checks if a frame needs to be drawn
    void load_rom(const uint8_t* rom, const size_t rom_size);  // Loads a ROM into memory
    void set_key_state(uint8_t key, bool is_pressed);
    bool is_key_pressed(uint8_t key);
};



#endif
