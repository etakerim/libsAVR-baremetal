#include <avr/io.h>
#include <avr/interrupt.h>
#include "CharLCD/charlcd.h"
#include <util/delay.h>
#include <inttypes.h>
#include <stdbool.h>

void adc_nastav(void)
{
    ADMUX |= _BV(REFS0);
    ADCSRA |= _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0);
    ADCSRA |= _BV(ADEN);
}

uint16_t adc_citaj(uint8_t ch)
{
    ADMUX |= (ch & 0x0F);
    ADCSRA |= _BV(ADSC);
    while (ADCSRA & _BV(ADSC))
        ;
    return ADC;
}

void stopky_nastav(void)
{
    TCCR1B |= _BV(CS12);   
	TCNT1 = 65535 - 62500;  
	TIMSK1 |= _BV(TOIE1);
}

volatile bool zmenene = true;
volatile unsigned long sekundy = 0;

ISR(TIMER1_OVF_vect)
{
    TCNT1 = 65535 - 62500;
	zmenene = true;
    sekundy++;
}

int main(void)
{
    CharLCD disp;
    uint16_t aconv;
    unsigned long mV;

    adc_nastav();
    lcd_init(&disp, 0x7E);
    stopky_nastav();
    sei();

    while (1) {
        if (zmenene) {
            zmenene = false;
            aconv = adc_citaj(ADC0D);
            mV = (5000.0 / 1024.0) * aconv;
            lcd_clear(&disp);
            lcd_printf(&disp, 0, 0, "U = %lu mV", mV); 
            lcd_printf(&disp, 1, 0, "t = %lu s", sekundy);
        }
    }
}
