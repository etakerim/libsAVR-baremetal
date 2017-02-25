#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>


void adc_nastav(void)
{
    /* AVCC s externým kondenzátorom na AREF */
    ADMUX |= (1<<REFS0);
    /* Nastav ADC hodinový delič na 256 */
    ADCSRA |= (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);
    /* Zapni ADC */
    ADCSRA |= (1<<ADEN);
}

uint16_t adc_citaj(uint8_t channel)
{
    /* Vyber s muliplexera správny vstup */
    ADMUX |= (channel & 0x0F);
    /* Začni konverziu a čakaj na jej dokončenie */
    ADCSRA |= (1<<ADSC);
    while (ADCSRA & (1<<ADSC)) 
        ;
    return ADC;  /* Vráť ADCH + ADCL */
}


const uint8_t digit[4] = {(1<<PORTB0), (1<<PORTB1), (1<<PORTB2), (1<<PORTB4)};
const uint8_t ciselny_kod[10] = { 
    0x77, 0x14, 0x5B, 0x5E, 0x3C, 0x6E, 0x6F, 0x74, 0x7F, 0x7E };


void disp_nastav(void)
{
    const uint8_t LCDSEGMENTY = digit[0] | digit[1] | digit[2] | digit[3];

    DDRD = 0xFF;           /* PortD = výstup */
    DDRB |= LCDSEGMENTY;   /* PortB segmenty = výstup */
    PORTD = 0x00;          /* PortD = LOW (rozsvieť = HIGH) */
    PORTB &= ~LCDSEGMENTY;  /* PortB seg = HIGH (vyber = LOW) */
}


void disp_multiplex(uint16_t cislo)
{
    uint8_t i;

    /* Zapíš postupne na display */
    for (i = 0; i < 4; i++) {
        PORTB |= digit[i];
        PORTD = ~ciselny_kod[cislo % 10];
        _delay_ms(4);
        PORTB &= ~digit[i];
        cislo /= 10;
    }
}

#define INTERRUPT_PERIOD 512
#define FINT (F_CPU / INTERRUPT_PERIOD) 

/* Predpočítané hodnoty sínusu */
const uint8_t PROGMEM sinetable[256] = {
  128,131,134,137,140,143,146,149,152,156,159,162,165,168,171,174,
  176,179,182,185,188,191,193,196,199,201,204,206,209,211,213,216,
  218,220,222,224,226,228,230,232,234,236,237,239,240,242,243,245,
  246,247,248,249,250,251,252,252,253,254,254,255,255,255,255,255,
  255,255,255,255,255,255,254,254,253,252,252,251,250,249,248,247,
  246,245,243,242,240,239,237,236,234,232,230,228,226,224,222,220,
  218,216,213,211,209,206,204,201,199,196,193,191,188,185,182,179,
  176,174,171,168,165,162,159,156,152,149,146,143,140,137,134,131,
  128,124,121,118,115,112,109,106,103,99, 96, 93, 90, 87, 84, 81,
  79, 76, 73, 70, 67, 64, 62, 59, 56, 54, 51, 49, 46, 44, 42, 39,
  37, 35, 33, 31, 29, 27, 25, 23, 21, 19, 18, 16, 15, 13, 12, 10,
  9,  8,  7,  6,  5,  4,  3,  3,  2,  1,  1,  0,  0,  0,  0,  0, 
  0,  0,  0,  0,  0,  0,  1,  1,  2,  3,  3,  4,  5,  6,  7,  8, 
  9,  10, 12, 13, 15, 16, 18, 19, 21, 23, 25, 27, 29, 31, 33, 35,
  37, 39, 42, 44, 46, 49, 51, 54, 56, 59, 62, 64, 67, 70, 73, 76,
  79, 81, 84, 87, 90, 93, 96, 99, 103,106,109,112,115,118,121,124
};

/* V závislosti na typu vlny bude pole obsahovať vzorky prehrávanej vlny */
uint8_t wavetable[256];

unsigned int frekv_koef = 100;
int soundEnabled = 1;
int zvukhra = 0;

int currentVoice = 0;

/* Prerušenie vzorkuje 8-bitové PCM na PWM výstup do reproduktora */
ISR(TIMER1_COMPA_vect)
{
    static unsigned int faza0;
    static unsigned int vzorka;
    static unsigned int tmpfaza;

    tmpfaza = faza0 + frekv_koef;
    vzorka = wavetable[faza0>>8];
    faza0 = tmpfaza;
    OCR2A = vzorka;   /* Daj vzorku na výstup */
}

void pwm_nastav(void)
{
    /* Nastav časovač 2 na pulzno-šíkovú moduláciu */
    /* Použi vnútorné hodiny a nastav fastPWM mód*/
    ASSR &= ~((1<<EXCLK) | (1<<AS2));       
    TCCR2A |= (1<<WGM21) | (1<<WGM20);
    TCCR2B &= ~(1<<WGM22);
    /* Neinvertované PWM na pine OCR2A */
    TCCR2A = (TCCR2A | (1<<COM2A1)) & ~(1<<COM2A0);
    TCCR2A &= ~((1<<COM2B1) | (1<<COM2B0));
    /* Žiadny delič časovača */
    TCCR2B = (TCCR2B & ~((1<<CS12) | (1<<CS11))) | (1<<CS10);
    /* Počiatočný pulz nastav na prvú vzorku */
    OCR2A = 0;

    /* Časovač1 použi na poslanie vzorky každé prerušenie */
    cli();
    /* Nastav CTC mód (Clear Timer on Compare Match) */
    TCCR1B = (TCCR1B & (1<<WGM13)) | (1<<WGM12);
    TCCR1A = TCCR1A & ~((1<<WGM11) | (1<<WGM10));
    /* Žiadny delič časovača */
    TCCR1B = (TCCR1B & ~((1<<CS12) | (1<<CS11))) | (1<<CS10);
    /* Nastav porovanávací register (OCR1A) */
    OCR1A = INTERRUPT_PERIOD;
    /* Povoľ prerušenie, keď TCNT1 == OCR1A */
    TIMSK1 |= (1<<OCIE1A);
    sei();
}

void zvuk_zapni(void)
{
    cli();
    TIMSK1 |= _BV(OCIE1A);
    sei();
    zvukhra = 1;
}

void zvuk_vypni(void)
{
    cli(); 
    TIMSK1 &= ~_BV(OCIE1A);
    sei();
    zvukhra = 0;
}

void frekv_nastav(unsigned int frekvencia)
{
    frekv_koef = frekvencia * 65536 / FINT;
}

void ton_nacitaj(uint8_t ton)
{
    int i, value;

    if(zvukhra) 
        zvuk_vypni(); 

    switch (ton) {
        /* Štvorec */
        case 0:
            for (i = 0; i < 128; ++i) 
                wavetable[i] = 255;
            for (i = 128; i < 256; ++i) 
                wavetable[i] = 0;
            break;
        /* Sinusovka */
        case 1:
            for (i = 0; i < 256; ++i) 
                wavetable[i] = pgm_read_byte_near(sinetable + i);
            break;
        /* Píla */
        case 2:
            for (i = 0; i < 256; ++i) 
                wavetable[i] = i; 
            break;
        /* Trojuholník */
        case 3:
            for (i = 0; i < 128; ++i) 
                wavetable[i] = i * 2;
            value = 255;
            for (i = 128; i < 256; ++i) {
                wavetable[i] = value;
                value -= 2;
            }
            break;
    }
    pwm_nastav();
    zvuk_zapni(); 
}

#define TLACIDLO  PORTB5
#define REPRAK    PORTB3
#define ADCPLEX   ADC0D

int main(void)
{
    uint16_t frekv, teraz, pred;
    uint8_t toni = 0;

    disp_nastav();
    adc_nastav();
    frekv = teraz = pred = 4 * adc_citaj(ADCPLEX);
    DDRB |= (1<<REPRAK);  /* Výstup => Reproduktor */
    DDRB &= ~(1<<TLACIDLO); /* Vstup Pullup => Tlačidlo */
    PORTB |= (1<<TLACIDLO);

    ton_nacitaj(toni);

    while (1) {
        /* Zamedzenie záchvevom vo vstupe adc */       
        teraz = 4 * adc_citaj(ADCPLEX);
        if (!(PINB & (1<<TLACIDLO))) { /* Ak je tlačidlo stlačené */
            toni = (toni + 1) % 4;
            ton_nacitaj(toni);  
            _delay_ms(500); 
        }   

        if (teraz == pred) {  
            if (teraz > 32) 
                frekv = teraz;
            else 
                frekv = 0;
            frekv_nastav(frekv);
        }

        /* Zobrazovanie frekvencie na displej */
        disp_multiplex(frekv);
        pred = teraz; 
    }
}
