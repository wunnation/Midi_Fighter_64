avrdude -p ATmega32u4 -c usbtiny -P usb -e -B 1 
avrdude -p ATmega32U4 -c usbtiny -P usb -U lfuse:w:0xde:m -U hfuse:w:0xd9:m -U efuse:w:0xc3:m 
avrdude.exe -p ATmega32u4 -c usbtiny -P usb -U flash:w:BootloaderDFU_mf64_003.hex -B1
avrdude.exe -p ATmega32u4 -c usbtiny -P usb -D -U flash:w:midifighter64_20170724_production.hex -B1
