#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "i2cmaster.h"

#define TWI_CLOCK           100000L
#define TWI_BUFFER_LENGTH   32
#define TWI_STATUS          (TWSR & 0xF8)

/* TWI Status Codes */
#define TWI_START_SENT			0x08 /* Start sent */
#define TWI_REP_START_SENT		0x10 /* Repeated Start sent */
/* Master Transmitter Mode */
#define TWI_MT_SLAW_ACK			0x18 /* SLA+W sent and ACK received */
#define TWI_MT_SLAW_NACK		0x20 /* SLA+W sent and NACK received */
#define TWI_MT_DATA_ACK			0x28 /* DATA sent and ACK received */
#define TWI_MT_DATA_NACK		0x30 /* DATA sent and NACK received */
/* Master Receiver Mode */
#define TWI_MR_SLAR_ACK			0x40 /* SLA+R sent, ACK received */
#define TWI_MR_SLAR_NACK		0x48 /* SLA+R sent, NACK received */
#define TWI_MR_DATA_ACK			0x50 /* Data received, ACK returned */
#define TWI_MR_DATA_NACK		0x58 /* Data received, NACK returned */

/* Miscellaneous States */
#define TWI_LOST_ARBIT			0x38  /* Arbitration has been lost */
#define TWI_NO_RELEVANT_INFO	0xF8  /* No relevant information available */
#define TWI_ILLEGAL_START_STOP	0x00  /* Illegal START or STOP condition has been detected */
#define TWI_SUCCESS				0xFF  /* Successful transfer */


#define i2c_start()		    (TWCR = _BV(TWINT) | _BV(TWSTA) | _BV(TWEN) | _BV(TWIE)) 
#define i2c_stop()		    (TWCR = _BV(TWINT) | _BV(TWSTO) | _BV(TWEN) | _BV(TWIE)) 
#define i2c_sendtransmit()	(TWCR = _BV(TWINT) | _BV(TWEN)  | _BV(TWIE)) 
#define i2c_ack()		    (TWCR = _BV(TWINT) | _BV(TWEN)  | _BV(TWIE) | _BV(TWEA)) 
#define i2c_nack()		    (TWCR = _BV(TWINT) | _BV(TWEN)  | _BV(TWIE)) 


typedef struct {
    volatile enum {
        TWI_READY,
        TWI_INITIALIZING,
        TWI_REPEATSTARTSENT,
        TWI_MASTERTX,
        TWI_MASTERRX,
        TWI_SLAVETX,
        TWI_SLAVERX   
    } mode;  
    volatile uint8_t error;
    volatile uint8_t repstart;
} TWI_InfoData;

typedef struct {
    uint8_t buffer[TWI_BUFFER_LENGTH];
    volatile uint8_t index;
    volatile uint8_t length;
} TWI_Buffer;

TWI_InfoData twiinfo;
TWI_Buffer twi_rxdata;
TWI_Buffer twi_txdata;


uint8_t is_twiready(void)
{
    if ((twiinfo.mode == TWI_READY) | twiinfo.mode == TWI_REPEATSTARTSENT)
        return 1;
    else 
        return 0;
}

void i2c_init(void)
{
    twiinfo.mode = TWI_READY;
    twiinfo.error = 0xFF;
    twiinfo.repstart = 0;
    /* Natavenie interného pull-up pre I2C */
	PORTC |= _BV(PC5) | _BV(PC4);
    /* Set pre-scalers (no pre-scaling) */
    TWSR = 0;
    /* Set bit rate */
    TWBR = ((F_CPU / TWI_CLOCK) - 16) / 2;
    /* Enable TWI and interrupt */
    TWCR = _BV(TWIE) | _BV(TWEN);
}

void i2c_disable(void)
{
    TWCR &= ~(_BV(TWEN) | _BV(TWIE) | _BV(TWEA));
    PORTC &= ~(_BV(PC5) | _BV(PC4));
}

#define i2c_isavaiable()    (twiinfo.error != 0xFF) 
#define i2c_begintrans()    (twiinfo.error = TWI_NO_RELEVANT_INFO)
#define i2c_waitfordata()   do { while (is_twiready) { _delay_us(1);} } while(0);


uint8_t i2c_write(uint8_t *txdata, uint8_t dtlen, uint8_t repstart) 
{   
    int i;

    if (TWI_BUFFER_LENGTH < dtlen) 
        return 1;
    while (!is_twiready()) 
        _delay_us(1);
    
    /* Copy data into transmit buffer */
    twiinfo.repstart = repstart;
    twi_txdata.length = dtlen;
    twi_txdata.index = 0;
    for (i = 0; i < twi_txdata.length; i++) 
        twi_txdata.buffer[i] = data[i];

    /*  If a repeated start has been sent, then devices are already listening
     * for an address and another start does not need to be sent. */
    if (twiinfo.mode == TWI_REPEATSTARTSENT) {
        twiinfo.mode = TWI_INITIALIZING;
        TWDR = twi_txdata.buffer[twi_txdata.index++];
        i2c_sendtransmit();
    } else {
        twiinfo.mode = TWI_INITIALIZING;
        i2c_start();
    }
    return 0;
}


uint8_t i2c_requestfrom(uint8_t addr, uint8_t dtlen, uint8_t repstart)
{
    uint8_t devaddr[1];

    if (TWI_BUFFER_LENGTH < dtlen) 
        return 1;
    twi_rxdata.index = 0;
    twi_rxdata.length = dtlen;
    devaddr[0] = addr << 1 | 0x01;
    i2c_write(devaddr, 1, repstart);

    return 0;
}

uint8_t i2c_read()
{
    if (twi_rxdata.index > 0)
        return twi_rxdata.buffer[twi_rxdata.index--];
    else 
        return 0;
}

ISR(TWI_vect) 
{
    switch (TWI_STATUS) {
        /* MASTER TRANSMITTER OR WRITING ADDRESS */
        case TWI_MT_SLAW_ACK:
            twiinfo.mode = TWI_MASTERTX;
        case TWI_START_SENT:
        case TWI_MT_DATA_ACK:
            /* There is more data to send */
            if (twi_txdata.index < twi_txdata.length) {
                TWDR = twi_txdata.buffer[twi_txdata.index++];
                twiinfo.error = TWI_NO_RELEVANT_INFO;
                i2c_sendtransmit();

            /* This transmission is complete however do not release bus yet */
            } else if (twiinfo.repstart) {
                twiinfo.error = 0xFF;
                i2c_start();

            /* All transmissions are complete, exit */
            } else {
                twiinfo.mode = TWI_READY;
                twiinfo.error = 0xFF;
                i2c_stop();
            }
            break;

        /* MASTER RECEIVER */
        case TWI_MR_SLAR_ACK: 
            twiinfo.mode = TWI_MASTERRX;
            /* If there is more than one byte to be read, receive data byte and return an ACK */
            if (twi_rxdata.index < twi_rxdata.length - 1) {
                twiinfo.error = TWI_NO_RELEVANT_INFO;
                i2c_ack();

            /* When a data byte (the only data byte) is received, return NACK*/
            } else {
                twiinfo.mode = TWI_NO_RELEVANT_INFO;
                i2c_nack();
            }
            break;

        case TWI_MR_DATA_ACK:
            twi_rxdata.buffer[twi_rxdata.index++] = TWDR;
            if (twi_rxdata.index < twi_rxdata.length - 1) {
                twiinfo.error = TWI_NO_RELEVANT_INFO;
                i2c_ack();
            } else {
                twiinfo.error = TWI_NO_RELEVANT_INFO;
                i2c_nack();
            }
            break;
        
        case TWI_MR_DATA_NACK:
            twi_rxdata.buffer[twi_rxdata.index++] = TWDR;
            if (twiinfo.repstart) {
                twiinfo.error = 0xFF;
                i2c_start();
            } else {
                twiinfo.mode = TWI_READY;
                twiinfo.error = 0xFF;
                i2c_stop();
            }
            break;

        /* MT and MR common  */
        case TWI_MR_SLAR_NACK: 
        case TWI_MT_SLAW_NACK: 
        case TWI_MT_DATA_NACK:
        case TWI_LOST_ARBIT:
            if (twiinfo.repstart) {
                twiinfo.error = TWI_STATUS;
                i2c_start();
            } else {
                twiinfo.mode = TWI_READY;
                twiinfo.error = TWI_STATUS;
                i2c_stop();
            }
            break;
        
        case TWI_REP_START_SENT:
            twiinfo.mode = TWI_REPEATSTARTSENT;
            break;
        case TWI_NO_RELEVANT_INFO:
            break;
        case TWI_ILLEGAL_START_STOP:
            twiinfo.error = TWI_ILLEGAL_START_STOP;
            twiinfo.mode = TWI_READY;
            i2c_stop();
            break;
    }
}

