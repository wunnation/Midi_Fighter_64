// Checks for and performs jump to boot loader
//
// Copyright (C) 2011 Michael Mitchell
//
// MichaelMitchell 2012-01-19

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

#include <avr/wdt.h>
  #include <avr/io.h>
  #include <util/delay.h>

  #include <LUFA/Common/Common.h>
  #include <LUFA/Drivers/USB/USB.h>

  uint32_t Boot_Key ATTR_NO_INIT;

  #define MAGIC_BOOT_KEY            0xDC42ACCA
  #define BOOTLOADER_START_ADDRESS  0x7000//-(FLASH_SIZE_BYTES - BOOTLOADER_SEC_SIZE_BYTES)
  
  void Bootloader_Jump_Check(void) ATTR_INIT_SECTION(3);
  void Bootloader_Jump_Check(void)
  {
      // If the reset source was the bootloader and the key is correct, clear it and jump to the bootloader
      if (((MCUSR & (1<<WDRF))) && (Boot_Key == MAGIC_BOOT_KEY))
      {
		  
          Boot_Key = 0;
		  __asm ("jmp 0x7000");   // !mark
      }
  }

  void Jump_To_Bootloader(void)
  {
      // If USB is used, detach from the bus
      USB_Disable();

      // Disable all interrupts
      cli();

      // Wait two seconds for the USB detachment to register on the host
      for (uint8_t i = 0; i < 128; i++)  // !mark: bootloader detection (make longer or shorter)
        _delay_ms(16);

      // Set the bootloader key to the magic value and force a reset
      Boot_Key = MAGIC_BOOT_KEY;
      wdt_enable(WDTO_250MS);
      for (;;); 
  }