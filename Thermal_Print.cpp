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
  Developed For Rasperry Pi Pico support
  MIT license, all text above must be included in any redistribution.
  ------------------------------------------------------------------------*/

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h> 
#include <unistd.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "Thermal_Print.h"

#define UART_ID uart0
#define BAUD_RATE 19200
//#define BAUDRATE 19200

#define ASCII_TAB '\t' // Horizontal tab
#define ASCII_LF  '\n' // Line feed
#define ASCII_FF  '\f' // Form feed
#define ASCII_CR  '\r' // Carriage return
#define ASCII_DC2  18  // Device control 2
#define ASCII_ESC  27  // Escape
#define ASCII_FS   28  // Field separator
#define ASCII_GS   29  // Group separator

// Because there's no flow control between the printer and Arduino,
// special care must be taken to avoid overrunning the printer's buffer.
// Serial output is throttled based on serial speed as well as an estimate
// of the device's print and feed rates (relatively slow, being bound to
// moving parts and physical reality).  After an operation is issued to
// the printer (e.g. bitmap print), a timeout is set before which any
// other printer operations will be suspended.  This is generally more
// efficient than using delay() in that it allows the parent code to
// continue with other duties (e.g. receiving or decoding an image)
// while the printer physically completes the task.

// Number of microseconds to issue one byte to the printer.  11 bits
// (not 8) to accommodate idle, start and stop bits.  Idle time might
// be unnecessary, but erring on side of caution here.
#define BYTE_TIME (((11L * 1000000L) + (BAUD_RATE / 2)) / BAUD_RATE)

// === Character commands ===

#define INVERSE_MASK       (1 << 1) // Not in 2.6.8 firmware (see inverseOn())
#define UPDOWN_MASK        (1 << 2)
#define BOLD_MASK          (1 << 3)
#define DOUBLE_HEIGHT_MASK (1 << 4)
#define DOUBLE_WIDTH_MASK  (1 << 5)
#define STRIKE_MASK        (1 << 6)

// This method sets the estimated completion time for a just-issued task.
void Thermal_Print::timeoutSet(unsigned long x) {
  resumeTime = time_us_32() + x;
}

// This function waits (if necessary) for the prior task to complete.
void Thermal_Print::timeoutWait() {
  while((long)(time_us_32() - resumeTime) < 0L); // (syntax is rollover-proof)
}

// Wake the printer from a low-energy state.
void Thermal_Print::wake() {
  timeoutSet(0);   // Reset timeout counter
  writeBytes(255); // Wake
  sleep_us(50000);
  writeBytes(ASCII_ESC, '8', 0, 0); // Sleep off (important!)
}

// Reset printer to default state.
void Thermal_Print::reset() {
  writeBytes(ASCII_ESC, '@'); // Init command
  prevByte      = '\n';       // Treat as if prior line is blank
  column        =    0;
  maxColumn     =   32;
  charHeight    =   24;
  lineSpacing   =    6;
  // Configure tab stops on recent printers
  writeBytes(ASCII_ESC, 'D'); // Set tab stops...
  writeBytes( 4,  8, 12, 16); // ...every 4 columns,
  writeBytes(20, 24, 28,  0); // 0 marks end-of-list.
}

// Printer performance may vary based on the power supply voltage,
// thickness of paper, phase of the moon and other seemingly random
// variables.  This method sets the times (in microseconds) for the
// paper to advance one vertical 'dot' when printing and when feeding.
// For example, in the default initialized state, normal-sized text is
// 24 dots tall and the line spacing is 30 dots, so the time for one
// line to be issued is approximately 24 * print time + 6 * feed time.
// The default print and feed times are based on a random test unit,
// but as stated above your reality may be influenced by many factors.
// This lets you tweak the timing to avoid excessive delays and/or
// overrunning the printer buffer.
void Thermal_Print::setTimes(unsigned long p, unsigned long f) {
  dotPrintTime = p;
  dotFeedTime  = f;
}

void Thermal_Print::begin(uint8_t heatTime) {

  // The printer can't start receiving data immediately upon power up --
  // it needs a moment to cold boot and initialize.  Allow at least 1/2
  // sec of uptime before printer can receive data.
  timeoutSet(500000L);

  // Set up our UART with the required speed.
  uart_init(UART_ID, BAUD_RATE);

// Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    gpio_set_function(0, GPIO_FUNC_UART);
    gpio_set_function(1, GPIO_FUNC_UART);

  wake();
  reset();

  // ESC 7 n1 n2 n3 Setting Control Parameter Command
  // n1 = "max heating dots" 0-255 -- max number of thermal print head
  //      elements that will fire simultaneously.  Units = 8 dots (minus 1).
  //      Printer default is 7 (64 dots, or 1/6 of 384-dot width), this code
  //      sets it to 11 (96 dots, or 1/4 of width).
  // n2 = "heating time" 3-255 -- duration that heating dots are fired.
  //      Units = 10 us.  Printer default is 80 (800 us), this code sets it
  //      to value passed (default 120, or 1.2 ms -- a little longer than
  //      the default because we've increased the max heating dots).
  // n3 = "heating interval" 0-255 -- recovery time between groups of
  //      heating dots on line; possibly a function of power supply.
  //      Units = 10 us.  Printer default is 2 (20 us), this code sets it
  //      to 40 (throttled back due to 2A supply).
  // More heating dots = more peak current, but faster printing speed.
  // More heating time = darker print, but slower printing speed and
  // possibly paper 'stiction'.  More heating interval = clearer print,
  // but slower printing speed.

  writeBytes(ASCII_ESC, '7');   // Esc 7 (print settings)
  writeBytes(11, heatTime, 40); // Heating dots, heat time, heat interval

  // Print density description from manual:
  // DC2 # n Set printing density
  // D4..D0 of n is used to set the printing density.  Density is
  // 50% + 5% * n(D4-D0) printing density.
  // D7..D5 of n is used to set the printing break time.  Break time
  // is n(D7-D5)*250us.
  // (Unsure of the default value for either -- not documented)

#define printDensity   10 // 100% (? can go higher, text is darker but fuzzy)
#define printBreakTime  2 // 500 uS

  writeBytes(ASCII_DC2, '#', (printBreakTime << 5) | printDensity);

  dotPrintTime   = 30000; // See comments near top of file for
  dotFeedTime    =  2100; // an explanation of these values.
  maxChunkHeight =   255;
}

void Thermal_Print::writeBytes(uint8_t a) {
  timeoutWait();
  write(a);
  timeoutSet(BYTE_TIME);
}

void Thermal_Print::writeBytes(uint8_t a, uint8_t b) {
  timeoutWait();
  write(a);
  write(b);
  timeoutSet(2 * BYTE_TIME);
}

void Thermal_Print::writeBytes(uint8_t a, uint8_t b, uint8_t c) {
  timeoutWait();
  write(a);
  write(b);
  write(c);
  timeoutSet(3 * BYTE_TIME);
}

void Thermal_Print::writeBytes(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
  timeoutWait();
  write(a);
  write(b);
  write(c);
  write(d);
  timeoutSet(4 * BYTE_TIME);
}

// The underlying method for all high-level printing (e.g. println()).
// The inherited Print class handles the rest!
size_t Thermal_Print::write(uint8_t c) {

  if(c != 0x13) { // Strip carriage returns
    timeoutWait();
    // Send out a character without any conversions
    uart_putc_raw(UART_ID, c);
    // Send out a character but do CR/LF conversions
    //uart_putc(UART_ID, 'B');
    unsigned long d = BYTE_TIME;
    if((c == '\n') || (column == maxColumn)) { // If newline or wrap
      d += (prevByte == '\n') ?
        ((charHeight+lineSpacing) * dotFeedTime) :             // Feed line
        ((charHeight*dotPrintTime)+(lineSpacing*dotFeedTime)); // Text line
      column = 0;
      c      = '\n'; // Treat wrap as newline on next pass
    } else {
      column++;
    }
    timeoutSet(d);
    prevByte = c;
  }

  return 1;
}

void Thermal_Print::setPrintMode(uint8_t mask) {
  printMode |= mask;
  writePrintMode();
  charHeight = (printMode & DOUBLE_HEIGHT_MASK) ? 48 : 24;
  maxColumn  = (printMode & DOUBLE_WIDTH_MASK ) ? 16 : 32;
}

void Thermal_Print::unsetPrintMode(uint8_t mask) {
  printMode &= ~mask;
  writePrintMode();
  charHeight = (printMode & DOUBLE_HEIGHT_MASK) ? 48 : 24;
  maxColumn  = (printMode & DOUBLE_WIDTH_MASK ) ? 16 : 32;
}

void Thermal_Print::writePrintMode() {
  writeBytes(ASCII_ESC, '!', printMode);
}

// Check the status of the paper using the printer's self reporting
// ability.  Returns true for paper, false for no paper.
// Might not work on all printers!
bool Thermal_Print::hasPaper() {
  writeBytes(ASCII_ESC, 'v', 0);
  int status = -1;
  for(uint8_t i=0; i<10; i++) {
    if(uart_is_readable(UART_ID)) {
      status = uart_getc(UART_ID);
      break;
    }
    sleep_us(100000);
  }

  return !(status & 0b00000100);
}

void Thermal_Print::test(){
  writeBytes('T', 'e', 's', 't');
  feed(2);
}

void Thermal_Print::testPage() {
  writeBytes(ASCII_DC2, 'T');
  timeoutSet(
    dotPrintTime * 24 * 26 +      // 26 lines w/text (ea. 24 dots high)
    dotFeedTime * (6 * 26 + 30)); // 26 text lines (feed 6 dots) + blank line
}

// Take the printer offline. Print commands sent after this will be
// ignored until 'online' is called.
void Thermal_Print::offline(){
  writeBytes(ASCII_ESC, '=', 0);
}

// Take the printer back online. Subsequent print commands will be obeyed.
void Thermal_Print::online(){
  writeBytes(ASCII_ESC, '=', 1);
}

// Put the printer into a low-energy state immediately.
void Thermal_Print::sleep() {
  sleepAfter(1); // Can't be 0, that means 'don't sleep'
}

// Put the printer into a low-energy state after the given number
// of seconds.
void Thermal_Print::sleepAfter(uint16_t seconds) {
  writeBytes(ASCII_ESC, '8', seconds, seconds >> 8);
}

void Thermal_Print::normal() {
  printMode = 0;
  writePrintMode();
}

// Reset text formatting parameters.
void Thermal_Print::setDefault(){
  online();
  justify('L');
  inverseOff();
  doubleHeightOff();
  setLineHeight(30);
  boldOff();
  underlineOff();
  setSize('s');
  setCharset();
  setCodePage();
}

void Thermal_Print::inverseOn(){
  writeBytes(ASCII_GS, 'B', 1);
}

void Thermal_Print::inverseOff(){
  writeBytes(ASCII_GS, 'B', 0);
}

void Thermal_Print::upsideDownOn(){
  setPrintMode(UPDOWN_MASK);
}

void Thermal_Print::upsideDownOff(){
  unsetPrintMode(UPDOWN_MASK);
}

void Thermal_Print::doubleHeightOn(){
  setPrintMode(DOUBLE_HEIGHT_MASK);
}

void Thermal_Print::doubleHeightOff(){
  unsetPrintMode(DOUBLE_HEIGHT_MASK);
}

void Thermal_Print::doubleWidthOn(){
  setPrintMode(DOUBLE_WIDTH_MASK);
}

void Thermal_Print::doubleWidthOff(){
  unsetPrintMode(DOUBLE_WIDTH_MASK);
}

void Thermal_Print::strikeOn(){
  setPrintMode(STRIKE_MASK);
}

void Thermal_Print::strikeOff(){
  unsetPrintMode(STRIKE_MASK);
}

void Thermal_Print::boldOn(){
  setPrintMode(BOLD_MASK);
}

void Thermal_Print::boldOff(){
  unsetPrintMode(BOLD_MASK);
}

void Thermal_Print::justify(char value){
  uint8_t pos = 0;
  switch(toupper(value)) {
    case 'L': pos = 0; break;
    case 'C': pos = 1; break;
    case 'R': pos = 2; break;
  }
  writeBytes(ASCII_ESC, 'a', pos);
}

// Feeds by the specified number of lines
void Thermal_Print::feed(uint8_t x) {
  writeBytes(ASCII_ESC, 'd', x);
  timeoutSet(dotFeedTime * charHeight);
  prevByte = '\n';
  column   =    0;
}

// Feeds by the specified number of individual pixel rows
void Thermal_Print::feedRows(uint8_t rows) {
  writeBytes(ASCII_ESC, 'J', rows);
  timeoutSet(rows * dotFeedTime);
  prevByte = '\n';
  column   =    0;
}

void Thermal_Print::flush() {
  writeBytes(ASCII_FF);
}

void Thermal_Print::underlineOn(uint8_t weight) {
  if(weight > 2) weight = 2;
  writeBytes(ASCII_ESC, '-', weight);
}

void Thermal_Print::underlineOff() {
  writeBytes(ASCII_ESC, '-', 0);
}

void Thermal_Print::tab() {
  writeBytes(ASCII_TAB);
  column = (column + 4) & 0b11111100;
}

void Thermal_Print::setCharSpacing(int spacing) {
  writeBytes(ASCII_ESC, ' ', spacing);
}

// Alters some chars in ASCII 0x23-0x7E range; see datasheet
void Thermal_Print::setCharset(uint8_t val) {
  if(val > 15) val = 15;
  writeBytes(ASCII_ESC, 'R', val);
}

// Selects alt symbols for 'upper' ASCII values 0x80-0xFF
void Thermal_Print::setCodePage(uint8_t val) {
  if(val > 47) val = 47;
  writeBytes(ASCII_ESC, 't', val);
}

void Thermal_Print::setLineHeight(int val) {
  if(val < 24) val = 24;
  lineSpacing = val - 24;

  // The printer doesn't take into account the current text height
  // when setting line height, making this more akin to inter-line
  // spacing.  Default line spacing is 30 (char height of 24, line
  // spacing of 6).
  writeBytes(ASCII_ESC, '3', val);
}

void Thermal_Print::setMaxChunkHeight(int val) {
  maxChunkHeight = val;
}

void Thermal_Print::setSize(char value){
  uint8_t size;
  switch(toupper(value)) {
   default:  // Small: standard width and height
    size       = 0x00;
    charHeight = 24;
    maxColumn  = 32;
    break;
   case 'M': // Medium: double height
    size       = 0x01;
    charHeight = 48;
    maxColumn  = 32;
    break;
   case 'L': // Large: double width and height
    size       = 0x11;
    charHeight = 48;
    maxColumn  = 16;
    break;
  }
  writeBytes(ASCII_GS, '!', size);
  prevByte = '\n'; // Setting the size adds a linefeed
}
