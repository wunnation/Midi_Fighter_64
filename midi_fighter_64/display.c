// LED Displays for DJTechTools Midifighter2
//
//   Copyright (C) 2011 DJ Techtools
//
// Robin   Green    2011-03-14
// Michael Mitchell 2012-01-19

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

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "key.h"
#include "midi.h"
#include "random.h"
#include "display.h"
#include "accel_gyro.h"
#include "led.h"
#include "eeprom.h"
#include "config.h" // for calling Midifighter_GetIncomingUsbMidiMessages()

// Globals --------------------------------------------------------------------

// Storage for the LED state
uint8_t g_display_buffer[64 * 3]; // This can be reduced to 64: we are treating two leds as one since they refer to a single button
// - other storage !review
uint8_t x_value;
uint8_t y_value;
uint16_t g_level_display_mask;

// Pulse LEDs: To get symmetrical cycle make sure that 
// rgb_freq  x 32 = n * 2pi
static float rgb_freq = 0.049f;

// Prototypes -----------------------------------------------------------------

void left_fade(const uint8_t value, uint8_t *buffer);
void right_fade(const uint8_t value, uint8_t *buffer);
void front_fade(const uint8_t value, uint8_t *buffer);
void back_fade(const uint8_t value, uint8_t *buffer);

// Locals ---------------------------------------------------------------------


// Four banks of RGB colors for the displays.
// Each bank has two states, default and active.
//
// NOTE: This should take up 16*3*4*2 = 384 bytes of EEPROM to store. We have 1KB.

uint8_t default_bank_inactive[2][64*3]; // Init now done in load_default_colors()
uint8_t default_bank_active[2][64*3]; // Init now done in load_default_colors()


enum DefaultColorIds {
	COLORID_OFF = 0,
	COLORID_RED = 1,
	COLORID_RED_DIM = 2,
	COLORID_ORANGE = 3,
	COLORID_ORANGE_DIM = 4,
	COLORID_YELLOW = 5,
	COLORID_YELLOW_DIM = 6,
	COLORID_CHARTREUSE = 7,
	COLORID_CHARTREUSE_DIM = 8,
	COLORID_GREEN = 9,
	COLORID_GREEN_DIM = 10,
	COLORID_CYAN = 11,
	COLORID_CYAN_DIM = 12,
	COLORID_BLUE = 13,
	COLORID_BLUE_DIM = 14,
	COLORID_LAVENDER = 15,
	COLORID_LAVENDER_DIM = 16,
	COLORID_PINK = 17,
	COLORID_PINK_DIM = 18,
	COLORID_WHITE = 19
	};

// This array holds the default colors, matched to those in the utility

const uint8_t default_color[20][3] = { // This array is used to remap sysex rgb values in to a small subset of colors.
	// Listed as RGB but actual order that this is sent to the led controller is BRG
	{0x00,0x00,0x00},   // 0: Off 
	{48,0x00,0x00},		// Red
	{24,0x00,0x00},		// Dim Red
	{40,12,0x00},   	// Orange // shows pink
	{20,6,0x00},		// Dim Orange
	{32,25,0x00},   	// Yellow
	{16,12,0x00},		// Dim Yellow
	{25,32,0x00},   // Chartreuse
	{12,16,0x00},	// 8: Dim Chartreuse
	{0x00,48,0x00},   	// Green
	{0x00,24,0x00},		// Dim Green
	{0x00,30,30},   // Cyan
	{0x00,15,15},	// Dim Cyan
	{0x00,0x00,48},   	// Blue
	{0x00,0x00,24},		// Dim Blue

	{25,7,32},   // Lavender (Dark Orchid) rgb(153,50,204)
	{13,3,17},	// 16: Dim Lavender

	{36,0x00,18},   // Pink
	{18,0x00,9},	// Dim Pink
	{24,24,24},   // 19: White
};

#if MIDI_FEEDBACK_MODE == MIDI_FEEDBACK_ABLETON_MODE
const uint8_t ableton_midi_feedback_colors[128][3] = {
	// - [0-49] provide 15 colors each with 4 shades. [60-127] provide 68 unique colors
	// - for full details on these colors that are mapped to midi velocities.
	// -- View the spreadsheet at 'repository root directory'/docs/ableton_live_color_scheme.xlsx'
	{0,0,0}, // 0
	{8,8,8},
	{16,16,16},
	{24,24,24},
	{40,12,12},
	{48,0,0},
	{32,0,0},
	{16,0,0},
	{30,22,12}, // 8
	{42,13,0},
	{28,9,0},
	{14,4,0},
	{28,28,8},
	{30,30,0},
	{20,20,0},
	{10,10,0},
		
	{20,38,11},  // 16
	{13,42,0},
	{9,28,0},
	{4,14,0},
	{11,40,11},
	{0,48,0},
	{0,32,0},
	{0,16,0},
	{11,38,14},	// 24
	{0,44,4},
	{0,29,3},
	{0,15,2},
	{10,36,19},
	{6,40,14},
	{4,26,9},
	{2,13,4},
		
	{9,32,22},	// 32
	{0,38,22},
	{0,25,14},
	{0,12,7},
	{9,24,32},
	{0,23,36},
	{0,15,24},
	{0,7,12},
	{10,19,36}, // 40
	{0,19,39},
	{0,13,26},
	{0,6,13},
	{11,11,40},
	{0,0,48},
	{0,0,32},
	{0,0,16},
		
	{19,10,36},	// 48
	{19,0,39},
	{13,0,26},
	{6,0,13},
	{28,8,28},
	{30,0,30},
	{20,0,20},
	{10,0,10},
	{36,10,19},	// 56
	{42,0,13},
	{28,0,8},
	{14,0,4},
	{44,3,0},
	{28,9,0},
	{22,15,0},
	{12,18,0},
		
	{0,10,0},	// 64
	{0,16,9},
	{0,15,23},
	{0,0,48},
	{0,12,14},
	{6,0,38},
	{16,16,16},
	{8,8,8},
	{48,0,0},	// 72
	{23,32,5},
	{24,33,1},
	{15,38,1},
	{3,26,0},
	{0,38,20},
	{0,25,38},
	{0,7,44},
		
	{15,0,42},	// 80
	{19,0,40},
	{33,4,23},
	{12,6,0},
	{42,12,0},
	{22,37,1},
	{18,40,3},
	{0,48,0},
	{9,42,6},	// 88
	{12,36,15},
	{7,32,25},
	{13,19,36},
	{9,15,37},
	{19,18,33},
	{24,3,30},
	{40,0,15},	
		
	{40,15,0},	// 96
	{34,14,0},
	{20,36,0},
	{24,17,1},
	{10,8,0},
	{3,14,3},
	{2,15,10},	
	{3,3,7},	
	{4,6,16},	// 104
	{19,11,5},
	{31,0,1},
	{37,13,10},
	{37,18,5},
	{30,27,5},
	{23,33,7},
	{19,34,3},
		
	{5,5,9},	// 112
	{24,28,12},
	{15,30,22},
	{19,19,32},
	{19,14,34},
	{6,6,6},
	{11,11,11},
	{21,24,24},
	{30,0,0},	// 120
	{9,0,0},
	{4,39,0},
	{1,12,0},
	{29,28,0},
	{11,9,0},
	{33,17,0},
	{14,3,0},
};
#endif
// local functions ------------------------------------------------------------

void load_default_colors(void) {
	uint8_t index = 0;
	for (uint8_t this_button=0; this_button < 64; this_button++) { // Quadrant 1: Lavender/ Green 64=NUM_BUTTONS
		default_bank_inactive[0][index] = default_color[COLORID_OFF][0];
		default_bank_inactive[0][index+1] = default_color[COLORID_OFF][1];
		default_bank_inactive[0][index+2] = default_color[COLORID_OFF][2];
		default_bank_active[0][index] = default_color[COLORID_BLUE][0];
		default_bank_active[0][index+1] = default_color[COLORID_BLUE][1];
		default_bank_active[0][index+2] = default_color[COLORID_BLUE][2];	
		index += 3;
	}
	index = 0;
	for (uint8_t this_button=0; this_button < 64; this_button++) { // Quadrant 1: Lavender/ Green 64=NUM_BUTTONS
		default_bank_inactive[1][index] = default_color[COLORID_WHITE][0];
		default_bank_inactive[1][index+1] = default_color[COLORID_WHITE][1];
		default_bank_inactive[1][index+2] = default_color[COLORID_WHITE][2];
		default_bank_active[1][index] = default_color[COLORID_GREEN][0];
		default_bank_active[1][index+1] = default_color[COLORID_GREEN][1];
		default_bank_active[1][index+2] = default_color[COLORID_GREEN][2];
		index += 3;
	}
}

void info_display_active(void) //const uint8_t bank, const uint16_t key_state, uint8_t *buffer)
				// info_display_active(g_bank_selected, g_key_state, g_display_buffer);
{
    // Overwrite active LEDs with active color state (LED Local Control by Button State: On)
    uint64_t key_bit = 1;
    const uint8_t *active_src = default_bank_active[g_bank_selected]; // active_state_buffer;
	const uint8_t *inactive_src = default_bank_inactive[g_bank_selected]; //inactive_state_buffer;
    uint8_t *dest = g_display_buffer;
    for (uint8_t i=0; i<NUM_BUTTONS; ++i) {  
		#if USB_RX_METHOD >= USB_RX_PERIODICALLY
		Midifighter_GetIncomingUsbMidiMessages();
		#endif

        if(g_key_state & key_bit) {
			// !review: LED Colors are inverted for the MF64 Hardware (led communications) (BRG instead of RGB), this is one place we reverse their order
			*dest++ = *(active_src+2);
            *dest++ = *(active_src); 
            *dest++ = *(active_src+1);
			active_src += 3;
			inactive_src += 3;
        } else {
			// !review: LED Colors are inverted for the MF64 Hardware (led communications) (BRG instead of RGB), this is one place we reverse their order
			*dest++ = *(inactive_src+2);
            *dest++ = *(inactive_src); 
            *dest++ = *(inactive_src+1);			
			active_src += 3;
			inactive_src += 3;
	        //dest += 3;
        }
        key_bit <<= 1;
    }
}

#if MIDI_FEEDBACK_MODE == MIDI_FEEDBACK_ABLETON_MODE
#warning ABLETON LIVE Midi Feedback Mode
void midi_color_state(const uint8_t bank, uint8_t *buffer) // (Midi Feedback - Button Colors (Usage)) g_bank_selected, g_display_buffer
{
	// Override the display state of a given arcade button with a color set by
	// the velocity of a Note On event of the same pitch.
	// - color scheme is that of ableton live 2017

	uint8_t bank_offset = g_bank_selected * NUM_BUTTONS;
	for (uint8_t i=bank_offset; i<bank_offset + NUM_BUTTONS; ++i) {
		uint8_t velocity = g_midi_note_state[0][i]; // Arcade button color info is stored in the first array
		if (velocity > 0 ){
			uint8_t key = i & BUTTON_ID_FLAGS; // treat as 64 buttons, only points to g_display_buffer which only stores active bank
			uint8_t *ptr = buffer + key * 3;
			// !review: LED Colors are inverted for the MF64 Hardware (led communications) (BRG instead of RGB), this is one place we reverse their order
			*ptr++ = ableton_midi_feedback_colors[velocity][2];
			*ptr++ = ableton_midi_feedback_colors[velocity][0];
			*ptr++ = ableton_midi_feedback_colors[velocity][1];		
		}
	}
}

#elif MIDI_FEEDBACK_MODE == MIDI_FEEDBACK_MF3D_MODE
#warning Midi Fighter 3D Midi Feedback Mode
void midi_color_state(const uint8_t bank, uint8_t *buffer) // (Midi Feedback - Button Colors (Usage)) g_bank_selected, g_display_buffer 
{
	// Override the display state of a given arcade button with a color set by
	// the velocity of a Note On event of the same pitch.
	// Color overrides are sent on the same channel
	// The settings are arrange as such
	// Velocity		Action
	//	      0     No Change
	//   1 - 120    Each of the 20 default colors, each color spans a velocity range of 6
	// 121 - 127    Active Color
	
	// pre banking uint8_t bank_offset = MIDI_BASENOTE + g_bank_selected * 64; //!bank64 probably needs adjustment
	uint8_t bank_offset = g_bank_selected * NUM_BUTTONS;
	for (uint8_t i=bank_offset; i<bank_offset + NUM_BUTTONS; ++i) {
		
		uint8_t velocity = g_midi_note_state[0][i]; // Arcade button color info is stored in the first array
		uint8_t key = i & BUTTON_ID_FLAGS; // treat as 64 buttons, only points to g_display_buffer which only stores active bank
		
		if (velocity > 0) {
			if (velocity <121) {
				// Create a color index from the velocity
				uint8_t color = clamp(((velocity-1)/6)-1,0,19);
				uint8_t *ptr = buffer + key * 3;
				// Overwrite the color information
				// !review: LED Colors are inverted for the MF64 (BRG instead of RGB), this is one place we reverse their order
				*ptr++ = default_color[color][2];
				*ptr++ = default_color[color][0];
				*ptr++ = default_color[color][1];
			}
			else if (velocity < 127) {
				// Create a color index from the velocity
				uint8_t color = COLORID_WHITE;
				uint8_t *ptr = buffer + key * 3;
				// Overwrite the color information
				// !review: LED Colors are inverted for the MF64 (BRG instead of RGB), this is one place we reverse their order
				*ptr++ = default_color[color][2];
				*ptr++ = default_color[color][0];
				*ptr++ = default_color[color][1];				
			}
			else {	
				uint8_t *src = default_bank_active[g_bank_selected] + key * 3 ;
				uint8_t *ptr = buffer + key * 3;
				// !review: LED Colors are inverted for the MF64 (BRG instead of RGB), this is one place we reverse their order
				*ptr++ = src[2];
				*ptr++ = src[0];
				*ptr++ = src[1];
			}
		}
	}
}
#endif //MIDI_FEEDBACK_MODE == MIDI_FEEDBACK_MF3D_MODE

// !review: move animations to a new file and keep this small? (animations.c and .h)
#define CONCURRENT_GEOMETRIC_ANIMATIONS 4
uint8_t geometric_animation_btn_id[CONCURRENT_GEOMETRIC_ANIMATIONS] = {0,0,0,0};
uint8_t geometric_animation_type[CONCURRENT_GEOMETRIC_ANIMATIONS] = {GEOMETRIC_ANIMATION_TYPE_SQUARE, GEOMETRIC_ANIMATION_TYPE_SQUARE, GEOMETRIC_ANIMATION_TYPE_SQUARE, GEOMETRIC_ANIMATION_TYPE_SQUARE};
uint8_t geometric_animation_pos[CONCURRENT_GEOMETRIC_ANIMATIONS] = {GEOMETRIC_ANIMATION_STEPS_SQUARE, GEOMETRIC_ANIMATION_STEPS_SQUARE, GEOMETRIC_ANIMATION_STEPS_SQUARE, GEOMETRIC_ANIMATION_STEPS_SQUARE};
uint8_t geometric_animation_color_id[CONCURRENT_GEOMETRIC_ANIMATIONS] = {0,0,0,0}; //  default_colors index
uint8_t * geometric_animation_color_ptr[CONCURRENT_GEOMETRIC_ANIMATIONS]; // array of pointers

uint8_t geometric_animation_id = 0;
uint8_t assign_geometric_animation_id = 0;


void set_geometric_animation_color_source(uint8_t button_id) {
	// read midi value and use that color to set the pointer
	// - note: may need to add keypos_to_midipos for other devices (on 64 keypos and midipos are the same)
	uint8_t velocity = g_midi_note_state[0][button_id+NUM_BUTTONS*g_bank_selected]; // Arcade button color info is stored in the first array
	if (velocity <= 0 || velocity >= 121) {
		geometric_animation_color_ptr[assign_geometric_animation_id] = default_bank_active[g_bank_selected] + button_id*3;
	}
	else {
		uint8_t color = clamp(((velocity-1)/6)-1,0,19);
		geometric_animation_color_ptr[assign_geometric_animation_id] = default_color[color];
	}
}

// Row, Column: starts at BOTTOM-LEFT Corner (bottom row is row0, left column is column0)
// - mf 64 is wired as 4 individual 4x4 grids, the get_geometric functions convert this wiring.
uint8_t get_geometric_button_row(void) {
	uint8_t this_row = (geometric_animation_btn_id[geometric_animation_id] >> 2 ) & 0x07;
	return this_row;
}
uint8_t get_geometric_button_column(void) {
	uint8_t this_column = geometric_animation_btn_id[geometric_animation_id] & 0x03;
	if (geometric_animation_btn_id[geometric_animation_id]  >= 32) {
		this_column = this_column + 4;
	}
	return this_column;
}
// - this converts geometry back to button_id
uint8_t get_button_id_from_row_column(uint8_t button_row, uint8_t button_column) {
	if (button_row >= GEOMETRIC_ANIMATION_ROWS) {
		return 0xFF; // NUM_BUTTONS
	} 
	else if (button_column >= GEOMETRIC_ANIMATION_COLS) {
		return 0xFF;
	}
	else {
		uint8_t button_id = button_row*4;
		if (button_column < 4) {  // Left half of m64
			button_id = button_id + button_column;
		}
		else { // Right half of mf64
			button_id = button_id + (button_column & 0x03) + 32;
		}
		return button_id;
	}
}


// Geometric Square Animation (start)
void start_geometric_square_animation(uint8_t button_id) {
	geometric_animation_btn_id[assign_geometric_animation_id] = button_id;
	geometric_animation_pos[assign_geometric_animation_id] = 0;
	geometric_animation_type[assign_geometric_animation_id] = GEOMETRIC_ANIMATION_TYPE_SQUARE;
	set_geometric_animation_color_source(button_id);
	g_led_counter[GEOMETRIC_ANIMATION_G_LED_IDX] = GEOMETRIC_ANIMATION_G_LED_LIMIT;
}
void run_geometric_square_animation(uint8_t *buffer, bool update_animation) {
	// Increment the Counter
	if (update_animation) {
		// increment the animation state
		geometric_animation_pos[geometric_animation_id] = geometric_animation_pos[geometric_animation_id] + 1;
		if (geometric_animation_pos[geometric_animation_id] >= GEOMETRIC_ANIMATION_STEPS_SQUARE) { // exit if animation complete
			return; // !review: i belieive this is redundant to what is being done in run_geometric_animation()
		}
	}	
	
	// Set the led states
	if (!geometric_animation_pos[geometric_animation_id]) { // at first, only the button itself is lit
		return;
	}
	// run the animation. Let's light up some leds!
	else {
		// - get the 'on' color of the current led
		// - get column row position of the current button
		uint8_t btn_row = get_geometric_button_row();
		uint8_t btn_col = get_geometric_button_column();
		// - calculate the column/row limits for the current geometric_animation_position (outer edges of the square)
		uint8_t min_row = btn_row - geometric_animation_pos[geometric_animation_id]; // Note: we are depending on overflow
		uint8_t max_row = btn_row + geometric_animation_pos[geometric_animation_id];
		uint8_t min_col = btn_col - geometric_animation_pos[geometric_animation_id];
		uint8_t max_col = btn_col + geometric_animation_pos[geometric_animation_id];
		// - calculate the limits of the mf64, so we can determine what to show, and what is out of bounds
		uint8_t start_col = min_col < GEOMETRIC_ANIMATION_COLS ? min_col:0; // note: depends on 'overflow' in calculation of min_col
		uint8_t end_col = max_col < GEOMETRIC_ANIMATION_COLS ? max_col:GEOMETRIC_ANIMATION_COLS-1;
		uint8_t start_row = min_row < GEOMETRIC_ANIMATION_ROWS ? min_row:0;
		uint8_t end_row = max_row < GEOMETRIC_ANIMATION_ROWS ? max_row:GEOMETRIC_ANIMATION_ROWS-1;
		uint8_t *src = geometric_animation_color_ptr[geometric_animation_id];
		
		// - Light all applicable leds (full upper row, full lower row, outer square edges only on other rows
		for (uint8_t this_row = start_row; this_row <= end_row; this_row++) {
			for (uint8_t this_col = start_col; this_col <= end_col; this_col++) {
				bool light_led = false;
				if (this_row == min_row || this_row == max_row) { // Case: light all columns
					light_led = true;
				} else if (this_col == min_col || this_col == max_col) { // Case: only light outer edges of the square
					light_led = true;
				}
				
				if (light_led ){
					//else {light_led = false;}
					uint8_t btn_id = get_button_id_from_row_column(this_row, this_col);
					uint8_t led_index = btn_id*3;
					uint8_t *ptr = buffer + led_index;  // Copy To LED Buffer for this LED
					// !review: LED Colors are inverted for the MF64 (BRG instead of RGB), this is one place we reverse their order
					*ptr++ = src[2];
					*ptr++ = src[0];
					*ptr++ = src[1];
				}
			}
		}
	}
}
// Geometric Square Animation (end)

// Geometric Circle Animation (start)

void start_geometric_circle_animation(uint8_t button_id) {
	//uint8_t geometric_animation_id = 0;
	geometric_animation_btn_id[assign_geometric_animation_id] = button_id;
	geometric_animation_pos[assign_geometric_animation_id] = 0;
	geometric_animation_type[assign_geometric_animation_id] = GEOMETRIC_ANIMATION_TYPE_CIRCLE;
	set_geometric_animation_color_source(button_id);
	g_led_counter[GEOMETRIC_ANIMATION_G_LED_IDX] = GEOMETRIC_ANIMATION_G_LED_LIMIT;
}

uint8_t get_distance_from_center_in_steps(uint8_t btn_row, uint8_t btn_col, uint8_t this_row, uint8_t this_col) {
	uint8_t row_distance = btn_row > this_row ? btn_row-this_row: this_row-btn_row;
	uint8_t col_distance = btn_col > this_col ? btn_col-this_col: this_col-btn_col;
	return (row_distance + col_distance);
}

void run_geometric_circle_animation(uint8_t *buffer, bool update_animation) {
	
	if (update_animation) {
		// increment the animation state
		geometric_animation_pos[geometric_animation_id] = geometric_animation_pos[geometric_animation_id] + 1;
		if (geometric_animation_pos[geometric_animation_id] >= GEOMETRIC_ANIMATION_STEPS_CIRCLE) { // exit if animation complete
			return; // !review: i belieive this is redundant to what is being done in run_geometric_animation()
		}
	}	
	
	// Set the led states
	if (!geometric_animation_pos[geometric_animation_id]) { // at first, only the button itself is lit
		return;
	}
	else {
		// - get the 'on' color of the current led
		// - get column row position of the current button
		uint8_t btn_row = get_geometric_button_row();
		uint8_t btn_col = get_geometric_button_column();
		// - calculate the column/row limits for the current geometric_animation_position (outer edges of the square)
		uint8_t min_row = btn_row - geometric_animation_pos[geometric_animation_id]; // Note: we are depending on overflow
		uint8_t max_row = btn_row + geometric_animation_pos[geometric_animation_id];
		uint8_t min_col = btn_col - geometric_animation_pos[geometric_animation_id];
		uint8_t max_col = btn_col + geometric_animation_pos[geometric_animation_id];
		// - calculate the limits of the mf64, so we can determine what to show, and what is out of bounds
		uint8_t start_col = min_col < GEOMETRIC_ANIMATION_COLS ? min_col:0; // note: depends on 'overflow' in calculation of min_col
		uint8_t end_col = max_col < GEOMETRIC_ANIMATION_COLS ? max_col:GEOMETRIC_ANIMATION_COLS-1;
		uint8_t start_row = min_row < GEOMETRIC_ANIMATION_ROWS ? min_row:0;
		uint8_t end_row = max_row < GEOMETRIC_ANIMATION_ROWS ? max_row:GEOMETRIC_ANIMATION_ROWS-1;
		uint8_t *src = geometric_animation_color_ptr[geometric_animation_id];

		uint8_t circle_incrementer = geometric_animation_pos[geometric_animation_id] >> 1; // for every two states, we will add 1 to each side of the circle edges
		circle_incrementer = circle_incrementer <= 3 ? circle_incrementer:3; 
		
		uint8_t minimum_distance_steps = geometric_animation_pos[geometric_animation_id]; // in steps (in one step you can move one led horizontally OR vertically) (no diagonal in this calc)
		uint8_t maximum_distance_steps = geometric_animation_pos[geometric_animation_id] == 1 ? minimum_distance_steps: minimum_distance_steps+circle_incrementer; // in steps

		// - Light all applicable leds ()
		for (uint8_t this_row = start_row; this_row <= end_row; this_row++) {
			for (uint8_t this_col = start_col; this_col <= end_col; this_col++) {
				bool light_led = false;
				
				uint8_t distance_in_steps = get_distance_from_center_in_steps(btn_row, btn_col, this_row, this_col);
				if (distance_in_steps < minimum_distance_steps) {
				}
				else if (distance_in_steps == minimum_distance_steps) {
					if (this_row == min_row || this_row == max_row || this_col == min_col || this_col == max_col) {
						light_led = true;
					}
				}
				else if (distance_in_steps <= maximum_distance_steps) {
					light_led = true;
				}
				
				if (light_led ){
					//else {light_led = false;}
					uint8_t btn_id = get_button_id_from_row_column(this_row, this_col);
					uint8_t led_index = btn_id*3;
					uint8_t *ptr = buffer + led_index;  // Copy To LED Buffer for this LED
					// !review: LED Colors are inverted for the MF64 (BRG instead of RGB), this is one place we reverse their order
					*ptr++ = src[2];
					*ptr++ = src[0];
					*ptr++ = src[1];
				}
			}
		}
	}	
	
}
// Geometric Circle Animation (end)

// Geometric Star Animation (start)
void start_geometric_star_animation(uint8_t button_id) {
	geometric_animation_btn_id[assign_geometric_animation_id] = button_id;
	geometric_animation_pos[assign_geometric_animation_id] = 0;
	geometric_animation_type[assign_geometric_animation_id] = GEOMETRIC_ANIMATION_TYPE_STAR;
	set_geometric_animation_color_source(button_id);
	g_led_counter[GEOMETRIC_ANIMATION_G_LED_IDX] = GEOMETRIC_ANIMATION_G_LED_LIMIT;
}
void run_geometric_star_animation(uint8_t *buffer, bool update_animation) {
	
	if (update_animation) {
		// increment the animation state
		geometric_animation_pos[geometric_animation_id] = geometric_animation_pos[geometric_animation_id] + 1;
		if (geometric_animation_pos[geometric_animation_id] >= GEOMETRIC_ANIMATION_STEPS_STAR) { // exit if animation complete
			return; // !review: i belieive this is redundant to what is being done in run_geometric_animation()
		}		
	}
	// Set the led states
	if (!geometric_animation_pos[geometric_animation_id]) { // at first, only the button itself is lit
		return;
	}
	// run the animation. Let's light up some leds!
	else {
		// - get the 'on' color of the current led
		// - get column row position of the current button
		uint8_t btn_row = get_geometric_button_row();
		uint8_t btn_col = get_geometric_button_column();
		// - calculate the column/row limits for the current geometric_animation_position (outer edges of the square)
		uint8_t min_row = btn_row - geometric_animation_pos[geometric_animation_id]; // Note: we are depending on overflow
		uint8_t max_row = btn_row + geometric_animation_pos[geometric_animation_id];
		uint8_t min_col = btn_col - geometric_animation_pos[geometric_animation_id];
		uint8_t max_col = btn_col + geometric_animation_pos[geometric_animation_id];
		uint8_t *src = geometric_animation_color_ptr[geometric_animation_id];

		// - Light Five Lines Top Left, Top Center, Top Right, Bottom Left, Bottom Right
		uint8_t counter = geometric_animation_pos[geometric_animation_id];
		uint8_t tail_position = 0;
		while(counter > 0 && tail_position < GEOMETRIC_STAR_MAX_TAIL_LENGTH) { // Light up a string of up to TAIL_LENGTH lights to make the star more obvious
			// -- Draw Top Center Line
			uint8_t btn_id = get_button_id_from_row_column(max_row-tail_position, btn_col);
			if (btn_id < 64) { // 64= NUM_BUTTONS
				uint8_t led_index = btn_id*3;
				uint8_t *ptr = buffer + led_index;  // Copy To LED Buffer for this LED
				// !review: LED Colors are inverted for the MF64 (BRG instead of RGB), this is one place we reverse their order
				*ptr++ = src[2];
				*ptr++ = src[0];
				*ptr++ = src[1];
			}
			// -- Draw Bottom Center Line (8pt star only)
			btn_id = get_button_id_from_row_column(min_row+tail_position, btn_col);
			if (btn_id < 64) { // 64= NUM_BUTTONS
				uint8_t led_index = btn_id*3;
				uint8_t *ptr = buffer + led_index;  // Copy To LED Buffer for this LED
				// !review: LED Colors are inverted for the MF64 (BRG instead of RGB), this is one place we reverse their order
				*ptr++ = src[2];
				*ptr++ = src[0];
				*ptr++ = src[1];
			}
			// -- Draw Top Left Line
			btn_id = get_button_id_from_row_column(max_row-tail_position, min_col+tail_position);
			if (btn_id < 64) { // 64= NUM_BUTTONS
				uint8_t led_index = btn_id*3;
				uint8_t *ptr = buffer + led_index;  // Copy To LED Buffer for this LED
				// !review: LED Colors are inverted for the MF64 (BRG instead of RGB), this is one place we reverse their order
				*ptr++ = src[2];
				*ptr++ = src[0];
				*ptr++ = src[1];
			}			
			// -- Draw Top Right Line
			btn_id = get_button_id_from_row_column(max_row-tail_position, max_col-tail_position);
			if (btn_id < 64) { // 64= NUM_BUTTONS
				uint8_t led_index = btn_id*3;
				uint8_t *ptr = buffer + led_index;  // Copy To LED Buffer for this LED
				// !review: LED Colors are inverted for the MF64 (BRG instead of RGB), this is one place we reverse their order
				*ptr++ = src[2];
				*ptr++ = src[0];
				*ptr++ = src[1];
			}
			// -- Draw Bottom Left Line
			btn_id = get_button_id_from_row_column(min_row+tail_position, min_col+tail_position);
			if (btn_id < 64) { // 64= NUM_BUTTONS
				uint8_t led_index = btn_id*3;
				uint8_t *ptr = buffer + led_index;  // Copy To LED Buffer for this LED
				// !review: LED Colors are inverted for the MF64 (BRG instead of RGB), this is one place we reverse their order
				*ptr++ = src[2];
				*ptr++ = src[0];
				*ptr++ = src[1];
			}			
			
			// -- Draw Bottom Right Line
			btn_id = get_button_id_from_row_column(min_row+tail_position, max_col-tail_position);
			if (btn_id < 64) { // 64= NUM_BUTTONS
				uint8_t led_index = btn_id*3;
				uint8_t *ptr = buffer + led_index;  // Copy To LED Buffer for this LED
				// !review: LED Colors are inverted for the MF64 (BRG instead of RGB), this is one place we reverse their order
				*ptr++ = src[2];
				*ptr++ = src[0];
				*ptr++ = src[1];
			}
			
			// -- Draw Left Line (8pt star only)
			btn_id = get_button_id_from_row_column(btn_row, min_col+tail_position);
			if (btn_id < 64) { // 64= NUM_BUTTONS
				uint8_t led_index = btn_id*3;
				uint8_t *ptr = buffer + led_index;  // Copy To LED Buffer for this LED
				// !review: LED Colors are inverted for the MF64 (BRG instead of RGB), this is one place we reverse their order
				*ptr++ = src[2];
				*ptr++ = src[0];
				*ptr++ = src[1];
			}			
			
			// -- Draw Right Line (8pt star only)
			btn_id = get_button_id_from_row_column(btn_row, max_col-tail_position);
			if (btn_id < 64) { // 64= NUM_BUTTONS
				uint8_t led_index = btn_id*3;
				uint8_t *ptr = buffer + led_index;  // Copy To LED Buffer for this LED
				// !review: LED Colors are inverted for the MF64 (BRG instead of RGB), this is one place we reverse their order
				*ptr++ = src[2];
				*ptr++ = src[0];
				*ptr++ = src[1];
			}
						
			// Go to the Next Set of Leds				
			counter -= 1;
			tail_position += 1;
		}
	}
}
// Geometric Star Animation (end)

// Geometric Triangle Animation (start)
void start_geometric_triangle_animation(uint8_t button_id) {
	geometric_animation_btn_id[assign_geometric_animation_id] = button_id;
	geometric_animation_pos[assign_geometric_animation_id] = 0;
	geometric_animation_type[assign_geometric_animation_id] = GEOMETRIC_ANIMATION_TYPE_TRIANGLE;
	set_geometric_animation_color_source(button_id);
	g_led_counter[GEOMETRIC_ANIMATION_G_LED_IDX] = GEOMETRIC_ANIMATION_G_LED_LIMIT;
}

void run_geometric_triangle_animation(uint8_t *buffer, bool update_animation) {
	
	if (update_animation) {
		// increment the animation state
		geometric_animation_pos[geometric_animation_id] = geometric_animation_pos[geometric_animation_id] + 1;
		if (geometric_animation_pos[geometric_animation_id] >= GEOMETRIC_ANIMATION_STEPS_TRIANGLE) { // exit if animation complete
			return; // !review: i belieive this is redundant to what is being done in run_geometric_animation()
		}
	}
	
	// Set the led states
	if (!geometric_animation_pos[geometric_animation_id]) { // at first, only the button itself is lit
		return;
	}
	// run the animation. Let's light up some leds!
	else {
		// - get the 'on' color of the current led
		// - get column row position of the current button
		uint8_t btn_row = get_geometric_button_row();
		uint8_t btn_col = get_geometric_button_column();
		// - calculate the column/row limits for the current geometric_animation_position (outer edges of the square)
		uint8_t min_row = btn_row - (geometric_animation_pos[geometric_animation_id]-1); // Note: we are depending on overflow
		uint8_t max_row = btn_row + geometric_animation_pos[geometric_animation_id];
		uint8_t min_col = btn_col - (1 + (geometric_animation_pos[geometric_animation_id]-1)*2);
		uint8_t max_col = btn_col + (1 + (geometric_animation_pos[geometric_animation_id]-1)*2);
		// - calculate the limits of the mf64, so we can determine what to show, and what is out of bounds
		uint8_t start_col = min_col < GEOMETRIC_ANIMATION_COLS ? min_col:0; // note: depends on 'overflow' in calculation of min_col
		uint8_t end_col = max_col < GEOMETRIC_ANIMATION_COLS ? max_col:GEOMETRIC_ANIMATION_COLS-1;
		uint8_t start_row = min_row < GEOMETRIC_ANIMATION_ROWS ? min_row:0;
		uint8_t end_row = max_row < GEOMETRIC_ANIMATION_ROWS ? max_row:GEOMETRIC_ANIMATION_ROWS-1;
		uint8_t *src = geometric_animation_color_ptr[geometric_animation_id];
		
		// - Light all applicable leds (single led on upper row, full lower row, Lines drawn between these two row endpoints
		// -- Light the bottom row (all leds in range)
		if (start_row == min_row) {
			for (uint8_t this_col = start_col; this_col <= end_col; this_col++) {
				uint8_t btn_id = get_button_id_from_row_column(start_row, this_col);
				uint8_t led_index = btn_id*3;
				uint8_t *ptr = buffer + led_index;  // Copy To LED Buffer for this LED
				// !review: LED Colors are inverted for the MF64 (BRG instead of RGB), this is one place we reverse their order
				*ptr++ = src[2];
				*ptr++ = src[0];
				*ptr++ = src[1];				
			}
			//start_row += 1; // skip this row in subsequent routines
		}
		// -- Light the top row (single led)
		if (end_row == max_row) { // if this button exists
			uint8_t btn_id = get_button_id_from_row_column(end_row, btn_col);	
			uint8_t led_index = btn_id*3;
			uint8_t *ptr = buffer + led_index;  // Copy To LED Buffer for this LED
			// !review: LED Colors are inverted for the MF64 (BRG instead of RGB), this is one place we reverse their order
			*ptr++ = src[2];
			*ptr++ = src[0];
			*ptr++ = src[1];
		}
		// -- Light the leds to connect the top and bottom rows
		
		for (uint8_t this_row = start_row; this_row <= end_row; this_row++) {
			uint8_t distance_from_bottom = this_row - min_row; // note: this depends on 8-bit overflow of min_row to
			uint8_t left_col = min_col + distance_from_bottom;
			uint8_t right_col = max_col - distance_from_bottom;
			
			if (left_col < GEOMETRIC_ANIMATION_COLS) {
				uint8_t btn_id = get_button_id_from_row_column(this_row, left_col);
				uint8_t led_index = btn_id*3;
				uint8_t *ptr = buffer + led_index;  // Copy To LED Buffer for this LED
				// !review: LED Colors are inverted for the MF64 (BRG instead of RGB), this is one place we reverse their order
				*ptr++ = src[2];
				*ptr++ = src[0];
				*ptr++ = src[1];				
			}
			if (right_col < GEOMETRIC_ANIMATION_COLS) {
				uint8_t btn_id = get_button_id_from_row_column(this_row, right_col);
				uint8_t led_index = btn_id*3;
				uint8_t *ptr = buffer + led_index;  // Copy To LED Buffer for this LED
				// !review: LED Colors are inverted for the MF64 (BRG instead of RGB), this is one place we reverse their order
				*ptr++ = src[2];
				*ptr++ = src[0];
				*ptr++ = src[1];				
			}
		}
	}
}

// Geometric Triangle Animation (end)


void start_geometric_animation(uint8_t button_id, uint8_t animation_id) {
	//bool update_animation = false;
	if (g_led_counter[1] == 0) { // !review: move this section to geometric_animation_state?
		// reset the counter.
		g_led_counter[1] = GEOMETRIC_ANIMATION_G_LED_LIMIT;
	}
	switch (animation_id) {
		case GEOMETRIC_ANIMATION_TYPE_CIRCLE:
			start_geometric_circle_animation(button_id);
			break;
		case GEOMETRIC_ANIMATION_TYPE_SQUARE:
			start_geometric_square_animation(button_id);
			break;
		case GEOMETRIC_ANIMATION_TYPE_STAR:
			start_geometric_star_animation(button_id);
			break;
		case GEOMETRIC_ANIMATION_TYPE_TRIANGLE:
			start_geometric_triangle_animation(button_id);
			break;
		default:
			break;
	}
	assign_geometric_animation_id = (assign_geometric_animation_id + 1) % CONCURRENT_GEOMETRIC_ANIMATIONS; // increment for the next time this is run
}

void geometric_animation_state(const uint8_t bank, uint8_t *buffer) {
	bool update_animation = false;
	if (g_led_counter[1] == 0) { // !review: move this section to geometric_animation_state?
		// reset the counter.
		g_led_counter[1] = GEOMETRIC_ANIMATION_G_LED_LIMIT;
		update_animation = true;
	}
		
	for (uint8_t this_animation = 0; this_animation < CONCURRENT_GEOMETRIC_ANIMATIONS; this_animation++)
	{
		geometric_animation_id = this_animation; //+ assign_geometric_animation_id) % CONCURRENT_GEOMETRIC_ANIMATIONS;
		
		if (geometric_animation_type[this_animation] < 1) { // GEOMETRIC_ANIMATION_TYPE_SQUARE
			if (geometric_animation_pos[this_animation] < GEOMETRIC_ANIMATION_STEPS_SQUARE) { // exit if animation complete
				run_geometric_square_animation(buffer, update_animation);
			}
		}
		else if (geometric_animation_type[this_animation] < 2) { // GEOMETRIC_ANIMATION_TYPE_CIRCLE
			if (geometric_animation_pos[this_animation] < GEOMETRIC_ANIMATION_STEPS_CIRCLE) { // exit if animation complete
				run_geometric_circle_animation(buffer, update_animation);
			}
		}
		else if (geometric_animation_type[this_animation] < 3) { // GEOMETRIC_ANIMATION_TYPE_STAR
			if (geometric_animation_pos[this_animation] < GEOMETRIC_ANIMATION_STEPS_STAR) { // exit if animation complete
				run_geometric_star_animation(buffer, update_animation); 
			}
		}
		else if (geometric_animation_type[this_animation] < 4) { // GEOMETRIC_ANIMATION_TYPE_TRIANGLE
			if (geometric_animation_pos[this_animation] < GEOMETRIC_ANIMATION_STEPS_TRIANGLE) { // exit if animation complete
				run_geometric_triangle_animation(buffer, update_animation);
			}
		}
	}
}

#define RGB_TEST_COUNTER_LIMIT 512
#if ENABLE_RGB_TEST > 0
void rgb_test_animation_state(uint8_t *buffer) {
	static uint8_t rgb_test_position = 0;
	static uint16_t rgb_test_counter = 0;
	
	// Scale Output, so colors display for a while
	if (rgb_test_counter++ >= RGB_TEST_COUNTER_LIMIT) {
		// Update Test Position
		rgb_test_position = rgb_test_position + 1 & 0x03; // limit to four states: off, red, green, blue
		rgb_test_counter = 0;
	}
	
	uint8_t *ptr = buffer; // + led_index;  // Copy To LED Buffer for this LED
	uint8_t red, green, blue;
	// !review: LED Colors are inverted for the MF64 (BRG instead of RGB), this is one place we reverse their order
	if (rgb_test_position <= 0) { // off
		red = default_color[COLORID_OFF][0];
		green = default_color[COLORID_OFF][1];
		blue = default_color[COLORID_OFF][2];
	} else if (rgb_test_position == 1) { // red
		red = default_color[COLORID_RED][0];
		green = default_color[COLORID_RED][1];
		blue = default_color[COLORID_RED][2];
	} else if (rgb_test_position == 2) { // green
		red = default_color[COLORID_GREEN][0];
		green = default_color[COLORID_GREEN][1];
		blue = default_color[COLORID_GREEN][2];
		
	} else if (rgb_test_position >= 3) { // blue
		red = default_color[COLORID_BLUE][0];
		green = default_color[COLORID_BLUE][1];
		blue = default_color[COLORID_BLUE][2];
	}
	
	for (uint8_t this_led = 0; this_led < 64; this_led++) // this_led
	{	
		// !review: LED Colors are inverted for the MF64 LEDS (BRG instead of RGB), this is one place we reverse their order
		*ptr++ = blue;
		*ptr++ = red;
		*ptr++ = green;
	}
}
#endif





void midi_animation_state(const uint8_t bank,uint8_t *buffer)
{
	// Override the display state of a given arcade button with an animation set by
	// the velocity of a Note On event of the same pitch.
	// Animation settings are sent on the next channel.
	uint8_t bank_offset = bank ? NUM_BUTTONS: 0; // !bank64 logic limited to two banks
	
	for (uint8_t i=bank_offset; i<bank_offset + 64; ++i) {
		#if USB_RX_METHOD >= USB_RX_PERIODICALLY
		Midifighter_GetIncomingUsbMidiMessages(); // !error: remove USB_RX_PERIODICALLY
		#endif
		
		uint8_t velocity = g_midi_note_state[1][i]; // Arcade button animation info is stored in the second array
		uint8_t key = i & BUTTON_ID_FLAGS; // truncate to 64 buttons, only points to g_display_buffer which only stores active bank
 
		if (velocity > 0) {
			// Single Button VU
			if (velocity < 18 ){
				// !review: make vu_meter work?
				//uint8_t *ptr = buffer + key * 3;
				//uint8_t level = velocity - 1;
				//uint8_t src[3] = {0x00,0x00,0xFF};
				//uint8_t *col = src;
				//vu_meter(level, true, col, ptr);
			}
			// Brightness
			else if (velocity < 34) {
				uint8_t *ptr = buffer + key * 3;
				uint8_t level = velocity-18;
				uint8_t result[3];
				
				result[0] = (ptr[0]*level)>>4;
				result[1] = (ptr[1]*level)>>4;
				result[2] = (ptr[2]*level)>>4;
				
				ptr[0] = result[0];
				ptr[1] = result[1];
				ptr[2] = result[2];
			}
			// Flash Animation - Gates the current color with a flash rate
			else if (velocity < 42) {
				if(!flash_animation(velocity-33)) {
					uint8_t *ptr = buffer + key * 3;
					ptr[0] = 0x00;
					ptr[1] = 0x00;
					ptr[2] = 0x00;
				}				
			}
			// Pulse Animation - Pulse the current color at a certain rate
			else if (velocity < 50) {
				
				uint8_t *ptr = buffer + key * 3;
				uint8_t cycle_level = pulse_animation(velocity-41);
				
				uint8_t result[3];
				
				result[0] = (ptr[0]*cycle_level)>>8;
				result[1] = (ptr[1]*cycle_level)>>8;
				result[2] = (ptr[2]*cycle_level)>>8;
				
				ptr[0] = result[0];
				ptr[1] = result[1];
				ptr[2] = result[2];
				
			}
			else if (velocity < 54) { // Geometric Animations
				// Trigger the start of the animation. It will be run in a separate process for the time being.
				// !review: !bank64: setting this when in a different bank will cause it to be triggered only after the bank is changed...
				start_geometric_animation(i & BUTTON_ID_FLAGS, velocity - 50);
				// These are one shot animations. Reset the value to 0, so the animation doesn't run again.
				g_midi_note_state[1][i] = 0; // Arcade button animation info is stored in the second array
			}
		}
		
		
	}
}

// Returns the flash state of the 8 available flash rates.
bool flash_animation(uint8_t flash_rate)
{
	if (display_flash_counter & (SIXTEENTH_FLASH_STATE<<(8-flash_rate))) {
		return true;
	}
	else {
		return false;
	}
}
// Returns a sin function brightness level for up to 8 different rates
uint8_t pulse_animation(uint8_t pulse_rate)
{
	uint8_t rgb_step;
	if(!midi_clock_enabled){
		rgb_step  = (uint8_t)(((display_flash_counter<<4)>>(8-pulse_rate)) & 0xFF);
	}
	else{
		rgb_step  = (uint8_t)(((display_flash_counter<<5)>>(8-pulse_rate)) & 0xFF);
	}
	uint8_t level = sin((rgb_freq*rgb_step)) * 127 + 127;
	return level;
}

// Global Functions -----------------------------------------------------------

void default_display_run(void) //const uint8_t bank, g_bank_selected
						 //const uint16_t key_state, , g_key_state
						//	   uint8_t *buffer), g_display_buffer
{
	// - Set LEDs based on Button Press State
	//info_display_inactive();//g_bank_selected, g_display_buffer);
	info_display_active(); //g_bank_selected, g_key_state, g_display_buffer);
		
	// ... and allow the user to write colors using MIDI input.
	midi_color_state(g_bank_selected, g_display_buffer);
	midi_animation_state(g_bank_selected, g_display_buffer);
	
	// Sleep Animation
	// Increment timing counters
	if (half_ms_counter >= 2000)
	{
		half_ms_counter = 0;
		one_second_counter += 1;
		if (one_second_counter >= 60)
		{
			sleep_minute_counter += 1;
			one_second_counter = 0;
		}
	}
		
	// After a certain period of inactivity display rainbow pattern until
	// next key press.
	// If the Sleep Time is set to 0 never activate the sleep mode
	if(G_EE_SLEEP_TIME)
	{
		if (sleep_minute_counter == G_EE_SLEEP_TIME)
		{
			ball_demo_setup();
			sleep_minute_counter +=1;
		}
		if (sleep_minute_counter > G_EE_SLEEP_TIME)
		{
			ball_demo_run(g_display_buffer);
		}
		if (g_key_down)
		{
			one_second_counter = 0;
			sleep_minute_counter=0;
		}
	}
	geometric_animation_state(g_bank_selected, g_display_buffer);
	
	#if ENABLE_RGB_TEST > 0
	rgb_test_animation_state(g_display_buffer);
	#endif	
}

#define LAVENDER_GREEN_LIMIT 0x24 // Can't be lavender if it has a lot of green (MF3D Patch)
#define MF3D_UTILITY_BRIGHT_COLOR_LIMIT 0x80
#define MF3D_UTILITY_DIM_COLOR_LIMIT 0x27
// Sysex Configuration Extensions -----------------------------------------------
// - !review (adust functions below): MF3D Compatibility Patch (allow 20 colors that are scaled for power)
// -- this could be updated to a simpler equation, but 
// --- you need to be very careful about power draw. 128 leds can pull up to 2-Amps
// ---- but usb devices are limited to 0.5-Amps
void adjust_inactive_bank_leds_for_power(uint8_t bank, uint8_t offset, uint8_t size)
{
	for (uint8_t this_byte = offset; this_byte < offset + size; this_byte+=3) {
		uint8_t this_rgb[3];
		uint8_t this_color_id;
		this_rgb[0] = default_bank_inactive[bank][this_byte];
		this_rgb[1] = default_bank_inactive[bank][this_byte+1];
		this_rgb[2] = default_bank_inactive[bank][this_byte+2];
		if (!this_rgb[0]) // No Red
		{
			if (!this_rgb[1]) // Blue or Off (No Red No Green)
			{
				if (!this_rgb[2]) { // Case: Off -> No Change actually necessary
					this_color_id = COLORID_OFF;
				}
				else { // BLUE
					if (this_rgb[2] == default_color[COLORID_BLUE][2]) {continue;} // Patch: do not change if already assigned as MF64 Color
					this_color_id = this_rgb[2] > MF3D_UTILITY_BRIGHT_COLOR_LIMIT ? COLORID_BLUE : COLORID_BLUE_DIM;
				}
			}
			else if (!this_rgb[2]) // GREEN only (no red no blue)
			{
				if (this_rgb[1] == default_color[COLORID_GREEN][1]) {continue;} // Patch: do not change if already assigned as MF64 Color
				this_color_id = this_rgb[1] > MF3D_UTILITY_BRIGHT_COLOR_LIMIT ? COLORID_GREEN : COLORID_GREEN_DIM;
			}
			else  // No Red, Yes Green, Yes Blue: CYAN
			{
				if (this_rgb[1] == default_color[COLORID_CYAN][1]) {continue;} // Patch: do not change if already assigned as MF64 Color
				this_color_id = this_rgb[1] > MF3D_UTILITY_BRIGHT_COLOR_LIMIT ? COLORID_CYAN : COLORID_CYAN_DIM;
			}
		}
		else if (!this_rgb[1]) // Yes Red, No Green
		{
			if (!this_rgb[2]) // RED ONLY
			{
				if (this_rgb[0] == default_color[COLORID_RED][0]) {continue;} // Patch: do not change if already assigned as MF64 Color
				this_color_id = this_rgb[0] > MF3D_UTILITY_BRIGHT_COLOR_LIMIT ? COLORID_RED : COLORID_RED_DIM;
			}
			else // Red and Blue (PINK) Note Lavender has some green in it
			{
				if (this_rgb[0] == default_color[COLORID_PINK][0]) {continue;} // Patch: do not change if already assigned as MF64 Color
				this_color_id = this_rgb[0] > MF3D_UTILITY_BRIGHT_COLOR_LIMIT ? COLORID_PINK : COLORID_PINK_DIM;
			}
			
		}
		else if (!this_rgb[2]) // Yes Red, Yes Green, No Blue (Yellow or 
		{
			// Yellow, Orange, or Chartreuse (rx midi (value is *2) chart:5f,7f,00, yellow: 7f,5f,00, orange: 7f,22,00)
			if (this_rgb[0] < this_rgb[1]) // CHARTRUESE: more green than red 
			{
				if (this_rgb[1] == default_color[COLORID_CHARTREUSE][1]) {continue;} // Patch: do not change if already assigned as MF64 Color
				this_color_id = this_rgb[1] > MF3D_UTILITY_BRIGHT_COLOR_LIMIT ? COLORID_CHARTREUSE : COLORID_CHARTREUSE_DIM;
			}
			else if (this_rgb[0] >> 1 >= this_rgb[1]) // ORANGE: more than twice as much red as green
			{
				if (this_rgb[0] == default_color[COLORID_ORANGE][0]) {continue;} // Patch: do not change if already assigned as MF64 Color
				this_color_id = this_rgb[0] > MF3D_UTILITY_BRIGHT_COLOR_LIMIT ? COLORID_ORANGE : COLORID_ORANGE_DIM;				
			}
			else // YELLOW 
			{
				if (this_rgb[0] == default_color[COLORID_YELLOW][0]) {continue;} // Patch: do not change if already assigned as MF64 Color
				this_color_id = this_rgb[0] > MF3D_UTILITY_BRIGHT_COLOR_LIMIT ? COLORID_YELLOW : COLORID_YELLOW_DIM;
			}
		}
		else // White or lavender
		{
			if (this_rgb[1] > LAVENDER_GREEN_LIMIT) {  // White
				this_color_id = COLORID_WHITE;//this_rgb[0] > MF3D_UTILITY_BRIGHT_COLOR_LIMIT ? COLORID_WHITE
			}
			else { // Lavender
				// - If blue is bright, assume lavendar is intended to be bright
				if (this_rgb[2] == default_color[COLORID_LAVENDER][2]) {continue;} // Patch: do not change if already assigned as MF64 Color
				this_color_id = this_rgb[0] > MF3D_UTILITY_BRIGHT_COLOR_LIMIT ? COLORID_LAVENDER : COLORID_LAVENDER_DIM;			
			}
			
		}
		default_bank_inactive[bank][this_byte] = default_color[this_color_id][0];
		default_bank_inactive[bank][this_byte+1] = default_color[this_color_id][1];
		default_bank_inactive[bank][this_byte+2] = default_color[this_color_id][2];
	}
}
void adjust_active_bank_leds_for_power(uint8_t bank, uint8_t offset, uint8_t size)
{
	//default_bank_inactive[bank][offset + idx] = ((*buffer++) * 2);
	for (uint8_t this_byte = offset; this_byte < offset + size; this_byte+=3) { 
		uint8_t this_rgb[3];
		uint8_t this_color_id;
		this_rgb[0] = default_bank_active[bank][this_byte];
		this_rgb[1] = default_bank_active[bank][this_byte+1];
		this_rgb[2] = default_bank_active[bank][this_byte+2];
		if (!this_rgb[0]) // No Red
		{
			if (!this_rgb[1]) // Blue or Off (No Red No Green)
			{
				if (!this_rgb[2]) { // Case: Off -> No Change actually necessary
					this_color_id = COLORID_OFF;
				}
				else { // BLUE
					if (this_rgb[2] == default_color[COLORID_BLUE][2]) {continue;} // Patch: do not change if already assigned as MF64 Color
					this_color_id = this_rgb[2] > MF3D_UTILITY_BRIGHT_COLOR_LIMIT ? COLORID_BLUE : COLORID_BLUE_DIM;
				}
			}
			else if (!this_rgb[2]) // GREEN only (no red no blue)
			{
				if (this_rgb[1] == default_color[COLORID_GREEN][1]) {continue;} // Patch: do not change if already assigned as MF64 Color
				this_color_id = this_rgb[1] > MF3D_UTILITY_BRIGHT_COLOR_LIMIT ? COLORID_GREEN : COLORID_GREEN_DIM;
			}
			else  // No Red, Yes Green, Yes Blue: CYAN
			{
				if (this_rgb[1] == default_color[COLORID_CYAN][1]) {continue;} // Patch: do not change if already assigned as MF64 Color
				this_color_id = this_rgb[1] > MF3D_UTILITY_BRIGHT_COLOR_LIMIT ? COLORID_CYAN : COLORID_CYAN_DIM;
			}
		}
		else if (!this_rgb[1]) // Yes Red, No Green
		{
			if (!this_rgb[2]) // RED ONLY
			{
				if (this_rgb[0] == default_color[COLORID_RED][0]) {continue;} // Patch: do not change if already assigned as MF64 Color
				this_color_id = this_rgb[0] > MF3D_UTILITY_BRIGHT_COLOR_LIMIT ? COLORID_RED : COLORID_RED_DIM;
			}
			else // Red and Blue (PINK) Note Lavender has some green in it
			{
				if (this_rgb[0] == default_color[COLORID_PINK][0]) {continue;} // Patch: do not change if already assigned as MF64 Color
				this_color_id = this_rgb[0] > MF3D_UTILITY_BRIGHT_COLOR_LIMIT ? COLORID_PINK : COLORID_PINK_DIM;
			}
			
		}
		else if (!this_rgb[2]) // Yes Red, Yes Green, No Blue (Yellow or 
		{
			// Yellow, Orange, or Chartreuse (rx midi (value is *2) chart:5f,7f,00, yellow: 7f,5f,00, orange: 7f,22,00)
			if (this_rgb[0] < this_rgb[1]) // CHARTRUESE: more green than red 
			{
				if (this_rgb[1] == default_color[COLORID_CHARTREUSE][1]) {continue;} // Patch: do not change if already assigned as MF64 Color
				this_color_id = this_rgb[1] > MF3D_UTILITY_BRIGHT_COLOR_LIMIT ? COLORID_CHARTREUSE : COLORID_CHARTREUSE_DIM;
			}
			else if (this_rgb[0] >> 1 >= this_rgb[1]) // ORANGE: more than twice as much red as green
			{
				if (this_rgb[0] == default_color[COLORID_ORANGE][0]) {continue;} // Patch: do not change if already assigned as MF64 Color
				this_color_id = this_rgb[0] > MF3D_UTILITY_BRIGHT_COLOR_LIMIT ? COLORID_ORANGE : COLORID_ORANGE_DIM;				
			}
			else // YELLOW 
			{
				if (this_rgb[0] == default_color[COLORID_YELLOW][0]) {continue;} // Patch: do not change if already assigned as MF64 Color
				this_color_id = this_rgb[0] > MF3D_UTILITY_BRIGHT_COLOR_LIMIT ? COLORID_YELLOW : COLORID_YELLOW_DIM;
			}
		}
		else // White or lavender
		{
			if (this_rgb[1] > LAVENDER_GREEN_LIMIT) {  // White
				this_color_id = COLORID_WHITE;//this_rgb[0] > MF3D_UTILITY_BRIGHT_COLOR_LIMIT ? COLORID_WHITE
			}
			else { // Lavender
				// - If blue is bright, assume lavendar is intended to be bright
				if (this_rgb[2] == default_color[COLORID_LAVENDER][2]) {continue;} // Patch: do not change if already assigned as MF64 Color
				this_color_id = this_rgb[0] > MF3D_UTILITY_BRIGHT_COLOR_LIMIT ? COLORID_LAVENDER : COLORID_LAVENDER_DIM;			
			}
			
		}
		default_bank_active[bank][this_byte] = default_color[this_color_id][0];
		default_bank_active[bank][this_byte+1] = default_color[this_color_id][1];
		default_bank_active[bank][this_byte+2] = default_color[this_color_id][2];
	}
}
