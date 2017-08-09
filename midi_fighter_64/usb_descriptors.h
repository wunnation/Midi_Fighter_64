// USB descriptors for Midifighter
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
// 2009-05-15: edited by Robin Green
// 2009-11-24: Updated to LUFA20090924

#ifndef _USB_DESCRIPTORS_H_INCLUDED
#define _USB_DESCRIPTORS_H_INCLUDED

#include <LUFA/Drivers/USB/USB.h>


#include <avr/pgmspace.h>


// USB Constants --------------------------------------------------------------

// The endpoint number for USB OUT messages (out from the host, into the
// Midifighter)
#define MIDI_STREAM_OUT_EPNUM 1

// The endpoint number for USB IN messages (in to the host, out from the
// Midifighter)
#define MIDI_STREAM_IN_EPNUM 2

// The size of a USB endpoint, 64 bytes, which will fit 16 normal USB-MIDI
// packets.
#define MIDI_STREAM_EPSIZE 64


// USB Descriptor -------------------------------------------------------------

// Type for the device configuration descriptor structure.
//
// This must be defined in the application code as this configuration
// contains several sub-descriptors which are unique to this device and are
// needed to describe the device to the host.
//
typedef struct {
    USB_Descriptor_Configuration_Header_t     Config;
    USB_Descriptor_Interface_t                Audio_ControlInterface;
    USB_Audio_Descriptor_Interface_AC_t       Audio_ControlInterface_SPC;
    USB_Descriptor_Interface_t                Audio_StreamInterface;
    USB_MIDI_Descriptor_AudioInterface_AS_t   Audio_StreamInterface_SPC;
    USB_MIDI_Descriptor_InputJack_t           MIDI_In_Jack_Emb;
    USB_MIDI_Descriptor_InputJack_t           MIDI_In_Jack_Ext;
    USB_MIDI_Descriptor_OutputJack_t          MIDI_Out_Jack_Emb;
    USB_MIDI_Descriptor_OutputJack_t          MIDI_Out_Jack_Ext;
    USB_Audio_Descriptor_StreamEndpoint_Std_t MIDI_In_Jack_Endpoint;
    USB_MIDI_Descriptor_Jack_Endpoint_t       MIDI_In_Jack_Endpoint_SPC;
    USB_Audio_Descriptor_StreamEndpoint_Std_t MIDI_Out_Jack_Endpoint;
    USB_MIDI_Descriptor_Jack_Endpoint_t       MIDI_Out_Jack_Endpoint_SPC;
} USB_Descriptor_Configuration_t;


// This function, required by LUFA, has a return value that may be
// unused. The macro at the end of this declaration is telling the compiler
// not to freak out about this.
//
uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue,
                                    const uint8_t wIndex,
                                    const void** const DescriptorAddress)
        ATTR_WARN_UNUSED_RESULT ATTR_NON_NULL_PTR_ARG(3);


#endif  // _USB_DESCRIPTORS_H_INCLUDED
