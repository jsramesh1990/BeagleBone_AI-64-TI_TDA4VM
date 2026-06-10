/**
 * I/O Port Configuration for 8051
 * Hardware-specific port definitions and configurations
 */

#ifndef IO_CONFIG_H
#define IO_CONFIG_H

#include <reg51.h>

/**
 * PORT DEFINITIONS
 */
#define LED_PORT     P1      // Port 1 for LED output
#define BUTTON_PORT  P0      // Port 0 for button input
#define DISPLAY_PORT P2      // Port 2 for 7-segment display
#define AUX_PORT     P3      // Port 3 for auxiliary functions

/**
 * PIN DEFINITIONS
 */
// LED pins on P1
#define LED0   0
#define LED1   1
#define LED2   2
#define LED3   3
#define LED4   4
#define LED5   5
#define LED6   6
#define LED7   7

// Button pins on P0
#define BUTTON0   0
#define BUTTON1   1
#define BUTTON2   2
#define BUTTON3   3

// Special function pins on P3
#define RX_PIN   0    // P3.0 - Serial Receive
#define TX_PIN   1    // P3.1 - Serial Transmit
#define INT0_PIN 2    // P3.2 - External Interrupt 0
#define INT1_PIN 3    // P3.3 - External Interrupt 1
#define T0_PIN   4    // P3.4 - Timer 0 External Input
#define T1_PIN   5    // P3.5 - Timer 1 External Input
#define WR_PIN   6    // P3.6 - External Memory Write Strobe
#define RD_PIN   7    // P3.7 - External Memory Read Strobe

/**
 * PORT DIRECTION CONFIGURATION
 * 1 = Input, 0 = Output
 * Note: For 8051, writing 1 to a pin makes it input (open-drain)
 *       Writing 0 makes it output
 */
#define PORT0_DIRECTION  0xFF    // All pins as input (with pull-ups)
#define PORT1_DIRECTION  0x00    // All pins as output (for LEDs)
#define PORT2_DIRECTION  0xF0    // Upper nibble input, lower nibble output
#define PORT3_DIRECTION  0xFF    // Default as input for special functions

/**
 * INITIALIZATION FUNCTIONS
 */
void init_all_ports(void);
void init_port0_as_input(void);
void init_port1_as_output(void);
void init_port2_bidirectional(void);
void init_port3_special(void);

/**
 * LED CONTROL FUNCTIONS
 */
void turn_on_led(unsigned char led_number);
void turn_off_led(unsigned char led_number);
void toggle_led(unsigned char led_number);
void set_led_pattern(unsigned char pattern);
void clear_all_leds(void);

/**
 * BUTTON READING FUNCTIONS
 */
unsigned char read_button(unsigned char button_number);
unsigned char read_all_buttons(void);
unsigned char wait_for_button_press(unsigned char button_number);

/**
 * PORT TESTING FUNCTIONS
 */
void test_port_output(unsigned char port);
void test_port_input(unsigned char port);
void verify_port_configuration(void);

#endif /* IO_CONFIG_H */
