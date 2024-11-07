#include <Arduino.h>
/*
*   Possible options:
*   #define SSD1306OLED // Automaticly chooses the address 0x3C
*   #define SSD1306OLED 0x3D // If you have a custom adress of your oled display
*
*/
#define SSD1306OLED 0x3C



#include "chip8.h"  
#include "roms.h" 
  
chip8 ch8;

void setup() {
Serial.begin(115200);
ch8.setup();
}

void loop() {
  while(ch8.play_game(space, sizeof(space),loop_extended)){
  };
}



void loop_extended(){

}