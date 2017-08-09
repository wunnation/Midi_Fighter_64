// Combo key functions for DJTechTools Midifighter
//
//   Copyright (C) 2011 DJ Techtools
//
// rgreen 2011-03-09

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

#ifndef _COMBO_H_INCLUDED
#define _COMBO_H_INCLUDED

#include <stdbool.h>
#include <stdint.h>

// Types ----------------------------------------------------------------------

// event keyups are DOWN + 5
typedef enum combo_action {
    COMBO_NONE,
    COMBO_A_DOWN = 1,
    COMBO_B_DOWN,
    COMBO_C_DOWN,
    COMBO_D_DOWN,
    COMBO_E_DOWN,
    COMBO_A_RELEASE = 6,
    COMBO_B_RELEASE,
    COMBO_C_RELEASE,
    COMBO_D_RELEASE,
    COMBO_E_RELEASE,
} combo_action_t;

// Functions -------------------------------------------------------------------

void combo_setup(void);
combo_action_t combo_recognize(void); //const uint16_t keydown,
                               //const uint16_t keyup,
                               //const uint16_t keystate);

// ----------------------------------------------------------------------------

#endif // _COMBO_H_INCLUDED
