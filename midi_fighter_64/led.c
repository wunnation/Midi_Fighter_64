// LED Driver Control functions
//
//   Copyright (C) 2011 DJ Techtools
//
// Robin Green       2011-05-22
// Michael Mitchell  2012-01-19

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

#include "led.h"
//#include "constants.h" // imported in led.h
#include "random.h"
#include "circular_buffer.h"
#include "eeprom.h"
#include "display.h"
#include "key.h"
#include "midi.h"

// Global variables ------------------------------------------------------------

//uint16_t g_led_state = 0x0000;  // LED state, 1 for on, 0 for off.

// Animation counters, decremented by the VBLANK interrupt service routine,
// allows events to be timed independently of the main loop speed.
uint16_t g_led_counter[4];
uint16_t display_flash_counter;  // animation counter that is currently truncated down to the upper 3 or 4 bits
uint16_t half_ms_counter;
uint8_t one_second_counter;
uint8_t sleep_minute_counter; // start with sleep animation enabled
//uint16_t set_spark=0; // mf64: no spark
uint16_t tick=0;



// Helper functions ------------------------------------------------------------

// Send an 8-bit value over the SPI bus.
//
// The SPI master will take the contents of the SPDR byte and clock it out
// through the MOSI (Master Out Slave In) pin to the selected device. It
// will then read the state of the MISO pin (Master In Slave Out) and shift
// that into the bottom bit of the SPDR register. After 8 bits of clocking,
// we will have sent one byte and received one byte.
//
/*static uint8_t spi_transmit(uint8_t byte)
{
    // Set SPI Data Register to the value to be transmitted.
    SPDR = byte;
    // While the SPI Status Register doesn't have the SPI Interrupt
    // Flag set, wait.
    while (!(SPSR & _BV(SPIF)));
    // Transmit completed, your input data is now in SPDR
    return SPDR;
}*/

// The LED Interrupt Service Routine (ISR).
//
// This is triggered off the Timer1 Overflow interrupt at about 200Hz.  It
// resets and triggers the LED_PWM greyscale clock on PC7 and toggles the
// LED_BLANK on PC6
//
ISR(TIMER1_OVF_vect)
{
    // The counter just overflowed, so reset the counter to the magic number.
    // (see calculation in "led_setup()")
	
    // NOTE: No need to guard the 16-bit write here as interrupts are turned
    // off inside an ISR.
    TCNT1 = 0;

    TCNT1 = 0xFFE0;

	/* //MF3D Ports
    // We have finished a grey scale cycle and need to toggle the LED_BLANK pin
    // to restart the counters.
    PORTC |= LED_BLANK;  // LED PWM counters reset on the rising edge.
	PORTC &= ~LED_BLANK; // LED PWM counters start on the falling edge.
	*/
	
    // Decrement each of the animation counters.
    for (uint8_t i=0; i<4; ++i) {
        if (g_led_counter[i] > 0) { --g_led_counter[i]; }
    }
	
	// If there is no MIDI clock use this timer to increment the display flash counter
	// every 150 cycles, this corresponds to a tempo of 128 BPM
	if(!midi_clock_enabled)
	{
		if(tick == 75)
		{
			tick = 1;
			display_flash_counter += 1;
		}
		else
		{
			tick += 1;
		}
	}

	half_ms_counter +=1;
	

    // PWM modulate the bank LEDs
    if (g_led_counter[3] == 0) {
        g_led_counter[3] = 16;
    }
}

// Exported functions ----------------------------------------------------------

// setup the LEDS for writing.
//
void led_setup(void)
{
    // Setup the output ports used by the LED system.
    // NOTE: LED_MISO remains an input port.
    //
	#if LED_CONFIGURATION == LED_CONFIGURATION_SINGLE_STRAND
      #warning Prototype LED Configuration! Not for Production!
	  // Group 1: ?s
      DDRB = LED_CLOCK + LED_MOSI + LED_LATCH + LED_MODE;
      DDRC = LED_BLANK + LED_PWM;
	#elif LED_CONFIGURATION == LED_CONFIGURATION_FOUR_STRANDS
	  // Groups 1-4 -> PB4-6, PC6
      DDRB = LED_CLOCK + LED_MOSI + LED_LATCH + LED_MODE;
      DDRC = LED_BLANK + LED_PWM;
	#endif

    // Set up Timer1 to trigger the LED_BLANK ISR.
    // -------------------------------------------
    // Turn off the Power Reduction Timer to free up Timer1.
    PRR0 &= ~_BV(PRTIM1);
    // Set Timer1 to Normal Mode (no compares).
    TCCR1A = 0;
    // Set the Timer1 prescaler to clock/256 (CS1x=100)
    TCCR1B |= _BV(CS12);
    TCCR1B &= ~_BV(CS11);
    TCCR1B &= ~_BV(CS10);
    // Turn off interrupts for a moment.
    cli();
    // Set up the 16-bit value that Timer1 will count up from.  NOTE: We are
    // incrementing the timer at 16MHz/256, so we have 16000000/256 = 62500
    // events a second. e need an event at 70Hz which gives us 893 cycles
    // between interrupts. So we set the timer to count up from
    // 0xFFFF-(893/2) = 0xFE40.
    TCNT1 = 0xFE40;
	
    // Enable the Timer1 Overflow Interrupt that will trigger the ISR.
    TIMSK1 |= _BV(TOIE1);
    // Turn on interrupts, this will start the ISR.
    //sei();

    // Zero out all of the animation counters.
    for (uint8_t i=0; i<4; ++i) {
        g_led_counter[i] = 0;
    }
}

void led_disable(void)
{
    // Zero out the Timer1 Interrupt Enable bit so interrupts will
    // no longer be generated.
    TIMSK1 &= ~(_BV(TOIE1));
}

void led_enable(void)
{
	// Set the Timer1 Interrupt Enable bit so interrupts are generated,
	// driving the display refresh
	TIMSK1 |= _BV(TOIE1);
}

// Set each LED to a white color from a 16-bit value.
//
// NOTE: added for compatibility with old Midifighter code.
// Have added extra parameter color, this can be used to set the RGB value

void led_set_state(uint16_t new_state, uint32_t color)
{
	return; // !review: temporarily disabled
}

void led_set_state_dfu(void)
{
	#if LED_CONFIGURATION == LED_CONFIGURATION_SINGLE_STRAND
	// ===== PROTOTYPES ====
	const uint32_t indicator_states[16] = {48,48, 0x00,0x00, 48,48, 0x00,0x00,   0x00,0x00, 48,48, 0x00,0x00, 48,48};
	uint32_t test_buffer; // = 0x000000FF; // Blue (Full Brightness)
	DDRC |= LED_ASYNC; // !review: overkill?
	// Turn off interrupts for a moment.
	cli();
	for (uint8_t this_led=0; this_led <= 127; this_led++) {	
		test_buffer = indicator_states[this_led & 0x0F]; // Checkerboard pattern (dim blue and off)
		uint32_t this_mask = 0x800000;
		for (uint8_t this_bit=0; this_bit < 24; this_bit++) {
			// Transmit this one bit
			if (test_buffer & this_mask) { // High Bit
				// Set Port Value to 1
				PORTC |= LED_ASYNC;  // data is latched on rising edge.
				// Wait 0.8us
				_delay_us(0.8); // !review: these delays can be reduced (note the 0.40 us delay was eliminated all together, perhaps do a nop instead of this delay.
				// Set Port Value to 0
				PORTC &= ~LED_ASYNC;  // data is latched on rising edge.
				// Wait 0.45us
				_delay_us(0.40); // We'll assume it takes .05 to go through the for loop (timing doesn't need be perfect)
			}
			else { // Low Bit
				// Set Port Value to 1
				PORTC |= LED_ASYNC;  // data is latched on rising edge.
				// Wait 0.4us
				//_delay_us(0.10);
				// Set Port Value to 0
				PORTC &= ~LED_ASYNC;  // data is latched on rising edge.
				// Wait	0.85us
				_delay_us(0.8);  // We'll assume it takes .05 to go through the for loop (timing doesn't need be perfect)
			}
			this_mask >>= 1;
		}
	}
	// Re-Enable Interrupts
	sei();	
	#elif LED_CONFIGURATION == LED_CONFIGURATION_FOUR_STRANDS
	// ===== Production Units ====
	const uint8_t indicator_states[48] = {
	48,0,0,    0,0,0,   48,0,0,   0,0,0,  0,0,0,  48,0,0, 0,0,0, 48,0,0,
	48,0,0,    0,0,0,   48,0,0,   0,0,0,  0,0,0,  48,0,0, 0,0,0, 48,0,0
	}; 
	DDRC |= LED_ASYNC_GROUP1; // !review: overkill?
	DDRB |= LED_ASYNC_GROUP0 | LED_ASYNC_GROUP2 | LED_ASYNC_GROUP3; // !review: overkill?
	cli();
	led_update_pixel_group0(indicator_states);
	led_update_pixel_group1(indicator_states);
	led_update_pixel_group2(indicator_states);
	led_update_pixel_group3(indicator_states);
	sei();	
	#endif
}

static uint32_t test_red = 0x0F;
static uint32_t test_green = 0x0F;
static uint32_t test_blue = 0x0F;
void led_set_test_colors(uint8_t red, uint8_t green, uint8_t blue) 
{
	// for use by 7-bit midi messages, ignore values 128+
	if (red < 0x80) {
		test_red = red;
	}
	if (green < 0x80) {
		test_green = green;
	}
	if (blue < 0x80) {
		test_blue = blue;
	}
}

#if ENABLE_TEST_IN_LED_CALIBRATION > 0
#warning LED Calibration is Enabled! Normal LED Functions will be overridden!
void led_calibration_test(void) {
	uint8_t *dest = g_display_buffer;
	for (uint8_t this_led = 0; this_led < NUM_BUTTONS; this_led++) {
		*dest++ = test_blue; //*(active_src+2);
		*dest++ = test_red; //*(active_src);  // !review: switch these orders to accomodate for LED Differences?
		*dest++ = test_green; //*(active_src+1);
	}
}
#endif

#if LED_CONFIGURATION == LED_CONFIGURATION_SINGLE_STRAND
void led_update_pixels(uint8_t *buffer)
{
	// MF 64 (LEDs with onboard PWM)
	// - Each LED Requires 24-bits
	// - There are 128 LEDs on MF 64
	// -- Each bit is of the following format
	// ---- 0: 1 for 0.4us, then 0 for 0.85us
	// ---- 1: 1 for 0.8us, then 0 for 0.45us 
	DDRC |= LED_ASYNC; // !review: overkill?

	// If buffer stores all led data it will be 384 (128*3) bytes long
	// - to start just set all leds to Red with this function. (255,0,0)
	//static uint32_t *test_buffer; // = 0x00000000;
	static uint32_t test_buffer = 0x00000000;
	uint32_t *single_led_buffer;
	
	// Turn off interrupts for a moment.
	cli();

	for (uint8_t this_led=0; this_led <= 127; this_led++) {
		single_led_buffer = (uint32_t *)(buffer);
		test_buffer = *single_led_buffer;
		if (this_led & 0x01) { // every other led changes button, we are treat each button as a single led (even though there are two per button)
			buffer = buffer + 3;
		}

		uint32_t this_mask = 0x800000;
		for (uint8_t this_bit=0; this_bit < 24; this_bit++) {
			// Transmit this one bit
			if (test_buffer & this_mask) { // High Bit
				// Set Port Value to 1
			    PORTC |= LED_ASYNC;  // data is latched on rising edge.
				//_delay_us(0.8);		// Spec, Wait 0.8us, two "nop"s works, and takes less time
				asm("nop");
				asm("nop");
				asm("nop");
				asm("nop");
				// Set Port Value to 0
			    PORTC &= ~LED_ASYNC;  // data is latched on rising edge.
				// Wait 0.45us
				//_delay_us(0.40); // We'll assume it takes .05 to go through the for loop (timing doesn't need be perfect)
			}
			else { // Low Bit
				// Set Port Value to 1
			    PORTC |= LED_ASYNC;  // data is latched on rising edge.
				// Wait 0.4us
				//_delay_us(0.10);
				// Set Port Value to 0
			    PORTC &= ~LED_ASYNC;  // data is latched on rising edge.
				// Wait	0.85us
				//_delay_us(0.8);  // We'll assume it takes .05 to go through the for loop (timing doesn't need be perfect)
			}
			this_mask >>= 1;		
		}
	}
	// Re-Enable Interrupts
	sei();
	// Leave Port low for at least 50us (we do this 'passively' here).
	//_delay_us(100);	
	return;
}
#elif LED_CONFIGURATION == LED_CONFIGURATION_FOUR_STRANDS
// Four strands

//volatile uint8_t *led_group_ports[4] = {&PORTB, &PORTC, &PORTB, &PORTB};
//uint8_t led_group_masks[4] = {LED_ASYNC_GROUP0, LED_ASYNC_GROUP1, LED_ASYNC_GROUP2, LED_ASYNC_GROUP3};
void led_update_pixel_group0(uint8_t *buffer)
{
		//volatile uint8_t *this_port = led_group_ports[group_id];
		//uint8_t port_bit_flag = led_group_masks[group_id];
	static uint32_t test_buffer = 0x00000000;
	uint32_t *single_led_buffer;
	// Turn off interrupts for a moment.
	//cli();
	for (uint8_t this_led=0; this_led <= 31; this_led++) {
		// Get One RGB State from Buffer
		single_led_buffer = (uint32_t *)(buffer);
		test_buffer = *single_led_buffer;
		if (this_led & 0x01) { // every other led changes button, we are treat each button as a single led (even though there are two per button)
			buffer = buffer + 3;
		}
		// Set individual LED States
		uint32_t this_mask = 0x800000;
		for (uint8_t this_bit=0; this_bit < 24; this_bit++) {
			//uint32_t this_value = test_buffer & this_mask; // !review: remove test_buffer?
			// Transmit this one bit
			if (test_buffer & this_mask) { // High Bit
				PORTB |= LED_ASYNC_GROUP0;  // Set Port Value to 1 (data is latched on rising edge).
				//_delay_us(0.8);		// Spec, Wait 0.8us, two "nop"s works, and takes less time
				asm("nop");
				asm("nop");
				asm("nop");
				asm("nop");
				PORTB &= ~LED_ASYNC_GROUP0;  // Set Port Value to 0
				//_delay_us(0.40); //  Spec, but proved unnecessary
			}
			else { // Low Bit
				// Set Port Value to 1
				PORTB |= LED_ASYNC_GROUP0;  // data is latched on rising edge.
				//_delay_us(0.10); 	// Spec, was Wait 0.4us, but proved unnecessary
				PORTB &= ~LED_ASYNC_GROUP0;  // Set Port Value to 0
				//_delay_us(0.8);  // Spec, was Wait 0.85us but proved unnecessary
			}
			this_mask >>= 1;
		}
	}
	//sei(); // Re-Enable Interrupts
	return;
}

void led_update_pixel_group1(uint8_t *buffer) 
{
	//DDRC |= LED_ASYNC;
	static uint32_t test_buffer = 0x00000000;
	uint32_t *single_led_buffer;	
	// Turn off interrupts for a moment.
	//cli();
	for (uint8_t this_led=0; this_led <= 31; this_led++) {
		// Get One RGB State from Buffer
		single_led_buffer = (uint32_t *)(buffer);
		test_buffer = *single_led_buffer;
		if (this_led & 0x01) { // every other led changes button, we are treat each button as a single led (even though there are two per button)
			buffer = buffer + 3;
		}
		// Set individual LED States
		uint32_t this_mask = 0x800000;
		for (uint8_t this_bit=0; this_bit < 24; this_bit++) {
			// Transmit this one bit
			if (test_buffer & this_mask) { // High Bit
				PORTC |= LED_ASYNC_GROUP1;  // Set Port Value to 1 (data is latched on rising edge).
				//_delay_us(0.8);		// LED Spec, Wait 0.8us, two "nop"s works, and takes less time
				asm("nop"); // !review: group1 requires 4 nops, portB ports only require 2... why?
				asm("nop");
				asm("nop");
				asm("nop");				
				PORTC &= ~LED_ASYNC_GROUP1;  // Set Port Value to 0
				//_delay_us(0.40); //  Spec, but proved unnecessary 
			}
			else { // Low Bit
				// Set Port Value to 1
				PORTC |= LED_ASYNC_GROUP1;  // data is latched on rising edge.
				//_delay_us(0.10); 	// Spec, was Wait 0.4us, but proved unnecessary
				PORTC &= ~LED_ASYNC_GROUP1;  // Set Port Value to 0
				//_delay_us(0.8);  // Spec, was Wait 0.85us but proved unnecessary
			}
			this_mask >>= 1;
		}
	}
	//sei(); // Re-Enable Interrupts
	return;
}

void led_update_pixel_group2(uint8_t *buffer)
{
	static uint32_t test_buffer = 0x00000000;
	uint32_t *single_led_buffer;
	// Turn off interrupts for a moment.
	//cli();
	for (uint8_t this_led=0; this_led <= 31; this_led++) {
		// Get One RGB State from Buffer
		single_led_buffer = (uint32_t *)(buffer);
		test_buffer = *single_led_buffer;
		if (this_led & 0x01) { // every other led changes button, we are treat each button as a single led (even though there are two per button)
			buffer = buffer + 3;
		}
		// Set individual LED States
		uint32_t this_mask = 0x800000;
		for (uint8_t this_bit=0; this_bit < 24; this_bit++) {
			// Transmit this one bit
			if (test_buffer & this_mask) { // High Bit
				PORTB |= LED_ASYNC_GROUP2;  // Set Port Value to 1 (data is latched on rising edge).
				//_delay_us(0.8);		// LED Spec, Wait 0.8us, two "nop"s works, and takes less time
				asm("nop");
				asm("nop");
				asm("nop");
				asm("nop");
				PORTB &= ~LED_ASYNC_GROUP2;  // Set Port Value to 0
				//_delay_us(0.40); //  Spec, but proved unnecessary
			}
			else { // Low Bit
				// Set Port Value to 1
				PORTB |= LED_ASYNC_GROUP2;  // data is latched on rising edge.
				//_delay_us(0.10); 	// Spec, was Wait 0.4us, but proved unnecessary
				PORTB &= ~LED_ASYNC_GROUP2;  // Set Port Value to 0
				//_delay_us(0.8);  // Spec, was Wait 0.85us but proved unnecessary
			}
			this_mask >>= 1;
		}
	}
	//sei(); // Re-Enable Interrupts
	return;
}

void led_update_pixel_group3(uint8_t *buffer)
{
	static uint32_t test_buffer = 0x00000000;
	uint32_t *single_led_buffer;
	// Turn off interrupts for a moment.
	//cli();
	for (uint8_t this_led=0; this_led <= 31; this_led++) {
		// Get One RGB State from Buffer
		single_led_buffer = (uint32_t *)(buffer);
		test_buffer = *single_led_buffer;
		if (this_led & 0x01) { // every other led changes button, we are treat each button as a single led (even though there are two per button)
			buffer = buffer + 3;
		}
		// Set individual LED States
		uint32_t this_mask = 0x800000;
		for (uint8_t this_bit=0; this_bit < 24; this_bit++) {
			// Transmit this one bit
			if (test_buffer & this_mask) { // High Bit
				PORTB |= LED_ASYNC_GROUP3;  // Set Port Value to 1 (data is latched on rising edge).
				//_delay_us(0.8);		// led Spec, Wait 0.8us, "nop"s works, and takes less time
				asm("nop");
				asm("nop");
				asm("nop");
				asm("nop");
				PORTB &= ~LED_ASYNC_GROUP3;  // Set Port Value to 0
				//_delay_us(0.40); //  Spec, but proved unnecessary
			}
			else { // Low Bit
				// Set Port Value to 1
				PORTB |= LED_ASYNC_GROUP3;  // data is latched on rising edge.
				//_delay_us(0.10); 	// Spec, was Wait up to 0.4us, but proved unnecessary
				PORTB &= ~LED_ASYNC_GROUP3;  // Set Port Value to 0
				//_delay_us(0.8);  // Spec, was Wait up to 0.85us but proved unnecessary
			}
			this_mask >>= 1;
		}
	}
	//sei(); // Re-Enable Interrupts
	return;
}

void led_update_pixels(uint8_t *buffer)
{
	DDRC |= LED_ASYNC_GROUP1; // !review: we don't need to set this every time
	DDRB |= LED_ASYNC_GROUP0 | LED_ASYNC_GROUP2 | LED_ASYNC_GROUP3; // !review: we don't need to set this every time
	cli(); // disable interrupts
	led_update_pixel_group0(buffer); // Test
	led_update_pixel_group1(buffer+48);
	led_update_pixel_group2(buffer+96);
	led_update_pixel_group3(buffer+144);
	sei(); // reenable interrupts
	return;
}
#endif // FOUR_STRANDS

// Lightshow effects -----------------------------------------------------------

// Rainbow ----------------------------------------------

#ifdef LIGHTSHOW

//static uint8_t rainbow_phase = 0;
//static uint8_t rainbow_scale_phase = 0;
//static uint8_t rainbow_scale_counter = 0;

// !review: move animations to a new file (animations.c/h)? 
// - (including this startup animation).

#define OVERRIDE_BALL_SLOWDOWN_FACTOR 0xF0 // must be less than 0xFF to avoid overflow
const uint8_t vertical_slowdown_factors[8] = { // 8 = GRID_COLS
	// At Top, 1 row down, 2 rows down, 3 rows down, 4 rows down, 5 rows down, 6 rows down, 7 rows down
	0x0C, 0x0A, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03
};

#define BALL_DEMO_MAX_BALLS 2
#define BALL_DEMO_BALL_INDEX_FLAGS 0x03
#define BALL_DEMO_NEW_BALL_DELAY_LIMIT 0x18
#define BALL_DEMO_MAX_SLOWDOWN_FACTOR 0xFF
static uint8_t ball_index = 0;
static bool ball_horizontal_direction[BALL_DEMO_MAX_BALLS];
static uint8_t ball_horizontal_slowdown_factor[BALL_DEMO_MAX_BALLS];
static uint8_t ball_horizontal_slowdown_counter[BALL_DEMO_MAX_BALLS];

static bool ball_vertical_direction[BALL_DEMO_MAX_BALLS];
static uint8_t ball_vertical_slowdown_counter[BALL_DEMO_MAX_BALLS];
static uint8_t ball_vertical_slowdown_factor[BALL_DEMO_MAX_BALLS];
static uint8_t ball_vertical_max[BALL_DEMO_MAX_BALLS];
static uint8_t ball_color[BALL_DEMO_MAX_BALLS];

static uint8_t ball_grid[8][8]; // row, column (vertical, horizontal)

void ball_demo_setup(void) {
	memset(ball_grid, BALL_DEMO_MAX_BALLS, sizeof(ball_grid)); // Clear the Grid!
}
// !review: perhaps do 'beams' instead of balls and avoid 'gravity' 
bool new_ball(void) {
	static uint8_t new_ball_delay_counter = BALL_DEMO_NEW_BALL_DELAY_LIMIT; // first ball will drop immediately, 
	static bool direction = true;
	new_ball_delay_counter += 1; // Increment Delay Counter
	
	if (new_ball_delay_counter < BALL_DEMO_NEW_BALL_DELAY_LIMIT) { // don't drop two new balls right next to each other
		return false;
	} 
	else { // New ball will be created, reset the counter
		new_ball_delay_counter = 0;
	}
	
	direction ^= 1;  // Toggle Direction, so each ball comes in on the opposite side of the last ball
	// Get Random Values
	uint8_t color =  random16() & 0x1F;
	color = color < 18 ? color+1: color-17;
	ball_horizontal_direction[ball_index] = direction;
	uint8_t ball_horizontal_position = direction ? 7:0;
	uint8_t ball_vertical_position = (random16() & 0x07) + 2; // Minimum Ball Max Column = 2
	ball_vertical_position = ball_vertical_position >= 8 ? 7: ball_vertical_position;
	if (ball_grid[ball_vertical_position][ball_horizontal_position] < BALL_DEMO_MAX_BALLS) {
		return false;  // Conflict exists, can't start a new ball where a ball currently resides
	} //else {}
		
	ball_grid[ball_vertical_position][ball_horizontal_position] = ball_index;
		
	ball_color[ball_index] = color;
	ball_horizontal_slowdown_factor[ball_index] = 8 + (random16() &0x1F);//10 + (random16() & 0x03); // inverted horizontal speed
	ball_vertical_max[ball_index] = ball_vertical_position;
	
	// Assign Fixed Values
	ball_vertical_direction[ball_index] = 0; // 1 = falling, 0 = rising
	ball_vertical_slowdown_factor[ball_index] = BALL_DEMO_MAX_SLOWDOWN_FACTOR;
	ball_horizontal_slowdown_counter[ball_index] = 0;
	ball_vertical_slowdown_counter[ball_index] = 0;
	
	// Increment ball_index for the next time this function is called
	ball_index = (ball_index+1) % BALL_DEMO_MAX_BALLS;
	return true; // ball was successfully created!
	
}

uint8_t move_ball_vertically(uint8_t ball_id, uint8_t this_row) {
	uint8_t new_row = this_row;
	uint8_t vertical_count = ball_vertical_slowdown_counter[ball_id] + 1;
	uint8_t base_row_offset = (ball_vertical_max[ball_id] - this_row) & 0x07; // emulate gravity: Move Faster the further from the max height you are
	if (vertical_count >= vertical_slowdown_factors[base_row_offset]) { // Case: Time to Move!
		ball_vertical_slowdown_counter[ball_id] = 0;
		new_row = ball_vertical_direction[ball_id] ? this_row - 1: this_row + 1;
		if (new_row <= ball_vertical_max[ball_id]) { // Case: moved to a valid row (note depends on overflow to 0xFF when going below 0)
			// Do Nothing
		}
		else if (new_row & 0x80){ // Case: Going Down -> Hit the floor
			new_row = this_row; // revert to current column
			// Change Direction (no change to column)
			bool direction = ball_vertical_direction[ball_id];
			direction ^= 1;		
			ball_vertical_direction[ball_id] = direction;	
			// Make sure it moves on the next turn
			ball_vertical_slowdown_counter[ball_id] = vertical_slowdown_factors[base_row_offset];
		}
		else { // Case: Going Up -> Hit Ceiling
			new_row = this_row; // revert to current column
			// Change Direction (no change to column)
			bool direction = ball_vertical_direction[ball_id];
			direction ^= 1;
			ball_vertical_direction[ball_id] = direction;
			// Make sure it moves on the next turn
			ball_vertical_slowdown_counter[ball_id] = vertical_slowdown_factors[base_row_offset];			
		}
	}
	else { // Case: No Movement
		ball_vertical_slowdown_counter[ball_id] = vertical_count;
	}
	return new_row;
}

uint8_t move_ball_horizontally(uint8_t ball_id, uint8_t this_col) {
	uint8_t new_col = this_col;
	uint8_t horizontal_count = ball_horizontal_slowdown_counter[ball_id] + 1;
	if (horizontal_count >= ball_horizontal_slowdown_factor[ball_id]) {
		bool direction = ball_horizontal_direction[ball_id];
		ball_horizontal_slowdown_counter[ball_id] = 0;
		new_col = direction ? this_col - 1: this_col + 1;
		if (new_col < 8) { // // Case: moved to a valid column -> note: this depends on overflow to 0xFF (when going below 0)
			// Valid Column, Do nothing
		}
		else { // Case: Hit the Wall
			new_col = this_col; // revert to current column
			// Change Direction (no change to column)
			direction ^= 1;
			ball_horizontal_direction[ball_id] = direction;
			// Make sure it moves on the next turn
			ball_horizontal_slowdown_counter[ball_id] = ball_horizontal_slowdown_factor[ball_id];			
		}
	}
	else { // Case: object did not move this round (no change to column)
		ball_horizontal_slowdown_counter[ball_id] = horizontal_count;
	}
	return new_col;
}

void change_ball_colors_after_collision(uint8_t this_ball, uint8_t that_ball) {
	uint16_t random_value = random16(); // !review: random seems clock based 
	// This Ball
	uint16_t color =  random_value & 0x1F;
	// That Ball
	color = random_value >> 5 & 0x1F;
	color = color < 18 ? color+1: color-17;
	ball_color[that_ball] = color;
};
	
#define REPEAT_BALL_COLLISION_LIMIT 0xFF
bool repeat_collision_limit_reached(uint8_t this_ball, uint8_t that_ball) {
	static uint8_t last_this_ball = 0xFF;
	static uint8_t last_that_ball = 0xFF;
	static uint8_t repeat_collision_counter = 0;
	
	if (this_ball == last_this_ball && that_ball == last_that_ball) { // repeat collision
		repeat_collision_counter += 1;
	}
	else if (this_ball == last_that_ball && that_ball == last_this_ball) { // repeat collision
		repeat_collision_counter += 1;
	}
	else { // New Collision
		repeat_collision_counter = 0;
		return false; // repeat collision count limit not reached
	}

	if (repeat_collision_counter >= REPEAT_BALL_COLLISION_LIMIT) { // limit reached!
		repeat_collision_counter = 0;
		return true;
	}
	else { // limit not reached
		return false;
	}
}

void ball_demo_run(uint8_t *buffer) {
	if (g_led_counter[2] == 0) {
		// restart the counter
		g_led_counter[2] = 0x30;
		bool disable_collision[64];
		memset(disable_collision, 0, sizeof(disable_collision));
		// update the animation
		// Move existing balls
		// - if an existing ball moves to a grid position that does not exist, then ball_demo_balls-=1;
		// - at full speed, a ball will move over one square and right one square
		uint8_t balls_on_grid = 0;
		// Check the Grid!
		for (uint8_t this_row = 0; this_row < 8; this_row++) {
			for (uint8_t this_col = 0; this_col < 8; this_col++) {
				uint8_t this_ball = ball_grid[this_row][this_col];
				if (this_ball < BALL_DEMO_MAX_BALLS) {
					uint8_t new_col = move_ball_horizontally(this_ball, this_col);
					uint8_t new_row = move_ball_vertically(this_ball, this_row);
					
					//if (new_row != this_row || new_col != this_col) { // redundant to 'that_ball != this_ball'
					uint8_t that_ball = ball_grid[new_row][new_col];
					if (that_ball >= BALL_DEMO_MAX_BALLS) { // Case: Moving to an open position
						ball_grid[this_row][this_col] = BALL_DEMO_MAX_BALLS; // This spot is now empty
						ball_grid[new_row][new_col] = this_ball; // Nothing is left!
						balls_on_grid += 1;
					}
					else if (that_ball == this_ball) { // Case: Did not move
						balls_on_grid += 1;
					}
					else { // Collision Detected!
						uint8_t btn_id = get_button_id_from_row_column(this_row, this_col);
						// Example B: Both Items Change Direction (in what ever direction they've collided).
						balls_on_grid += 1;
						
						// - Check for 'Repeat Collisions'
						if (disable_collision[this_ball]) {
							// Do nothing
						}
						else if (repeat_collision_limit_reached(this_ball, that_ball)) {
							// Example A: Both Items Explode
							start_geometric_animation(btn_id, random16()&0x03); // both items Explode!
							ball_grid[this_row][this_col] = BALL_DEMO_MAX_BALLS;
							ball_grid[new_row][new_col] = BALL_DEMO_MAX_BALLS; // This spot is now empty
							if (new_row < this_row || new_col < this_col ) { // If we already passed the destroyed ball, we subtract from balls_on_grid
								balls_on_grid -=2;
							}					
						} 
						// - Check for 'Vertical Collision'
						else if (new_row > this_row) { // Case: vertical collision: this_ball is going up (on bottom)
							ball_vertical_direction[this_ball] = false; // go back down after collision
							ball_vertical_direction[that_ball] = true; // go up after collision
							// move on next turn
							ball_vertical_slowdown_counter[this_ball] = OVERRIDE_BALL_SLOWDOWN_FACTOR;
							change_ball_colors_after_collision(this_ball, that_ball);
							
						}
						else if (new_row < this_row) { // Case: vertical collision: this_ball is going down (on top)
							ball_vertical_direction[this_ball] = true; // go back up after collision
							ball_vertical_direction[that_ball] = false; // go down after collision		
							// move on next turn
							ball_vertical_slowdown_counter[this_ball] = OVERRIDE_BALL_SLOWDOWN_FACTOR;
							change_ball_colors_after_collision(this_ball, that_ball);
						}
						//else {} // Case: no Vertical Collision
						
						// - Check for 'Horizontal Collision'
						if (new_col > this_col) { // Case: horizontal collision: this_ball is going right (on left)
							ball_horizontal_direction[this_ball] = false; // go back left after collision
							ball_horizontal_direction[that_ball] = true; // go right after collision
							// move on next turn
							ball_horizontal_slowdown_counter[this_ball] = OVERRIDE_BALL_SLOWDOWN_FACTOR;
							change_ball_colors_after_collision(this_ball, that_ball);
						}
						else if (new_col < this_col) { // Case: horizontal collision: this_ball is going left (on right)
							ball_horizontal_direction[this_ball] = true; // go back right after collision
							ball_horizontal_direction[that_ball] = false; // go left after collision
							// move on next turn
							ball_horizontal_slowdown_counter[this_ball] = OVERRIDE_BALL_SLOWDOWN_FACTOR;
							change_ball_colors_after_collision(this_ball, that_ball);
						}
						//else {} // Case: no Horizontal Collision
						// neither ball actually moves
						disable_collision[that_ball] = true; // disable redundant collisions from 
								
						// Example A: Both Items Explode
						//start_geometric_animation(btn_id, random16()&0x03); // both items Explode!
						//ball_grid[this_row][this_col] = BALL_DEMO_MAX_BALLS;
						//ball_grid[new_row][new_col] = BALL_DEMO_MAX_BALLS; // This spot is now empty
						//if (new_row < this_row || new_col < this_col ) { // If we already passed the destroyed ball, we subtract from balls_on_grid
						//	balls_on_grid -=1;
						//}
					}
					//}
					//else { // Did note Move
						//balls_on_grid += 1;
					//}
					

				}
			}
		}

		// If we don't have enough existing balls add a new one!
		if (balls_on_grid < BALL_DEMO_MAX_BALLS) {
			bool status = new_ball();
			if (status) {
				balls_on_grid += 1;
			}
		} 
	} // update the animation (END)
	
	// Light/ Darken All LEDs
	for (uint8_t this_row = 0; this_row < 8; this_row++) {
		for (uint8_t this_col = 0; this_col < 8; this_col++) {
			uint8_t btn_id = get_button_id_from_row_column(this_row, this_col);
			uint8_t ball_id = ball_grid[this_row][this_col];
			uint8_t *display_ptr = buffer;
			if (ball_id >= BALL_DEMO_MAX_BALLS) {
				// Black: No adjustment to color_ptr needed.
				*(display_ptr+3*btn_id) = default_color[0][2];
				*(display_ptr+3*btn_id+1) = default_color[0][0];
				*(display_ptr+3*btn_id+2) = default_color[0][1];				
			}
			else {
				uint8_t color = ball_color[ball_id];
				*(display_ptr+3*btn_id) = default_color[color][2];
				*(display_ptr+3*btn_id+1) = default_color[color][0];
				*(display_ptr+3*btn_id+2) = default_color[color][1];
			}
		}
	}
}
#endif // LIGHTSHOW

