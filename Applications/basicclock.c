#include "rtc.h"
#include "charlcd.h"
#include <util/delay.h>

int main(void)
{
    CharLCD disp;
    RTCTime tm;

    lcd_init(&disp, 0x7E);

    while (1) {
        rtc_read(&tm);
        lcd_printf(&disp, 0, 0, "%02d.%02d.%4d", tm.tm_mday, tm.tm_mon, tm.tm_year + 2000);
       	lcd_printf(&disp, 1, 2, "%02d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec); 
		_delay_ms(500); 
    }
}
