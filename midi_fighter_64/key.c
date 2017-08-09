// Key reading functions for DJTT Midifighter2
//
//   Copyright (C) 2011 DJ Techtools
//
// rgreen 2009-07-07

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

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "key.h"
#include "constants.h"

#include "led.h"
#include "midi.h" // for g_midi_note_off_counter

// Globals ---------------------------------------------------------------------

uint64_t g_key_debounce_buffer[DEBOUNCE_BUFFER_SIZE]; // The debounce buffer
uint64_t g_key_state = 0;      // Current state of the keys after debounce.
uint64_t g_key_prev_state = 0; // State of the keys when last polled.
uint64_t g_key_up = 0;         // Key was released since last poll.
uint64_t g_key_down= 0;       // Key was pressed since last poll.
uint64_t g_key_bank_last_up = 0;         // Bank key was pressed in (for note off messages) 64: only supports two banks (1 bit per button)

volatile uint16_t system_time_ms = 0; // 0 to 65 seconds
uint16_t last_led_refresh_time_ms = 0;
#define TIMER_TIMEOUT_1MS	0xD0

// Key Functions --------------------------------------------------

// Setup the keys IO ports for reading and set up the timer interrupt
// for servicing the debounce algorithm.
//
void key_setup(void)
{
    //   (PB5) = input for switches 1-8
    //    PB6  = clock, shared by both chips.
    //    PB5  = data latch, shared by both chips.

    // latch and clock outputs
    DDRD |= KEY_CLOCK;
    DDRD |= KEY_LATCH;
    // key input
    DDRC &= ~KEY_BIT;
    PORTC |= KEY_BIT; // enable pull-up resistor on key input.

    // Parallal Load (PL) on the chips is triggered low, so start it off
    // high and make sure we return it there after use. The clock pulse (CP)
    // should be high before latching (according to the datasheet), so we
    // also start it off high.
    PORTD |= KEY_CLOCK;
    PORTD |= KEY_LATCH;

    // Start the debounce buffer in an empty state.
    memset(g_key_debounce_buffer, 0, DEBOUNCE_BUFFER_SIZE*sizeof(uint32_t));

    // Setup TIMER0 to trigger an overflow interrupt 1000 times a second.
    // Our counter is incremented every 256 / 16000000 = 0.000016 seconds.
    // Our key debounce buffer is 10 samples, meaning we need the counter
    // to count to x where (256*10*x)/16000000 = 0.001 seconds. This gives us
    // A counter value of 62.5 (rounded to 62), but the counter increments
    // from a base number to the overflow point at 255 so we need to set
    // the counter to (255 - 62) = 193 = 0xC1
    // Strictly, we're polling the buttons at 1008Hz, but who's counting...
	// - in reality, the time spent in this interrupt also comes into play
	// -- for mf64 this number was custom adjusted to 0xD0 by monitoring the 
	// --- firmware using the ENABLE_TEST_OUT options in contants.h
	

    // Set the Timer0 prescaler to clock/256 (CS0x=100)
    TCCR0B |= _BV(CS02);
    TCCR0B &= ~_BV(CS01);
    TCCR0B &= ~_BV(CS00);
    // Setup Timer0 to count up from 192 (see calculations above).
    TCNT0 = TIMER_TIMEOUT_1MS;
	// !review: performance: if TIMER_TIMEOUT was increased to 2ms,
	// - processor would have a lot more execution time between interrupts
	// -- note that if you do this DEBOUNCE_BUFFER_SIZE must be cut in half
	// --- or button delay will be added.
	
    // Set the Timer0 Overflow Interrupt Enable bit.
    TIMSK0 |= _BV(TOIE0);
    // Enable all interrupts.
    sei();

    // setup the global key state variables to empty values.
    g_key_state = 0;
    g_key_prev_state = 0;
    g_key_up = 0;
    g_key_down = 0;
}

// Disable the timer interrupt. This is needed during teardown before
// entering the bootloader.
//
void key_disable(void)
{
    // Zero out the Timer0 Overflow Interrupt Enable bit so interrupts will
    // no longer be generated.
    TIMSK0 &= ~(_BV(TOIE0));
}

// The key read Interrupt Service Routine (ISR).
//
ISR(TIMER0_OVF_vect)
{
    // Where to write the next value in the ring buffer.
    static uint8_t buffer_pos = 0;
	
    // The counter just overflowed, so reset the counter to the magic number
    // 193 (see above).
    TCNT0 = TIMER_TIMEOUT_1MS;
	
	// Read in all Button States
    // Latch the key, reads on a falling edge.
	PORTD |= KEY_LATCH;
	PORTD &= ~KEY_LATCH;
    // Latching the inputs also presents the first bit to the output
    // pin. Shift the captured bits back to the CPU.
	uint64_t value = 0;
	uint64_t bit = 0x1;
	for (uint8_t i=0; i<64; i++) {
        PORTD &= ~KEY_CLOCK;  // clock falling edge does nothing.
        value |= (PINC & KEY_BIT) ? 0 : bit;
        bit <<= 1;
        PORTD |= KEY_CLOCK; // clock works on the rising edge, leave it high after use.		
	}
    g_key_debounce_buffer[buffer_pos] = ~value; // Note: MF64 has inverted buttons (compared to 3D). Only logically matters right here! '~'
    buffer_pos = (buffer_pos + 1) % DEBOUNCE_BUFFER_SIZE;
	
	system_time_ms += 1;
  	return;
}

// Read the current keystate by reconstructing the key samples from the
// debounce buffer by ANDing together all the samples. Each bit represents a
// single sample of one key, so the columns line up to represent that state
// of a single key over time. The result of this read is stored in the
// global variable "g_key_state".
//
// NOTE: If any bit is set in the column, the key is considered "activated",
// so this debouncer is good at recognizing bounces on press/release but
// doesn't filter or surpress transitory EMF bits. If you've got EMF bits
// being triggered on a Midifighter, you have bigger problems than
// debouncing. For more, read Jack Ganssle's short guide to debounce
// algorithms:
//
//     http://www.ganssle.com/debouncing.pdf
//
// g_key_state corresponds to the shift register inputs as below
//    U6       U5       U4
// HGFEDCBA HGEFDCBA HGEFDCBA

uint32_t key_read(void)
{
    // Debounce the keys by ANDing the columns of key samples together.
    g_key_state = 0xffffffffffffffff;
    for(uint8_t i=0; i<DEBOUNCE_BUFFER_SIZE; ++i) {
        g_key_state &= g_key_debounce_buffer[i];
    }
    return g_key_state;
}

// Update the key up and key down global variables. We need to separate this
// from reading the key state as we may read the state many times to update
// the LEDs while waiting for a USB endpoint to become available.
//
// We only regenerate these key diffs just before we are about to send MIDI
// events, as these transitions are the events we are looking for.
//
void key_calc(void)
{
    // If a bit has changed, and it is 1 in the current state, it's a KeyDown.
	g_key_down = (g_key_prev_state ^ g_key_state) & g_key_state;
    // If a bit has changed and it was 1 in the previous state, it's a KeyUp.
    g_key_up = (g_key_prev_state ^ g_key_state) & g_key_prev_state;
    // Demote the current state to history.
    g_key_prev_state = g_key_state;
}
