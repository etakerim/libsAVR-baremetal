#ifndef RTC_DS1307_H_
#define RTC_DS1307_H_

#include <stdint.h>

typedef struct {
    uint8_t tm_sec;
    uint8_t tm_min;
    uint8_t tm_hour;
    uint8_t tm_wday;
    uint8_t tm_mday;
    uint8_t tm_mon;
    uint8_t tm_year;
} RTCTime;

typedef enum {
    RTC_SEC = 0x00,
    RTC_MIN = 0x01,
    RTC_HOUR = 0x02,
    RTC_WDAY = 0x03,
    RTC_MDAY = 0x04,
    RTC_MON = 0x05,
    RTC_YEAR = 0x06
} RTCRegister;

typedef enum {NONE, _1HZ, _4KHZ, _8KHZ, _32KHZ} RTCSqwave;

uint8_t bcd_to_dec(uint8_t bcd);
uint8_t dec_to_bcd(uint8_t d);

void rtc_init(void);
int8_t rtc_read(RTCTime *clk);
int8_t rtc_write(const RTCTime *clk);

int8_t rtc_readreg(RTCRegister reg);
int8_t rtc_writereg(RTCRegister reg, uint8_t writebyte);

int8_t rtc_sqwout(RTCSqwave mode);

#endif
