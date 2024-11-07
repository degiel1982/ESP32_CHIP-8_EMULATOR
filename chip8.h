#ifndef CHIP8_h
#define CHIP8_H

#include <Arduino.h>
#include "chip8_core.h"

#ifdef SSD1306OLED
  #include "ssd1306oled.h" 
#endif

class chip8{
  public:
    typedef void (*emulator_loop_callback)();

    void setup(){
      #ifdef SSD1306OLED
        #if SSD1306OLED != 0x3C && SSD1306OLED != NULL
          oled.setup(SSD1306OLED);
        #else
          oled.setup();
        #endif
      #endif
    }

    bool play_game(const uint8_t* rom, size_t rom_size, emulator_loop_callback loop_callback = nullptr){
        if(!chip8_core::getInstance().is_init_and_ready()){
          chip8_core::getInstance().load_rom(rom, rom_size);
          if(!chip8_core::getInstance().start()){
            return false;
          }
        }
        else{
          if(chip8_core::getInstance().is_running()){
            chip8_core::getInstance().loop();
            #ifdef SSD1306OLED
              oled.draw();
            #endif
            if (loop_callback != nullptr) {
              loop_callback();
            }
          }
          else{
            return false; 
          }
        }
        return true;
      }
    
  private:
    #ifdef SSD1306OLED
      ssd1306oled oled;
    #endif
};




#endif