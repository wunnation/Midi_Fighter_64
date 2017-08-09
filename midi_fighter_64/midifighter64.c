// Main loop for the Midifighter 64 firmware
//
//   Copyright (C) 2017 - DJ Tech Tools
//


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
#include <math.h>

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/boot.h>
#include <avr/power.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include <LUFA/Version.h>                 // Library Version Information
#include <LUFA/Drivers/USB/USB.h>         // USB Functionality


//#include "i2c.h"
#include "key.h"
#include "led.h"
#include "midi.h"
#include "combo.h"
#include "eeprom.h"
#include "random.h"
#include "display.h"
#include "constants.h"
#include "accel_gyro.h"
#include "usb_descriptors.h"
#include "jumptoboot.h"
#include "sysex.h"
#include "config.h"



// Declare Fuses for the ATmega32U4 ---------------------------------------------

// *********** FAR TOO DANGEROUS TO USE *************
//FUSES = {
//  //.low = FUSE_CKOUT | 0XEE,  // external 8MHz-, startup 1K CK + 0ms
//  //.high = (FUSE_BOOTSZ0 | FUSE_BOOTSZ1 | FUSE_EESAVE | FUSE_SPIEN | FUSE_JTAGEN),
//  //.extended = EFUSE_DEFAULT,
//
//    .low = 0x9E,
//    .high = 0x99,
//    .extended = 0xF3,
//};
// *********** FAR TOO DANGEROUS TO USE *************


// Forward Declarations --------------------------------------------------------

// Declare the MIDI function.
void MIDI_Task(void);

// The USB Callback events handled by this program.
void EVENT_USB_Device_Connect(void);
void EVENT_USB_Device_Disconnect(void);
void EVENT_USB_Device_ConfigurationChanged(void);
void EVENT_USB_Device_UnhandledControlRequest(void);

static bool watchdog_flag = false;

#if ENABLE_TEST_OUT_NOTE_COUNTERS > 0
#warning Note On and Note Off Counter Output has been enabled
uint16_t note_on_count = 0; // Note On messages Received that apply to bank 1
uint16_t note_off_count = 0; // Note Off messages Received that apply to bank 1
uint16_t note_off_delay_count = 0; // Number of times a Note has been turned off by the 'delay' routine
uint16_t note_off_delay_skip_count =  0; // Number of times a Note off has been skipped by the 'delay' routine
#endif

#if ENABLE_TEST_OUT_USB_PACKETS_PER_INTERVAL > 0
uint16_t usb_packets_per_interval_max = 0;
#endif

// USB Tasks and Events --------------------------------------------------------

// We are in the process of enumerating but not yet ready to generate MIDI.
//
void EVENT_USB_Device_Connect(void)
{
    // Indicate that USB is enumerating.
	led_enable();
}

// The device is no longer connected to a host.
//
void EVENT_USB_Device_Disconnect(void)
{
    // Indicate that USB is disconnected.
}

// Device has enumerated. Set up the Endpoints.
//
void EVENT_USB_Device_ConfigurationChanged(void)
{
    // Indicate that USB is now ready to use if desired 

    // Allow the LUFA MIDI Class drivers to configure the USB endpoints.
    if (!MIDI_Device_ConfigureEndpoints(g_midi_interface_info)) {
        // Setting up the endpoints failed, display the error state, and return.
		return;
    }

    // Success. Enable the display and do the power on light show
	led_enable();
	// power_on_lightshow();
	// Now enable watchdog timer
	wdt_enable(WDTO_2S);
}

// Any other USB control command that we don't recognize is handled here.
//
void EVENT_USB_Device_UnhandledControlRequest(void)
{
    // Let the LUFA MIDI Class handle this request.
    MIDI_Device_ProcessControlRequest(g_midi_interface_info);
}

// ***************************
// This code is here to debug the LED freeze when connected but not being
// listened to under Windows.
// **************************
//
// bool My_Device_ReceiveEventPacket(
//         USB_ClassInfo_MIDI_Device_t* const MIDIInterfaceInfo,
//         MIDI_EventPacket_t* const Event)
// {
//     if (USB_DeviceState != DEVICE_STATE_Configured) {
//         led_set_state(0x0001);
//         return false;
//     }
//
//     Endpoint_SelectEndpoint(MIDIInterfaceInfo->Config.DataOUTEndpointNumber);
//
//     led_set_state(0x0002);
//
//     if (!(Endpoint_IsReadWriteAllowed())) {
//         led_set_state(0x0004);
//         return false;
//     }
//
//     Endpoint_Read_Stream_LE(Event,
//                             sizeof(MIDI_EventPacket_t),
//                             NO_STREAM_CALLBACK);  // !mark: NO_STREAM_CALLBACK? What are the other options? USB Config...
//
//     led_set_state(0x0008);
//
//     if (!(Endpoint_IsReadWriteAllowed())) {
//         led_set_state(0x0010);
//         Endpoint_ClearOUT();
//     }
//
//     led_set_state(0x0020);
//
//     return true;
// }

// The MIDI processing task.
//
// Read the buttons to generate MIDI notes. This routine
// is the heart of the MidiFighter.
//
//#define USB_RX_FAIL_LIMIT 100 // seems to work (0 can see squares sometimes)
void update_note_off_feedback_delay(void) {
	uint16_t sys_time_7 = system_time_ms & 0x7F; // !note: this must stay as a uin16_t, for some reason, setting this as a uint8_t appears to grab the MSB for system_time_ms
    for (uint8_t this_key = 0; this_key < (NUM_BANKS*NUM_BUTTONS); this_key++) {
		uint8_t note_off_time = g_midi_note_off_counter[this_key];
		if (note_off_time & 0x80) {
			note_off_time &= 0x7F; // Strip off the 'flag'
			if ((sys_time_7 - note_off_time) >= NOTE_OFF_FEEDBACK_DELAY_LIMIT) {
				g_midi_note_state[0][this_key] = 0; // set value to 0, banks 1 and 2 are on [0][this_key]
				g_midi_note_off_counter[this_key] = 0; // reset counter so it doesn't happen again				
				#if ENABLE_TEST_OUT_NOTE_COUNTERS > 0
				note_off_delay_count += 1;
				#endif
			}
			else {
				#if ENABLE_TEST_OUT_NOTE_COUNTERS > 0
				note_off_delay_skip_count += 1;
				#endif			
			}
		}
    }
}

// !review: many of these key_ functions can be moved to key.c and key.h
const uint8_t bank_select_key_ids[NUM_BANKS] = {28,63};
void key_pressed(uint8_t this_key) { // hold functions
    for (uint8_t this_bank = 0; this_bank < NUM_BANKS; this_bank++) {
		if (this_key == bank_select_key_ids[this_bank]) {
			uint16_t sys_time_16 = system_time_ms;
			sys_time_16 = sys_time_16 > 0 ? sys_time_16 : 0xFFFF; // Go back to 0xFFFF if time is 0 (0 is a flag for 'disabled') 
			memset(g_bank_select_counter, 0, sizeof(g_bank_select_counter)); // Reset Bank Select counters			
			g_bank_select_counter[this_bank] = sys_time_16; // new press, start the counter!
		}
	}
}

void key_released(uint8_t this_key) {
    for (uint8_t this_bank = 0; this_bank < NUM_BANKS; this_bank++) {
		if (this_key == bank_select_key_ids[this_bank]) {
			g_bank_select_counter[this_bank] = 0; // new release, reset the counter
		}
	}
}

#define MF64_BANK_CC 3 // CC # that will be used to change the bank of this device.

void change_bank(uint8_t this_bank) {
	uint8_t this_key = this_bank ? bank_select_key_ids[1]: bank_select_key_ids[0];
	uint8_t this_animation = G_EE_ANIMATIONS < GEOMETRIC_ANIMATION_TYPES ? G_EE_ANIMATIONS : GEOMETRIC_ANIMATION_TYPE_SQUARE;
    //uint8_t last_bank = g_bank_selected;
	g_bank_selected = this_bank;  // Change Bank
	start_geometric_animation(this_key, this_animation);
	memset(g_bank_select_counter, 0, sizeof(g_bank_select_counter)); // Reset Bank Select counters

}
void send_change_bank_notification(uint8_t this_bank) {
	    // Send bank change MIDI output (Twister protocol with no off messages)
	    // - will send on bank's channel
	    midi_stream_raw_cc(G_EE_MIDI_CHANNEL, MF64_BANK_CC, this_bank); // Turn on the next bank
}

void service_bank_select_buttons(void) {
	if (!G_EE_SIDE_BANK) { // only service if selected
		return;
	}
	uint16_t counter_limit = G_EE_SIDE_BANK > 1 ? 1:G_BANK_SELECT_COUNTER_LIMIT; // 0= off, 1=Hold 2 seconds, 2=Change Immediately on Press
	uint16_t sys_time_16 = system_time_ms;
	for (uint8_t this_bank = 0; this_bank < NUM_BANKS; this_bank++) {
		uint16_t this_time = g_bank_select_counter[this_bank];
		if (this_time > 0 && sys_time_16 - this_time >= counter_limit) { // updated by Timer0 Interrupt every 1 ms
			change_bank(this_bank);
			send_change_bank_notification(this_bank);
			break; // exit for loop
		}
	}
}

bool InterpretUsbMidiMessage(MIDI_EventPacket_t input_event) {
	// Parse the USB-MIDI packet to see what it contains
	#if USE_LUFA_2015 > 0
	#warning USING LUFA USB 2015
	uint8_t command = input_event.Event & 0x0F;
	#else
	uint8_t command = input_event.Command;
	#endif
	if (command == 0) {
		return false; // was not a valid message, return false
	}
	
	
	switch (command) {
		case 0xB : // MIDI CC
		{
			// if channel is in correct range...
			uint8_t channel = input_event.Data1 & 0x0f;
			if (channel == G_EE_MIDI_CHANNEL) { // Bank 1: key_id 0-63
				uint8_t cc = input_event.Data2;
				uint8_t velocity = input_event.Data3;
				if (cc == MF64_BANK_CC) {
					uint8_t bank = velocity > 0 ? 1:0;
					change_bank(bank); // do not send a notification back!
				}
			}

			// !test: led colors and power
			#if ENABLE_TEST_IN_LED_CALIBRATION > 0
			uint8_t cc = input_event.Data2;
			uint8_t cc_value = input_event.Data3;
			if (cc == 0) {
				led_set_test_colors(cc_value, 0xFF, 0xFF); // Red (> 0x7F means no change)
			}
			else if (cc == 1) {
				// Green
				led_set_test_colors(0xFF, cc_value, 0xFF);
			}
			else if (cc == 2) {
				// Blue
				led_set_test_colors(0xFF, 0xFF, cc_value);
			}
			#endif
			
		}
	    case 0xF :
		{
        // This is a single byte Real Time message.
			switch (input_event.Data1) {
			case 0xF8 :
			// Midi Clock Event
				midi_clock_enable(true);
				midi_clock();
				break;
			case 0xFA :
			// Midi Clock Start Event
				midi_clock_enable(true);
				break;
			case 0xFC :
			// Midi Clock Stop Event
				midi_clock_enable(false);
				break;
			}
		}
		break;
        case 0x9 :
        {
			// A NoteOn event was found, if the Channel is within the
			// correct range update the stored velocity
			uint8_t channel = input_event.Data1 & 0x0f;
			if (channel == G_EE_MIDI_CHANNEL) { // Bank 1: key_id 0-63
				uint8_t note = input_event.Data2;
				uint8_t velocity = input_event.Data3;
				uint8_t channel_index =MIDI_CHANNEL_INDEX_CONTROL_BANKS;
				uint8_t key_id = note - MIDI_BASENOTE;
				if (key_id < NUM_BUTTONS) {
					g_midi_note_state[channel_index][key_id] = velocity;
					#if ENABLE_NOTE_OFF_FEEDBACK_DELAY > 0
						g_midi_note_off_counter[key_id] = 0;
					#endif			
					#if ENABLE_TEST_OUT_NOTE_COUNTERS > 0
						note_on_count += 1;
					#endif			
				}

			}
			else if (channel == ((G_EE_MIDI_CHANNEL-1) & 0x0F) ) { // Bank 2: key_id 64-127
				uint8_t note = input_event.Data2;
				uint8_t velocity = input_event.Data3;
				uint8_t channel_index = MIDI_CHANNEL_INDEX_CONTROL_BANKS;
				uint8_t key_id = note - MIDI_BASENOTE;
				if (key_id < NUM_BUTTONS) {
					key_id += NUM_BUTTONS;
					g_midi_note_state[channel_index][key_id] = velocity;
					#if ENABLE_NOTE_OFF_FEEDBACK_DELAY > 0
						g_midi_note_off_counter[key_id] = 0; // !review: key_id = 64-127
					#endif					
				}
			}
			else if (channel == ((G_EE_MIDI_CHANNEL+1) & 0x0F) ) { // Animation Bank
				uint8_t note = input_event.Data2;
				uint8_t velocity = input_event.Data3;
				uint8_t channel_index = MIDI_CHANNEL_INDEX_ANIMATIONS;
				g_midi_note_state[channel_index][note] = velocity;
			}
        }
        break;
        case 0x8 :
        {
	        // A NoteOff event, so record a zero in the MIDI
	        // keystate. Yes, a noteoff can have a "velocity",
	        // but we're relying on the keystate to be zero when
	        // we have a noteoff, otherwise the LEDs won't match
	        // the state when we come to calculate them.
	        uint8_t channel = input_event.Data1 & 0x0f;
			if (channel == G_EE_MIDI_CHANNEL) { // Bank 1: key_id 0-63
				uint8_t note = input_event.Data2;
				//uint8_t velocity = input_event.Data3;
				uint8_t key_id = note - MIDI_BASENOTE;
				if (key_id < NUM_BUTTONS) {
					#if ENABLE_NOTE_OFF_FEEDBACK_DELAY <= 0
					  uint8_t channel_index = MIDI_CHANNEL_INDEX_CONTROL_BANKS;
					  g_midi_note_state[channel_index][key_id] = 0;
					#else // NOTE OFF Feedback delay enabled
					  uint8_t time_8bit = (system_time_ms & 0x7F) | 0x80;
					  g_midi_note_off_counter[key_id] = time_8bit; // set flag, will auto-update g_midi_note_state later
					#endif
					#if ENABLE_TEST_OUT_NOTE_COUNTERS > 0
						note_off_count += 1;
					#endif
				}
			}
			else if (channel == ((G_EE_MIDI_CHANNEL-1) & 0x0F) ) { // Bank 2: key_id 64-127
				uint8_t note = input_event.Data2;
				//uint8_t velocity = input_event.Data3;
				uint8_t key_id = note - MIDI_BASENOTE;
				if (key_id < NUM_BUTTONS) {
					key_id += NUM_BUTTONS; // increment key_id for bank 2
					#if ENABLE_NOTE_OFF_FEEDBACK_DELAY <= 0
					  uint8_t channel_index = MIDI_CHANNEL_INDEX_CONTROL_BANKS;
					  g_midi_note_state[channel_index][key_id] = 0;
					#else // NOTE OFF Feedback delay enabled
					  uint8_t time_8bit = (system_time_ms & 0x7F) | 0x80;
					  g_midi_note_off_counter[key_id] = time_8bit; // set flag, will auto-update g_midi_note_state later
					#endif					
				}
			}
			else if (channel == ((G_EE_MIDI_CHANNEL+1) & 0x0F) ) { // Animation Bank
				uint8_t note = input_event.Data2;
				//uint8_t velocity = input_event.Data3;
				uint8_t channel_index = MIDI_CHANNEL_INDEX_ANIMATIONS;
				g_midi_note_state[channel_index][note] = 0;  // animation note off's occur immediately and use note and not key_id (both banks on single channel)
			}			
			
        }
        break;
        case 0x4 :
            {
                // 3 byte sysex start or continue
                sysex_handle_3sc(&input_event);                
            }
            break;
        case 0x5 :
            {
                // One byte sysex end or system common
                sysex_handle_1e(&input_event);
            }
            break;
        case 0x6 :
            {
                // 2 byte sysex end
                sysex_handle_2e(&input_event);
            }
            break;
        case 0x7 :
            {
                // 3 byte sysex end
                sysex_handle_3e(&input_event);
            }
            break;
		case 0xA:
		case 0xC:
		case 0xE:
			break;
        default:
            // unrecognized
			//return false; // returning false interprets unhandled messages as an error
			break;
        } // end USB-MIDI packet parse
		return true; // is a valid midi packet, signal the sender!
}

#if ENABLE_LUFA_2015_LARGE_PACKET_UPGRADE > 0


void Midifighter_GetIncomingUsbMidiMessages(void) {
	static MIDI_EventPackets_t input_events;
	uint16_t usb_rx_fail_count = 0;
	uint16_t usb_rx_packets = 0;
	
	while (1) {
		if (usb_rx_packets >= USB_RX_PACKET_LIMIT) {
			break;
		}
		// Receive another packet
		memset(&input_events, 0, sizeof(input_events));
		bool status = MIDI_Device_ReceiveLargeEventPacket(g_midi_interface_info, &input_events, 32);
		if (!status) { // No packet was received
			// return;		
			usb_rx_fail_count += 1;
			if (usb_rx_fail_count >= USB_RX_FAIL_LIMIT) {
				break;
			} 
			else {  // 200us on Mac, up to 400us on windows
				wdt_reset();
				//_delay_us(50.0); // Consecutive USB Packets can be just 10us to 200us apart in practice
				continue;
			}
		}
		else { // Packet was received!
			usb_rx_packets += 1;
			usb_rx_fail_count = 0;
	
			// - we add this delay so that we may get a full frame of leds (often 4 or 5 usb packets) at once.		
	
			// Received at least one packet
			for (uint8_t this_event = 0; this_event < 16; this_event++) {
				if (!InterpretUsbMidiMessage(input_events.events[this_event])) { // read the 'next' message
					break;  // if the message did not match USB-MIDI Protocol, then the packet was complete, exit loop!
				}
			}
		}
	}
}
#else // DISABLE_LUFA_2015_LARGE_PACKET_UPGRADE
void Midifighter_GetIncomingUsbMidiMessages(void) {
    // If there is data in the Endpoint for us to read, get a USB-MIDI
    // packet to process. Endpoint_IsReadWriteAllowed() returns true if
    // there is data remaining inside an OUT endpoint or if an IN endpoint
    // has space left to fill. The same function doing two jobs, confusing
    // but there you are.

    MIDI_EventPacket_t input_event;
	uint16_t usb_rx_fail_count = 0;
	uint16_t usb_rx_packets = 0;
		
	while (1) {
		//break; // !test: no LED Feedback reading
		//#if USB_RX_METHOD < USB_RX_PERIODICALLY
		// Wait here and actively detect incoming USB Messages continuously for up to 1ms
		if (usb_rx_packets >= USB_RX_PACKET_LIMIT) {
			break;
		}
	    else if (!MIDI_Device_ReceiveEventPacket(g_midi_interface_info,
	    &input_event)) {  // 
			usb_rx_fail_count += 1;
			if (usb_rx_fail_count >= USB_RX_FAIL_LIMIT) {
				break;
			} else {  // 200us on Mac, up to 400us on windows
				wdt_reset();
				continue;
			}
			// - we add this delay so that we may get a full frame of leds (often 4 or 5 usb packets) at once.
		} else {
			//Endpoint_ClearOUT(); // !Windows Test: Clear Endpoing Manually (no effect)
			usb_rx_packets += 1;
			usb_rx_fail_count = 0;
			
			#if ENABLE_TEST_OUT_USB_PACKETS_PER_INTERVAL > 0
			if (usb_rx_packets > usb_packets_per_interval_max) {
				usb_packets_per_interval_max = usb_rx_packets;
			}
			#endif
		}

        // Assuming all virtual MIDI cables are intended for us, ensure that
        // this event is being sent on our current MIDI channel.
        //
        // The lower 4-bits (".Command") of the USB_MIDI event packet tells
        // us what kind of data it contains, and whether to expect more data
        // in the same message. Commands are:
        //
        //     0x0 = Reserved for Misc
        //     0x1 = Reserved for Cable events
        //     0x2 = 2-byte System Common
        //     0x3 = 3-byte System Common
        //     0x4 = 3-byte Sysex starts or continues
        //     0x5 = 1-byte System Common or Sysex ends
        //     0x6 = 2-byte Sysex ends
        //     0x7 = 3-byte Sysex ends
        //     0x8 = Note On
        //     0x9 = Note Off
        //     0xA = Poly KeyPress
        //     0xB = Control Change (CC)
        //     0xC = Program Change
        //     0xD = Channel Pressure
        //     0xE = PitchBend Change
        //     0xF = 1-byte message
        //

        // Parse the USB-MIDI packet to see what it contains
		#if USE_LUFA_2015 > 0
		 #warning USING LUFA USB 2015
		 uint8_t command = input_event.Event & 0x0F;
		 switch (command) {
		#else 
         switch (input_event.Command) {
		#endif
			case 0xF :
			{
				// This is a single byte Real Time message.
				switch (input_event.Data1) {
					case 0xF8 :
					// Midi Clock Event
					midi_clock_enable(true);
					midi_clock();
					break;
					case 0xFA :
					// Midi Clock Start Event
					midi_clock_enable(true);
					break;
					case 0xFC :
					// Midi Clock Stop Event
					midi_clock_enable(false);
					break;
				}
			}
			break;

			 // MIDI Feedback and MIDI Sysex Rx
			case 0x9 :
			{
				 // A NoteOn event was found, if the Channel is within the
				// correct range update the stored velocity
				uint8_t channel = input_event.Data1 & 0x0f;
				if (channel == G_EE_MIDI_CHANNEL) { // Bank 1: key_id 0-63
					uint8_t note = input_event.Data2;
					uint8_t velocity = input_event.Data3;
					uint8_t channel_index = MIDI_CHANNEL_INDEX_CONTROL_BANKS;
					uint8_t key_id = note - MIDI_BASENOTE;
					if (key_id < NUM_BUTTONS) {
						g_midi_note_state[channel_index][key_id] = velocity;
						#if ENABLE_NOTE_OFF_FEEDBACK_DELAY > 0
						  g_midi_note_off_counter[key_id] = 0;
						#endif						
						#if ENABLE_TEST_OUT_NOTE_COUNTERS > 0
						note_on_count += 1;
						#endif
					}
				
				}
				else if (channel == ((G_EE_MIDI_CHANNEL-1) & 0x0F) ) { // Bank 2: key_id 64-127
					uint8_t note = input_event.Data2;
					uint8_t velocity = input_event.Data3;
					uint8_t channel_index = MIDI_CHANNEL_INDEX_CONTROL_BANKS;
					uint8_t key_id = note - MIDI_BASENOTE;
					if (key_id < NUM_BUTTONS) {
						key_id += NUM_BUTTONS;
						g_midi_note_state[channel_index][key_id] = velocity;
						#if ENABLE_NOTE_OFF_FEEDBACK_DELAY > 0
						  g_midi_note_off_counter[key_id] = 0;
						#endif					
					}
				}
				else if (channel == ((G_EE_MIDI_CHANNEL+1) & 0x0F) ) { // Animation Bank
					uint8_t note = input_event.Data2;
					uint8_t velocity = input_event.Data3;
					uint8_t channel_index = MIDI_CHANNEL_INDEX_ANIMATIONS;
					g_midi_note_state[channel_index][note] = velocity;
				}
			}
			break;
			case 0x8 :
			{
				// A NoteOff event, so record a zero in the MIDI
				// keystate. Yes, a noteoff can have a "velocity",
				// but we're relying on the keystate to be zero when
				// we have a noteoff, otherwise the LEDs won't match
				// the state when we come to calculate them.
				uint8_t channel = input_event.Data1 & 0x0f;
				if (channel == G_EE_MIDI_CHANNEL) { // Bank 1: key_id 0-63
					uint8_t note = input_event.Data2;
					//uint8_t velocity = input_event.Data3;
					uint8_t key_id = note - MIDI_BASENOTE;
					if (key_id < NUM_BUTTONS) {
						#if ENABLE_NOTE_OFF_FEEDBACK_DELAY <= 0
						  uint8_t channel_index = MIDI_CHANNEL_INDEX_CONTROL_BANKS;
						  g_midi_note_state[channel_index][key_id] = 0;
						#else // NOTE OFF Feedback delay enabled
						  uint8_t time_8bit = (system_time_ms & 0x7F) | 0x80;
						  g_midi_note_off_counter[key_id] = time_8bit; // set flag, will auto-update g_midi_note_state later
						#endif
						
						#if ENABLE_TEST_OUT_NOTE_COUNTERS > 0
						  note_off_count += 1;
						#endif
					}
				}
				else if (channel == ((G_EE_MIDI_CHANNEL-1) & 0x0F) ) { // Bank 2: key_id 64-127
					uint8_t note = input_event.Data2;
					//uint8_t velocity = input_event.Data3;
					uint8_t key_id = note - MIDI_BASENOTE;
					if (key_id < NUM_BUTTONS) {
						key_id += NUM_BUTTONS; // increment key_id for bank 2
						#if ENABLE_NOTE_OFF_FEEDBACK_DELAY <= 0
						  uint8_t channel_index = MIDI_CHANNEL_INDEX_CONTROL_BANKS;
						  g_midi_note_state[channel_index][key_id] = 0;
						#else // NOTE OFF Feedback delay enabled
						  uint8_t time_8bit = (system_time_ms & 0x7F) | 0x80;
						  g_midi_note_off_counter[key_id] = time_8bit; // set flag, will auto-update g_midi_note_state later
						#endif					
					}
				}
				else if (channel == ((G_EE_MIDI_CHANNEL+1) & 0x0F) ) { // Animation Bank
					uint8_t note = input_event.Data2;
					//uint8_t velocity = input_event.Data3;
					uint8_t channel_index = MIDI_CHANNEL_INDEX_ANIMATIONS;
					g_midi_note_state[channel_index][note] = 0;  // animation note off's occur immediately and use note and not key_id (both banks on single channel)
				}			
			}
			break;


			case 0xB : // MIDI CC
			{
				// if channel is in correct range...
				uint8_t channel = input_event.Data1 & 0x0f;
				if (channel == G_EE_MIDI_CHANNEL) { // Bank 1: key_id 0-63
					uint8_t cc = input_event.Data2;
					uint8_t velocity = input_event.Data3;                
					if (cc == MF64_BANK_CC) {
						uint8_t bank = velocity > 0 ? 1:0;
						change_bank(bank); // do not send a notification back!
					}
				}

				// !test: led colors and power
				#if ENABLE_TEST_IN_LED_CALIBRATION > 0
				uint8_t cc = input_event.Data2;
				uint8_t cc_value = input_event.Data3;
				if (cc == 0) {
					led_set_test_colors(cc_value, 0xFF, 0xFF); // Red (> 0x7F means no change)
				}
				else if (cc == 1) {
					// Green
					led_set_test_colors(0xFF, cc_value, 0xFF);
				}
				else if (cc == 2) {
					// Blue
					led_set_test_colors(0xFF, 0xFF, cc_value);
				}
				#endif
			}
			break;
			case 0x4 :
				{
					// 3 byte sysex start or continue
					sysex_handle_3sc(&input_event);                
				}
				break;
			case 0x5 :
				{
					// One byte sysex end or system common
					sysex_handle_1e(&input_event);
				}
				break;
			case 0x6 :
				{
					// 2 byte sysex end
					sysex_handle_2e(&input_event);
				}
				break;
			case 0x7 :
				{
					// 3 byte sysex end
					sysex_handle_3e(&input_event);
				}
				break;
			default:
				// do nothing.
				//g_midi_note_state[0][36] = 100; // !test: unhandled messages
				break;
			} // end USB-MIDI packet parse
    } // end while
}
#endif // else # DISABLE_LUFA_2015_LARGE_PACKET_UPGRADE

void Midifighter_Task(void)
{
	#if ENABLE_TEST_OUT_MAINLOOP_COUNT > 0
	#warning TEST: MAINLOOP Counter Output is ENABLED!
	static uint16_t loop_count = 0;
	static uint16_t last_sent_loop_count = 0;
	loop_count += 1;
	if (loop_count-last_sent_loop_count >= 100) {
		last_sent_loop_count = loop_count;
		midi_stream_raw_cc(9, (loop_count >> 7) & 0x7F, loop_count & 0x7F);
		//midi_stream_raw_cc(11, system_time_ms >> 7 & 0x7F, system_time_ms & 0x7F); // System Time Check
	}
	#endif
	
    // If the Midifighter is not completely enumerated by the USB Host,
    // don't go any further - no updating of LEDs, no reading from
    // endpoints, we wait for the USB to connect.
	#if ENABLE_RGB_TEST <= 0
	if (USB_DeviceState != DEVICE_STATE_Configured) { // don't go any further if we don't have a USB Connection
		// !review: add LED Feedback for this state?
        return;
    }
	#endif

    // The state of all the active notes is kept in an array of bytes with
    // the value being the velocity of the note. A nonzero velocity is a
    // NoteOn and a zero velocity is a NoteOff. We update the keystate from
    // the outside world first, from the keyboard second, 
    // and generate the LED display from the resulting table at the end.

    // INPUT MIDI from USB -----------------------------------------------------
	#if USB_RX_METHOD < USB_RX_PERIODICALLY
	Midifighter_GetIncomingUsbMidiMessages();
    #endif

    // OUTPUT key presses ------------------------------------------------------
	// - !review: performance improvement - key checking doesn't need to be done every loop (only when there's been another interrupt)
	key_read();  // Read the debounce buffer to generate a keystate.
    key_calc();  // Use the new keystate to update keydown/keyup state.
	// key_send();
    // - Loop over all of the 16 arcade keys and send MIDI messages, converting key numbers
    // - to MIDI notes using the mapping table.
    {
        uint64_t key_bit = 0x0001;
		// update sleep timer (if necessary)
		if (g_key_down) {
			sleep_minute_counter=0;
		}
		// Service Each Individual Button
        for(uint8_t i=0; i<64; ++i) {
			#if USB_RX_METHOD >= USB_RX_PERIODICALLY
			Midifighter_GetIncomingUsbMidiMessages();
			#endif
            if (g_key_down & key_bit) {
                // There's a key down, put a NoteOn and/or CC event into the stream.
                uint8_t note = midi_64_key_to_note(i);

				if (G_EE_MIDI_OUTPUT_MODE < MIDI_OUTPUT_MODE_CCS_ONLY) {
				    midi_stream_note(note, true);
                }

                if (G_EE_MIDI_OUTPUT_MODE > MIDI_OUTPUT_MODE_NOTES_ONLY)
                {
				    midi_stream_raw_cc(G_EE_MIDI_CHANNEL,note,127);
                }
				// record what bank this 'on' message occured in
				if (g_bank_selected <= 0) {
					g_key_bank_last_up &= ~key_bit;
				} else {
					g_key_bank_last_up |= key_bit;
					
				}
				
				// Trigger Animation if Applicable
				if (G_EE_ANIMATIONS < GEOMETRIC_ANIMATION_TYPES) {
					start_geometric_animation(i, G_EE_ANIMATIONS);
				}
				
				key_pressed(i); // button hold functions
            }
            if (g_key_up & key_bit) {
                // There's a key up, put a NoteOff event onto the stream.
                uint8_t note = midi_64_key_to_note(i);
				// Adjust channel based on where the note was triggered (so bank changes don't result in stuck notes
				uint8_t channel = g_key_bank_last_up & key_bit ? (G_EE_MIDI_CHANNEL-1) & 0x0F : G_EE_MIDI_CHANNEL;
				// Output Note Message
				if (G_EE_MIDI_OUTPUT_MODE < MIDI_OUTPUT_MODE_CCS_ONLY) {
				    midi_stream_note_ch(channel, note, false);
			    }
				// Output CC Message
                if (G_EE_MIDI_OUTPUT_MODE > MIDI_OUTPUT_MODE_NOTES_ONLY)
                {
					midi_stream_raw_cc(G_EE_MIDI_CHANNEL,note,0);
				}
				// Service Button 'Hold' functions
				key_released(i); // button hold functions
            }
            key_bit <<= 1;
        }
    }
	// Combos: Detect Button Combination presses
    if (G_EE_COMBOS_ENABLE)
    {
            // Recognize combo key events
            // --------------------------
            combo_action_t action = combo_recognize(); //g_key_down, g_key_up, g_key_state);
            uint8_t channel = g_bank_selected <= 0 ? G_EE_MIDI_CHANNEL : ((G_EE_MIDI_CHANNEL - 1) & 0x0F);
            switch (action) {
            case COMBO_A_DOWN:
                midi_stream_note_ch(channel,8, true);
                break;
            case COMBO_A_RELEASE:
                midi_stream_note_ch(channel,8, false);
                break;
            case COMBO_B_DOWN:
                midi_stream_note_ch(channel,9, true);
                break;
            case COMBO_B_RELEASE:
                midi_stream_note_ch(channel,9, false);
                break;
            case COMBO_C_DOWN:
                midi_stream_note_ch(channel,10, true);
                break;
            case COMBO_C_RELEASE:
                midi_stream_note_ch(channel,10, false);
                break;
            case COMBO_D_DOWN:
                midi_stream_note_ch(channel,11, true);
                break;
            case COMBO_D_RELEASE:
                midi_stream_note_ch(channel,11, false);
                break;
            case COMBO_E_DOWN:
                midi_stream_note_ch(channel,12, true);
                break;
            case COMBO_E_RELEASE:
                midi_stream_note_ch(channel,12, false);
                break;
            default:
                // do nothing.
                break;
            }
    } 

    // Finished generating MIDI events, flush the endpoints. (otherwise it won't send until it's full!)
	MIDI_Device_Flush(g_midi_interface_info); // MIDI_Device_USBTask calls Flush, but has redundant checks involved

	// Finally update the display with current frame
	if (system_time_ms - last_led_refresh_time_ms >= LED_REFRESH_LIMIT) { // is it time to transmit to the LEDs?
		// Store this update time
		last_led_refresh_time_ms = system_time_ms;
    	 
		// Perform Test Operations (if desired
		#if ENABLE_TEST_OUT_LED_REFRESH_COUNT > 0
		#warning TEST: LED REFRESH Counter Output is ENABLED!
		static uint16_t refresh_count = 0;
		static uint16_t last_sent_refresh_count = 0;
		refresh_count += 1;
		if (refresh_count-last_sent_refresh_count >= 100) {
			last_sent_refresh_count = refresh_count;
			midi_stream_raw_cc(10, (refresh_count >> 7) & 0x7F, refresh_count & 0x7F);
		}
        #endif

		#if ENABLE_TEST_OUT_NOTE_COUNTERS > 0
		// !test: note off delay reception
		static uint16_t test_out_note_count = 0;
		static uint16_t last_test_out_note_count = 0;
		test_out_note_count += 1;
		if (test_out_note_count-last_test_out_note_count >= 100) {
			last_test_out_note_count = test_out_note_count;
			midi_stream_raw_cc(11, (note_on_count >> 7) & 0x7F, note_on_count & 0x7F);
			midi_stream_raw_cc(12, (note_off_count >> 7) & 0x7F, note_off_count & 0x7F);
			midi_stream_raw_cc(13, (note_off_delay_count >> 7) & 0x7F, note_off_delay_count & 0x7F);
			midi_stream_raw_cc(14, (note_off_delay_skip_count >> 7) & 0x7F, note_off_delay_skip_count & 0x7F);
		}
		#endif		

		#if ENABLE_TEST_OUT_USB_PACKETS_PER_INTERVAL > 0
		// !test: usb packet reception
		static uint16_t test_out_usb_packet_count = 0;
		static uint16_t last_test_out_usb_packet_count = 0;
		test_out_usb_packet_count += 1;
		if (test_out_usb_packet_count-last_test_out_usb_packet_count >= 100) {
			last_test_out_usb_packet_count = test_out_usb_packet_count;
			midi_stream_raw_cc(8, (usb_packets_per_interval_max >> 7) & 0x7F, usb_packets_per_interval_max & 0x7F);
			usb_packets_per_interval_max = 0; // reset, so we can continually see, what we're sending
		}
		#endif

		// Handle Bank Select Buttons // !review: this doesn't really need to be here...		
		service_bank_select_buttons();// !review: positioned here because it is called less frequently


		// Handle 'Note Off Delay', since it only actually matters here
		#if ENABLE_NOTE_OFF_FEEDBACK_DELAY > 0
		update_note_off_feedback_delay();
		#endif

	    default_display_run(); //g_bank_selected, g_key_state, g_display_buffer);
		
		#if ENABLE_TEST_IN_LED_CALIBRATION > 0
		led_calibration_test(); // This overrides all led data with color values received via cc0-2
		#endif 
		
		// Send Data to the LEDs
		led_update_pixels(g_display_buffer);
	}
	
	watchdog_flag = true;
}


// Main -----------------------------------------------------------------------
// Set up ports and peripherals, start the scheduler and never return.
int main(void)
{
    // Disable watchdog timer to prevent endless resets if we just used it
    // to soft-reset the machine.
	
    MCUSR &= ~(1 << WDRF);  // clear the watchdog reset flag
    wdt_disable();          // turn off the watchdog

    // Disable clock prescaling so we're working at full 16MHz speed.
    // (You'll find this function in winavr/avr/include/avr/power.h)
    clock_prescale_set(clock_div_1);
	// Turn off JTAG - JTAG will work with reset held low
	// Must write same bit twice in four clock cycles for this to work
 	MCUCR = (1 << JTD);
 	MCUCR = (1 << JTD);
	 
	// - !review: device slowdown when no output port is connect
	// USB_PLL_On(); // Works when you disable 'AUTO_PLL' in makefile
	// while (!(USB_PLL_IsReady()));
	// - but device still hangs on MIDI_Device_flush (waiting 100ms as is defined in lufa by a timeout)

	// Start up the subsystems.
    eeprom_setup();   // setup global settings from the EEPROM
    led_setup();      // startup the LED chips.
	led_disable();	  // and disable display until USB is connected
    key_setup();      // startup the key debounce interrupt.
    midi_setup();     // startup the MIDI keystate and LUFA MIDI Class interface.
    //bank_setup();     // startup the bank select buttons.
	config_setup();   // setup the configuration system
#ifdef COMBO
	combo_setup();    // init the combo key recognizer.
#endif
 	// Slight delay befor we read the buttons to check for any special start up
	// configuration
 	_delay_ms(20);
	key_read();
	key_calc();

    // Check to see if the bootloader has been requested by the user holding
    // down the four corner keys at power on time. We have to do this before
    // the USB scheduler starts because shutting down these subsystems
    // before entering the bootloader is a little involved.
    //if ((g_key_state == 0xC09009)) {
    if ((g_key_state & 0x01)) {
        // Drop to Bootloader:
        //  # . . #
        //  . . . .
        //  . . . .
        //  # . . #

        //led_set_state(0xA5A5,0x00ffffff);
		led_set_state_dfu();  // !mark: firmware update soft brick?
		Jump_To_Bootloader();

        // We should never reach here. If you see this LED pattern: then
        // something went horribly wrong and we don't have a bootloader in
        // the machine.
        //  o . . .
        //  . o . .
        //  . . o .
        //  . . . o

        //led_set_state(0x8421,0x00ffffff);
        while(1);
    }  
	
    // Start up USB system now that everything else is safely squared away
    // and our globals are setup.
    USB_Init();
    // enable global interrupts.
    sei();
    // Start Device With Sleep Animation Enabled
	sleep_minute_counter = G_EE_SLEEP_TIME;
	
    // Enter an endless loop. (the main loop)
    for(;;) {
        // Read keys and motion tracking for User and MIDI events to process,
        // setting LEDs to display the resulting state.
		Midifighter_Task();
				
        // Let the LUFA MIDI Device drivers have a go.
		// MIDI_Device_USBTask(g_midi_interface_info);
        // - upon tracing this actually just calls midi device flush, 
		// - which is now called within Midifighter_Task, so this call was removed

        // Update the USB state.
        USB_USBTask();
        
		// Reset the watch dog timer, dawg
		if (watchdog_flag)
		{
			wdt_reset();
			//!review: why ever not reset watchdog? watchdog_flag = false;
		}		

    }
}
// -----------------------------------------------------------------------------
