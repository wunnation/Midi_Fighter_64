
 /* config.c
 * DJTT - MIDI Fighter 64 - Embedded Software License
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

#include <util/delay.h>
#include <string.h>
#include <avr/wdt.h>

#include "config.h"
#include "sysex.h"
#include "eeprom.h"

// For the settings
#include "led.h"
#include "display.h" // !review: MF3D Patching only
#include "key.h"
#include "midi.h"
#include "eeprom.h"
#include "combo.h"


// SysEx command constants
#define SYSEX_COMMAND_PUSH_CONF    0x1
#define SYSEX_COMMAND_PULL_CONF    0x2
#define SYSEX_COMMAND_SYSTEM       0x3
#define SYSEX_COMMAND_BULK_XFER    0x4

uint8_t g_auto_update = 0;

// Command structure
#define TV_TABLE_SIZE 24
typedef union {
    struct {
        // Message data                TAG

        // BASIC
        uint8_t midiChannel;        // 0
        uint8_t midiVelocity;       // 1
        uint8_t keypressLeds;       // 2
        uint8_t fourBanksMode;      // 3 
        uint8_t expansionDigital;   // 4 Not supported by 3D
        uint8_t expansionAnalog;    // 5 Not supported by 3D
        uint8_t autoUpdate;         // 6 Not supported by 3D
        uint8_t softwareMode;       // 7
        uint8_t combos;             // 8
        uint8_t multiplexer;        // 9 Not supported by 3D
        
        // DISPLAY
        uint8_t animation;          // 10
        
        // MOTION
		uint8_t rotation;           // 11 (absolute = 2, relative = 1)
        uint8_t tilt;               // 12 (back = 8, right = 4, front = 2, left = 1)
        uint8_t tiltMode;           // 13 (0 = side lock, 1 = side mix)
		uint8_t tiltSens;           // 14
		uint8_t pitchSens;          // 15 Not Used
		uint8_t tiltRange;          // 16
		uint8_t pitchRange;         // 17
		uint8_t tiltDead;           // 18
		uint8_t pitchDead;          // 19 Not Used
		uint8_t tiltAxis;           // 20
		uint8_t pickSens;           // 21
		uint8_t sleepTime;          // 22
		uint8_t sideBank;           // 23

    };
    uint8_t bytes[TV_TABLE_SIZE];
} tvtable_t;

void tv_table_decode(tvtable_t* table, uint8_t* buffer, uint8_t size)
{
    uint8_t idx = 0;
    uint8_t tag;
    while (idx < size - 1) {
        tag = buffer[idx++];
        if (tag < TV_TABLE_SIZE) {
            table->bytes[tag] = buffer[idx];
        }
        ++idx;
    }
}


void sysExCmdPushConfig (uint8_t length, uint8_t* buffer) // Store Configuration data received via MIDI Sysex
{
	uint8_t side_bank_prev_state = G_EE_SIDE_BANK; 
	// First disable watch dog as EEPROM is slow
	wdt_disable();	
	
    tvtable_t config = {{0}};
    tv_table_decode(&config, buffer, length);

    // Change settings
    G_EE_MIDI_CHANNEL          = config.midiChannel - 1;
    G_EE_MIDI_VELOCITY         = config.midiVelocity;

    G_EE_FOUR_BANKS_MODE    = config.fourBanksMode;
    G_EE_MIDI_OUTPUT_MODE   = config.softwareMode;
    G_EE_COMBOS_ENABLE      = config.combos;
    // MSB || RESV || RESV || RESV || RESV || RESV || Side Wash || Cycle || Sparkle  || LSB
    G_EE_ANIMATIONS         = config.animation;
    //  MSB || BACK_TILT || RIGHT TILT || FRONT_TILT || LEFT TILT || Z_AXIS_ABSOLUTE || Z_AXIS_RELATIVE || RESV || RESV  || LSB
    G_EE_TILT_MASK          = (config.tilt << 4) | (config.rotation);
    G_EE_TILT_MODE          = config.tiltMode + 1;
 	G_EE_TILT_SENSITIVITY   = config.tiltSens;
 	G_EE_TILT_RANGE         = config.tiltRange;
 	G_EE_PITCH_RANGE        = config.pitchRange;
 	G_EE_TILT_DEADZONE      = config.tiltDead;
 	G_EE_TILT_AXIS          = config.tiltAxis;
	G_EE_PICK_SENSITIVITY   = config.pickSens;
	G_EE_SLEEP_TIME         = config.sleepTime;
	G_EE_SIDE_BANK          = config.sideBank;
    
	if (G_EE_SIDE_BANK != side_bank_prev_state) {
		if (!G_EE_SIDE_BANK) { // if side_bank button was newly disabled, switch to bank 1
			change_bank(0);
		}
		//else () {} // if newly disabled consider other operations
	}
	
	// Change to Bank 1 if G_EE_SIDE_BANK is now disabled
    // Flash LEDs to signal new configuration
	// - geometric triangle animation from the bottom corner
	
	start_geometric_animation(0, GEOMETRIC_ANIMATION_TYPE_TRIANGLE);
	
	// Save to EEPROM
    eeprom_save_edits();
    send_config_data();
	// Load the new settings
	eeprom_setup();  // !review: this is redundant because save_edits already puts the settings in these places
	// - calling eeprom_setup is a double check allows us to see if the Eeeprom was written correctly
	// -- causing the settings to immediately fail if the eeprom was not written correctly.
	// - !revsision: consider calling eeprom_check() (which would just 'check' that the read values match the current ones)
     
	// Enable watchdog again
	wdt_enable(WDTO_2S);
}

void send_config_data (void)
{
    uint8_t payload[] = {0xf0, 0x00, MANUFACTURER_ID >> 8, MANUFACTURER_ID & 0x7f,
                                SYSEX_COMMAND_PULL_CONF,
                                0x1, // 0x0 = request, 0x1 = response
                                0 , G_EE_MIDI_CHANNEL + 1,    // midi channel
                                1 , G_EE_MIDI_VELOCITY,       // midi note velocity
                                3 , G_EE_FOUR_BANKS_MODE,
                                7 , G_EE_MIDI_OUTPUT_MODE,
                                8 , G_EE_COMBOS_ENABLE,
                                10, G_EE_ANIMATIONS,
                                11, (G_EE_TILT_MASK) & 0x3, // rotation absolute and relative
                                12, (G_EE_TILT_MASK >> 4) & 0xf, // back, right, front, left
                                13, G_EE_TILT_MODE-1,
 								14, G_EE_TILT_SENSITIVITY,
 								15, G_EE_PITCH_SENSITIVITY,
 								16, G_EE_TILT_RANGE,
								17, G_EE_PITCH_RANGE,
 								18, G_EE_TILT_DEADZONE,
 								19, G_EE_PITCH_DEADZONE,
 								20, G_EE_TILT_AXIS,
								21, G_EE_PICK_SENSITIVITY,
								22, G_EE_SLEEP_TIME,
								23, G_EE_SIDE_BANK,
                                0xf7};
    midi_stream_sysex(/* number of bytes in payload:  */ sizeof(payload), payload);
}

void sysExCmdPullConfig (uint8_t length, uint8_t* buffer)
{
    // Change settings
    if (length > 0 && *buffer == 0x0) { // Received request
        send_config_data();
    }
}

void Jump_To_Bootloader (void);  //!mark: bootloader crashes: just include jumptoboot.h instead of declaring here?
void menu (void);
void eeprom_factory_reset (void);

void sysExCmdSystem (uint8_t length, uint8_t* buffer)
{
    if (length == 0) return;
    
    switch (*buffer) {
    case 0:
        {
            // The MF SPECTRA has no menu mode
        }
        break;
    case 1:
        {
			wdt_disable();
            // Bootloader mode
            //led_set_state(0xA5A5,0x00ffffff);
			led_set_state_dfu();
            Jump_To_Bootloader();
        }
        break;
    case 2:
        {
			wdt_disable();	
            // Factory reset EEPROM
            eeprom_factory_reset();

            // Flash to signal success.
            led_set_state(0xffff,0x00ffffff);
            _delay_ms(100);
            led_set_state(0x0000,0x00ffffff);
            _delay_ms(100);
            led_set_state(0xffff,0x00ffffff);
            _delay_ms(100);
			
			// No Reset Method: Send out updated configuration data in case MF Utility is listening
			send_config_data();
			wdt_enable(WDTO_2S);
			
			// Reset Method: Force Device Reset to Force Resend of Configuration Data 
			// - mf64 - usb reset is fast and not recognized by mf utility reliably (test pre-reset delays but not post-reset delays)
			//wdt_enable(WDTO_15MS);
			//while(true){}; // !review: Force Reset (why? when you could just call load_default_settings?)
        }
        break;
    default:
        break;
    }
}

extern uint8_t default_bank_inactive[2][64*3];
extern uint8_t default_bank_active[2][64*3];

/**********
Bulk Transfer Protocol:
    Bulk transfer sysex protocol is designed to support:
        - Multiple transfer modes (push, pull)
        - Multi-part transfer messages (each part payload has a maximum size of 24 bytes)
        - Maximum of 255 parts; maximum transfer size = 6KB
        - Up to 256 transfer "destinations" (identified by the tag)

    SysEx message format of a bulk transfer message:
    0xf0 0x0 0x1 0x79 0x4 CMD TAG CMD-SPECIFIC 0xf7
        CMD:        0   Push
                    1   Pull Request
        TAG:        A byte identifying the bulk data transfer
        Push:
            0xf0 0x0 0x1 0x79 0x4 0x0 TAG PART TOTAL SIZE PAYLOAD 0xf7
                PART:       The part number (1-based! Part 0 is not a valid part number)
                TOTAL:      Total number of parts
                SIZE:       Size of payload (must be 24 or lower)
                PAYLOAD:    SIZE number of bytes
        Pull:
            0xf0 0x0 0x1 0x79 0x4 0x1 TAG 0xf7
    If TAG os 0x0, then the format is instead:
    0xf0 0x0 0x1 0x79 0x4 CMD 0x0 TAG.0 TAG.1 CMD-SPECIFIC 0xf7
    Where TAG.0 and TAG.1 make up a two byte tag. Combined total number of available tags is therefore 65791


This implementation is hard coded to support only a subset of the protocol:
    - Only two tags are supported:
        0x1     Idle button color data
        0x2     Active button color data

NOTE: Binary data must either avoid setting the MSB, or encode octets as packed septets, as MIDI will interpret octets with the MSB set as special SysEx commands.

**********/

void sysExCmdBulkXfer(uint8_t length, uint8_t* buffer)
{   
    if (length > 2) {
        uint8_t command = *buffer++; // Push or pull
        uint8_t tag = *buffer++; // What is being transferred
        
        if (command == 0) { // PUSH
            if (length > 5) {
                if (tag == 0x0) return; // Extended tags not supported
                uint8_t part = *buffer++; // Transfers may consist of multiple parts
                if (part == 0) return; // Invalid part number
                uint8_t total = *buffer++;
                uint8_t size = *buffer++;
                if (size > length - 5) return; // Not enough data to support payload
                
                // Copy the data
				if (part > 16) { // NUM_BUTTONS*NUM_BANKS/8 - > 8 buttons of led data per 24 bytes
					return;
				}
				
				uint8_t bank = (part-1) >> 3; // 0 or 1 (8 parts per bank)
				uint8_t offset = ((part-1) & 0x07) * 24; // 0to7 * 24 -> to default_bank_inactive base position
                //uint8_t table[16][2] = { 
				///* Part 1: */ {0,0}, /* Part 2: */ {0,24},
                ///* Part 3: */ {0,48}, /* Part 4: */ {0,72},
                ///* Part 5: */ {0,96}, /* Part 6: */ {0,120},
                ///* Part 7: */ {0,144}, /* Part 8: */ {0,168},
                ///* Part 9: */ {1,0}, /* Part 10: */ {1,24}, // !bank64
                ///* Part 11: */ {1,48}, /* Part 12: */ {1,72},
                ///* Part 13: */ {1,96}, /* Part 14: */ {1,120},
                ///* Part 15: */ {1,144}, /* Part 16: */ {1,168}};
                //uint8_t bank   = table[part - 1][0];
                //uint8_t offset = table[part - 1][1];

                if (tag == 1) {
                    for (uint8_t idx = 0; idx < size; ++idx) {
                        default_bank_inactive[bank][offset + idx] = ((*buffer++) * 2);
                    }
					adjust_inactive_bank_leds_for_power(bank, offset, size);
                } else if (tag == 2) {
                    for (uint8_t idx = 0; idx < size; ++idx) {
                        default_bank_active[bank][offset + idx] = ((*buffer++) * 2);
                    }
					adjust_active_bank_leds_for_power(bank, offset, size);
                }
            }
        } else if (command == 1) { // PULL
            uint8_t* source;
            if (tag == 1) {
                source = (uint8_t*)default_bank_inactive;
            } else if (tag == 2) {
                source = (uint8_t*)default_bank_active;
            } else {
                return; // Invalid tag
            }
            
            // Total number of bytes to transfer
            uint16_t bytes_remaining = NUM_BANKS * NUM_BUTTONS * 3; // banked: 4 * 64 * 3; !bank64
            // Total number of parts in transfer
            uint16_t total = bytes_remaining / 24;
            uint16_t index=0;
            for (uint16_t part=1; part<=total; ++part) {
                // Size, in bytes, of current part
                uint16_t size = bytes_remaining > 24 ? 24 : bytes_remaining;
                bytes_remaining -= 24;
                // Message template
                uint8_t payload[] = {0xf0, 0x00, MANUFACTURER_ID >> 8, MANUFACTURER_ID & 0x7f,
                                SYSEX_COMMAND_BULK_XFER,
                                0x0, // Command: 0x0 = push, 0x1 = pull
                                tag,
                                part, // Part 'part' of 'total'
                                total,
                                size,
                                // 24 bytes maximum size
                                0xf7,0xf7,0xf7,0xf7,0xf7,0xf7,0xf7,0xf7,0xf7,0xf7,0xf7,0xf7,
                                0xf7,0xf7,0xf7,0xf7,0xf7,0xf7,0xf7,0xf7,0xf7,0xf7,0xf7,0xf7,
                                0xf7};
                // Copy the data into the payload
               // memcpy(payload + 10, source, size);
                for (uint8_t idx=10; idx < size+10; ++idx) {
                    // Convert Firmware Color Code to 7-bit midi sysex color code
                    // mf64 (4 to 5bit to 7-bit)
                    uint16_t this_color = source[index++];
                    this_color = this_color * DISPLAY_SCALING_COLOR_OUT_MAX_VALUE / DISPLAY_SCALING_COLOR_IN_MAX_VALUE;
                    //this_color = this_color * (uint16_t)(DISPLAY_SCALING_COLOR_OUT_MAX_VALUE) / (uint16_t)(DISPLAY_SCALING_COLOR_IN_MAX_VALUE);
                    payload[idx] = this_color;
                    //payload[idx] = (source[index++]) / 2;  // mf3d (8-bit to 7-bit)
					
                }
                
                // Send the message
                midi_stream_sysex(11 + size, payload);
				MIDI_Device_Flush(g_midi_interface_info); // MIDI_Device_USBTask calls flush, but has redundant checks that we are avoiding.
            }
        }
    }
}

void config_setup (void)
{
    // Install SysEx command handlers
    sysex_install(SYSEX_COMMAND_PUSH_CONF, sysExCmdPushConfig);
    sysex_install(SYSEX_COMMAND_PULL_CONF, sysExCmdPullConfig);
    sysex_install(SYSEX_COMMAND_SYSTEM,    sysExCmdSystem);
    sysex_install(SYSEX_COMMAND_BULK_XFER, sysExCmdBulkXfer);
}
