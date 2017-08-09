avrdude.exe -p ATmega32u4 -c usbtiny -P usb -U flash:w:BootloaderDFU_mf64_003.hex -B1
avrdude.exe -p ATmega32u4 -c usbtiny -P usb -D -U flash:w:midifighter64.hex -B1
