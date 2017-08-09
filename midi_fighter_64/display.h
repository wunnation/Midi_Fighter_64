// LED Displays for DJTechTools Midifighter2
//
//   Copyright (C) 2011 DJ Techtools
//
// rgreen 2011-03-14

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

#ifndef _DISPLAY_H_INCLUDED
#define _DISPLAY_H_INCLUDED

#include <stdint.h>
#include <stdbool.h>

// Constants ------------------------------------------------------------------
#define ENABLE_RGB_TEST 0

#define SIXTEENTH_FLASH_STATE   0x01

#define DISPLAY_SCALING_COLOR_IN_MAX_VALUE 48  // should usually be the max value displayed in default_bank_inactive
#define DISPLAY_SCALING_COLOR_OUT_MAX_VALUE 127

// - Geometric Animations
// -- Grid Properties
#define GEOMETRIC_ANIMATION_ROWS 8
#define GEOMETRIC_ANIMATION_COLS 8
#define GEOMETRIC_ANIMATION_G_LED_IDX 3
#define GEOMETRIC_ANIMATION_G_LED_LIMIT	0x70 // Time delay between animation steps = LIMIT*0.5ms
// -- Animation Types
#define GEOMETRIC_ANIMATION_TYPES 4
#define GEOMETRIC_ANIMATION_TYPE_SQUARE 0
#define GEOMETRIC_ANIMATION_TYPE_CIRCLE 1
#define GEOMETRIC_ANIMATION_TYPE_STAR 2 // or 'x's
#define GEOMETRIC_ANIMATION_TYPE_TRIANGLE 3
// --- Animation Lengths by Type
#define GEOMETRIC_ANIMATION_STEPS_SQUARE 8
#define GEOMETRIC_ANIMATION_STEPS_CIRCLE 14
#define GEOMETRIC_ANIMATION_STEPS_STAR 11 // Should be 8+STAR_MAX_TAIL_LENGTH
#define GEOMETRIC_ANIMATION_STEPS_TRIANGLE 15

#define GEOMETRIC_STAR_MAX_TAIL_LENGTH 3 // how many squares should the star expand through?

// Globals --------------------------------------------------------------------

// Storage for the LED state
extern uint8_t g_display_buffer[64 * 3];
extern uint16_t g_level_display_mask;
extern const uint8_t default_color[20][3];

// functions ------------------------------------------------------------------
// - LED Refreshing
void load_default_colors(void);
void default_display_run(void); 

// - Animations
uint8_t pulse_animation(uint8_t pulse_rate);
bool flash_animation(uint8_t flash_rate);
void start_geometric_square_animation(uint8_t button_id);
void start_geometric_circle_animation(uint8_t button_id);
void start_geometric_star_animation(uint8_t button_id);
void start_geometric_triangle_animation(uint8_t button_id);
void start_geometric_animation(uint8_t button_id, uint8_t animation_id);
uint8_t get_button_id_from_row_column(uint8_t button_row, uint8_t button_column);

// - Sysex Configuration Extensions
void adjust_inactive_bank_leds_for_power(uint8_t bank, uint8_t offset, uint8_t size);
void adjust_active_bank_leds_for_power(uint8_t bank, uint8_t offset, uint8_t size);

// ----------------------------------------------------------------------------

#endif // _DISPLAY_H_INCLUDED
