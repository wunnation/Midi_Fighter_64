// Configuration functions for DJTechTools Midifighter
//
//   Copyright (C) 2009 Robin Green
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
//
// rgreen 2009-05-22

#ifndef _CONFIG_H_INCLUDED
#define _CONFIG_H_INCLUDED

// Variables

// SysEx functions -----------------------------------------------

void config_setup (void);

void send_config_data (void);

// Functions needed from elsewhere
extern void change_bank(uint8_t this_bank);
extern void send_change_bank_notification(uint8_t this_bank);
extern void Midifighter_GetIncomingUsbMidiMessages(void);

#endif // _SYSEX_H_INCLUDED
