#include "rtc.h"
#include "uart.h"
#include <stdlib.h>
#include <avr/interrupt.h>
#include <util/delay.h>

int main(void)
{
    RTCTime clock;
    char buf[5];

    rtc_init();
    clock.tm_sec = 0;
    clock.tm_min = 4;
    clock.tm_hour = 19;
    clock.tm_wday = 0;
    clock.tm_mday = 20;
    clock.tm_mon = 3;
    clock.tm_year = 17;
    rtc_write(&clock);

    uart_init(UART_BAUD_SELECT(9600, F_CPU));
    sei();

    while (1) {
        _delay_ms(800);
        rtc_read(&clock);
        itoa(clock.tm_hour, buf, 10);
        uart_puts(buf);
        uart_putc(':');
        
        itoa(clock.tm_min, buf, 10);
        uart_puts(buf);
        uart_putc(':');

        itoa(clock.tm_sec, buf, 10);
        uart_puts(buf);
        uart_puts("\r\n");
    }
}
