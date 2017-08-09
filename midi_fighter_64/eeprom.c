// EEPROM functions for DJTechTools Midifighter
//
//   Copyright (C) 2011 DJ Techtools
//
// rjgreen 2009-05-22

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

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "key.h"
#include "midi.h"
#include "display.h"
#include "eeprom.h"
#include "constants.h"
#include "accel_gyro.h"

uint8_t G_EE_COMBOS_ENABLE;
uint8_t G_EE_MIDI_OUTPUT_MODE;
uint8_t G_EE_FOUR_BANKS_MODE;
uint8_t G_EE_TILT_MODE;
uint8_t G_EE_TILT_MASK;
uint8_t G_EE_ANIMATIONS;
uint8_t G_EE_DEVICE_MODE;
uint8_t G_EE_TILT_SENSITIVITY;
uint8_t G_EE_PITCH_SENSITIVITY;
uint8_t G_EE_TILT_RANGE;
uint8_t G_EE_PITCH_RANGE;
uint8_t G_EE_TILT_DEADZONE;
uint8_t G_EE_PITCH_DEADZONE;
uint8_t G_EE_TILT_AXIS;
uint8_t G_EE_PICK_SENSITIVITY;
uint8_t G_EE_SLEEP_TIME;
uint8_t G_EE_TILT_TYPE;
uint8_t G_EE_SIDE_BANK;

uint8_t g_self_test_passed; // for legacy purposes only, not used by mf64
extern uint8_t default_bank_inactive[2][64*3];
extern uint8_t default_bank_active[2][64*3];


// EEPROM functions ------------------------------------------------------------

// Write an 8-bit value to EEPROM memory.
//
void eeprom_write(uint16_t address, uint8_t data)
{
    // Wait for completion of previous write
    while(EECR & (1<<EEPE)) {}  // !review: add timeout?
    // Set up Address and Data Registers
    EEAR = address & 0x0fff; // mask out 512 bytes
    EEDR = data;
    // This next critical section cannot have an interrupt occur between the
    // two writes or the write will fail and we will lose the information,
    // so disable all interrupts for a moment.
    cli();
    // Write logical one to EEMPE (Master Program Enable) to allow us to
    // write.
    EECR |= (1<<EEMPE);
    // Then within 4 cycles, initiate the eeprom write by writing to the
    // EEPE (Program Enable) strobe.
    EECR |= (1<<EEPE);
    // Reenable interrupts.
    sei();
}

// Read an 8-bit value from EEPROM memory.
//
uint8_t eeprom_read(uint16_t address)
{
    // Wait for completion of previous write
    while(EECR & (1<<EEPE)) {} // !review: add timeout?
    // Set up address register
    EEAR = address;
    // Start eeprom read by writing EERE (Read Enable)
    EECR |= (1<<EERE);
    // Return data from Data Register
    return EEDR;
}


// EEPROM settings ------------------------------------------------------------

// Set up the EEPROM system for use and read out the settings into the
// global values.
//
// This includes checking the layout version and, if the version tag written
// to the EEPROM doesn't match this software version we're running, we reset
// the EEPROM values to their default settings.
//
void eeprom_setup(void)
{
    // If our EEPROM layout has changed, reset everything.
    if (eeprom_read(EE_EEPROM_VERSION) != EEPROM_LAYOUT) {
        eeprom_factory_reset();
    }
	else {
	}

    // Read the EEPROM into the global settings.
    g_self_test_passed = eeprom_read(EE_FIRST_BOOT_CHECK);
    G_EE_MIDI_CHANNEL = eeprom_read(EE_MIDI_CHANNEL);
    G_EE_MIDI_VELOCITY = eeprom_read(EE_MIDI_VELOCITY);
	
	G_EE_COMBOS_ENABLE = eeprom_read(EE_COMBOS_ENABLE);
	G_EE_MIDI_OUTPUT_MODE = eeprom_read(EE_MIDI_OUTPUT_MODE);
	G_EE_FOUR_BANKS_MODE = eeprom_read(EE_FOUR_BANKS_MODE);
	G_EE_TILT_MODE = eeprom_read(EE_TILT_MODE);
	G_EE_TILT_MASK = eeprom_read(EE_TILT_MASK);
	G_EE_ANIMATIONS = eeprom_read(EE_ANIMATIONS);
	G_EE_TILT_SENSITIVITY = eeprom_read(EE_TILT_SENSITIVITY);
 	G_EE_PITCH_SENSITIVITY = eeprom_read(EE_PITCH_SENSITIVITY);
 	G_EE_TILT_RANGE = eeprom_read(EE_TILT_RANGE);
 	G_EE_PITCH_RANGE = eeprom_read(EE_PITCH_RANGE);
 	G_EE_TILT_DEADZONE = eeprom_read(EE_TILT_DEADZONE);
 	G_EE_PITCH_DEADZONE = eeprom_read(EE_PITCH_DEADZONE);
 	G_EE_TILT_AXIS = eeprom_read(EE_TILT_AXIS);
	G_EE_SIDE_BANK = eeprom_read(EE_SIDE_BANK);

	 
    G_EE_PICK_SENSITIVITY = eeprom_read(EE_PICK_SENSITIVITY);
    G_EE_SLEEP_TIME = eeprom_read(EE_SLEEP_TIME);
	
	// If 'saved' data already exists, then overwrite the default colors table with the saved data
	for (uint16_t bank=0; bank<NUM_BANKS; ++bank) { // mf64 only 1 bank
		// Load the idle colors from EEPROM
		uint8_t* colors = default_bank_inactive[bank];
		for (uint16_t i=0; i<64*3; ++i) {  // 64 = NUM_BUTTONS
			colors[i] = eeprom_read(EE_COLORS_IDLE+(NUM_BUTTONS*3*bank)+i);
		}
		// Load the active colors from EEPROM
		colors = default_bank_active[bank];
		for (uint16_t i=0; i<64*3; ++i) { // 64 = NUM_BUTTONS
			colors[i] = eeprom_read(EE_COLORS_ACTIVE+(NUM_BUTTONS*3*bank)+i);
		}
	}
}

// Used by the menu system, if we have edited any of the global values then
// save them off to the EEPROM.
//
void eeprom_save_edits(void)  // Save Settings and Color Scheme settings to EEPROM
{
    eeprom_write(EE_MIDI_CHANNEL, G_EE_MIDI_CHANNEL);
    eeprom_write(EE_MIDI_VELOCITY, G_EE_MIDI_VELOCITY);
	eeprom_write(EE_COMBOS_ENABLE, G_EE_COMBOS_ENABLE);
	eeprom_write(EE_MIDI_OUTPUT_MODE, G_EE_MIDI_OUTPUT_MODE);
	eeprom_write(EE_FOUR_BANKS_MODE, G_EE_FOUR_BANKS_MODE);
	eeprom_write(EE_TILT_MODE, G_EE_TILT_MODE);
	eeprom_write(EE_TILT_MASK, G_EE_TILT_MASK);
	eeprom_write(EE_ANIMATIONS, G_EE_ANIMATIONS);
 	eeprom_write(EE_TILT_SENSITIVITY,G_EE_TILT_SENSITIVITY);
 	eeprom_write(EE_PITCH_SENSITIVITY,G_EE_PITCH_SENSITIVITY);
 	eeprom_write(EE_TILT_RANGE,G_EE_TILT_RANGE);
 	eeprom_write(EE_PITCH_RANGE,G_EE_PITCH_RANGE);
 	eeprom_write(EE_TILT_DEADZONE,G_EE_TILT_DEADZONE);
 	eeprom_write(EE_PITCH_DEADZONE,G_EE_PITCH_DEADZONE);
 	eeprom_write(EE_TILT_AXIS, G_EE_TILT_AXIS);
	eeprom_write(EE_PICK_SENSITIVITY, G_EE_PICK_SENSITIVITY);
	eeprom_write(EE_SLEEP_TIME, G_EE_SLEEP_TIME);
	eeprom_write(EE_SIDE_BANK, G_EE_SIDE_BANK);
	
	for (uint16_t bank=0; bank<NUM_BANKS; ++bank) {
		// Save the idle colors to EEPROM
		uint8_t* colors = default_bank_inactive[bank];  // !review: pressing 'send to midifighter' rapidly in utility, frequently results in the 'active' color being sat as the 'idle' color
		for (uint16_t i=0; i<64*3; ++i) {
			eeprom_write(EE_COLORS_IDLE+(64*3*bank)+i, colors[i]);  
		}
		// Save the active colors to EEPROM
		colors = default_bank_active[bank];
		for (uint16_t i=0; i<64*3; ++i) {
			eeprom_write(EE_COLORS_ACTIVE+(64*3*bank)+i, colors[i]);
		}
	}

}

// Return the EEPROM values to their factory default values, erasing any
// customizations you may have made. Sorry dude!
//
void eeprom_factory_reset(void)
{
     // NOTE: hardware check on first boot only occurs if the eeprom was zeroed.
    // and not every time we reflash an eeprom. That keeps it a rare event.

    eeprom_write(EE_EEPROM_VERSION, EEPROM_LAYOUT);
	// We dont want to reset the first boot check otherwise user must
	// perform button test after a factory reset
    //eeprom_write(EE_FIRST_BOOT_CHECK,      0xff); // Enable for factory firmware only

    // Reset the global variables to their default versions, as they were
    // read with their old values before the factory reset happened and they
    // could get written back if the user leaves menu mode through the exit
    // button.
    g_self_test_passed = 0xff;
    G_EE_MIDI_CHANNEL = 2; // MIDI channel (3)
    G_EE_MIDI_VELOCITY = 127; // MIDI velocity (127)
	G_EE_COMBOS_ENABLE = 0x01; // Currently Enabled by default
	G_EE_MIDI_OUTPUT_MODE = MIDI_OUTPUT_MODE_NOTES_ONLY;
	G_EE_FOUR_BANKS_MODE = 0x00;
	G_EE_TILT_MODE = 0x02;
	G_EE_TILT_MASK = 0xF1;
	G_EE_ANIMATIONS = GEOMETRIC_ANIMATION_TYPES; // MF64->Geometric Animations (Default: Disabled)
 	G_EE_TILT_SENSITIVITY = 0x1E;
 	G_EE_PITCH_SENSITIVITY = 0x7F;
 	G_EE_TILT_RANGE = 0x46;
 	G_EE_PITCH_RANGE = 0x3C;
 	G_EE_TILT_DEADZONE = 0x0C;
 	G_EE_PITCH_DEADZONE = 0x7F;
 	G_EE_TILT_AXIS = 0x00;
	G_EE_PICK_SENSITIVITY = 0x40;
	G_EE_SIDE_BANK = 0x00; // Corner Bank Change
	G_EE_SLEEP_TIME = 0x3C;
 	// Load the Default Color Scheme
 	load_default_colors();
	// Save the edits to settings and color scheme
	eeprom_save_edits();
}


// -----------------------------------------------------------------------------
