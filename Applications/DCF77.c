#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <stdbool.h>
#include "uart.h"

#define DCFDDR                  DDRD
#define DCFPORT                 PORTD
#define DCFPIN                  PD2
#define DCFREAD                 PIND
#define io_read(PORT, PIN)      ((PORT) & _BV(PIN))
#define set_bit(P, B)           ((P) |= _BV(B))
#define clr_bit(P, B)           ((P) &= ~_BV(B))
#define TM_4MS                  (255 - 250)

/* Štruktúra obsahuje spracované dáta z bežiaceho cirkulárneho bitového poľa
 *  [/Dalo sa rozložiť aj pomocou pretypovania na bitovú štruktúru. Plusom
 *  toho prístupu je ušetrenie pamäti na druhej strane je prístup k bitom na
 *  AVR zložitejší (asm) a tiež endianita a zarovanie je nešpecifikované!]
 * Zvolená obyčajná štruktúra má veľkosť ~13 bytov (+5B) a obsahuje len
 * využiteľné dáta, čiže môže byť poskytnutá ako API. Dáta sú už prevedené z BCD.
 */
typedef struct {
    uint16_t weather;  /* Bity 1 - 14 */
    uint8_t  isCEST;   /* Bit 17 */
    uint8_t  isCET;    /* Bit 18 */
    uint8_t  tm_min;   /* Bity 21 - 27 (7) */
    uint8_t  p1;       /* Bit 28 */
    uint8_t  tm_hour;  /* Bity 29 - 34 (6) */
    uint8_t  p2;       /* Bit 35 */
    uint8_t  tm_mday;  /* Bity 36 - 41 (6) */
    uint8_t  tm_wday;  /* Bity 42 - 44 (3) */
    uint8_t  tm_mon;   /* Bity 45 - 49 (5) */
    uint8_t  tm_year;  /* Bity 50 - 57 (8) */
    uint8_t  p3;       /* Bit 58  */
} DCF77;

typedef enum {
    DCF77_OK = 0,
    DCF77_DATA_NOT_READY,
    DCF77_PARITY_ERR
} Error;


/* ------------------------------------------ */
/*volatile uint64_t cmpbuf2; */
static volatile uint64_t cmpbuf1;
static volatile uint64_t dcfbuff;
static volatile uint8_t i_buf;

static volatile uint16_t nabeh;
static volatile bool jepulz;
static volatile bool jesec;
static volatile bool jeready;

void close_buffer(void)
{
    cmpbuf1 = dcfbuff;
    if (i_buf == 59)
        jeready = 1;
    uart_putc('\n');
    dcfbuff = 0;
    i_buf = 0;
}

ISR(TIMER0_OVF_vect)
{
    TCNT0 = TM_4MS;
    nabeh += 4;

    if (io_read(DCFREAD, DCFPIN)) {
        /* __-- Nabezna hrana */
        if (!jepulz) {
            jepulz = true;
            if (nabeh > 750 && nabeh < 1200) {
               jesec = true;            /* 1s */
            } else if (nabeh > 1700 && nabeh < 2300) {
                jesec = true;           /* 1 min - znacka */
                close_buffer();
            }
            nabeh = 0;
        }

    } else {
        /* --__ Zostupna hrana */
        if (jepulz) {
            jepulz = false;
            if (jesec) {
                jesec = false;
                if (nabeh > 70 && nabeh < 130) {
                    uart_putc('0');
                    dcfbuff |= (0 << i_buf); /* 0 */
                } else if (nabeh > 170 && nabeh < 230) {
                    dcfbuff |= (1 << i_buf); /* 1 */
                    uart_putc('1');
                }

                if (++i_buf > 59)
					close_buffer();
            }
        }
    }
}

/* TODO: Optimalizovať */
static uint8_t parity_byte(uint8_t w)
{
    w ^= (w >> 4 | w << 4);
    w ^= (w >> 2);
    w ^= (w >> 1);
    return w & 1;
}

static uint8_t parity_32bit(uint32_t w)
{
    w ^= (w >> 1);
    w ^= (w >> 2);
    w ^= (w >> 4);
    w ^= (w >> 8);
    w ^= (w >> 16);
    return w & 1;
}

static uint8_t bcd2int(uint8_t bcd)
{
    return bcd - 6 * (bcd >> 4);
}

uint8_t dcf77_dekomprimuj(DCF77 *data)
{
    uint64_t input = cmpbuf1;
    uint8_t p1_calc, p2_calc, p3_calc;

    if (!jeready)
        return DCF77_DATA_NOT_READY;

    p1_calc = parity_byte((cmpbuf1 >> 21) & 0x7F);
    p2_calc = parity_byte((cmpbuf1 >> 29) & 0x3F);
    p3_calc = parity_32bit((cmpbuf1 >> 36) & 0x7FFFFF);

    data->weather = (input >> 1) & 0x3FFF;
    input >>= 17;
    data->isCEST  = input & 0x1;
    data->isCET   = (input >> 1) & 0x1;
    input >>= 4;
    data->tm_min  = bcd2int(input & 0x7F);
    input >>= 7;
    data->p1      = input & 0x1;
    data->tm_hour = bcd2int((input >> 1) & 0x3F);
    input >>= 7;
    data->p2      = input & 0x1;
    data->tm_mday = bcd2int((input >> 1) & 0x3F);
    input >>= 7;
    data->tm_wday = bcd2int(input & 0x7);
    input >>= 3;
    data->tm_mday = bcd2int(input & 0x1F);
    input >>= 5;
    data->tm_year = bcd2int(input & 0xFF);
    input >>= 8;
    data->p3      = input & 0x1;

    if (p1_calc != data->p1 || p2_calc != data->p2 || p3_calc != data->p3) {
        return DCF77_PARITY_ERR;
    }

    return DCF_OK; /* DCF_SYNC ked je druha buffer porovnana a ready */
}


void dcf77_nastav(void)
{
    clr_bit(DCFDDR, DCFPIN); /* Vstup */
    set_bit(TCCR0B, CS02);   /* Predelič 256 */
    TCNT0 = TM_4MS;
    set_bit(TIMSK0, TOIE0);
}

int main(void)
{
    dcf77_nastav();
    DDRB |= _BV(PB5);
    uart_init(UART_BAUD_SELECT(9600, F_CPU));
    sei();
    while (1)
       ;
}
