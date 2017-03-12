#ifndef BMP180_H_
#define BMP180_H_

#include <stdbool.h>

#define MODE_ULTRA_LOW_POWER    0 
#define MODE_STANDARD           1 
#define MODE_HIGHRES            2 
#define MODE_ULTRA_HIGHRES      3 

/* Mean Sea Level Pressure = 1013.25 hPA (1hPa = 100Pa = 1mbar) */
#define MSLP                    101325

typedef struct {
    int16_t ac1, ac2, ac3, b1, b2, mb, mc, md;
    uint16_t ac4, ac5, ac6;
    
    int16_t oss;
    int32_t cm_offset, pa_offset;
    int32_t param_datum, param_cm;

    int32_t b5, old_ema;
    uint8_t bufi_[3];
} BMP180Sensor;


void bmp180_init(BMP180Sensor *s);

void bmp180_initfull(BMP180Sensor *s, uint8_t bmpmode, int32_t val, bool isalt);

void bmp180_setaltoffset(BMP180Sensor *s, int32_t cm);

void bmp180_sethpaoffset(BMP180Sensor *s, int32_t pa);

void bmp180_zerocal(BMP180Sensor *s, int32_t pa, int32_t cm);

void bmp180_setlocalabsalt(BMP180Sensor *s, int32_t cm);

void bmp180_setlocalpressure(BMP180Sensor *s, int32_t pa);


void bmp180_calctruetemp(BMP180Sensor *s);

long bmp180_gettemperature(BMP180Sensor *s);

long bmp180_getaltitute(BMP180Sensor *s);

long bmp180_getpressure(BMP180Sensor *s);

long bmp180_calctruepressure(BMP180Sensor *s);


#endif
