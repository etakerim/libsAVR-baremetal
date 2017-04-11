#include <avr/io.h>
#include <avr/interrupt.h>
#include <inttypes.h>
#include <util/delay.h>

#define CNTVZORY    2
#define LEDPOCET    4
#define LED1        PB5
#define LED2        PB4
#define LED3        PB3
#define LED4        PB2
#define TLACIDLO    PD2
const uint8_t LEDKY[LEDPOCET] = {LED1, LED2, LED3, LED4};
typedef void (*func)(void);

void onoff_vrade(void)
{
    uint8_t i;
    
    for (i = 0; i < LEDPOCET; i++) { 
        PORTB |= _BV(LEDKY[i]);
        _delay_ms(500);
        PORTB &= ~_BV(LEDKY[i]);
    }
}

void onoff_cele(void) 
{
    uint8_t i;

    for (i = 0; i < LEDPOCET; i++) { 
        PORTB |= _BV(LEDKY[i]);
        _delay_ms(500);
    }

    for (i = 0; i < LEDPOCET; i++) { 
        PORTB &= ~_BV(LEDKY[i]);
        _delay_ms(500);
    }
}

volatile uint8_t vzor = 0;

ISR(INT0_vect)
{
   vzor = (vzor + 1) % CNTVZORY; 
}

int main(void)
{
    uint8_t i;
    const func vzory[CNTVZORY] = {onoff_vrade, onoff_cele};  

    /* Nastav LEDky na výstup a tlačidlo na vstup pull-up */
    for (i = 0; i < LEDPOCET; i++) 
        DDRB |= _BV(LEDKY[i]);
    DDRD &= ~_BV(TLACIDLO);
    PORTD |= _BV(TLACIDLO);
    cli();
    EICRA |= _BV(ISC01);
    EICRA &= ~_BV(ISC00);
    EIMSK |= _BV(INT0);
    sei();

    while (1) {
        vzory[vzor]();
    }
}
