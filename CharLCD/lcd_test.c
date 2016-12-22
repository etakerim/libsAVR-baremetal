#include "charlcd.h"

int main(void)
{
    CharLCD disp;
	char font[] = {4,4,14,14,31,31,4,4};

    lcd_init(&disp, 0x7E);
	lcd_loadcustomchar(&disp, 0, font); 
	lcd_cursorpos(&disp, 0,0);
	lcd_putchar(&disp, 0);
	lcd_cputs(&disp, "Vianoce 2016");
    lcd_putchar(&disp, 0);
    while(1);
}
