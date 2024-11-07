#include "chip8_core.h"

// Hardware timer instances
hw_timer_t* timer0 = NULL;
hw_timer_t* timer1 = NULL;

// Mutex for timer synchronization
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

// Atomic flags for timer interrupts
std::atomic<bool> timer0_flag = false;
std::atomic<bool> timer1_flag = false;

// Define various emulator state flags for tracking different statuses
#define HARDWARE_TIMERS       0   ///< Flag indicating if hardware timers are enabled
#define ROM_IS_LOADED         1   ///< Flag indicating if a ROM has been loaded
#define EMULATOR_STATE        2   ///< Flag indicating if the emulator is running
#define INITIALIZED           3   ///< Flag indicating if the emulator has been initialized
#define CPU_CYCLE_DRAW_FLAG   4   ///< Flag indicating if the CPU cycle needs a draw update
#define GPU_CYCLE_DRAW_FLAG   5   ///< Flag indicating if the GPU cycle needs a draw update
#define SOUND                 6   ///< Flag indicating if sound should be played
#define PAUSE                 7   ///< Flag indicating if the emulator is paused

/**
 * @brief CHIP-8 font set loaded into emulator memory.
 * Each character is represented as a 4x5 pixel sprite.
 */
uint16_t FONTSET[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Loads a ROM into the emulator's RAM starting at address 0x200.
 *
 * This function clears the RAM, loads the default font set, and copies the ROM data into memory.
 *
 * @param rom Pointer to the ROM data.
 * @param rom_size Size of the ROM data in bytes.
 */
void chip8_core::load_rom(const uint8_t* rom, const size_t rom_size) {
  
    //Clears the RAM before loading a ROM
    memset(RAM, 0, sizeof(RAM));

    // Load the default font set into RAM
    load_fontset();

    // Copy ROM data to memory starting at address 0x200
    memcpy(RAM + 0x200, rom, rom_size);

    // Set the flag indicating that a ROM has been loaded
    flag.set(ROM_IS_LOADED, true);
}

/**
 * @brief Interrupt Service Routine for Timer0.
 *
 * Sets the timer0_flag to true when the timer interrupt occurs.
 */
void IRAM_ATTR onTimer0() {
    timer0_flag.store(true, std::memory_order_relaxed);
}

/**
 * @brief Interrupt Service Routine for Timer1.
 *
 * Sets the timer1_flag to true when the timer interrupt occurs.
 */
void IRAM_ATTR onTimer1() {
    timer1_flag.store(true, std::memory_order_relaxed);
}

/**
 * @brief Loads the font set into emulator RAM at address 0x50.
 *
 * This function copies the predefined FONTSET array into the emulator's RAM.
 */
void chip8_core::load_fontset() {
    memcpy(RAM + 0x50, FONTSET, sizeof(FONTSET));
}

/**
 * @brief Starts the hardware timers for CPU and GPU cycles.
 *
 * Initializes and configures Timer0 and Timer1 with specified intervals.
 */
void chip8_core::ht_start() {
    timer0 = timerBegin(1000000);
    timerAttachInterrupt(timer0, &onTimer0);
    timerAlarm(timer0, 2000, true, 0);

    timer1 = timerBegin(1000000);
    timerAttachInterrupt(timer1, &onTimer1);
    timerAlarm(timer1, 16667, true, 0);
}

/**
 * @brief Stops the hardware timers for CPU and GPU cycles.
 *
 * Disables and deinitializes Timer0 and Timer1, and resets the timer flags.
 */
void chip8_core::ht_stop() {
    if (timer0 != NULL) {
        timerEnd(timer0);
        timer0 = NULL;
    }
    if (timer1 != NULL) {
        timerEnd(timer1);
        timer1 = NULL;
    }
    timer0_flag.store(false, std::memory_order_relaxed);
    timer1_flag.store(false, std::memory_order_relaxed);
}

/**
 * @brief Enables hardware timers for the emulator.
 *
 * Sets the HARDWARE_TIMERS flag to indicate that hardware timers should be used.
 */
void chip8_core::enable_hardware_timers() {
    flag.set(HARDWARE_TIMERS, true);
}

/**
 * @brief Checks if the CPU timer flag is set.
 *
 * @return True if Timer0 has triggered; otherwise, false.
 */
bool chip8_core::cpu_timer_flag() {
    return timer0_flag.load(std::memory_order_relaxed);
}

/**
 * @brief Resets the CPU timer flag.
 *
 * Clears the Timer0 flag after it has been handled.
 */
void chip8_core::reset_cpu_timer_flag() {
    timer0_flag.store(false, std::memory_order_relaxed);
}

/**
 * @brief Checks if the GPU timer flag is set.
 *
 * @return True if Timer1 has triggered; otherwise, false.
 */
bool chip8_core::gpu_timer_flag() {
    return timer1_flag.load(std::memory_order_relaxed);
}

/**
 * @brief Resets the GPU timer flag.
 *
 * Clears the Timer1 flag after it has been handled.
 */
void chip8_core::reset_gpu_timer_flag() {
    timer1_flag.store(false, std::memory_order_relaxed);
}

/**
 * @brief Initializes the emulator by resetting memory, registers, and other state variables.
 *
 * This function sets the program counter, index register, clears the stack,
 * resets timers, and clears the display buffer.
 */
void chip8_core::initialize() {
    // Set registers and program counter to initial state
    reg.PC = 0x200; // Program counter starts at 0x200 where CHIP-8 programs are loaded
    reg.INDEX = 0;  // Clear the index register
    memset(reg.STACK, 0, sizeof(reg.STACK)); // Clear the stack
    reg.SP = 0;     // Reset stack pointer
    reg.DELAYTIMER = 0; // Clear delay timer
    reg.SOUNDTIMER = 0; // Clear sound timer
    memset(reg.V, 0, sizeof(reg.V)); // Clear all general-purpose registers V0-VF
    memset(DISPLAYBUFFER, 0, sizeof(DISPLAYBUFFER));
    memset(dirty_flags, 1, sizeof(dirty_flags));

    // Reset the timing for CPU and GPU cycles
    last_CPU_cycle = 0;
    last_GPU_cycle = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Checks if the emulator is currently running.
 *
 * @return True if the emulator is in the running state; otherwise, false.
 */
bool chip8_core::is_running() {
    return flag.get(EMULATOR_STATE); // Check if emulator is in running state
}

/**
 * @brief Starts the emulator if a ROM is loaded and it is not already running.
 *
 * This function initializes the emulator, starts hardware timers if enabled,
 * and sets the necessary state flags.
 *
 * @return True if the emulator started successfully; otherwise, false.
 */
bool chip8_core::start() {
    bool rom_is_loaded = flag.get(ROM_IS_LOADED);
    bool state = flag.get(EMULATOR_STATE);
    if (rom_is_loaded && !state) {
        flag.set(EMULATOR_STATE, true); // Set the emulator state flag to running
        initialize(); // Initialize the emulator's RAM and registers
        flag.set(INITIALIZED, true); // Set the initialized flag
        if (flag.get(HARDWARE_TIMERS)) {
            ht_start();
        }
        return true; // Successfully started
    } else {
        return false; // Cannot start if ROM isn't loaded or already running
    }
}

/**
 * @brief Retrieves the display buffer for rendering.
 *
 * @return Pointer to the display buffer.
 */
uint8_t* chip8_core::get_display_buffer() {
    return DISPLAYBUFFER;
}

/**
 * @brief Stops the emulator, clearing all state flags.
 *
 * This function stops hardware timers, clears all emulator-related flags,
 * and resets the emulator state.
 *
 * @return True if the emulator was running and has been stopped; otherwise, false.
 */
bool chip8_core::stop() {
    if (flag.get(EMULATOR_STATE)) {
        ht_stop();
        flag.clear_all(); // Clear all emulator-related flags
        return true; // Successfully stopped
    } else {
        return false; // Emulator wasn't running
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Executes a CPU cycle, processing instructions based on timing.
 *
 * This function checks if enough time has elapsed since the last CPU cycle
 * and executes an instruction if the CPU timer flag is set or if hardware timers are disabled.
 */
void chip8_core::cpu_cycle() {
    unsigned long currentTime = millis(); // Get the current time in milliseconds
    if ((currentTime - last_CPU_cycle >= CPU_TIMER_INTERVAL && !flag.get(HARDWARE_TIMERS)) ||
        (flag.get(HARDWARE_TIMERS) && cpu_timer_flag())) {
        execute();
        if (!flag.get(HARDWARE_TIMERS)) {
            last_CPU_cycle = currentTime; // Update the last CPU cycle time
        } else {
            reset_cpu_timer_flag();
        }
    }
}

/**
 * @brief Executes a GPU cycle, handling timers and drawing operations based on timing.
 *
 * This function checks if enough time has elapsed since the last GPU cycle
 * and performs necessary updates such as drawing the display and handling timers.
 */
void chip8_core::gpu_cycle() {
    unsigned long currentTime = millis(); // Get the current time in milliseconds
    if ((currentTime - last_GPU_cycle >= GPU_TIMER_INTERVAL && !flag.get(HARDWARE_TIMERS)) ||
        (flag.get(HARDWARE_TIMERS) && gpu_timer_flag())) {
        // Check if the CPU has set the draw flag
        bool cpu_gave_draw_instruction = flag.get(CPU_CYCLE_DRAW_FLAG);
        if (cpu_gave_draw_instruction) {
            flag.set(CPU_CYCLE_DRAW_FLAG, false); // Clear the CPU cycle draw flag
            flag.set(GPU_CYCLE_DRAW_FLAG, true);  // Set the GPU cycle draw flag
        }
        // Decrement the delay timer if it's greater than 0
        if (reg.DELAYTIMER > 0) {
            reg.DELAYTIMER--;
        }
        // Decrement the sound timer if it's greater than 0 and handle sound state
        if (reg.SOUNDTIMER > 0) {
            flag.set(SOUND, true); // Set the SOUND flag if sound is active
            reg.SOUNDTIMER--;
            if (reg.SOUNDTIMER == 0) {
                flag.set(SOUND, false); // Clear the SOUND flag when the timer reaches zero
            }
        }
        if (!flag.get(HARDWARE_TIMERS)) {
            last_GPU_cycle = currentTime; // Update the last GPU cycle time
        } else {
            reset_gpu_timer_flag();
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Checks if there is a need to draw based on the GPU draw flag.
 *
 * @return True if a draw is needed; otherwise, false.
 */
bool chip8_core::need_to_draw() {
    return flag.get(GPU_CYCLE_DRAW_FLAG); // Returns true if a draw is needed
}

/**
 * @brief Resets the draw flag after a frame has been drawn.
 *
 * This function clears the GPU cycle draw flag to indicate that drawing is complete.
 */
void chip8_core::reset_draw() {
    flag.set(GPU_CYCLE_DRAW_FLAG, false); // Clear the GPU cycle draw flag
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Main loop that handles both CPU and GPU cycles if the emulator is initialized.
 *
 * This function should be called repeatedly to process emulator cycles.
 * It ensures that both CPU instructions and GPU updates are handled appropriately.
 */
void chip8_core::loop() {
    if (flag.get(INITIALIZED)) {
        cpu_cycle(); // Perform a CPU cycle to execute instructions
        gpu_cycle(); // Perform a GPU cycle to handle timers and drawing
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Checks if the sound flag is set, indicating if sound should be played.
 *
 * @return True if the SOUND flag is set; otherwise, false.
 */
bool chip8_core::sound() {
    return flag.get(SOUND); // Returns true if the SOUND flag is set
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Sets the state of a specific key.
 *
 * @param key The key index (0x0 to 0xF).
 * @param is_pressed True if the key is pressed; otherwise, false.
 */
void chip8_core::set_key_state(uint8_t key, bool is_pressed) {
    if (key < 16) { // Ensure the key is within valid range (0x0 to 0xF)
        key_states[key] = is_pressed ? 1 : 0; // Set state to 1 if pressed, 0 if released
    }
}

/**
 * @brief Checks if a specific key is pressed.
 *
 * @param key The key index (0x0 to 0xF).
 * @return True if the key is within range and pressed; otherwise, false.
 */
bool chip8_core::is_key_pressed(uint8_t key) {
    return key < 16 && key_states[key] == 1; // Return true if key is within range and pressed
}

/**
 * @brief Checks if the emulator has been initialized and is ready to run.
 *
 * @return True if the emulator is initialized; otherwise, false.
 */
bool chip8_core::is_init_and_ready() {
    return flag.get(INITIALIZED); // Check if emulator is in initialized state
}

/**
 * @brief Retrieves the currently pressed key.
 *
 * @return The key value if pressed; otherwise, -1.
 */
int8_t chip8_core::get_pressed_key() {
    for (uint8_t key = 0; key < 16; key++) {
        if (key_states[key] == 1) {
            return key; // Return the key value if pressed
        }
    }
    return -1; // Return -1 if no key is pressed
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Executes the current opcode.
 *
 * This function fetches the opcode from memory, decodes it, and executes the corresponding instruction.
 * It handles all CHIP-8 instruction sets including system, flow control, arithmetic, and graphics operations.
 */
void chip8_core::execute() {
    uint16_t OPCODE = (RAM[reg.PC] << 8) | RAM[reg.PC + 1];
    switch (OPCODE & 0xF000) {
        case 0x0000:
            switch (OPCODE & 0x00FF) {
                case 0xE0:
                    // 00E0: Clear the display
                    memset(DISPLAYBUFFER, 0, sizeof(DISPLAYBUFFER));
                    memset(dirty_flags, 1, sizeof(dirty_flags));
                    flag.set(CPU_CYCLE_DRAW_FLAG, true); // Set draw flag
                    reg.PC += 2;
                    break;
                case 0xEE:
                    // 00EE: Return from subroutine
                    reg.PC = reg.STACK[--reg.SP];
                    break;
                case 0xFD:
                    // 00FD: Exit CHIP-8/SCHIP interpreter
                    stop(); // Custom function to halt the emulator
                    break;
                default:
                    break;
            }
            break;
        case 0x1000:
            // 1NNN: Jump to address NNN
            reg.PC = OPCODE & 0x0FFF;
            break;
        case 0x2000:
            // 2NNN: Call subroutine at NNN
            if (reg.SP < 16) {
                reg.STACK[reg.SP++] = reg.PC + 2; // Push the current PC onto the stack
                reg.PC = OPCODE & 0x0FFF;         // Jump to the address specified by the opcode
            } else {
                reg.PC += 2; // If stack is full, increment PC to skip this instruction
            }
            break;
        case 0x3000:
            // 3XNN: Skip next instruction if Vx equals NN
            if (reg.V[(OPCODE & 0x0F00) >> 8] == (OPCODE & 0x00FF)) {
                reg.PC += 4; // Skip next instruction
            } else {
                reg.PC += 2;
            }
            break;
        case 0x4000:
            // 4XNN: Skip next instruction if Vx does not equal NN
            if (reg.V[(OPCODE & 0x0F00) >> 8] != (OPCODE & 0x00FF)) {
                reg.PC += 4; // Skip next instruction
            } else {
                reg.PC += 2;
            }
            break;
        case 0x5000:
            // 5XY0: Skip next instruction if Vx equals Vy
            if (reg.V[(OPCODE & 0x0F00) >> 8] == reg.V[(OPCODE & 0x00F0) >> 4]) {
                reg.PC += 4;
            } else {
                reg.PC += 2;
            }
            break;
        case 0x6000:
            // 6XNN: Set Vx = NN
            reg.V[(OPCODE & 0x0F00) >> 8] = (OPCODE & 0x00FF);
            reg.PC += 2;
            break;
        case 0x7000:
            // 7XNN: Set Vx = Vx + NN
            reg.V[(OPCODE & 0x0F00) >> 8] += (OPCODE & 0x00FF);
            reg.PC += 2;
            break;
        case 0x8000: {
            // 8XYN: Various arithmetic and logical operations
            uint8_t X = (OPCODE & 0x0F00) >> 8;
            uint8_t Y = (OPCODE & 0x00F0) >> 4;
            switch (OPCODE & 0x000F) {
                case 0x0:
                    // 8XY0: Set Vx = Vy
                    reg.V[X] = reg.V[Y];
                    reg.PC += 2;
                    break;
                case 0x1:
                    // 8XY1: Set Vx = Vx OR Vy
                    reg.V[X] |= reg.V[Y];
                    reg.PC += 2;
                    break;
                case 0x2:
                    // 8XY2: Set Vx = Vx AND Vy
                    reg.V[X] &= reg.V[Y];
                    reg.PC += 2;
                    break;
                case 0x3:
                    // 8XY3: Set Vx = Vx XOR Vy
                    reg.V[X] ^= reg.V[Y];
                    reg.PC += 2;
                    break;
                case 0x4: {
                    // 8XY4: Set Vx = Vx + Vy, set VF = carry
                    uint16_t sum = reg.V[X] + reg.V[Y];
                    reg.V[0xF] = (sum > 0xFF) ? 1 : 0;
                    reg.V[X] = sum & 0xFF;
                    reg.PC += 2;
                } break;
                case 0x5:
                    // 8XY5: Set Vx = Vx - Vy, set VF = NOT borrow
                    reg.V[0xF] = (reg.V[X] > reg.V[Y]) ? 1 : 0;
                    reg.V[X] -= reg.V[Y];
                    reg.PC += 2;
                    break;
                case 0x6:
                    // 8XY6: Set Vx = Vx SHR 1, set VF = least significant bit before shift
                    reg.V[0xF] = (reg.V[X] & 0x1);
                    reg.V[X] >>= 1;
                    reg.PC += 2;
                    break;
                case 0x7:
                    // 8XY7: Set Vx = Vy - Vx, set VF = NOT borrow
                    reg.V[0xF] = (reg.V[Y] > reg.V[X]) ? 1 : 0;
                    reg.V[X] = reg.V[Y] - reg.V[X];
                    reg.PC += 2;
                    break;
                case 0xE:
                    // 8XYE: Set Vx = Vx SHL 1, set VF = most significant bit before shift
                    reg.V[0xF] = (reg.V[X] & 0x80) ? 1 : 0;
                    reg.V[X] <<= 1;
                    reg.PC += 2;
                    break;
                default:
                    reg.PC += 2;
                    break;
            }
        } break;
        case 0x9000:
            // 9XY0: Skip next instruction if Vx != Vy
            if (reg.V[(OPCODE & 0x0F00) >> 8] != reg.V[(OPCODE & 0x00F0) >> 4]) {
                reg.PC += 4;
            } else {
                reg.PC += 2;
            }
            break;
        case 0xA000:
            // ANNN: Set I = NNN
            reg.INDEX = (OPCODE & 0x0FFF);
            reg.PC += 2;
            break;
        case 0xB000:
            // BNNN: Jump to address NNN + V0
            reg.PC = ((OPCODE & 0x0FFF) + reg.V[0]);
            break;
        case 0xC000:
            // CXNN: Set Vx = random byte AND NN
            reg.V[(OPCODE & 0x0F00) >> 8] = ((esp_random() & 0xFF) & (OPCODE & 0x00FF));
            reg.PC += 2;
            break;
        case 0xD000: {
            // DXYN: Draw a sprite at coordinate (Vx, Vy) with width 8 pixels and height N pixels
            uint8_t X = reg.V[(OPCODE >> 8) & 0x0F];
            uint8_t Y = reg.V[(OPCODE >> 4) & 0x0F];
            uint8_t height = OPCODE & 0x000F;
            reg.V[0xF] = 0;

            for (uint8_t yline = 0; yline < height; yline++) {
                uint8_t pixel = RAM[reg.INDEX + yline];
                uint8_t y_coord = (Y + yline) & 31;

                for (uint8_t xline = 0; xline < 8; xline++) {
                    if (pixel & (0x80 >> xline)) {
                        uint8_t x_coord = (X + xline) & 63;
                        uint16_t bit_index = x_coord + y_coord * 64;
                        uint8_t& byte = DISPLAYBUFFER[bit_index >> 3];
                        uint8_t bit = 7 - (bit_index & 0x07);

                        if (byte & (1 << bit)) reg.V[0xF] = 1;
                        byte ^= (1 << bit);  // Toggle pixel

                        // Mark the corresponding byte in dirty_flags as dirty
                        dirty_flags[y_coord][x_coord / 8] = 1;
                    }
                }
            }

            reg.PC += 2;
            flag.set(CPU_CYCLE_DRAW_FLAG, true);
        } break;
        case 0xE000: {
            // EX9E and EXA1: Key operations
            uint8_t X = (OPCODE & 0x0F00) >> 8;
            uint8_t key = reg.V[X];
            switch (OPCODE & 0x00FF) {
                case 0x9E:
                    // EX9E: Skip next instruction if key with the value of Vx is pressed
                    if (is_key_pressed(key)) {
                        reg.PC += 4;
                    } else {
                        reg.PC += 2;
                    }
                    break;
                case 0xA1:
                    // EXA1: Skip next instruction if key with the value of Vx is not pressed
                    if (!is_key_pressed(key)) {
                        reg.PC += 4;
                    } else {
                        reg.PC += 2;
                    }
                    break;
                default:
                    reg.PC += 2;
                    break;
            }
        } break;
        case 0xF000: {
            // FXNN: Various operations related to timers, memory, and input
            uint8_t X = (OPCODE & 0x0F00) >> 8;
            switch (OPCODE & 0x00FF) {
                case 0x07:
                    // FX07: Set Vx = delay timer value
                    reg.V[X] = reg.DELAYTIMER;
                    reg.PC += 2;
                    break;
                case 0x15:
                    // FX15: Set delay timer = Vx
                    reg.DELAYTIMER = reg.V[X];
                    reg.PC += 2;
                    break;
                case 0x18:
                    // FX18: Set sound timer = Vx
                    reg.SOUNDTIMER = reg.V[X];
                    reg.PC += 2;
                    break;
                case 0x0A: {
                    // FX0A: Wait for a key press, then store the value of the key in Vx
                    int8_t pressed_key = get_pressed_key();
                    if (pressed_key != -1) {
                        reg.V[X] = pressed_key;
                        reg.PC += 2;
                    }
                    // If no key is pressed, do not increment PC to wait for key press
                } break;
                case 0x1E:
                    // FX1E: Set I = I + Vx, set VF = carry
                    reg.INDEX += reg.V[X];
                    reg.V[0xF] = (reg.INDEX > 0xFFF) ? 1 : 0;
                    reg.INDEX &= 0xFFF;
                    reg.PC += 2;
                    break;
                case 0x29:
                    // FX29: Set I = location of sprite for digit Vx
                    reg.INDEX = 0x50 + (reg.V[X] * 5);
                    reg.PC += 2;
                    break;
                case 0x30:
                    // FX30: Set I = location of 10-byte font sprite for digit Vx (SCHIP)
                    reg.INDEX = 0xA0 + (reg.V[X] * 10);
                    reg.PC += 2;
                    break;
                case 0x33:
                    // FX33: Store BCD representation of Vx in memory locations I, I+1, and I+2
                    RAM[reg.INDEX] = reg.V[X] / 100;
                    RAM[reg.INDEX + 1] = (reg.V[X] / 10) % 10;
                    RAM[reg.INDEX + 2] = reg.V[X] % 10;
                    reg.PC += 2;
                    break;
                case 0x55:
                    // FX55: Store registers V0 through Vx in memory starting at location I
                    for (uint8_t reg1 = 0; reg1 <= X; ++reg1) {
                        RAM[reg.INDEX + reg1] = reg.V[reg1];
                    }
                    reg.PC += 2;
                    break;
                case 0x65:
                    // FX65: Read registers V0 through Vx from memory starting at location I
                    for (uint8_t reg1 = 0; reg1 <= X; ++reg1) {
                        reg.V[reg1] = RAM[reg.INDEX + reg1];
                    }
                    reg.PC += 2;
                    break;
                default:
                    reg.PC += 2;
                    break;
            }
        } break;
        default:
            reg.PC += 2;
            break;
    }
}
