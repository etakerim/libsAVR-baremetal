#include <avr/io.h>
#include <util/delay.h>

int main(void)
{
    DDRB |= _BV(PB5);

    while (1) {
        PORTB ^= _BV(PB5);
        _delay_ms(2000);
    }
}
