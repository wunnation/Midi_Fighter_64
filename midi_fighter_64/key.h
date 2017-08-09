// LED functions for DJTT Midifighter3
//
//   Copyright (C) 2011 DJ Techtools
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

#ifndef _KEY_H_INCLUDED
#define _KEY_H_INCLUDED

#include <stdbool.h>
#include <stdint.h>
#include <avr/interrupt.h>
#include "constants.h"

// Extern Globals --------------------------------------------------------------

// The key debounce buffer.
extern uint64_t g_key_debounce_buffer[DEBOUNCE_BUFFER_SIZE];

// The key states (after debounce).
extern uint64_t g_key_state;      // Current state of the keys.
extern uint64_t g_key_prev_state; // State of the keys when last polled.
extern uint64_t g_key_up;         // Key was released since last poll.
extern uint64_t g_key_down;       // Key was pressed since last poll.
extern uint64_t g_key_bank_last_up;         // Bank key was pressed in (for note off messages) 64: only supports two banks (1 bit per button)

#define LED_REFRESH_LIMIT 25 // 25 was working value
extern volatile uint16_t system_time_ms; // 0 to 65 seconds
extern uint16_t last_led_refresh_time_ms;

// Interrupt service routine ---------------------------------------------------
ISR(TIMER0_OVF_vect);

// Key functions ---------------------------------------------------------------
void key_setup(void);
void key_disable(void);
uint32_t key_read(void);
void key_calc(void);

#endif // _KEY_H_INCLUDED
