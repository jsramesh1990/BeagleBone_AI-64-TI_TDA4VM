/**
 * LED Pattern Generator
 * Various LED patterns using bitwise operations
 */

#include "bitwise_operations.h"
#include "io_config.h"
#include "delay.h"

/**
 * Knight Rider scanning pattern
 */
void knight_rider_pattern(void) {
    unsigned char pattern = 0x01;
    int i;
    
    // Move left to right
    for(i = 0; i < 7; i++) {
        LED_PORT = pattern;
        pattern = pattern << 1;
        delay_ms(100);
    }
    
    // Move right to left
    for(i = 0; i < 7; i++) {
        LED_PORT = pattern;
        pattern = pattern >> 1;
        delay_ms(100);
    }
    
    LED_PORT = 0x00;
}

/**
 * Binary counter pattern
 */
void binary_counter_pattern(void) {
    unsigned char count;
    
    for(count = 0; count <= 255; count++) {
        LED_PORT = count;
        delay_ms(200);
    }
}

/**
 * Running lights pattern
 */
void running_lights(void) {
    int i;
    
    // Forward running lights
    for(i = 0; i < 8; i++) {
        SET_BIT(LED_PORT, i);
        delay_ms(100);
        CLEAR_BIT(LED_PORT, i);
    }
    
    // Backward running lights
    for(i = 7; i >= 0; i--) {
        SET_BIT(LED_PORT, i);
        delay_ms(100);
        CLEAR_BIT(LED_PORT, i);
    }
}

/**
 * Alternating pattern
 */
void alternating_pattern(void) {
    int i;
    
    for(i = 0; i < 10; i++) {
        // Even LEDs on, odd LEDs off
        LED_PORT = 0xAA;  // 10101010
        delay_ms(300);
        
        // Odd LEDs on, even LEDs off
        LED_PORT = 0x55;  // 01010101
        delay_ms(300);
    }
    
    LED_PORT = 0x00;
}

/**
 * Breathing LED effect (PWM simulation)
 */
void breathing_led(void) {
    int i, j;
    
    // Fade in
    for(i = 0; i < 10; i++) {
        for(j = 0; j < 8; j++) {
            if(j < i) {
                SET_BIT(LED_PORT, j);
            } else {
                CLEAR_BIT(LED_PORT, j);
            }
        }
        delay_ms(100);
    }
    
    // Fade out
    for(i = 10; i > 0; i--) {
        for(j = 0; j < 8; j++) {
            if(j < i) {
                SET_BIT(LED_PORT, j);
            } else {
                CLEAR_BIT(LED_PORT, j);
            }
        }
        delay_ms(100);
    }
    
    LED_PORT = 0x00;
}

/**
 * Random pattern generator
 */
void random_pattern(void) {
    unsigned char pattern;
    int i;
    
    // Simple pseudo-random pattern
    pattern = 0x01;
    
    for(i = 0; i < 20; i++) {
        LED_PORT = pattern;
        
        // Generate next pattern (simple LFSR)
        if(pattern & 0x01) {
            pattern = (pattern >> 1) | 0x80;
        } else {
            pattern = pattern >> 1;
        }
        
        delay_ms(150);
    }
    
    LED_PORT = 0x00;
}
