// MIDI send, receive and decode functions for DJTechTools MidiFighter 3D
//
//   Copyright (C) 2011 DJ Techtools
//
// Robin   Green    2009-10-17
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

#include <string.h>  // for memset()
#include <avr/io.h>
#include <stdlib.h>
#include "constants.h"
#include "key.h"

#include "usb_descriptors.h"
#include "display.h"
#include "midi.h"
#include "led.h"
#include "eeprom.h"


// Global variables ------------------------------------------------------------
bool midi_clock_enabled = false;
uint8_t ticks = 0;
uint8_t g_bank_selected =0;
uint8_t g_side_offset = 0;


// Interface object for the high level LUFA MIDI Class Drivers. This gets
// passed into every MIDI call so it can potentially keep track of many
// interfaces. The Midifighter only needs the one.
#if USE_LUFA_2015 > 0
#define MIDI_STREAM_IN_EPADDR (ENDPOINT_DIR_IN | MIDI_STREAM_IN_EPNUM)
#define MIDI_STREAM_OUT_EPADDR (ENDPOINT_DIR_OUT | MIDI_STREAM_OUT_EPNUM)
static USB_ClassInfo_MIDI_Device_t s_midi_interface = {
	.Config = {
		.StreamingInterfaceNumber = 1, // 1 = audio stream, 0 = audio control
		.DataINEndpoint = 
		{
			.Address          = MIDI_STREAM_IN_EPADDR,
			.Size             = 64, // 64
			.Banks            = 1, // !review: 0,1,2,+ none has any effect on throughput, why?
		},
		.DataOUTEndpoint = 
		{
			.Address          = MIDI_STREAM_OUT_EPADDR,
			.Size             = 64, // 64
			.Banks            = 1,	// !review: 0,1,2,+ none has any effect on throughput, why? Can we set to 3ms, and then read a bunch of messages continuously?
		},
	},
};


#else
static USB_ClassInfo_MIDI_Device_t s_midi_interface = {
    .Config = {
        .StreamingInterfaceNumber = 1,
        .DataINEndpointNumber      = MIDI_STREAM_IN_EPNUM,
        .DataINEndpointSize        = MIDI_STREAM_EPSIZE,
        .DataINEndpointDoubleBank  = false,
        .DataOUTEndpointNumber     = MIDI_STREAM_OUT_EPNUM,
        .DataOUTEndpointSize       = MIDI_STREAM_EPSIZE,
        .DataOUTEndpointDoubleBank = false,
    },
};
#endif 

USB_ClassInfo_MIDI_Device_t* g_midi_interface_info;

// This table maps key numbers to midi note offsets, to match
// up the notes with Ableton Live drum racks, e.g. key 5 will initially
// send NoteOn G#3 = 44, with the rest of the keypad sending:
//
//     C4  C#4 D4  D#4
//     G#3 A3  A#3 B3
//     E3  F3  F#3 G3
//     C3  C#3 D3  D#3
//
// The table is coded as note offsets so we can re-base the pad at another
// MIDI note, with the default offset being C3 = 36. The "PROGMEM" setting
// forces the table into program memory so it won't take up precious RAM.
//
const uint8_t kNoteMap[16] PROGMEM = {
    12, 13, 14, 15,
     8,  9, 10, 11,
     4,  5,  6,  7,
     0,  1,  2,  3,
};


// NOTE(rgreen): These assignments here are debug MIDI values, you should
// never see these, the actual values will be read and set from the EEPROM
// table during startup.
//
uint8_t G_EE_MIDI_CHANNEL = 14;      // MIDI channel to listen and send on (0..15)
uint8_t G_EE_MIDI_VELOCITY = 74;     // Default velocity for NoteOn (0..127)
uint8_t g_midi_note_state[2][MIDI_MAX_NOTES]; 
// - velocity of MIDI notes we track: For [0] and [1]: 0-63 are bank1 (G_EE_MIDI_CHANNEL) and 64-127 are bank 2 (G_EE_MIDI_CHANNEL-1)
uint8_t g_midi_note_off_counter[MIDI_MAX_NOTES]; //  0-63 are bank 1, 64-127 are bank 2
// - note in minimal interrupts mode, this stores system time in 8-bit, in maximum interrupt modes, each item is a ms counter

uint16_t g_bank_select_counter[NUM_BANKS]; // Counts a 3 second delay
uint8_t g_midi_sysex_channel = 5;   // fixed channel for now *** FIX THIS ***
bool g_midi_sysex_is_reading = false;
bool g_midi_sysex_is_valid = false;

// MIDI functions -------------------------------------------------------------

// Initialize the MIDI key state.
void midi_setup(void)
{
    // Set up the global LUFA MIDI class interface pointer.
    g_midi_interface_info = &s_midi_interface;

    // basenote, expnote, channel and velocity have already been set up via
    // the EEPROM settings. Clear the MIDI keystate.
    memset(g_midi_note_state, 0, sizeof(g_midi_note_state)); // review: why do we have two*MIDI_MAX_NOTES, but only save one. Is one unused?
    memset(g_midi_note_off_counter, 0, sizeof(g_midi_note_off_counter)); // review: why do we have two*MIDI_MAX_NOTES, but only save one. Is one unused?
    memset(g_bank_select_counter, 0, sizeof(g_bank_select_counter)); // review: why do we have two*MIDI_MAX_NOTES, but only save one. Is one unused?
}

void midi_stream_raw_note(const uint8_t channel,
                          const uint8_t pitch,
                          const bool onoff,
                          const uint8_t velocity)
{
    uint8_t command = ((onoff)? 0x90 : 0x80);
	uint8_t midi_channel = G_EE_MIDI_CHANNEL - g_bank_selected; // !bank64

	
    MIDI_EventPacket_t midi_event;
	
	#if USE_LUFA_2015 > 0
      midi_event.Event = command >> 4;  // USB-MIDI virtual cable (0..15)
	#else
      midi_event.CableNumber = 0x0;  // USB-MIDI virtual cable (0..15)
      midi_event.Command     = command >> 4;   // 0..15	
	#endif
    midi_event.Data1       = command | (midi_channel & 0x0f);  // 0..15
    midi_event.Data2       = pitch & 0x7f;   // 0..127
    midi_event.Data3       = velocity & 0x7f; // 0..127
    MIDI_Device_SendEventPacket(g_midi_interface_info, &midi_event);
}


// Used to send a note on a specific channel
void midi_stream_note_ch(const uint8_t channel,
						 const uint8_t pitch,           
                         const bool onoff)

{
  // Check if the message should be a NoteOn or NoteOff event.
    uint8_t command = ((onoff)? 0x90 : 0x80);

    // Assemble a USB-MIDI event packet, remembering to mask off the values
    // to the correct bit fields.
    MIDI_EventPacket_t midi_event;
	#if USE_LUFA_2015 > 0
	  midi_event.Event = command >> 4;  // USB-MIDI virtual cable (0..15)
	#else
	  midi_event.CableNumber = 0x0;  // USB-MIDI virtual cable (0..15)
	  midi_event.Command     = command >> 4;   // 0..15
	#endif
    midi_event.Data1       = command | (channel & 0x0f);  // 0..15
    midi_event.Data2       = pitch & 0x7f;   // 0..127
    midi_event.Data3       = G_EE_MIDI_VELOCITY & 0x7f; // 0..127

    MIDI_Device_SendEventPacket(g_midi_interface_info, &midi_event);
}

void midi_stream_raw_cc(const uint8_t channel,
                        const uint8_t cc,
                        const uint8_t value)
{
    const uint8_t command = 0xb0;  // the Channel Change command.
    MIDI_EventPacket_t midi_event;
	#if USE_LUFA_2015 > 0
	  midi_event.Event = command >> 4;  // USB-MIDI virtual cable (0..15)
	#else
	  midi_event.CableNumber = 0x0;  // USB-MIDI virtual cable (0..15)
	  midi_event.Command     = command >> 4;   // 0..15
	#endif
    midi_event.Data1       = command | (channel & 0x0f); // 0..15
    midi_event.Data2       = cc & 0x7f;   // 0..127
    midi_event.Data3       = value & 0x7f;  // 0..127
    MIDI_Device_SendEventPacket(g_midi_interface_info, &midi_event);
}


// Append a MIDI note change event (note on or off) to the currently
// selected USB endpoint. If the endpoint is full it will be flushed.
//
//  pitch    Pitch of the note to turn on or off.
//  onoff    True for a NoteOn, false for a NoteOff.
//
// NOTE: The endpoint can contain 64 bytes and each MIDI-USB message is 4
// bytes giving us just enough space to fit in, for example, 16 keydown
// messages.
//
void midi_stream_note(const uint8_t pitch, const bool onoff)
{
    // Check if the message should be a NoteOn or NoteOff event.
    uint8_t command = ((onoff)? 0x90 : 0x80);
	uint8_t midi_channel = G_EE_MIDI_CHANNEL - g_bank_selected; // !bank64

    // Assemble a USB-MIDI event packet, remembering to mask off the values
    // to the correct bit fields.
    MIDI_EventPacket_t midi_event;
	#if USE_LUFA_2015 > 0
	  midi_event.Event = command >> 4;  // USB-MIDI virtual cable (0..15)
	#else
	  midi_event.CableNumber = 0x0;  // USB-MIDI virtual cable (0..15)
	  midi_event.Command     = command >> 4;   // 0..15
	#endif
    midi_event.Data1       = command | (midi_channel & 0x0f);  // 0..15
    midi_event.Data2       = pitch & 0x7f;   // 0..127
    midi_event.Data3       = G_EE_MIDI_VELOCITY & 0x7f; // 0..127

    MIDI_Device_SendEventPacket(g_midi_interface_info, &midi_event);
}

// Append a Control Change Event to the currently selected USB Endpoint. If
// the endpoint is full it will be flushed.
//
//  controller   Number of the controller to alter.
//  value        Value to send to the CC.
//
void midi_stream_cc(const uint8_t controller, const uint8_t value)
{
    //  Assign this MIDI event to cable 0.
    const uint8_t command = 0xb0;  // the Channel Change command.
	uint8_t midi_channel = G_EE_MIDI_CHANNEL - g_bank_selected; // !bank64
    MIDI_EventPacket_t midi_event;
	#if USE_LUFA_2015 > 0
	  midi_event.Event = command >> 4;  // USB-MIDI virtual cable (0..15)
	#else
	  midi_event.CableNumber = 0x0;  // USB-MIDI virtual cable (0..15)
	  midi_event.Command     = command >> 4;   // 0..15
	#endif
    midi_event.Data1       = command | (midi_channel & 0x0f); // 0..15
    midi_event.Data2       = controller & 0x7f;   // 0..127
    midi_event.Data3       = value & 0x7f;  // 0..127

    MIDI_Device_SendEventPacket(g_midi_interface_info, &midi_event);
}

// Append a SysEx Event to the currently selected USB Endpoint. If
// the endpoint is full it will be flushed.
//
//  length       Number of bytes in the message.
//  data         SysEx message data buffer.
//
void midi_stream_sysex (const uint8_t length, uint8_t* data)
{
    //  Assign this MIDI event to cable 0.
    MIDI_EventPacket_t midi_event;
    
    //     0x2 = 2-byte System Common
    //     0x3 = 3-byte System Common
    //     0x4 = 3-byte Sysex starts or continues
    //     0x5 = 1-byte System Common or Sysex ends
    //     0x6 = 2-byte Sysex ends
    //     0x7 = 3-byte Sysex ends

    #if USE_LUFA_2015 <= 0
    const uint8_t midi_virtual_cable = 0;
    midi_event.CableNumber = midi_virtual_cable << 4;
    #endif
	
    uint8_t num = length;// + 1;
    bool first = true;
    while (num > 3) {
		#if USE_LUFA_2015 > 0
          midi_event.Event     = 0x4;
        #else
          midi_event.Command     = 0x4;
        #endif
		
        if (first) {
            first = false;
            midi_event.Data1       = *data++;
        } else {
            midi_event.Data1       = *data++;
        }
        midi_event.Data2       = *data++;
        midi_event.Data3       = *data++;
        MIDI_Device_SendEventPacket(g_midi_interface_info, &midi_event);
        num -= 3;
    }
    if (num) {
 		#if USE_LUFA_2015 > 0
 		  midi_event.Event     = 0x5;
 		#else
 		  midi_event.Command     = 0x5;
 		#endif
        //midi_event.Command      = 0x5;
        midi_event.Data1        = *data++;
        midi_event.Data2        = 0;
        midi_event.Data3        = 0;
        if (num == 2) {
			#if USE_LUFA_2015 > 0
			midi_event.Event     = 0x6;
			#else
			midi_event.Command     = 0x6;
			#endif
            //midi_event.Command  = 0x6;
            midi_event.Data2    = *data++;
        } else if (num == 3) {
            if (first) {
				#if USE_LUFA_2015 > 0
				midi_event.Event     = 0x3;
				#else
				midi_event.Command     = 0x3;
				#endif
                //midi_event.Command     = 0x3;
            } else {
				#if USE_LUFA_2015 > 0
				midi_event.Event     = 0x7;
				#else
				midi_event.Command     = 0x7;
				#endif
                //midi_event.Command  = 0x7;
            }
            midi_event.Data2    = *data++;
            midi_event.Data3    = *data++;
        }
        MIDI_Device_SendEventPacket(g_midi_interface_info, &midi_event);
    }
}

// Convert a MIDI note number on the keypad to an LED number
//
uint8_t midipos_to_keypos(const uint8_t note)  // Reorder Button IDs and MIDI Output Order if desired
{
	// Received MIDI Input ID
	// Return Button ID (aka. Key ID)
	return note; // mf64 hardware does not need reordering (Hardware lines up with mid
}

// Convert a key number to a note number in fourbanks mode.
//
uint8_t midi_fourbanks_key_to_note(const uint8_t keynum)
{
    uint8_t offset = MIDI_BASENOTE + g_bank_selected * 16;
    return pgm_read_byte(&kNoteMap[keynum]) + offset;
}

uint8_t midi_64_key_to_note(const uint8_t keynum) 
{
	uint8_t note = MIDI_BASENOTE + keynum;
	return note;
}

// Convert a fourbanks note to a key number, taking into account whether we
// have internal or external fourbanks mode.
//
// NOTE(rgreen): this function has no safeguard as to whether the note being
// requested is actually visible on the selected bank. Use with care.
//
uint8_t midi_fourbanks_note_to_key(const uint8_t note)
{
    uint8_t keynum = note - MIDI_BASENOTE - (g_bank_selected * 16);
    return pgm_read_byte(&kNoteMap[keynum]);
}

// In response to a Non Realtime request to identify this device,
// stream out the MIDI SYSEX Identify response.
//
//    0xF0  Start of SysEx
//    0x7E  Non-Realtime
//    0xnn  The SysEx channel, from 0x00 to 0x7F. 0x7F = "disregard channel".
//    0x06  Sub-ID -- General Information
//    0x02  Sub-ID2 -- Identity Reply
//    0x00  Manufacturer's ID
//    0xnn  Manufacturer's ID
//    0xnn  Manufacturer's ID
//    0xf1  The f1 and f2 bytes make up the family code, LSB first
//    0xf2
//    0xp1  The p1 and p2 bytes make up the model number, LSB first
//    0xp2
//    0xv1  The v1, v2, v3 and v4 bytes make up the version number.
//    0xv2
//    0xv3
//    0xv4
//    0xF7  End of SysEx
//
// This structure takes up 17 bytes which must be packed into 3-byte
// USB-MIDI packets with the correct comand codes to transmit correctly. See
// the USB MIDI Specification 1.0, table 4-1: "Code Index Number
// Classifications", p16.
//
void midi_stream_sysex_identify()  // DEPRECATED (not used), has been replaced by "sysex_handle()"
{
    MIDI_EventPacket_t midi_event;
	#if USE_LUFA_2015 > 0
		midi_event.Event     = 0x4;
	#else
		midi_event.CableNumber = 0x0;  // USB-MIDI virtual cable (0..15)
		midi_event.Command     = 0x4;  // SYSEX start or continue (3 bytes)
	#endif
    midi_event.Data1       = 0xF0; // Start of Sysex
    midi_event.Data2       = 0x7E; // Non-Realtime
    midi_event.Data3       = 0x05; // ID of this Device (constant for now)
    MIDI_Device_SendEventPacket(g_midi_interface_info, &midi_event);
	#if USE_LUFA_2015 > 0
	  midi_event.Event     = 0x4;
	#else
	  midi_event.Command     = 0x4;  // SYSEX start or continue (3 bytes)
	#endif
    midi_event.Data1       = 0x06; // MIDI - General Information
    midi_event.Data2       = 0x7E; // MIDI - Identity Reply
    midi_event.Data3       = MIDI_MFR_ID_0; // MIDI = Manufacturer's ID byte 0
    MIDI_Device_SendEventPacket(g_midi_interface_info, &midi_event);
	#if USE_LUFA_2015 > 0
	  midi_event.Event     = 0x4;
	#else
	  midi_event.Command     = 0x4;  // SYSEX start or continue (3 bytes)
	#endif
    midi_event.Data1       = MIDI_MFR_ID_1; // MIDI = Manufacturer's ID byte 1
    midi_event.Data2       = MIDI_MFR_ID_2; // MIDI = Manufacturer's ID byte 2 = DJTechTools
    midi_event.Data3       = DEVICE_FAMILY_LSB;  // Family ID (LSB)
    MIDI_Device_SendEventPacket(g_midi_interface_info, &midi_event);
	#if USE_LUFA_2015 > 0
	  midi_event.Event     = 0x4;
	#else
	  midi_event.Command     = 0x4;  // SYSEX start or continue (3 bytes)
	#endif
    midi_event.Data1       = DEVICE_FAMILY_MSB; // Family ID (MSB) = 0x0003 = Midifighter3D
    midi_event.Data2       = 0x01; // Model ID (LSB)
    midi_event.Data3       = 0x00; // Model ID(MSB) = 0x0001 = basic model
    MIDI_Device_SendEventPacket(g_midi_interface_info, &midi_event);
	#if USE_LUFA_2015 > 0
	  midi_event.Event     = 0x4;
	#else
	  midi_event.Command     = 0x4;  // SYSEX start or continue (3 bytes)
	#endif
    midi_event.Data1       = DEVICE_VERSION_DAY; // Firmware Version (LSB)
    midi_event.Data2       = DEVICE_VERSION_MONTH; // Firmware Version
    midi_event.Data3       = DEVICE_VERSION_YEAR_LSB; // Firmware Version
    MIDI_Device_SendEventPacket(g_midi_interface_info, &midi_event);
	#if USE_LUFA_2015 > 0
	  midi_event.Event     = 0x6;
	#else
	  midi_event.Command     = 0x6;  // SYSEX start or continue (3 bytes)
	#endif
    midi_event.Data1       = DEVICE_VERSION_YEAR_MSB; // Firmware Version (MSB) = 0x20110724
    midi_event.Data2       = 0xf7; // And of sysex
    midi_event.Data3       = 0x00; // PADDING
    MIDI_Device_SendEventPacket(g_midi_interface_info, &midi_event);
}

void midi_clock(void)
{
	// If not enabled enable MIDI Clock
	if(!midi_clock_enabled){display_flash_counter=0;midi_clock_enable(true);}
	if(ticks == 3)
	{
		ticks = 1;
		display_flash_counter += 1;
	}
	else
	{
		ticks += 1;
	}
}


void midi_clock_enable(bool state)
{
	if (state) 
	{
		midi_clock_enabled =  true;
	}
	else       
	{
		midi_clock_enabled =  false;
	}
}
// ----------------------------------------------------------------------------
