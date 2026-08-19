/**
 * Bitwise Operations Library for 8051
 * Common bit manipulation macros and functions
 */

#ifndef BITWISE_OPERATIONS_H
#define BITWISE_OPERATIONS_H

/**
 * MACRO DEFINITIONS FOR BIT MANIPULATION
 */

// Set a specific bit (make it HIGH/1)
#define SET_BIT(port, bit)       ((port) |= (1 << (bit)))

// Clear a specific bit (make it LOW/0)
#define CLEAR_BIT(port, bit)     ((port) &= ~(1 << (bit)))

// Toggle a specific bit (flip its state)
#define TOGGLE_BIT(port, bit)    ((port) ^= (1 << (bit)))

// Check if a bit is set (returns non-zero if set)
#define IS_BIT_SET(port, bit)    ((port) & (1 << (bit)))

// Check if a bit is clear (returns non-zero if clear)
#define IS_BIT_CLEAR(port, bit)  (!((port) & (1 << (bit))))

// Set multiple bits using a mask
#define SET_MULTIPLE_BITS(port, mask)    ((port) |= (mask))

// Clear multiple bits using a mask
#define CLEAR_MULTIPLE_BITS(port, mask)  ((port) &= ~(mask))

// Write a specific value to a bit
#define WRITE_BIT(port, bit, value) \
    ((value) ? SET_BIT((port), (bit)) : CLEAR_BIT((port), (bit)))

// Rotate left with wrap-around
#define ROTATE_LEFT(value, shift) \
    (((value) << (shift)) | ((value) >> (8 - (shift))))

// Rotate right with wrap-around
#define ROTATE_RIGHT(value, shift) \
    (((value) >> (shift)) | ((value) << (8 - (shift))))

// Extract upper nibble (bits 7-4)
#define GET_UPPER_NIBBLE(value)  (((value) >> 4) & 0x0F)

// Extract lower nibble (bits 3-0)
#define GET_LOWER_NIBBLE(value)  ((value) & 0x0F)

// Combine two nibbles into a byte
#define COMBINE_NIBBLES(upper, lower) \
    (((upper) << 4) | ((lower) & 0x0F))

/**
 * FUNCTION PROTOTYPES
 */

// Initialize port as input with pull-up
void init_port_as_input(unsigned char *port);

// Initialize port as output
void init_port_as_output(unsigned char *port);

// Read a specific pin from input port
unsigned char read_pin(unsigned char port, unsigned char pin);

// Write to a specific pin on output port
void write_pin(unsigned char *port, unsigned char pin, unsigned char value);

// Read entire port
unsigned char read_port(unsigned char port);

// Write to entire port
void write_port(unsigned char *port, unsigned char value);

// Create a blinking pattern on specified bits
void blink_pattern(unsigned char *port, unsigned char pattern, unsigned int duration);

// Debounce button input
unsigned char debounce_button(unsigned char port, unsigned char pin);

#endif /* BITWISE_OPERATIONS_H */
