#include <avr/io.h>
#include <avr/interrupt.h>
#include <inttypes.h>
#include <util/delay.h>
#include <stdbool.h>

#define DISPBUS_PORT    PORTD
#define DISPBUS_DDR     DDRD
#define DISPDIGIT_PORT  PORTB
#define DISPDIGIT_DDR   DDRB

#define DIGIT1          _BV(PB1)
#define DIGIT2          _BV(PB2)
#define DIGIT3          _BV(PB3)
#define DIGIT4          _BV(PB4)
#define DOT             _BV(PB5)

#define A_DISPSEG       _BV(PD7)
#define B_DISPSEG       _BV(PD5)
#define C_DISPSEG       _BV(PD2)
#define D_DISPSEG       _BV(PD1)
#define E_DISPSEG       _BV(PD0)
#define F_DISPSEG       _BV(PD6)
#define G_DISPSEG       _BV(PD4)

#define TLACIDLO1		_BV(PD3)    /* PLAY/PAUSE */
#define TLACIDLO2       _BV(PC2)    /* ZERO */

void disp_init(void)
{
    const uint8_t digits = DIGIT1 | DIGIT2 | DIGIT3 | DIGIT4 | DOT; 
    const uint8_t segments = A_DISPSEG | B_DISPSEG | C_DISPSEG | D_DISPSEG |
                             E_DISPSEG | F_DISPSEG | G_DISPSEG;
   /* Nastav všetky piny na výstupy */
    DISPDIGIT_DDR |= digits;    
    DISPBUS_DDR   |= segments;
    /* Zbernica = LOW[vyber = HIGH],  CISLA = HIGH[vyber=LOW(GND)] */
    DISPDIGIT_PORT |= digits;
    DISPBUS_PORT  &= ~segments;
}

void disp_multiplex(uint16_t sec)
{
    uint8_t i;
    const uint8_t digitstab[] = {DIGIT1, DIGIT2, DIGIT3, DIGIT4};
    const uint8_t numtab[10] = {
        A_DISPSEG | B_DISPSEG | C_DISPSEG | D_DISPSEG | E_DISPSEG | F_DISPSEG,
        B_DISPSEG | C_DISPSEG,
        A_DISPSEG | B_DISPSEG | G_DISPSEG | E_DISPSEG | D_DISPSEG,
        A_DISPSEG | B_DISPSEG | G_DISPSEG | C_DISPSEG | D_DISPSEG,
        F_DISPSEG | G_DISPSEG | B_DISPSEG | C_DISPSEG,
        A_DISPSEG | F_DISPSEG | G_DISPSEG | C_DISPSEG | D_DISPSEG, 
        A_DISPSEG | F_DISPSEG | G_DISPSEG | C_DISPSEG | D_DISPSEG | E_DISPSEG,
        F_DISPSEG | A_DISPSEG | B_DISPSEG | C_DISPSEG,
        A_DISPSEG | B_DISPSEG | C_DISPSEG | D_DISPSEG | E_DISPSEG | F_DISPSEG | G_DISPSEG,
        A_DISPSEG | B_DISPSEG | C_DISPSEG | D_DISPSEG | F_DISPSEG | G_DISPSEG
    };
    
    for (i = 0; i < 4; i++) { // od konca na začiatok 
		DISPDIGIT_PORT |= digitstab[i];
        DISPBUS_PORT = ~numtab[sec % 10];  /* musím zapísať [=] a nie [|=]*/
		_delay_ms(4);
		if (i == 0)      /* Vypíš desatinnú čiarku */
			DISPDIGIT_PORT &= ~DOT;
		else 
			DISPDIGIT_PORT |= DOT;
		DISPDIGIT_PORT &= ~digitstab[i];
		sec /= 10;
    }
}

volatile uint16_t desatiny;
volatile bool timeron; 

void stopwatch_set(void)
{
	/* Vstup pull-up - INT1*/
	DDRD &= ~TLACIDLO1;     
	PORTB |= TLACIDLO1;

    DDRC &= ~TLACIDLO2;
    PORTC |= TLACIDLO2;
	/* Prerušenie pri falling edge */
    cli();
	EICRA |= (_BV(ISC11) & ~_BV(ISC10));
	EIMSK |= _BV(INT1);

    PCICR |= _BV(PCIE1);    
    PCMSK1 |= _BV(PCINT9);

	TCCR1B |= (_BV(CS12) & ~_BV(CS11) & ~_BV(CS10));  /* Predelič 256 [100]*/
	TCNT1 = 59285;	                                  /* 65535-6250*/      
	timeron = true;
	desatiny = 0;
    sei();
}

ISR(PCINT1_vect)
{
    cli();
    TIMSK1 &= ~_BV(TOIE1);
    timeron = false;
    desatiny = 0;
}
 
ISR(INT1_vect)
{
	/* Zapne alebo spustí časovač podľa stavu */
	cli();
	if (timeron) 
        TIMSK1 &= ~_BV(TOIE1);		
	else 
        TIMSK1 |= _BV(TOIE1);
    sei();
    timeron = !timeron;  
}

ISR(TIMER1_OVF_vect)
{
	/* každú 1 ms zväčší programové počítadlo */ 
    desatiny++;
    TCNT1 = 59285;
} 

int main(void)
{
    disp_init();
	stopwatch_set();

    while (1) {
        disp_multiplex(desatiny);
    }
}
