#include "fmradio.h"
#include "uart.h"
#include <avr/interrupt.h>

int main(void)
{
    FMRadio radio;

    fmradio_init(&radio, 0x20, &PORTB, &DDRB, PB0);
    fmradio_poweron(&radio);
    fmradio_setvolume(&radio, 12);
    fmradio_setfrequency(&radio, 10180);

    RDSRadioData rds;
    rds_init(&rds);
    rds_calladd_psname(&rds, uart_puts);
    rds_calladd_text(&rds, uart_puts);

    uart_init(UART_BAUD_SELECT(9600, F_CPU));
    sei();

    while (1) {
        rds_check(&radio, &rds);
    }
}


