// Printer_Test
#include "Thermal_Print.h"
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
#include "pico/stdio.h"
#include "hardware/uart.h"
// Send out a character without any conversions
    //uart_putc_raw(UART_ID, 'A');

// Send out a character but do CR/LF conversions
    //uart_putc(UART_ID, 'B');

// Send out a string, with CR/LF conversions
    //uart_puts(UART_ID, " Hello, UART!\n");

Thermal_Print Thermal;

#define UART_ID uart0

int main()
{
Thermal.begin();
uart_puts(UART_ID, "\n\n");
Thermal.inverseOn();
Thermal.justify('C');
Thermal.setSize('M');
uart_puts(UART_ID, "Hello World \n\n\n\n");
Thermal.setDefault();

uart_puts(UART_ID, "Follow my insta: \n");
Thermal.boldOn();
uart_puts(UART_ID, "@Engineering_Applied \n\n");
Thermal.boldOff();
}
