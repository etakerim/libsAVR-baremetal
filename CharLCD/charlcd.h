#ifndef CHAR_LCD_H_
#define CHAR_LCD_H_

#include <stdint.h>

typedef struct {
    uint8_t i2caddr;
    uint8_t cols;
    uint8_t lines;

    uint8_t rs;
    uint8_t en;
    uint8_t iopins[4];

    uint8_t bckstate;
    uint8_t dispfunc;
    uint8_t dispctrl;
    uint8_t dispmode;
} CharLCD;

#define FMTBUF_SIZE         32 

#define COMMAND             0
#define DATA                1
#define FOUR_BITS           2

#define RS      0
#define EN      2
#define D4      4
#define D5      5
#define D6      6
#define D7      7

/* Commands */
#define LCD_CLEARDISPLAY    0x01
#define LCD_RETURNHOME      0x02
#define LCD_ENTRYMODESET    0x04
#define LCD_DISPLAYCONTROL  0x08
#define LCD_CURSORSHIFT     0x10
#define LCD_FUNCTIONSET     0x20
#define LCD_SETCGRAMADDR    0x40
#define LCD_SETDDRAMADDR    0x80

/* Entry flags */
#define LCD_ENTRYRIGHT      0x00
#define LCD_ENTRYLEFT       0x02
#define LCD_ENTRYSHIFTINC   0x01
#define LCD_ENTRYSHIFTDEC   0x00

/* Control flags */
#define LCD_DISPLAYON       0x04
#define LCD_DISPLAYOFF      0x00
#define LCD_CURSORON        0x02
#define LCD_CURSOROFF       0x00
#define LCD_BLINKON         0x01
#define LCD_BLINKOFF        0x00

/* Move flags */
#define LCD_DISPLAYMOVE     0x08
#define LCD_CURSORMOVE      0x00
#define LCD_MOVERIGHT       0x04
#define LCD_MOVELEFT        0x00

/* Function set flags */
#define LCD_8BITMODE        0x10
#define LCD_4BITMODE        0x00
#define LCD_2LINE           0x08
#define LCD_1LINE           0x00
#define LCD_5x10DOTS        0x04
#define LCD_5x8DOTS         0x00

/* Backlight control */
#define LCD_BACKLIGHT       0x08
#define LCD_NOBACKLIGHT     0x00


/* Inline funkcie - optimalizacia cca 400B */
#define lcd_command(LCD, VAL)  lcd_send((LCD), (VAL), COMMAND)
#define lcd_write(LCD, VAL)    lcd_send((LCD), (VAL), DATA)
#define lcd_clear(LCD)                              \
    do { 			                                \
		lcd_command((LCD), LCD_CLEARDISPLAY);		\
		_delay_ms(2);								\
	} while (0)																

#define lcd_home(LCD)		   						\
	do { 											\
		lcd_command((LCD), LCD_RETURNHOME);			\
		_delay_ms(2);								\
	} while (0)										
#define lcd_init(LCD, ADDR)    lcd_fullconfig((LCD), (ADDR), RS, EN, D4, D5, D6, D7, 16, 2)
#define lcd_putchar(LCD, CH)   lcd_write((LCD), (CH))

void lcd_fullconfig(CharLCD *lcd, uint8_t i2caddr, uint8_t rs, uint8_t en, 
                   uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7, 
                   uint8_t cols, uint8_t lines);
void lcd_send(CharLCD *lcd, uint8_t value, uint8_t mode);

void lcd_backlight(CharLCD *lcd, uint8_t state);

void lcd_display(CharLCD *lcd, uint8_t state);

void lcd_cursor(CharLCD *lcd, uint8_t state);

void lcd_cursorpos(CharLCD *lcd, uint8_t line, uint8_t col);

void lcd_blink(CharLCD *lcd, uint8_t state);

void lcd_scroll_left(CharLCD *lcd);

void lcd_scroll_right(CharLCD *lcd);

void lcd_lefttoright(CharLCD *lcd);

void lcd_righttoleft(CharLCD *lcd);

void lcd_movecursor_right(CharLCD *lcd);

void lcd_movecursor_left(CharLCD *lcd);

void lcd_autoscroll(CharLCD *lcd, uint8_t state);
    
void lcd_runstate(CharLCD *lcd, uint8_t state);

void lcd_loadcustomchar(CharLCD *lcd, uint8_t location, uint8_t fontdata[8]);

void lcd_cputs(CharLCD *lcd, const char *str);

void lcd_puts(CharLCD *lcd, uint8_t row, uint8_t col, const char *str);

void lcd_printf(CharLCD *lcd, uint8_t row, uint8_t col, const char *fmt, ...);

#endif
