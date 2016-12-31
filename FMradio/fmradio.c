#include <avr/io.h>
#include <stdint.h>
#include <string.h>
#include <util/delay.h>
#include "fmradio.h"
#include "i2cmaster.h"

/* Názvy registrov */
#define DEVICEID        0x00
#define CHIPID          0x01
#define POWERCFG        0x02
#define CHANNEL         0x03
#define SYSCONFIG1      0x04
#define SYSCONFIG2      0x05
#define SYSCONFIG3      0x06
#define TEST1           0x07
#define TEST2           0x08
#define BOOTCONFIG      0x09
#define STATUSRSSI      0x0A
#define READCHAN        0x0B
#define RDSA            0x0C
#define RDSB            0x0D
#define RDSC            0x0E
#define RDSD            0x0F

/* Register 0x02 - POWERCFG */
#define DSMUTE          15
#define DMUTE           14
#define SETMONO         13
#define RDSM            11
#define SKMODE          10
#define SEEKUP          9
#define SEEK            8
#define ENABLE          0

/* Register 0x03 - CHANNEL */
#define TUNE            15

/* Register 0x04 - SYSCONFIG1 */
#define RDSIEN          15
#define STCIEN          14
#define RDS             12
#define DE              11
#define AGCD            10
#define BLNDADJ         6
#define GPIO3           4
#define GPIO2           2
#define GPIO1           0

/* Register 0x05 - SYSCONFIG2 */
#define SEEKTH_MASK     0xFF00
#define SEEKTH_MIN      0x0000
#define SEEKTH_MID      0x1000
#define SEEKTH_MAX      0x7F00
#define SPACE1          5
#define SPACE0          4

/* Register Ox06 - SYSCONFIG3 */
#define SKSNR_MASK      0x00F0
#define SKSNR_OFF       0x0000
#define SKSNR_MIN       0x0010
#define SKSNR_MID       0x0030
#define SKSNR_MAX       0x0070

#define SKCNT_MASK      0x000F
#define SKCNT_OFF       0x0000
#define SKCNT_MIN       0x000F
#define SKCNT_MID       0x0003
#define SKCNT_MAX       0x0001

/* Register 0x0A - STATUSRSSI */
#define RDSR            0x8000   /* RDS ready */
#define STC             0x4000   /* Seek Tune Complete */
#define SFBL            0x2000   /* Seek Fail Band Limit */
#define AFCRL           0x1000
#define RDSS            0x0800   /* RDS synchronized */
#define SI              0x0100   /* Stereo Indicator */
#define RSSI            0x00FF 

#define FREQLOW         8750
#define FREQHIGH        10800
#define FREQSTEP        10

static void read_registers(FMRadio *fm)
{
    uint8_t busdata[32];
    uint8_t i, x;

    /* Priame čítanie z I2C zbernice */
    i2c_start(fm->i2caddr + I2C_READ);
    for (i = 0; i < 31; i++) 
        busdata[i] = i2c_readAck();
    busdata[31] = i2c_readNak();
    i2c_stop();
    i = 0;

    /* Preklápanie surových hodnôt do správnej podoby */
    for (x = 0x0A; ; x++) { 
        if(x == 0x10) 
            x = 0; 
        /* We get 32 bytes, but we need 16 2-byte pairs (1 reg = 16 bits) */
        fm->registers[x] = (busdata[i] << 8) + busdata[i + 1];
        i += 2; 
                                        
        if(x == 0x09) 
            break; 
    } 
}

static void write_registers(FMRadio *fm)
{
    /* Write the current 9 control registers (0x02 to 0x07) to the Si4703
     * The Si4703 assumes you are writing to 0x02 first, then increments */
    uint8_t regspot;

    i2c_start(fm->i2caddr + I2C_WRITE);
    for (regspot = 0x02 ; regspot < 0x08 ; regspot++) {
        i2c_write(fm->registers[regspot] >> 8);
        i2c_write(fm->registers[regspot] & 0x00FF);
    }
    i2c_stop();
}

static void waitforset(FMRadio *fm)
{
    do {
        read_registers(fm);
    } while ((fm->registers[STATUSRSSI] & STC) == 0);
  
  
    read_registers(fm);
    fm->registers[POWERCFG] &= ~(1 << SEEK);
    fm->registers[CHANNEL]  &= ~(1 << TUNE); 
    write_registers(fm);   
}

void fmradio_init(FMRadio *fm, uint8_t addr, volatile uint8_t *rstport, 
                volatile uint8_t *rstddr, uint8_t rst) 
{
    fm->i2caddr = addr;
    fm->rstport = rstport;
    fm->rstpin = rst;
    fm->ismono = 0;
    
    *rstddr |= (1 << fm->rstpin);  /* Reset ako OUTPUT */
}

void fmradio_poweron(FMRadio *fm)
{
    /* Resetovanie Si4703 */
    DDRC |= (1 << PC4);         /* SDIO ako OUTPUT */
    PORTC &= ~(1 << PC4);       /* SDIO - low */
    *fm->rstport &= ~(1 << fm->rstpin);  /* RESET - low */
    _delay_ms(1);
    *fm->rstport |= (1 << fm->rstpin);   /* RESET - high */
    _delay_ms(1);

    /* Započatie I2C komunikácie a nastavenie PULLUP */
    DDRC &= ~((1 << PC4) | (1 << PC5));
    PORTC |= (1 << PC4) | (1 << PC5);
    i2c_init();

    read_registers(fm);
    fm->registers[0x07] = 0x8100;   /* Enable the oscillator, from AN230 page 12 */
    write_registers(fm); 
    
    _delay_ms(500);                 /* Wait for clock to settle - from AN230 page 9 */

    read_registers(fm);
    fm->registers[POWERCFG] = 0x4001;           /* Enable the IC */
    fm->registers[SYSCONFIG1] |= (1 << RDS);    /* Enable RDS */

    fm->registers[SYSCONFIG1] |= (1 << DE);     /* 50kHz Europe setup */
    fm->registers[SYSCONFIG2] |= (1 << SPACE0); /* 100kHz channel spacing */

    fm->registers[SYSCONFIG2] &= 0xFFF0;        /* Clear volume bits */
    fm->registers[SYSCONFIG2] |= 0x0001;        /* Set volume to lowest */
    fm->registers[SYSCONFIG2] |= SEEKTH_MID;    /* Set volume */

    /* Set seek parameters */
    fm->registers[SYSCONFIG3] &= ~(SKSNR_MASK); /* Clear seek mask bits */
    fm->registers[SYSCONFIG3] |= SKSNR_MID;     /* Set volume */
    fm->registers[SYSCONFIG3] &= ~(SKCNT_MASK); /* Clear seek mask bits */
    fm->registers[SYSCONFIG3] |= SKCNT_MID;     /* Set volume */

    write_registers(fm);
    _delay_ms(110);
}

void fmradio_shutdown(FMRadio *fm)
{
    read_registers(fm);
    /* Powerdown IC as defined in AN230 page 13 rev 0.9 */
    fm->registers[TEST1] = 0x7C04;
    fm->registers[POWERCFG] = 0x002A;
    fm->registers[SYSCONFIG1] = 0x0041;
    write_registers(fm);
}


void fmradio_setvolume(FMRadio *fm, uint8_t volume)
{
    if (volume > 15) 
        volume = 15;
        
    read_registers(fm);    
    fm->registers[SYSCONFIG2] &= 0xFFF0;    /* Clear volume bits */
    fm->registers[SYSCONFIG2] |= volume;    /* Set new volume */    
    write_registers(fm); 
} 

uint8_t fmradio_getvolume(FMRadio *fm)
{
    read_registers(fm);
    return (fm->registers[SYSCONFIG2] & 0x000F);
}

void fmradio_setfrequency(FMRadio *fm, uint16_t newfreq)
{
    uint16_t newchannel;
    
    if (newfreq < FREQLOW)  
        newfreq = FREQLOW;
    else if (newfreq > FREQHIGH)   
        newfreq = FREQHIGH;

    read_registers(fm);
    newchannel = (newfreq - FREQLOW) / FREQSTEP;

    /* These steps come from AN230 page 20 rev 0.5 */
    fm->registers[CHANNEL] &= 0xFE00;       /* Clear out the channel bits */
    fm->registers[CHANNEL] |= newchannel;      /* Mask in the new channel */
    fm->registers[CHANNEL] |= (1 << TUNE);  /* Set the TUNE bit to start */
  
    write_registers(fm);
    waitforset(fm);
}

uint16_t fmradio_getfrequency(FMRadio *fm)
{
    read_registers(fm);
    /* Mask everything, but lower 10 bits */
    return (((fm->registers[READCHAN] & 0x03FF) * FREQSTEP) + FREQLOW);
}

#define fmradio_seekup(fm)      fmradio_seek((fm), 1)
#define fmradio_seekdown(fm)    fmradio_seek((fm), 0)

void fmradio_seek(FMRadio *fm, uint8_t is_seekup) 
{
    uint16_t reg;

    read_registers(fm);
    /* not wrapping around. */
    reg = fm->registers[POWERCFG] & ~((1 << SKMODE) | (1 << SEEKUP));

    if (is_seekup)
        reg |= (1 << SEEKUP); /*  Set the Seek-up bit */

    reg |= (1 << SEEK);       /*  Start seek now */

    /* save the registers and start seeking... */
    fm->registers[POWERCFG] = reg;
    write_registers(fm);
    waitforset(fm);   
}

void fmradio_setmono(FMRadio *fm, uint8_t state)
{
    read_registers(fm);
    if (state) 
        fm->registers[POWERCFG] |= (1 << SETMONO); 
    else 
        fm->registers[POWERCFG] &= ~(1 << SETMONO);
    write_registers(fm); 
} 

void fmradio_setmute(FMRadio *fm, uint8_t state)
{
	read_registers(fm);
    if (state) 
        fm->registers[POWERCFG] &= ~(1 << DMUTE); 
    else 
        fm->registers[POWERCFG] |= (1 << DMUTE);     
    write_registers(fm);
}

void fmradio_setsoftmute(FMRadio *fm, uint8_t state)
{
    read_registers(fm);
    if (state) 
        fm->registers[POWERCFG] &= ~(1 << DSMUTE); 
    else 
        fm->registers[POWERCFG] |= (1 << DSMUTE); 
    write_registers(fm);   
}
 
void fmradio_setrdsverbose(FMRadio *fm, uint8_t state)
{
    read_registers(fm);
    if (state) 
        fm->registers[POWERCFG] |= (1 << RDSM); 
    else 
        fm->registers[POWERCFG] &= ~(1 << RDSM); 
    write_registers(fm);
}

uint8_t fmradio_getrdsstate(FMRadio *fm)
{
    read_registers(fm);
    if (fm->registers[STATUSRSSI] & RDSS)
        return 1;
    else 
        return 0;
}

uint8_t fmradio_getrssi(FMRadio *fm)
{
    read_registers(fm);
    return (fm->registers[STATUSRSSI] & RSSI);
} 

/* ----------- RDS Parser -------------------- */
/* Dá sa zisťovať aj cez INT GPIO1 */
uint8_t rds_check(FMRadio *fm, RDSRadioData *rds)
{
    read_registers(fm);
    if (fm->registers[STATUSRSSI] & RDSR) {
        rds_process(rds, fm->registers[RDSA], fm->registers[RDSB], 
                         fm->registers[RDSC], fm->registers[RDSD]);
        return 1;
    } else  {
        return 0;
	}
}

void rds_init(RDSRadioData *rdsdata)
{
    memset(rdsdata, 0, sizeof(RDSRadioData));
}

void rds_empty(RDSRadioData *rdsdata)
{
    memset(rdsdata->psname, '\0', 10);    
    memset(rdsdata->psname1, '\0', 10);
    memset(rdsdata->psname2, '\0', 10);
    memset(rdsdata->rdstext, '\0', 66);
    rdsdata->lasttextab = 0;
    rdsdata->lasttextidx = 0;
    rdsdata->lastRDSMinutes = 0;
}

void rds_calladd_psname(RDSRadioData *rdsdata, void (*callback)(char *))
{
    rdsdata->rdspsname_callback = callback;
}

void rds_calladd_text(RDSRadioData *rdsdata, void (*callback)(char *))
{
    rdsdata->rdstext_callback = callback;
}

void rds_calladd_time(RDSRadioData *rdsdata, void (*callback)(uint8_t, uint8_t))
{
    rdsdata->rdstime_callback = callback;
}

void rds_process(RDSRadioData *rds, 
                 uint16_t block1, uint16_t block2, uint16_t block3, uint16_t block4)
{   
    uint8_t rdsgrouptype; /* rdsTP, rdsPTY ; */
    uint8_t textab;
    uint8_t  idx;   /* index of rds text */
    char c1, c2;

    uint16_t mins; /* RDS time in minutes */
    uint8_t off;   /* RDS time offset and sign */

    if (block1 == 0) {      /* Send empty RDS data */
        rds_empty(rds); 
        if (rds->rdspsname_callback != NULL) 
            rds->rdspsname_callback("");
        if (rds->rdstext_callback != NULL)
            rds->rdstext_callback("");
        rds->rdstext_callback("");
    }

    rdsgrouptype = 0x0A | ((block2 & 0xF000) >> 8) | ((block2 & 0x0800) >> 11);
    /* rdsTP = (block2 & 0x0400);
    rdsPTY = (block2 & 0x0400); */

    switch (rdsgrouptype) {
        case 0x0A:
        case 0x0B:
            /* The data received is part of the Service Station Name */
            if (rds->rdspsname_callback == NULL)
                return; 
            idx = 2 * (block2 & 0x0003);
        
            /* new data is 2 chars from block 4 */
            c1 = block4 >> 8;
            c2 = block4 & 0x00FF;

            /* check that the data was received successfully twice
               before publishing the station name */
            if ((rds->psname1[idx] == c1) && (rds->psname1[idx + 1] == c2)) {
                rds->psname2[idx] = c1;
                rds->psname2[idx + 1] = c2;
                rds->psname2[8] = '\0';

                if ((idx == 6) && (strcmp(rds->psname1, rds->psname2) == 0) 
                    && strcmp(rds->psname2, rds->psname) != 0) {
                        /* publish station name */
                        strcpy(rds->psname, rds->psname2);
                        rds->rdspsname_callback(rds->psname); 
                }
            }

            if ((rds->psname1[idx] != c1) || (rds->psname1[idx + 1] != c2)) {
                rds->psname1[idx] = c1;
                rds->psname1[idx + 1] = c2;
                rds->psname1[8] = '\0';
            } 
            break;

        case 0x2A:
            /*  The data received is part of the RDS Text */
            if (rds->rdstext_callback == NULL)
                return;
            textab = (block2 & 0x0010);
            idx = 4 * (block2 & 0x000F);

            /* the existing text might be complete because the index 
               is starting at the beginning again. */
            if (idx < rds->lasttextidx) 
                rds->rdstext_callback(rds->rdstext);
            rds->lasttextidx = idx;
      
            if (textab != rds->lasttextab) {
                rds->lasttextab = textab;
                memset(rds->rdstext, 0, sizeof(rds->rdstext));
            } 


            /* new data is 2 chars from block 3 */
            rds->rdstext[idx++] = (block3 >> 8);     
            rds->rdstext[idx++] = (block3 & 0x00FF); 

            /* new data is 2 chars from block 4 */
            rds->rdstext[idx++] = (block4 >> 8);     
            rds->rdstext[idx++] = (block4 & 0x00FF);
            break;
    
        case 0x4A:
            /* Clock time and date */
            if (rds->rdstime_callback == NULL)
                return;
            off = (block4) & 0x3F; 
            mins = (block4 >> 6) & 0x3F; 
            mins += 60 * (((block3 & 0x0001) << 4) | ((block4 >> 12) & 0x0F));
     
            /* adjust offset */
            if (off & 0x20) 
                mins -= 30 * (off & 0x1F);
            else 
                mins += 30 * (off & 0x1F);
            
            if (mins != rds->lastRDSMinutes) {
                rds->lastRDSMinutes = mins;
                rds->rdstime_callback(mins / 60, mins % 60);
            } 
            break;

        default:
            break;      
    }
}
