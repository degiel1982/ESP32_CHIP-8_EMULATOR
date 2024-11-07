#include "chip8_core.h"

// Define various emulator state flags for tracking different statuses
#define HARDWARE_TIMERS 0
#define ROM_IS_LOADED 1           // Flag indicating if a ROM has been loaded
#define EMULATOR_STATE 2          // Flag indicating if the emulator is running
#define INITIALIZED 3             // Flag indicating if the emulator has been initialized
#define CPU_CYCLE_DRAW_FLAG 4     // Flag indicating if the CPU cycle needs a draw update
#define GPU_CYCLE_DRAW_FLAG 5     // Flag indicating if the GPU cycle needs a draw update
#define SOUND 6                   // Flag indicating if sound should be played
#define PAUSE 7                   

// CHIP-8 font set loaded into emulator memory, each character is 4x5 pixels
uint16_t FONTSET[80] =
{
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
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Loads a ROM into the emulator's RAM starting at address 0x200
void chip8_core::load_rom(const uint8_t* rom, const size_t rom_size) {
  memset(RAM, 0, sizeof(RAM));
  // Load the default font set into RAM
  load_fontset();
  memcpy(RAM + 0x200, rom, rom_size);  // Copy ROM data to memory starting at 0x200
  flag.set(ROM_IS_LOADED, true);  // Set the flag indicating that a ROM has been loaded
}

// Loads the font set into emulator RAM at address 0x50
void chip8_core::load_fontset() {
  memcpy(RAM + (0x50), FONTSET, sizeof(FONTSET));
}

// Initializes the emulator, resetting memory, registers, and other state
void chip8_core::initialize() {
  // Set registers and program counter to initial state
  reg.PC = 0x200;  // Program counter starts at 0x200 where CHIP-8 programs are loaded
  reg.INDEX = 0;   // Clear the index register
  memset(reg.STACK, 0, sizeof(reg.STACK));  // Clear the stack
  reg.SP = 0;      // Reset stack pointer
  reg.DELAYTIMER = 0;  // Clear delay timer
  reg.SOUNDTIMER = 0;  // Clear sound timer
  memset(reg.V, 0, sizeof(reg.V));  // Clear all general-purpose registers V0-VF
  memset(DISPLAYBUFFER, 0, sizeof(DISPLAYBUFFER));


  // Reset the timing for CPU and GPU cycles
  last_CPU_cycle = 0;
  last_GPU_cycle = 0;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Returns true if the emulator is currently running
bool chip8_core::is_running() {
  return flag.get(EMULATOR_STATE);  // Check if emulator is in running state
}

// Starts the emulator if a ROM is loaded and it is not already running
bool chip8_core::start() {
  bool rom_is_loaded = flag.get(ROM_IS_LOADED);
  bool state = flag.get(EMULATOR_STATE);
  if (rom_is_loaded && !state) {
    flag.set(EMULATOR_STATE, true);  // Set the emulator state flag to running
    initialize();  // Initialize the emulator's RAM and registers
    flag.set(INITIALIZED, true);  // Set the initialized flag
    return true;  // Successfully started
  } else {
    return false;  // Cannot start if ROM isn't loaded or already running
  }
}
uint8_t* chip8_core::get_display_buffer() {
    return DISPLAYBUFFER;
}
// Stops the emulator, clearing all state flags
bool chip8_core::stop() {
  if (flag.get(EMULATOR_STATE)) {
    flag.clear_all();  // Clear all emulator-related flags
    return true;  // Successfully stopped
  } else {
    return false;  // Emulator wasn't running
  }
}




/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void chip8_core::cpu_cycle() {
  unsigned long currentTime = millis();  // Get the current time in milliseconds
  if (currentTime - last_CPU_cycle >= CPU_TIMER_INTERVAL) {
    execute();
    last_CPU_cycle = currentTime;  // Update the last CPU cycle time
  }
}

void chip8_core::gpu_cycle() {
  unsigned long currentTime = millis();  // Get the current time in milliseconds
  #ifndef HARDWARE_TIMERS
  if (currentTime - last_GPU_cycle >= GPU_TIMER_INTERVAL) {
  #else
  if (currentTime - last_GPU_cycle >= GPU_TIMER_INTERVAL) {
  #endif
    // Check if the CPU has set the draw flag
    bool cpu_gave_draw_instruction = flag.get(CPU_CYCLE_DRAW_FLAG);
    if (cpu_gave_draw_instruction) {
      flag.set(CPU_CYCLE_DRAW_FLAG, false);  // Clear the CPU cycle draw flag
      flag.set(GPU_CYCLE_DRAW_FLAG, true);   // Set the GPU cycle draw flag
    }
    // Decrement the delay timer if it's greater than 0
    if (reg.DELAYTIMER > 0) {
      reg.DELAYTIMER--;
    }
    // Decrement the sound timer if it's greater than 0 and handle sound state
    if (reg.SOUNDTIMER > 0) {
      flag.set(SOUND, true);  // Set the SOUND flag if sound is active
      reg.SOUNDTIMER--;
      if (reg.SOUNDTIMER == 0) {
        flag.set(SOUND, false);  // Clear the SOUND flag when the timer reaches zero
      }
    }
    #ifndef HARDWARE_TIMERS
    last_GPU_cycle = currentTime;  // Update the last GPU cycle time
    #endif
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Checks if there is a need to draw based on the GPU draw flag
bool chip8_core::need_to_draw() {
  return flag.get(GPU_CYCLE_DRAW_FLAG);  // Returns true if a draw is needed
}

// Resets the draw flag after a frame has been drawn
void chip8_core::reset_draw() {
  flag.set(GPU_CYCLE_DRAW_FLAG, false);  // Clear the GPU cycle draw flag
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Main loop that handles both CPU and GPU cycles if the emulator is initialized
void chip8_core::loop() {
  if (flag.get(INITIALIZED)) {
    cpu_cycle();  // Perform a CPU cycle to execute instructions
    gpu_cycle();  // Perform a GPU cycle to handle timers and drawing
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Checks if the sound flag is set, indicating if sound should be played
bool chip8_core::sound() {
  return flag.get(SOUND);  // Returns true if the SOUND flag is set
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void chip8_core::set_key_state(uint8_t key, bool is_pressed) {
    if (key < 16) {  // Ensure the key is within valid range (0x0 to 0xF)
        key_states[key] = is_pressed ? 1 : 0;  // Set state to 1 if pressed, 0 if released
    }
}

// Check if a specific key is pressed
bool chip8_core::is_key_pressed(uint8_t key) {
    return key < 16 && key_states[key] == 1;  // Return true if key is within range and pressed
}
bool chip8_core::is_init_and_ready() {
  return flag.get(INITIALIZED);  // Check if emulator is in running state
}
int8_t chip8_core::get_pressed_key() {
    for (uint8_t key = 0; key < 16; key++) {
        if (key_states[key] == 1) {
            return key;  // Return the key value if pressed
        }
    }
    return -1;  // Return -1 if no key is pressed
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void chip8_core::execute() {
    uint16_t OPCODE = (RAM[reg.PC] << 8) | RAM[reg.PC + 1];
    switch (OPCODE & 0xF000) {
        case 0x0000:
            switch (OPCODE & 0x00FF) {
                case 0xE0:
                    // Clear display buffer
                    memset(DISPLAYBUFFER,0,sizeof(DISPLAYBUFFER));
                    memset(dirty_flags, 1, sizeof(dirty_flags));
                    reg.PC += 2;
                    break;
                case 0xEE:
                  reg.PC = reg.STACK[--reg.SP];
                  break;
                default:
                    break;
            }
            break;
        case 0x1000:
            reg.PC = OPCODE & 0x0FFF;
            break;
case 0x2000:
    if (reg.SP < 16) {
        reg.STACK[reg.SP++] = reg.PC + 2;      // Push the current PC onto the stack
        reg.PC = OPCODE & 0x0FFF;           // Jump to the address specified by the opcode
    } else {
        reg.PC += 2;                        // If stack is full, increment PC to skip this instruction
    }
    break;
        case 0x3000:
            // 3XNN: Skip next instruction if Vx equals NN
            if (reg.V[(OPCODE & 0x0F00) >> 8] == (OPCODE & 0x00FF)) {
                reg.PC += 4;  // Skip next instruction
            } else {
                reg.PC += 2;
            }
            break;
        case 0x4000:
            // 4XNN: Skip next instruction if Vx does not equal NN
            if (reg.V[(OPCODE & 0x0F00) >> 8] != (OPCODE & 0x00FF)) {
                reg.PC += 4;
            } else {
                reg.PC += 2;
            }
            break;
        case 0x5000:
            if (reg.V[(OPCODE & 0x0F00) >> 8] == reg.V[(OPCODE & 0x00F0) >> 4]) {
                reg.PC += 4;
            } else {
                reg.PC += 2;
            }
            break;
        case 0x6000:
            reg.V[(OPCODE & 0x0F00) >> 8] = (OPCODE & 0x00FF);
            reg.PC += 2;
            break;
        case 0x7000:
            reg.V[(OPCODE & 0x0F00) >> 8] += (OPCODE & 0x00FF);
            reg.PC += 2;
            break;
        case 0x8000:
            {
                uint8_t X = (OPCODE & 0x0F00) >> 8;
                uint8_t Y = (OPCODE & 0x00F0) >> 4;
                switch (OPCODE & 0x000F) {
                    case 0x0:
                        reg.V[X] = reg.V[Y];
                        reg.PC += 2;
                        break;
                    case 0x1:
                        reg.V[X] |= reg.V[Y];
                        reg.PC += 2;
                        break;
                    case 0x2:
                        reg.V[X] &= reg.V[Y];
                        reg.PC += 2;
                        break;
                    case 0x3:
                        reg.V[X] ^= reg.V[Y];
                        reg.PC += 2;
                        break;
                    case 0x4: {
                        uint16_t sum = reg.V[X] + reg.V[Y];
                        reg.V[0xF] = (sum > 0xFF) ? 1 : 0;
                        reg.V[X] = sum & 0xFF;
                        reg.PC += 2;
                    } break;
                    case 0x5:
                        reg.V[0xF] = (reg.V[X] > reg.V[Y]) ? 1 : 0;
                        reg.V[X] -= reg.V[Y];
                        reg.PC += 2;
                        break;
                    case 0x6:
                        reg.V[0xF] = (reg.V[X] & 0x1);
                        reg.V[X] >>= 1;
                        reg.PC += 2;
                        break;
                    case 0x7:
                        reg.V[0xF] = (reg.V[Y] > reg.V[X]) ? 1 : 0;
                        reg.V[X] = reg.V[Y] - reg.V[X];
                        reg.PC += 2;
                        break;
                    case 0xE:
                        reg.V[0xF] = (reg.V[X] & 0x80) ? 1 : 0;
                        reg.V[X] <<= 1;
                        reg.PC += 2;
                        break;
                    default:
                        reg.PC += 2;
                        break;
                }
            }
            break;
        case 0x9000:
            if (reg.V[(OPCODE & 0x0F00) >> 8] != reg.V[(OPCODE & 0x00F0) >> 4]) {
                reg.PC += 4;
            } else {
                reg.PC += 2;
            }
            break;
        case 0xA000:
            reg.INDEX = (OPCODE & 0x0FFF);
            reg.PC += 2;
            break;
        case 0xB000:
            reg.PC = ((OPCODE & 0x0FFF) + reg.V[0]);
            break;
        case 0xC000:
            reg.V[(OPCODE & 0x0F00) >> 8] = ((esp_random() & 0xFF) & (OPCODE & 0x00FF));
            reg.PC += 2;
            break;
        case 0xD000: {
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
            uint8_t X = (OPCODE & 0x0F00) >> 8;
            uint8_t key = reg.V[X];
            switch (OPCODE & 0x00FF) {
                case 0x9E:
                    if (is_key_pressed(key)) {
                        reg.PC += 4;
                    } else {
                        reg.PC += 2;
                    }
                    break;
                case 0xA1:
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
            uint8_t X = (OPCODE & 0x0F00) >> 8;
            switch (OPCODE & 0x00FF) {
                case 0x07:
                    reg.V[X] = reg.DELAYTIMER;
                    reg.PC += 2;
                    break;
                case 0x15:
                    reg.DELAYTIMER = reg.V[X];
                    reg.PC += 2;
                    break;
                case 0x18:
                    reg.SOUNDTIMER = reg.V[X];
                    reg.PC += 2;
                    break;
                case 0x0A: {
                    int8_t pressed_key = get_pressed_key();
                    if (pressed_key != -1) {
                        reg.V[X] = pressed_key;
                        reg.PC += 2;
                    }
                } break;
                case 0x1E:
                    reg.INDEX += reg.V[X];
                    reg.V[0xF] = (reg.INDEX > 0xFFF) ? 1 : 0;
                    reg.INDEX &= 0xFFF;
                    reg.PC += 2;
                    break;
                case 0x29:
                    reg.INDEX = 0x50 + (reg.V[X] * 5);
                    reg.PC += 2;
                    break;
                case 0x33:
                    RAM[reg.INDEX] = reg.V[X] / 100;
                    RAM[reg.INDEX + 1] = (reg.V[X] / 10) % 10;
                    RAM[reg.INDEX + 2] = reg.V[X] % 10;
                    reg.PC += 2;
                    break;
                case 0x55:
                    for (uint8_t reg1 = 0; reg1 <= X; ++reg1) {
                        RAM[reg.INDEX + reg1] = reg.V[reg1];
                    }
                    reg.PC += 2;
                    break;
                case 0x65:
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


void IRAM_ATTR onTimer0() {
    timer0_flag.store(true, std::memory_order_relaxed);
}
void IRAM_ATTR onTimer1() {
    timer1_flag.store(true, std::memory_order_relaxed);

}