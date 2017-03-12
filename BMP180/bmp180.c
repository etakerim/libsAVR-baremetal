#include <math.h>
#include <string.h>
#include <avr/io.h>
#include <util/delay.h>

#include "i2cmaster.h"
#include "bmp180.h"

#define BMP185_ADDR                 0xEE      
#define BUFFER_SIZE                 3
#define AUTO_UPDATE_TEMPERATURE     true   
       
/* ---- Registers ---- */
#define CAL_AC1           0xAA   /* Calibration data (16 bits) */
#define CAL_AC2           0xAC  
#define CAL_AC3           0xAE      
#define CAL_AC4           0xB0  
#define CAL_AC5           0xB2  
#define CAL_AC6           0xB4  
#define CAL_B1            0xB6  
#define CAL_B2            0xB8  
#define CAL_MB            0xBA  
#define CAL_MC            0xBC  
#define CAL_MD            0xBE  
#define CONTROL           0xF4  /* Control register */ 
#define CONTROL_OUTPUT    0xF6  /* Output registers 0xF6=MSB, 0xF7=LSB, 0xF8=XLSB */
                 
/* Control register */
#define READ_TEMPERATURE        0x2E 
#define READ_PRESSURE           0x34 


static void bmp180_writemem(uint8_t regaddr, uint8_t val)
{
    if (i2c_start(BMP185_ADDR + I2C_WRITE) == 1) 
        return;
    i2c_write(regaddr);
    i2c_write(val);
    i2c_stop();
}

static void bmp180_readmem(uint8_t regaddr, uint8_t nbytes, uint8_t *buf) 
{
    int i;

    if (i2c_start(BMP185_ADDR + I2C_WRITE) == 1)
        return;
    i2c_write(regaddr);

    if (i2c_start(BMP185_ADDR + I2C_READ) == 1)
        return;

    for (i = 0; i < nbytes - 1; i++)
        buf[i] = i2c_readAck();
    buf[i] = i2c_readNak();
    i2c_stop();
}

static void bmp180_getcaldata(BMP180Sensor *s)
{
    bmp180_readmem(CAL_AC1, 2, s->bufi_);
    s->ac1 = ((int16_t)s->bufi_[0] << 8 | ((int16_t)s->bufi_[1]));

    bmp180_readmem(CAL_AC2, 2, s->bufi_);
    s->ac2 = ((int16_t)s->bufi_[0] << 8 | ((int16_t)s->bufi_[1]));

    bmp180_readmem(CAL_AC3, 2, s->bufi_);
    s->ac3 = ((int16_t)s->bufi_[0] << 8 | ((int16_t)s->bufi_[1]));

    bmp180_readmem(CAL_AC4, 2, s->bufi_);
    s->ac4 = ((uint16_t)s->bufi_[0] << 8 | ((uint16_t)s->bufi_[1]));

    bmp180_readmem(CAL_AC5, 2, s->bufi_);
    s->ac5 = ((uint16_t)s->bufi_[0] << 8 | ((uint16_t)s->bufi_[1]));

    bmp180_readmem(CAL_AC6, 2, s->bufi_);
    s->ac6 = ((uint16_t)s->bufi_[0] << 8 | ((uint16_t)s->bufi_[1])); 

    bmp180_readmem(CAL_B1, 2, s->bufi_);
    s->b1 = ((int16_t)s->bufi_[0] << 8 | ((int16_t)s->bufi_[1])); 

    bmp180_readmem(CAL_B2, 2, s->bufi_);
    s->b2 = ((int16_t)s->bufi_[0] << 8 | ((int16_t)s->bufi_[1])); 

    bmp180_readmem(CAL_MB, 2, s->bufi_);
    s->mb = ((int16_t)s->bufi_[0] << 8 | ((int16_t)s->bufi_[1]));

    bmp180_readmem(CAL_MC, 2, s->bufi_);
    s->mc = ((int16_t)s->bufi_[0] << 8 | ((int16_t)s->bufi_[1]));

    bmp180_readmem(CAL_MD, 2, s->bufi_);
    s->md = ((int16_t)s->bufi_[0] << 8 | ((int16_t)s->bufi_[1])); 
}


void bmp180_init(BMP180Sensor *s)
{
    bmp180_initfull(s, MODE_STANDARD, 0, true);
}

void bmp180_initfull(BMP180Sensor *s, uint8_t bmpmode, int32_t val, bool isalt)
{
    PORTC |= _BV(PC5) | _BV(PC4);
	i2c_init();
    memset(s, 0, sizeof(*s));
    bmp180_getcaldata(s);
    bmp180_calctruetemp(s);
    s->oss = bmpmode;
    if (isalt)
        bmp180_setlocalabsalt(s, val);
    else
        bmp180_setlocalpressure(s, val);
}

void bmp180_setaltoffset(BMP180Sensor *s, int32_t cm)
{
    s->cm_offset = cm;
}

void bmp180_sethpaoffset(BMP180Sensor *s, int32_t pa)
{
    s->pa_offset = pa;
}

void bmp180_zerocal(BMP180Sensor *s, int32_t pa, int32_t cm)
{
    bmp180_setaltoffset(s, cm - s->param_cm);
    bmp180_sethpaoffset(s, pa - s->param_datum);
}

void bmp180_setlocalabsalt(BMP180Sensor *s, int32_t cm)
{
    s->param_cm = cm;
    s->param_datum = bmp180_getpressure(s);
}

void bmp180_setlocalpressure(BMP180Sensor *s, int32_t pa)
{
    s->param_datum = pa;
    s->param_cm = bmp180_getaltitute(s);
}

void bmp180_calctruetemp(BMP180Sensor *s) 
{
    long ut, x1, x2;
    
    bmp180_writemem(CONTROL, READ_TEMPERATURE);
    _delay_ms(5);                           /* min. 4.5ms read Temp delay */
    bmp180_readmem(CONTROL_OUTPUT, 2, s->bufi_); 
    ut = ((long)s->bufi_[0] << 8 | ((long)s->bufi_[1]));    

    x1 = ((long)ut - s->ac6) * s->ac5 >> 15;
    x2 = ((long)s->mc << 11) / (x1 + s->md);
    s->b5 = x1 + x2;
}

long bmp180_gettemperature(BMP180Sensor *s)
{
    bmp180_calctruetemp(s);
    return ((s->b5 + 8) >> 4);
}

long bmp180_getaltitute(BMP180Sensor *s)
{
    /* converting from float to int32_t truncates toward zero, (max 1 cm error) . */
    long truepressure = bmp180_calctruepressure(s);
    return (4433000 * (1 - pow((truepressure / (float)s->param_datum), 0.1903)) 
            + s->cm_offset);
}

long bmp180_getpressure(BMP180Sensor *s)
{
    long truepressure = bmp180_calctruepressure(s);
    return (truepressure / pow((1 - (float)s->param_cm / 4433000), 5.255) 
            + s->pa_offset);
}

long bmp180_calctruepressure(BMP180Sensor *s)
{
    long up,x1,x2,x3,b3,b6,p;
    unsigned long b4,b7;
    int32_t tmp;

    #if AUTO_UPDATE_TEMPERATURE
    bmp180_calctruetemp(s);         
    #endif

    /* Read raw pressure from sensor */
    bmp180_writemem(CONTROL, READ_PRESSURE + (s->oss << 6));
    _delay_ms(26); ////pressure_waittime[oss]);
    bmp180_readmem(CONTROL_OUTPUT, 3, s->bufi_);
    up = ((((long)s->bufi_[0] << 16) | ((long)s->bufi_[1] << 8) 
             | ((long)s->bufi_[2])) >> (8 - s->oss));
    
    /* Calculate true pressure */
    b6 = s->b5 - 4000;            
    x1 = (s->b2 * (b6 * b6 >> 12)) >> 11;
    x2 = s->ac2 * b6 >> 11;
    x3 = x1 + x2;
    tmp = s->ac1;
    tmp = (tmp * 4 + x3) << s->oss;
    b3 = (tmp + 2) >> 2;
    x1 = s->ac3 * b6 >> 13;
    x2 = (s->b1 * (b6 * b6 >> 12)) >> 16;
    x3 = ((x1 + x2) + 2) >> 2;
    b4 = (s->ac4 * (uint32_t) (x3 + 32768)) >> 15;
    b7 = ((uint32_t)up - b3) * (50000 >> s->oss);
    p = b7 < 0x80000000 ? (b7 << 1) / b4 : (b7 / b4) << 1;
    x1 = (p >> 8) * (p >> 8);
    x1 = (x1 * 3038) >> 16;
    x2 = (-7357 * p) >> 16;

    return (p + ((x1 + x2 + 3791) >> 4));
}

