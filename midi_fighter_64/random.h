// Random number generator for Midifighter 2
//
//   Copyright (C) 2010-2011 Robin Green
//
// rgreen 2010-11-10

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

#ifndef _RANDOM_H_INCLUDED
#define _RANDOM_H_INCLUDED

#include <stdint.h>

// Macros and inline ----------------------------------------------------------

// Constants ------------------------------------------------------------------
extern const uint16_t gamma16_table[256];
extern const uint8_t gamma8_table[256];

// Functions ------------------------------------------------------------------

void set_seed16(uint16_t seed);
uint16_t random16(void);

void set_seed32(uint32_t seed);
uint32_t random32(void);

int8_t sintable(uint8_t angle);
uint8_t lerp(uint8_t high, uint8_t low, uint8_t t);
int8_t clamp(int8_t value, int8_t low, int8_t high);
float clampf(float value, float low, float high);
uint8_t smoothstep(uint8_t min, uint8_t max, uint8_t t);
uint32_t random_color(void);
// ----------------------------------------------------------------------------

#endif // _RANDOM_H_INCLUDED
