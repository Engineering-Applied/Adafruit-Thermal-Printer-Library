/*------------------------------------------------------------------------
  An Arduino library for the Adafruit Thermal Printer:

  https://www.adafruit.com/product/597

  These printers use TTL serial to communicate.  One pin (5V or 3.3V) is
  required to issue data to the printer.  A second pin can OPTIONALLY be
  used to poll the paper status, but not all printers support this, and
  the output on this pin is 5V which may be damaging to some MCUs.

  Adafruit invests time and resources providing this open source code.
  Please support Adafruit and open-source hardware by purchasing products
  from Adafruit!

  Written by Joshua Harrell of University of North Florida
  Originally based on
  Thermal library from bildr.org
  Developed For Raspberry Pi Pico support
  MIT license, all text above must be included in any redistribution.
  ------------------------------------------------------------------------*/

#ifndef Thermal_Print_H
#define Thermal_Print_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>      
#include <ctype.h>
#include <math.h> 
#include <unistd.h>
#include <string.h>
#include "pico/stdlib.h"
//#include "pico/stdio.h"
#include "hardware/uart.h"

#ifdef __cplusplus
extern "C" {
#endif


class Thermal_Print {

 public:

  size_t
    write(uint8_t c);               // Check Name
  void
    begin(uint8_t heatTime=120),    // Check  Name
    boldOff(),                      // Check  Name
    boldOn(),                       // Check  Name
    doubleHeightOff(),              // Check  Name
    doubleHeightOn(),               // Check  Name
    doubleWidthOff(),               // Check  Name
    doubleWidthOn(),                // Check  Name
    feed(uint8_t x=1),              // Check  Name
    feedRows(uint8_t),              // Check  Name
    flush(),                        // Check  Name
    inverseOff(),                 // Check  Name
    inverseOn(),                  // Check  Name
    justify(char value),          // Check  Name
    offline(),                    // Check  Name
    online(),                     // Check  Name
    normal(),                     // Check  Name
    reset(),                      // Check  Name
    setCharSpacing(int spacing=0), // Check Name
    setCharset(uint8_t val=0),     // Check Name
    setCodePage(uint8_t val=0),   // Check  Name
    setDefault(),                 // Check  Name
    setLineHeight(int val=30),    // Check  Name
    setMaxChunkHeight(int val=256),// Check Name
    setSize(char value),          // Check  Name
    setTimes(unsigned long, unsigned long),     // Check  Name
    sleep(),                                    // Check  Name
    sleepAfter(uint16_t seconds),               // Check  Name
    strikeOff(),                                // Check  Name
    strikeOn(),                                 // Check  Name
    tab(),                       // Check Name
    test(),                      // Check Name
    testPage(),                  // Check Name
    timeoutSet(unsigned long),   // Check Name
    timeoutWait(),                // Check  Name
    underlineOff(),               // Check  Name
    underlineOn(uint8_t weight=1),// Check  Name
    upsideDownOff(),              // Check  Name
    upsideDownOn(),               // Check  Name
    wake();                     // Check  Name
  bool
    hasPaper();                 // Check  Name

 private:

  uint8_t
    printMode,
    prevByte,      // Last character issued to printer
    column,        // Last horizontal column printed
    maxColumn,     // Page width (output 'wraps' at this point)
    charHeight,    // Height of characters, in 'dots'
    lineSpacing,   // Inter-line spacing (not line height), in dots
    maxChunkHeight;
  unsigned long
    resumeTime,    // Wait until micros() exceeds this before sending byte
    dotPrintTime,  // Time to print a single dot line, in microseconds
    dotFeedTime;   // Time to feed a single dot line, in microseconds
  void
    writeBytes(uint8_t a),                                    // Check  Name
    writeBytes(uint8_t a, uint8_t b),                         // Check  Name
    writeBytes(uint8_t a, uint8_t b, uint8_t c),              // Check  Name
    writeBytes(uint8_t a, uint8_t b, uint8_t c, uint8_t d),   // Check  Name
    setPrintMode(uint8_t mask),                               // Check  Name
    unsetPrintMode(uint8_t mask),                             // Check  Name
    writePrintMode();                                         // Check  Name

};

#ifdef __cplusplus
}
#endif

#endif // Thermal_Print_H
