#include <stdint.h>

#include <avr/io.h>
#include "../I2C/i2cmaster.h"
#include "rtc.h"

#define DS1307ADDR      0xD0
 /* RTC control register names */
#define OUT             (1 << 7)
#define SQWE            (1 << 4)
#define RS1             (1 << 1)
#define RS0             (1 << 0)


uint8_t bcd_to_dec(uint8_t bcd)
{
    return ((bcd & 0xF0) >> 4) * 10 + (bcd & 0x0F);
}

uint8_t dec_to_bcd(uint8_t d)
{
    return ((d / 10) << 4) + (d % 10);
}

void rtc_init(void)
{
    PORTC |= (1 << PC4) | (1 << PC5);
    i2c_init();
}

int8_t rtc_readreg(RTCRegister reg)
{
    int8_t value;
    
    if (i2c_start(DS1307ADDR + I2C_WRITE) == 1)
        return -1;
    i2c_write((uint8_t)reg);
    i2c_stop();

    if (i2c_start(DS1307ADDR + I2C_READ) == 1)
        return -1;
    value = bcd_to_dec(i2c_readNak());
    i2c_stop();
    return value;
}

int8_t rtc_writereg(RTCRegister reg, uint8_t writebyte)
{
    if (i2c_start(DS1307ADDR + I2C_WRITE) == 1)
        return -1;
    i2c_write((uint8_t)reg);
    i2c_write(dec_to_bcd(writebyte));
    i2c_stop();
    return 0;
}


int8_t rtc_read(RTCTime *clk)
{
    if (i2c_start(DS1307ADDR + I2C_WRITE) == 1)
        return -1;
    i2c_write(0x00);
    i2c_stop();

	if (i2c_start(DS1307ADDR + I2C_READ) == 1)
        return -1;
	clk->tm_sec = bcd_to_dec(i2c_readAck());
    clk->tm_min = bcd_to_dec(i2c_readAck());
    clk->tm_hour = bcd_to_dec(i2c_readAck());
    clk->tm_wday = bcd_to_dec(i2c_readAck());
    clk->tm_mday = bcd_to_dec(i2c_readAck());
    clk->tm_mon = bcd_to_dec(i2c_readAck());
    clk->tm_year = bcd_to_dec(i2c_readNak());
	i2c_stop();

    return 0;
}

int8_t rtc_write(const RTCTime *clk)
{
    if (i2c_start(DS1307ADDR + I2C_WRITE) == 1)
        return -1;
    i2c_write(0x00);
	i2c_write((dec_to_bcd(clk->tm_sec)));
    i2c_write((dec_to_bcd(clk->tm_min)));
    i2c_write((dec_to_bcd(clk->tm_hour)));
    i2c_write((dec_to_bcd(clk->tm_wday)));
    i2c_write((dec_to_bcd(clk->tm_mday)));
    i2c_write((dec_to_bcd(clk->tm_mon)));
    i2c_write((dec_to_bcd(clk->tm_year)));
	i2c_stop();

    return 0;
}

int8_t rtc_sqwout(RTCSqwave mode)
{ 
    uint8_t ctrlreg = 0x0;

    switch (mode) {
    case NONE:
        break;
    case _1HZ:
        ctrlreg |= SQWE;
        break;
    case _4KHZ:
        ctrlreg |= SQWE | RS0;
        break;
    case _8KHZ:
        ctrlreg |= SQWE | RS1;
        break;
    case _32KHZ:
        ctrlreg |= SQWE | RS0 | RS1;
        break;   
    }

    if (i2c_start(DS1307ADDR + I2C_WRITE) == 1)
        return -1;
    i2c_write(0x07);
    i2c_write(ctrlreg);
    i2c_stop();

	return 0;
}

	
	

