/**
 * 8051 I/O Port Configuration using Bitwise Operations
 * Main Program File
 * Date: [Current Date]
 * Author: [Your Name]
 */

#include <reg51.h>
#include "bitwise_operations.h"
#include "io_config.h"
#include "delay.h"

// Function prototypes
void initialize_ports(void);
void test_bitwise_operations(void);
void led_knight_rider(void);
void button_led_control(void);
void binary_counter(void);

/**
 * Main function
 */
void main(void) {
    // Initialize all ports
    initialize_ports();
    
    // Main program loop
    while(1) {
        // Test 1: Basic bitwise operations
        test_bitwise_operations();
        delay_ms(1000);
        
        // Test 2: LED knight rider pattern
        led_knight_rider();
        
        // Test 3: Button controlled LEDs
        button_led_control();
        
        // Test 4: Binary counter on LEDs
        binary_counter();
    }
}

/**
 * Initialize all I/O ports
 */
void initialize_ports(void) {
    // Configure P0 as input port (with pull-up resistors)
    P0 = 0xFF;  // Write 1s to input port
    
    // Configure P1 as output port for LEDs
    P1 = 0x00;  // Start with all LEDs OFF
    
    // Configure P2 as bidirectional (some input, some output)
    P2 = 0x0F;  // Lower nibble output, upper nibble input
    
    // Configure P3 for special functions (optional)
    P3 = 0xFF;
}

/**
 * Demonstrate various bitwise operations
 */
void test_bitwise_operations(void) {
    unsigned char value = 0x0F;
    
    // 1. Setting a specific bit (bit 3)
    SET_BIT(P1, 3);
    delay_ms(500);
    
    // 2. Clearing a specific bit (bit 3)
    CLEAR_BIT(P1, 3);
    delay_ms(500);
    
    // 3. Toggling bits (bit 0-3)
    TOGGLE_BIT(P1, 0);
    TOGGLE_BIT(P1, 1);
    TOGGLE_BIT(P1, 2);
    TOGGLE_BIT(P1, 3);
    delay_ms(500);
    
    // 4. Checking if a bit is set
    if(IS_BIT_SET(P0, 0)) {
        // Button pressed on P0.0
        SET_BIT(P1, 7);
    } else {
        CLEAR_BIT(P1, 7);
    }
    
    // 5. Setting multiple bits
    SET_MULTIPLE_BITS(P1, 0xAA);  // Pattern 10101010
    delay_ms(500);
    CLEAR_MULTIPLE_BITS(P1, 0xAA);
    
    // 6. Rotating bits
    value = ROTATE_LEFT(value, 2);
    P1 = value;
    delay_ms(500);
    
    // 7. Extracting nibbles
    unsigned char upper_nibble = GET_UPPER_NIBBLE(P2);
    unsigned char lower_nibble = GET_LOWER_NIBBLE(P2);
    
    // Display on P1
    P1 = (upper_nibble << 4) | lower_nibble;
    delay_ms(1000);
}

/**
 * Knight Rider LED pattern
 */
void led_knight_rider(void) {
    unsigned char pattern = 0x01;
    int i, direction = 1;
    
    for(i = 0; i < 20; i++) {
        P1 = pattern;
        delay_ms(100);
        
        if(direction) {
            pattern = pattern << 1;
            if(pattern == 0x80) direction = 0;
        } else {
            pattern = pattern >> 1;
            if(pattern == 0x01) direction = 1;
        }
    }
    P1 = 0x00;
}

/**
 * Button controlled LEDs
 */
void button_led_control(void) {
    unsigned char button_state;
    
    // Read buttons from P0 (assumes buttons on lower nibble)
    button_state = P0 & 0x0F;
    
    // Map button presses to LED patterns on P1
    switch(button_state) {
        case 0x0E:  // Button 0 pressed
            P1 = 0x01;
            break;
        case 0x0D:  // Button 1 pressed
            P1 = 0x03;
            break;
        case 0x0B:  // Button 2 pressed
            P1 = 0x07;
            break;
        case 0x07:  // Button 3 pressed
            P1 = 0x0F;
            break;
        default:
            P1 = 0x00;
    }
    delay_ms(200);
}

/**
 * Binary counter on LEDs
 */
void binary_counter(void) {
    unsigned char count;
    
    for(count = 0; count < 16; count++) {
        P1 = count;  // Display binary count on LEDs
        delay_ms(500);
    }
}
