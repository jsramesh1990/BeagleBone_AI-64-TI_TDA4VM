/**
 * Delay Functions for 8051
 * Provides precise timing delays for various operations
 */

#ifndef DELAY_H
#define DELAY_H

/**
 * Delay in milliseconds
 * @param ms: Number of milliseconds to delay
 * Note: Assuming 11.0592 MHz crystal frequency
 */
void delay_ms(unsigned int ms);

/**
 * Delay in microseconds
 * @param us: Number of microseconds to delay
 * Note: Approximate delay for small values
 */
void delay_us(unsigned int us);

/**
 * Delay in seconds
 * @param seconds: Number of seconds to delay
 */
void delay_seconds(unsigned char seconds);

/**
 * Busy wait delay using NOP instructions
 * @param count: Number of iterations
 */
void busy_delay(unsigned int count);

/**
 * Configure timer for precise delays
 */
void timer_delay_init(void);
void timer_delay_ms(unsigned int ms);

#endif /* DELAY_H */
