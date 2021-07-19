# Adafruit Thermal Printer Library [![Build Status](https://github.com/adafruit/Adafruit-Thermal-Printer-Library/workflows/Arduino%20Library%20CI/badge.svg)](https://github.com/adafruit/Adafruit-Thermal-Printer-Library/actions) 

Official Adafruit Source Page: (http://adafruit.github.io/Adafruit-Thermal-Printer-Library/html/index.html)

This is the setup for the RPi Pico with the Adafruit Thermal Printer.         
Pick one up at --> https://www.adafruit.com/product/597
These printers use TTL serial to communicate, 3 pins(RX/TX/Common Ground) are required.

Full tutorial with wiring diagrams, images, etc. is available at:
https://learn.adafruit.com/mini-thermal-receipt-printer

Adafruit invests time and resources providing this open source code.  Please support Adafruit and open-source hardware by purchasing products from Adafruit!

Written by Limor Fried/Ladyada for Adafruit Industries, with contributions from the open source community.  Originally based on Thermal library from bildr.org
MIT license, all text above must be included in any redistribution.

Modified By: Engineering By Josh
This setup works on the Raspberry Pi and Nvidia Jetson Nano

## ////Raspberry Pi LIBRARY LOCATION////
On your Linux box: (home directory)/pico/<_Insert New Folder_>/            
*I used Printer_Test as my folder name for example*

_Due to my pico folder being on an external SSD my setup looks slightly different_

_Also added the STASH folder for any extraneous files such as .zip_

![Image of Main Directory](https://github.com/Engineering-Applied/Adafruit-Thermal-Printer-Library/blob/master/media/images/Main%20Directory.png)

### Insert the github files into the folder you just created
![Image of internals of the new folder](https://github.com/Engineering-Applied/Adafruit-Thermal-Printer-Library/blob/master/media/images/Internals%20of%20Directory.png)

_The build folder is not provided as you must create it yourself per the following instructions_

## Setting up the build per Official RPi Pico Instructions
(https://datasheets.raspberrypi.org/pico/getting-started-with-pico.pdf)

1. Modify the Printer_Test.c file to your liking\
    _Thermal Printer commands are found through the official Adafruit learn guide_\
    (https://learn.adafruit.com/mini-thermal-receipt-printer)\
    
    _Pico Serial printing commands are found through the RPi Pico documentation website_\
    (https://raspberrypi.github.io/pico-sdk-doxygen/group__hardware__uart.html)    
2. Copy the pico_sdk_import.cmake file from the external folder in your pico-sdk installation to your test project folder.          
    `$ cp ../pico-sdk/external/pico_sdk_import.cmake .`                
3. Build the program with cmake (Version 3.13 or higher required)\
    `$ mkdir build`\
`$ cd build`\
`$ export PICO_SDK_PATH=../../pico-sdk`\
`$ cmake ..`\
`$ make `
4. Press and hold the _BOOTSEL_ button on your RPi Pico while plugging it into your Raspberry Pi
5. Place the _.UF2_ file from your build directory into your RPi Pico
6. Enjoy your new Thermal Printer Interface with the RPi Pico

### Feel free to ask any questions as this process was very confusing to figure out on my own

