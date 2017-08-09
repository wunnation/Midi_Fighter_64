// MIDI send, receive and decode functions for DJTechTools MidiFighter
//
//   Copyright (C) 2011 DJ Techtools
//
// rgreen 2009-10-17

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

#ifndef _MIDI_H_INCLUDED
#define _MIDI_H_INCLUDED

#include <stdbool.h>
#include <stdint.h>
#include <LUFA/Drivers/USB/USB.h>


#include "constants.h"

// MIDI types ------------------------------------------------------------------

// Type define for a USB-MIDI event packet.
typedef struct USB_MIDI_EventPacket
{
    unsigned char Command     : 4; // MIDI command number
    unsigned char CableNumber : 4; // Virtual cable number
    uint8_t Data1; // First byte of data in the MIDI event
    uint8_t Data2; // Second byte of data in the MIDI event
    uint8_t Data3; // Third byte of data in the MIDI event
} USB_MIDI_EventPacket_t;

// MIDI global variables -------------------------------------------------------

extern USB_ClassInfo_MIDI_Device_t* g_midi_interface_info;
extern uint8_t g_bank_selected;
extern uint8_t G_EE_MIDI_CHANNEL;
extern uint8_t G_EE_MIDI_VELOCITY;
extern uint8_t g_midi_note_state[2][MIDI_MAX_NOTES]; //[0]=Midi Feedback, [1]=Animation State
extern uint8_t g_midi_note_off_counter[MIDI_MAX_NOTES]; //
extern uint16_t g_bank_select_counter[NUM_BANKS]; //
extern uint8_t g_midi_sysex_channel;

extern bool g_midi_sysex_is_reading;
extern bool g_midi_sysex_is_valid;
	
extern bool midi_clock_enabled;

// MIDI function prototypes ----------------------------------------------------

void midi_setup(void);
void midi_stream_note_ch(const uint8_t channel, const uint8_t note, const bool onoff);
void midi_stream_note(const uint8_t note, const bool onoff);
void midi_stream_cc(const uint8_t cc, const uint8_t value);
void midi_flush(void);
uint8_t midipos_to_keypos(const uint8_t notepos);
uint8_t midi_fourbanks_note_to_key(const uint8_t note);
uint8_t midi_fourbanks_key_to_note(const uint8_t keynum);
uint8_t midi_64_key_to_note(const uint8_t keynum);



void midi_stream_raw_note(const uint8_t channel,
                          const uint8_t pitch,
                          const bool onoff,
                          const uint8_t velocity);

void midi_stream_raw_cc(const uint8_t channel,
                        const uint8_t cc,
                        const uint8_t value);

void midi_stream_sysex_identify(void);
void midi_stream_sysex (const uint8_t length, uint8_t* data);

void midi_clock(void);

void midi_clock_enable(bool state);

#endif // _MIDI_H_INCLUDED
