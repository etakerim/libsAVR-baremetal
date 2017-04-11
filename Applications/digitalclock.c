#include "rtc.h"
#include "charlcd.h"
#include <util/delay.h>
#include <stdbool.h>
#include <avr/io.h>
#include <avr/interrupt.h>

RTCTime tm;
CharLCD disp;
volatile bool changed = true;

/* Synchronizácia každých 255 s RTC 
void rtc_sync_timer(void)
{  
	DDRD &= ~(1 << PD4);
	PORTD |= (1 << PD4);   // Nastav T0 pin 
	rtc_sqwout(_1HZ);
    TCCR0B = (1 << CS02) | (1 << CS01);  // Nastav na externý vstup T0 falling edge  
	TCNT0 = 255 - 60;	
	TIMSK0 = (1 << TOIE0);
}

ISR(TIMER0_OVF_vect)
{
	changed = true;
    TCNT0 = 255 - 60;
    changed = true;
	rtc_read(&tm);
}
*/
void rtc_count_timer(void)
{
    TCCR1B |= (1 << CS12);  // Predelič 256 
	TCNT1 = 65535 - 62500;  
	TIMSK1 |= (1 << TOIE1);
}

// Počítadlo sekúnd - interná správa 
ISR(TIMER1_OVF_vect)
{
    TCNT1 = 65535 - 62500;
	changed = true;
	if (++tm.tm_sec > 59) {
		tm.tm_sec = 0; 
		if (++tm.tm_min > 59) {
			tm.tm_min = 0;
			if (++tm.tm_hour > 23) {
				tm.tm_hour = 0;
				if (++tm.tm_wday > 7) {
					tm.tm_wday = 1;
					if (++tm.tm_mday > 30) { // Zlá normalizácia 
						tm.tm_mday = 1;
						if (++tm.tm_mon > 12) {
							tm.tm_mon = 1;
							if (++tm.tm_year > 99)
								tm.tm_year = 0;
						} 
					}										
				}
			}
		}
	}
}

int main(void)
{

    lcd_init(&disp, 0x7E);
    rtc_read(&tm);
    //rtc_sync_timer();
  	rtc_count_timer();
    sei();

    while (1) {
		if (changed) {
			changed = false;
        	lcd_printf(&disp, 0, 0, "%02d.%02d.%4d", tm.tm_mday, tm.tm_mon, tm.tm_year + 2000);
       		lcd_printf(&disp, 1, 2, "%02d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);  
		}
    }
}
