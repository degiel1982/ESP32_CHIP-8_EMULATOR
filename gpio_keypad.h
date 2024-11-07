#ifndef GPIO_KEYPAD_H
#define GPIO_KEYPAD_H

#include <Arduino.h>
#include "chip8_core.h"

#define NUM_KEYS 16
const uint8_t key_pins[16] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
class gpio_keypad{
  private:
      const unsigned long DEBOUNCE_DELAY = 50; // milliseconds
      unsigned long last_debounce_time[NUM_KEYS] = {0};
      bool last_key_state[NUM_KEYS] = {false};
  public:
  void setup(){
    for(uint8_t i = 0; i < NUM_KEYS; i++) {
      if(key_pins[i] != (uint8_t)-1) { // Check if the pin is valid
        pinMode(key_pins[i], INPUT_PULLUP); // Assuming buttons connect to GND when pressed
        last_debounce_time[i] = 0;
        delay(100);
      }
    }
  }
  void handle_keys() {
    unsigned long current_time = millis();
    for (uint8_t key = 0; key < NUM_KEYS; key++) {
      if(key_pins[key] == (uint8_t)-1) continue; // Skip invalid pins
      bool reading = (digitalRead(key_pins[key]) == LOW); // Assuming LOW when pressed
      if (reading != last_key_state[key]) {
        last_debounce_time[key] = current_time;
      }
      if ((current_time - last_debounce_time[key]) > DEBOUNCE_DELAY) {
        if (reading != chip8_core::getInstance().is_key_pressed(key)) {
          chip8_core::getInstance().set_key_state(key, reading);
          // Optional: Debugging output
          Serial.print("Key ");
          Serial.print(key, HEX);
          Serial.print(reading ? " pressed" : " released");
          Serial.println();
        }
      }
      last_key_state[key] = reading;  
    }
  }
};
#endif