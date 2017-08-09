avrdude.exe -p ATmega32u4 -c usbtiny -P usb -V -U flash:w:BootloaderDFU_mf64.hex -B1
avrdude.exe -p ATmega32u4 -c usbtiny -P usb -D -V -U flash:w:midifighter64.hex -B1
