#ifndef FLAG_MANAGER_H
#define FLAG_MANAGER_H

// Include the atomic library for thread-safe flag manipulation
#include <atomic>

template<typename T>
class flag_manager {
  private:
    // The flags variable is stored as an atomic type to ensure thread-safe operations
    std::atomic<T> flags = 0;

  public:
    // Retrieves the value of the flag at the specified 'position'
    bool get(uint8_t position) const {
        // Load the current value of flags atomically with memory order acquire for synchronization
        // Apply a bitmask to check if the specific bit at 'position' is set
        return (flags.load(std::memory_order_acquire) & (static_cast<T>(1) << position)) != 0;
    }

    // Sets or clears the bit at the specified 'position' based on the value parameter
    void set(uint8_t position, bool value) {
        T bitmask = static_cast<T>(1) << position;  // Create a bitmask to manipulate the specific bit

        if (value) {
            // Set the bit at 'position' using an atomic OR operation
            flags.fetch_or(bitmask, std::memory_order_release);
        } else {
            // Clear the bit at 'position' using an atomic AND operation with the inverse of the bitmask
            flags.fetch_and(~bitmask, std::memory_order_release);
        }
    }

    // Clears all the flags by resetting 'flags' to 0
    void clear_all() {
      flags = 0;  // Direct assignment to clear all bits
    }
};

#endif
