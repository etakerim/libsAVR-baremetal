/* Popis: Ovládanie znakového LCD displeja s kontrolérom Hitachi HD44780
 *        cez I2C expander - PCF8574
 * Technická poznámka: Časovania sú veľmi dôležité, pretože ak displej ešte
 *  neskončil spracovanie predchádzajúceho príkazu bude ignorovať alebo 
 *  skomolí ďalší
 *
 * Autor:   Miroslav Hájek
 * Dátum:   22.12.2016
 * Licencia: GNU LGPLv2
 */ 

#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <avr/io.h>
#include <util/delay.h>
#include "../I2C/i2cmaster.h"
#include "charlcd.h"

static const uint8_t CURSOR_OFFSET[] = {0x00, 0x40, 0x14, 0x54};
static const uint8_t CURSOR_OFFSET_16x4[] = {0x00, 0x40, 0x10, 0x50};

static void lcd_strobe(CharLCD *lcd, uint8_t data)
{
    i2c_start(lcd->i2caddr+I2C_WRITE);
    i2c_write(data | lcd->en);
    i2c_stop();

    i2c_start(lcd->i2caddr+I2C_WRITE);
    i2c_write(data & ~lcd->en);
    i2c_stop();
}

static void lcd_write4bits(CharLCD *lcd, uint8_t value, uint8_t mode)
{
    uint8_t i;
    uint8_t pinmapval = 0;

    for (i = 0; i < 4; i++) {    
        if ((value & 0x1) == 1)
            pinmapval |= lcd->iopins[i];
        value = (value >> 1);
    }

    if (mode == DATA) 
        mode = lcd->rs;
    pinmapval |= mode | lcd->bckstate;
    lcd_strobe(lcd, pinmapval);
}

void lcd_send(CharLCD *lcd, uint8_t value, uint8_t mode)
{
    if (mode == FOUR_BITS) {
        lcd_write4bits(lcd, (value & 0x0F), COMMAND);
    } else {
        lcd_write4bits(lcd, (value >> 4), mode);
        lcd_write4bits(lcd, (value & 0x0F), mode);
    }
}

void lcd_fullconfig(CharLCD *lcd, uint8_t i2caddr, uint8_t rs, uint8_t en, 
                   uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7, 
                   uint8_t cols, uint8_t lines)
{
    lcd->i2caddr = i2caddr;
    lcd->rs      = (1 << rs);
    lcd->en      = (1 << en);
    lcd->cols    = cols;
    lcd->lines   = lines;
    lcd->iopins[0] = (1 << d4);
    lcd->iopins[1] = (1 << d5);
    lcd->iopins[2] = (1 << d6);
    lcd->iopins[3] = (1 << d7);

    lcd->bckstate = LCD_BACKLIGHT;
    lcd->dispfunc = LCD_4BITMODE | LCD_2LINE | LCD_5x8DOTS;
    lcd->dispctrl = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
    lcd->dispmode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDEC;

	/* Natavenie interného pull-up pre I2C */
	PORTC |= (1 << PC5) | (1 << PC4);
    
    i2c_init();
    _delay_ms(100);
    lcd_send(lcd, 0x03, FOUR_BITS); 
    _delay_us(4500);
    lcd_send(lcd, 0x03, FOUR_BITS);
    _delay_us(150);
    lcd_send(lcd, 0x03, FOUR_BITS);
    _delay_us(150);
    lcd_send(lcd, 0x02, FOUR_BITS);
    _delay_us(150);
    lcd_command(lcd, LCD_FUNCTIONSET | lcd->dispfunc);
    lcd_command(lcd, LCD_DISPLAYCONTROL | lcd->dispctrl);
    lcd_command(lcd, LCD_ENTRYMODESET | lcd->dispmode);
    lcd_clear(lcd);
    lcd_backlight(lcd, 1);
}

void lcd_backlight(CharLCD *lcd, uint8_t state)
{
    if (state) 
        lcd->bckstate = LCD_BACKLIGHT;
    else 
        lcd->bckstate = LCD_NOBACKLIGHT;
    i2c_start(lcd->i2caddr+I2C_WRITE);
    i2c_write(lcd->bckstate);
    i2c_stop();
}


void lcd_display(CharLCD *lcd, uint8_t state)
{
    if (state)
        lcd->dispctrl |= LCD_DISPLAYON;
    else 
        lcd->dispctrl &= ~LCD_DISPLAYOFF;
    lcd_command(lcd, LCD_DISPLAYCONTROL | lcd->dispctrl);
}

void lcd_cursor(CharLCD *lcd, uint8_t state)
{
    if (state)
        lcd->dispctrl |= LCD_CURSORON;
    else 
        lcd->dispctrl &= ~LCD_CURSORON;
    lcd_command(lcd, LCD_DISPLAYCONTROL | lcd->dispctrl);
}

void lcd_cursorpos(CharLCD *lcd, uint8_t line, uint8_t col)
{
    /* Ak je 16x2 display tak rozsah je 0 až n-1*/
    if (lcd->lines == 2 && lcd->cols == 16)
        lcd_command(lcd, LCD_SETDDRAMADDR | (col + CURSOR_OFFSET_16x4[line]));
    else 
        lcd_command(lcd, LCD_SETDDRAMADDR | (col + CURSOR_OFFSET[line]));
}

void lcd_blink(CharLCD *lcd, uint8_t state)
{
    if (state)
        lcd->dispctrl |= LCD_BLINKON; 
    else  
        lcd->dispctrl &= ~LCD_BLINKON;
    lcd_command(lcd, LCD_DISPLAYCONTROL | lcd->dispctrl);
}

void lcd_scroll_left(CharLCD *lcd)
{
    lcd_command(lcd, LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}

void lcd_scroll_right(CharLCD *lcd)
{
    lcd_command(lcd, LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

void lcd_lefttoright(CharLCD *lcd)
{
    lcd->dispmode |= LCD_ENTRYLEFT;
    lcd_command(lcd, LCD_ENTRYMODESET | lcd->dispmode);
}

void lcd_righttoleft(CharLCD *lcd)
{
    lcd->dispmode &= ~LCD_ENTRYRIGHT;
    lcd_command(lcd, LCD_ENTRYMODESET | lcd->dispmode);
}

void lcd_movecursor_right(CharLCD *lcd)
{
    lcd_command(lcd, LCD_CURSORSHIFT | LCD_CURSORMOVE | LCD_MOVERIGHT);
}

void lcd_movecursor_left(CharLCD *lcd)
{
    lcd_command(lcd, LCD_CURSORSHIFT | LCD_CURSORMOVE | LCD_MOVELEFT);
}

void lcd_autoscroll(CharLCD *lcd, uint8_t state)
{
    if (state)
        lcd->dispmode |= LCD_ENTRYSHIFTINC;
    else 
        lcd->dispmode &= ~LCD_ENTRYSHIFTINC;
    lcd_command(lcd, LCD_ENTRYMODESET | lcd->dispmode);
}

void lcd_runstate(CharLCD *lcd, uint8_t state)
{
    lcd_display(lcd, state);
    lcd_backlight(lcd, state);
}

void lcd_loadcustomchar(CharLCD *lcd, uint8_t location, uint8_t fontdata[8])
{
    uint8_t i;

    location &= 0x7;
    lcd_command(lcd, LCD_SETCGRAMADDR | (location << 3));
    _delay_us(30);

    for (i = 0; i < 8; i++) {
        lcd_write(lcd, fontdata[i]);
        _delay_us(40);
    }
}

void lcd_cputs(CharLCD *lcd, const char *str)
{
    while (*str != '\0')
        lcd_write(lcd, *str++);
}

void lcd_puts(CharLCD *lcd, uint8_t row, uint8_t col, const char *str)
{
    lcd_cursorpos(lcd, row, col);
    lcd_cputs(lcd, str);
}

void lcd_printf(CharLCD *lcd, uint8_t row, uint8_t col, const char *fmt, ...) 
{
    char buf[FMTBUF_SIZE];
    va_list args;

	lcd_cursorpos(lcd, row, col);

    va_start(args, fmt);
    vsnprintf(buf, FMTBUF_SIZE, fmt, args);
    va_end(args);
    lcd_cputs(lcd, buf);
}

