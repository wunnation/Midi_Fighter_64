// LED functions for DJTT Midifighter2
//
//   Copyright (C) 2009-2011 Robin Green
//
// rgreen 2009-05-22

 /* DJTT - MIDI Fighter 64 - Embedded Software License
 * Copyright (c) 2016: DJ Tech Tools
 * Permission is hereby granted, free of charge, to any person owning or possessing 
 * a DJ Tech-Tools MIDI Fighter 64 Hardware Device to view and modify this source 
 * code for personal use. Person may not publish, distribute, sublicense, or sell 
 * the source code (modified or un-modified). Person may not use this source code 
 * or any diminutive works for commercial purposes. The permission to use this source 
 * code is also subject to the following conditions:
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,  FITNESS FOR A 
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION 
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
	*/

#ifndef _LED_H_INCLUDED
#define _LED_H_INCLUDED

#include <stdbool.h>
#include <stddef.h>
#include <string.h>    // for memset()
#include <stdlib.h>    // for div()
#include <math.h>

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/wdt.h>

#include "constants.h" // For firmware configuration options

// Import globals -------------------
#define LED_CONFIGURATION_SINGLE_STRAND 0 // prototypes
#define LED_CONFIGURATION_FOUR_STRANDS 1 // production units
#define LED_CONFIGURATION LED_CONFIGURATION_FOUR_STRANDS

// animation counters
extern uint16_t g_led_counter[4];
extern uint8_t display_cycle_counter;
extern uint16_t display_flash_counter;
extern uint16_t midi_clock_counter;
extern uint16_t half_ms_counter;
extern uint8_t one_second_counter;
extern uint8_t one_minute_counter;
extern uint8_t sleep_minute_counter;

// Basic functions ------------------
void led_setup(void);
void led_disable(void);
void led_enable(void);
void led_update_pixels(uint8_t *buffer);

// for compatibility with original MF code
void led_update_pixel_group0(uint8_t *buffer);
void led_update_pixel_group1(uint8_t *buffer);
void led_update_pixel_group2(uint8_t *buffer);
void led_update_pixel_group3(uint8_t *buffer);
void led_set_state(uint16_t new_state, uint32_t color);
void led_set_state_dfu(void);

// Background effects ----------------
void rainbow_setup(void);
bool rainbow_run(uint8_t *buffer);

void ball_demo_setup(void);
void ball_demo_run(uint8_t *buffer);

// Tests
#if ENABLE_TEST_IN_LED_CALIBRATION > 0
void led_calibration_test(void);
void led_set_test_colors(uint8_t red, uint8_t green, uint8_t blue);
#endif

// -----------------------------------

#endif // _LED_H_INCLUDED
