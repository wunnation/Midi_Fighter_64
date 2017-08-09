// Random number generator for Midifighter 30mm RGB
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

#include <avr/io.h>
#include <avr/pgmspace.h>
#include "random.h"

// global data ----------------------------------------------------------------

// Sine and Gradient tables generated from Python using:
//
//    table = [int(math.sin(x * 2.0 * math.pi / 32) * 128) for x in range(0,33)]
//    grad = [ table[i+1] - table[i] for i in range(0,32)]
//
static const int8_t sin_table[32] PROGMEM = {
      0,   24,   48,   70,   89,  105,  117,  124,
    127,  124,  117,  105,   89,   70,   48,   24,
      0,  -24,  -48,  -70,  -89, -105, -117, -124,
   -127, -124, -117, -105,  -89,  -70,  -48,  -24
};

static const int8_t grad_table[32] PROGMEM = {
     24,  24,  22,  19,  16,  12,   7,   3,
     -3,  -7, -12, -16, -19, -22, -24, -24,
    -24, -24, -22, -19, -16, -12,  -7,  -3,
      3,   7,  12,  16,  19,  22,  24,  24
};


// Valid seeds for the random number generators.
static uint16_t g_seed16 = 36243;
static uint32_t g_seed32 = 2463534242;


// Mathematical functions -----------------------------------------------------

// Generate pseudorandom numbers, based on "Xorshift RNGs", George
// Marsaglia, 2003
//
//    http://www.jstatsoft.org/v08/i14/paper
//

// Set the seed for the 16-bit pseudorandom sequence.
void set_seed16(uint16_t seed) { g_seed16 = seed; }

// Return a 16-bit pseudorandom value.
uint16_t random16(void)
{
    g_seed16 ^= (g_seed16 << 13);
    g_seed16 ^= (g_seed16 >> 9);
    g_seed16 ^= (g_seed16 << 7);
    return g_seed16;
}


// Set the seed for the 32-bit pseudorandom sequence.
void set_seed32(uint32_t seed) { g_seed32 = seed; }

// Return a 32-bit pseudorandom value.
//
uint32_t random32(void)
{
    g_seed32 ^= (g_seed32 << 13);
    g_seed32 ^= (g_seed32 >> 17);
    g_seed32 ^= (g_seed32 << 5);
    return g_seed32;
}

// Sine and cosine functions.
// Angles are in the 0..255 radians format.
//
int8_t sintable(uint8_t angle)
{
   uint8_t index = angle >> 3;
   int8_t result = pgm_read_byte(&sin_table[index]);
   // add in the signed gradient
   int8_t interp = angle & 0x7;
   int8_t grad = pgm_read_byte(&grad_table[index]);
   result += (interp * grad) >> 3;
   return result;
}

// Linear interpolation between two values.
// 8-bit fixed point values where 0 = 0.0f and 255 = 1.0f
//
uint8_t lerp(uint8_t high, uint8_t low, uint8_t t)
{
    int16_t temp = t * (high-low);
    return low + (temp >> 8);
}

// restrict a signed value to within a range.
//
int8_t clamp(int8_t value, int8_t low, int8_t high)
{
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

// restrict a float to within a range.
//
float clampf(float value, float low, float high)
{
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

int8_t clamp_cc(int8_t value)
{
	if (value <= 0) return 0;
	if (value >= 127) return 127;
	return value;
}


uint8_t smoothstep(uint8_t min, uint8_t max, uint8_t t)
{
    if (t < min) return 0;
    if (t > max) return 255;
    uint8_t x = (t-min) / (max - min);
    return 3 * x * x - 2 * x * x * x;
}

// return a 32-bit value containing a random color on the spectrum
// (i.e. a full saturation hue with no intensity).
uint32_t random_color(void)
{
    uint8_t phase = random16() >> 8;
    uint8_t r = sintable(phase) + 127;
    uint8_t g = sintable(phase + 85) + 127;   // plus 2Pi/3
    uint8_t b = sintable(phase + 170) + 127;  // plus 2*2Pi/3
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

// ----------------------------------------------------------------------------
