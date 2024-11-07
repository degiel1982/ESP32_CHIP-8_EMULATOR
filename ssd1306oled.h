#ifndef SSD1306OLED_H
#define SSD1306OLED_H

#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include "chip8_core.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

class ssd1306oled{
  private:
    

    void draw_oled() {
      uint8_t (*dirty_flags)[8] = chip8_core::getInstance().get_dirty_flags();  // Access dirty flags array
      for (uint8_t y = 0; y < 32; y++) {
        for (uint8_t byte_index = 0; byte_index < 8; byte_index++) {
          if (dirty_flags[y][byte_index]) {  // Only update if marked dirty
            for (uint8_t bit = 0; bit < 8; bit++) {
              uint8_t x = byte_index * 8 + bit;
              uint16_t bit_index = x + y * 64;
              uint16_t buffer_byte_index = bit_index >> 3;
              uint8_t display_bit = 7 - (bit_index & 0x07);
              // Check if pixel should be on or off
              if (chip8_core::getInstance().get_display_buffer()[buffer_byte_index] & (1 << display_bit)) {
                uint8_t display_x = x * 2;
                uint8_t display_y = y * 2;
                // Draw a 2x2 white square to represent an active CHIP-8 pixel
                display.drawPixel(display_x, display_y, SSD1306_WHITE);
                display.drawPixel(display_x + 1, display_y, SSD1306_WHITE);
                display.drawPixel(display_x, display_y + 1, SSD1306_WHITE);
                display.drawPixel(display_x + 1, display_y + 1, SSD1306_WHITE);
              } 
              else {
                uint8_t display_x = x * 2;
                uint8_t display_y = y * 2;
                display.drawPixel(display_x, display_y, SSD1306_BLACK);
                display.drawPixel(display_x + 1, display_y, SSD1306_BLACK);
                display.drawPixel(display_x, display_y + 1, SSD1306_BLACK);
                display.drawPixel(display_x + 1, display_y + 1, SSD1306_BLACK);
              }
            }
            dirty_flags[y][byte_index] = 0;  // Clear the dirty flag after updating
          }
        }
      }
      display.display();  // Push the update to the OLED
    }
  public:
  
    bool setup(uint16_t OledAddress = 0x3C){
      if(!display.begin(SSD1306_SWITCHCAPVCC, OledAddress)) { 
        return false;
      }
      delay(500);
      display.clearDisplay();
      display.display();
      delay(500);
      return true;
    }

    void draw(){
      if (chip8_core::getInstance().need_to_draw()) {
        draw_oled();
        chip8_core::getInstance().reset_draw(); 
      }
    }
};

#endif