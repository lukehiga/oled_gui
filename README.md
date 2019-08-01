# Graphical User Interface: OLED SH1106 Driver + Menus

Low resource graphical user interface for Atmega328 using an SH1106 SPI OLED display (SSD1306 should work with some minor tweaks) and a rotary encoder such as the KY-040 module. Its purpose is to be able to modify/specify parameters at run time and a way to display text without using lots of resources. 


* Made from scratch 
* Since fonts could take a huge amount of FLASH memory, this program can enlarge regular fonts (8x5 bit) to 2X, 3X and 4X size at run time. Since there are 94 chars, at 5 bytes per char, this uses 320 bytes. Having a 2X font in FLASH memory would take 4*320 bytes and 3X and 4X would take 9*320 and 16*320 bytes respectively (almost 10kB of FLASH).
* Using menus and OLED text driver with font enlargement takes around 5kB of FLASH memory, depending on the amount of menu options.  
* Using menus and OLED text driver without font enlargement would take around 3.5kB.
* No menus nor font enlargement takes 1.5kB. This may be useful as a simple debugging tool. 
* Doesn't use interrupts or timers since these may be used for something more important. Polling speed should be fast enough to detect a falling edge of input A and check if input B is still high or it was low before at the time of the falling edge. 
* By using a couple of 100nF on the encoder outputs there seems to be no bouncing, so no debouncing algorithm used. KY-040 uses 10k pullup resistors which would give a time constant of 1ms. 



