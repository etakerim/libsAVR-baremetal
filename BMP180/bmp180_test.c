#include "uart.h"
#include <stdio.h>
#include "bmp180.h"
#include <util/delay.h>
#include <avr/interrupt.h>

int main(void)
{
    char dst[50];
    //char buf[100];
    BMP180Sensor dps;

    uart_init(UART_BAUD_SELECT(9600, F_CPU));
    bmp180_init(&dps);

    sei();
    while (1) {
      /*  snprintf(buf, 100, "\nREGISTRE --- \nAC1 = %d, AC2 = %d, AC3 = %d, AC4 = %u, AC5 = %u, AC6 = %u\n"
                       "B1 = %d, B2 = %d, MB = %d, MC = %d, MD = %d\n", ac1, ac2, ac3, ac4, ac5, ac6, b1, b2, mb, mc, md);          uart_puts(buf);  
    */
        snprintf(dst, 50, "TEMP: %ld,  TLAK: %ld,  VYSKA: %ld\n", 
		bmp180_gettemperature(&dps), bmp180_getpressure(&dps), bmp180_getaltitute(&dps));
        uart_puts(dst);
        _delay_ms(1000);
    }
}
