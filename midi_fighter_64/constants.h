// Constant values shared by all subsystems for DJTechTools Midifighter.
//
//   Copyright (C) 2011 DJ Techtools
//
// rgreen 2011-01-27
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
#ifndef _CONSTANTS_H_INCLUDED
#define _CONSTANTS_H_INCLUDED

// Hardware Counts
#define NUM_BUTTONS 64
#define BUTTON_ID_FLAGS 0x3F

#define MIDI_CHANNEL_INDEX_CONTROL_BANKS 0
#define MIDI_CHANNEL_INDEX_ANIMATIONS 1

// Device Configuration Constants
// - USB Backend
#define USE_LUFA_2015 1

// - USB Receive - Wait and Catch
#define USB_RX_ONCE_PER_MAINLOOP 0
// !review: remove periodically as an option or improve it so it's as good or better than the alternative?
#define USB_RX_PERIODICALLY 1 // 100+ times throughout main loop
#define USB_RX_METHOD USB_RX_ONCE_PER_MAINLOOP
// - How many packets to look for per iteration
#if USB_RX_METHOD < USB_RX_PERIODICALLY
#define USB_RX_FAIL_LIMIT 120
#define USB_RX_PACKET_LIMIT 512
#else
#define USB_RX_FAIL_LIMIT 160
#define USB_RX_PACKET_LIMIT 2
#endif

#define ENABLE_LUFA_2015_LARGE_PACKET_UPGRADE 0

// - MIDI Feedback
#define ENABLE_NOTE_OFF_FEEDBACK_DELAY 1
#define NOTE_OFF_FEEDBACK_DELAY_LIMIT 2 // !review: working value was 20, works at '1' with increased throughput, works at '2'
#define DEBOUNCE_BUFFER_SIZE 10

#define MIDI_FEEDBACK_MF3D_MODE 0  // 20 colors
#define MIDI_FEEDBACK_ABLETON_MODE 1 // 15 2-bit dimable colors, 68 custom colors
#define MIDI_FEEDBACK_MODE MIDI_FEEDBACK_ABLETON_MODE

// - BANKING
#define NUM_BANKS 2
#define G_BANK_SELECT_COUNTER_LIMIT 1000
// - Metric Testing
#define ENABLE_TEST_OUT_MAINLOOP_COUNT 0
#define ENABLE_TEST_OUT_LED_REFRESH_COUNT 0
#define ENABLE_TEST_OUT_USB_RECEIVE 0
#define ENABLE_TEST_OUT_NOTE_COUNTERS 0
#define ENABLE_TEST_OUT_USB_PACKETS_PER_INTERVAL 0
#define ENABLE_TEST_IN_LED_CALIBRATION 0

// CPU port constants ---------------------------------------------------------


#define LED_ASYNC  _BV(PC6)  // MF64 Prototype (note production unit has 3 more pins)
#define LED_ASYNC_GROUP0	_BV(PB6)  // Production Units (has 4 pins)
#define LED_ASYNC_GROUP1	_BV(PC6)
#define LED_ASYNC_GROUP2	_BV(PB5)
#define LED_ASYNC_GROUP3	_BV(PB4)

// MF64 LEDS - !review: DEPRECATE! (Start)
// !error: when we initialize ports we use 3D nomenclature. Fix this!
#define LED_LATCH  _BV(PB0)  // Latch the state to the LEDs (pulse high)
#define LED_CLOCK  _BV(PB1)  // SPI clock pin
#define LED_MOSI   _BV(PB2)  // SPI master out slave in
#define LED_MISO   _BV(PB3)  // SPI master in slave out
#define LED_MODE   _BV(PB7)  // LED Driver mode

#define LED_BLANK  _BV(PC6)  // Turn off all LEDs
#define LED_PWM    _BV(PC7)  // LED PWM clock output
// MF3D LEDS - !review: DEPRECATE! (End)

// MF64 reads keys from a single pin.
#define KEY_BIT    _BV(PC7)  // Key read input low bit
#define KEY_CLOCK  _BV(PD7)  // Key read clock pin
#define KEY_LATCH  _BV(PD6)  // Key latch state to shift-out registers

// Device ID constants --------------------------------------------------------

// 14bit device family ID, LSB first
#define DEVICE_FAMILY_LSB   0x06 // 5 is twister, 6 is 64
#define DEVICE_FAMILY_MSB	0x00
#define DEVICE_FAMILY   	DEVICE_FAMILY_LSB, DEVICE_FAMILY_MSB

// 14bit device model ID, LSB first
#define DEVICE_MODEL_LSB	0x01
#define DEVICE_MODEL_MSB    0x00
#define DEVICE_MODEL    	DEVICE_MODEL_LSB, DEVICE_MODEL_MSB

// Should be the date of this firmware release, in hex, in the following format: 0xYYYYMMDD
#define DEVICE_VERSION_YEAR 	0x2017
#define DEVICE_VERSION_YEAR_LSB 0x17
#define DEVICE_VERSION_YEAR_MSB 0x20
#define DEVICE_VERSION_MONTH	0x07
#define DEVICE_VERSION_DAY		0x24
#define DEVICE_VERSION  		((DEVICE_VERSION_YEAR << 16) | (DEVICE_VERSION_MONTH << 8) | DEVICE_VERSION_DAY)

// How many minutes of inactivity before sleep?
#define SLEEP_TIME_PERIOD    60

// How often is the usage data saved (in minutes)
#define USAGE_DATA_PERIOD     1

// MIDI map constants ---------------------------------------------------------

// DJTechTools MIDI Manufacturer ID
// see http://www.midi.org/techspecs/manid.php
//
#define MIDI_MFR_ID_0            0x00
#define MIDI_MFR_ID_1            0x01
#define MIDI_MFR_ID_2            0x79
#define MANUFACTURER_ID        0x0179

#define MIDI_BASENOTE              36  // Note number for the lowest key.
#define MIDI_SIDE_BASENOTE         20  // Note number for the highest side key
#define MIDI_MAX_NOTES            128  // Number of notes to track.
#define MIDI_MAX_SYSEX             64  // max number of bytes in a valid sysex message.

// EEPROM constants -----------------------------------------------------------

// Increment this when the EEPROM layout requires resetting to the factory
// default.
#define EEPROM_LAYOUT                 1

// EEPROM memory locations of persistent settings
#define EE_EEPROM_VERSION        0x0000  // Is the EEPROM layout current?
#define EE_FIRST_BOOT_CHECK      0x0001  // Hardware passed mfr testing?
#define EE_MIDI_CHANNEL          0x0002  // MIDI channel byte (0..15)
#define EE_MIDI_VELOCITY         0x0003  // MIDI velocity byte (0..127)
#define EE_KEY_KEYPRESS_LED      0x0004  // Light the LED of pressed keys (bool)
#define EE_FOUR_BANKS_MODE       0x0005  // Use Deckalized colors on the info display.
#define EE_AUTO_UPDATE			 0x0008  // Legacy Four Banks Settings.
#define EE_MIDI_OUTPUT_MODE         0x0009  // Rotation Setting 0 = defualt
#define EE_COMBOS_ENABLE		 0x000A  // Combos enabled (1-bit)
#define EE_TILT_MODE             0x000C  // Tilt action Locked 0x00 or Mixed 0x01
#define EE_TILT_MASK			 0x000D  // Masks midi for tilts MSB LEFT FWD RIGHT BACK Z_ABS _Z_REL 
#define EE_ANIMATIONS            0x000E  // Masks for animations MSB - - - - - - - SPARKLE

#define EE_TILT_SENSITIVITY		 0x000F  // Changes the damping coefficient of the first order complementary filter
#define EE_PITCH_SENSITIVITY     0x0010  // TODO - Changes the sensitivity of the pitch
#define EE_TILT_RANGE            0x0011  // Sets the physical range which the tilt CCs are mapped over (30 - 90 deg)
#define EE_PITCH_RANGE           0x0012  // Sets the physical range which the pitch CC's are mapped over (30 - 180 deg)
#define EE_TILT_DEADZONE         0x0013  // Sets the size of the tilt dead zone (1 - 30 deg)
#define EE_PITCH_DEADZONE        0x0014  // TODO Sets the size of the pitch dead zone (1 - 30 deg)
#define EE_TILT_AXIS             0x0015  // Controls if tilt is sent as 1 CC per direction (4 total) or 1 per axis (2 total)
#define EE_PICK_SENSITIVITY      0x0016  // Sets the sensitivity of the pickup detection.
#define EE_SLEEP_TIME            0x0017  // Sets time period (1 - 60 Minutes) for sleep timer, 0 to disable
#define EE_SIDE_BANK             0x0018  // If enabled then side button number changes with bank

#define EE_COLORS_IDLE			 0x006F  // Start of idle color map, size = 2*64*3
#define EE_COLORS_ACTIVE		 0x01EF  // Start of active color map, size = 2*64*3
#define EE_COLORS_LAST		 	 0x036F  // 0x018F is the next free EEPROM slot for use
#define EE_FACTORY_RESET_FLAG    0x038F  // Stores the EEPROM factory reset flag
    
// Device Output Modes
#define MIDI_OUTPUT_MODE_NOTES_ONLY  0x00
#define MIDI_OUTPUT_MODE_NOTES_AND_CCS  0x01
#define MIDI_OUTPUT_MODE_CCS_ONLY 0x02

// ----------------------------------------------------------------------------
#endif // _CONSTANTS_H_INCLUDED
  