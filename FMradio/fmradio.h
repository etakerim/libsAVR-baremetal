#ifndef FM_RADIO_SI4703_H_
#define FM_RADIO_SI4703_H_

#include <stdint.h>

typedef struct {
    uint8_t i2caddr;
    volatile uint8_t *rstport;     
    uint8_t rstpin;
    uint8_t ismono;
    uint16_t registers[16];
} FMRadio;

void fmradio_init(FMRadio *fm, uint8_t addr, volatile uint8_t *rstport, 
                    volatile uint8_t *rstddr, uint8_t rst);
void fmradio_poweron(FMRadio *fm);
void fmradio_shutdown(FMRadio *fm);
    
void fmradio_setvolume(FMRadio *fm, uint8_t volume);
uint8_t fmradio_getvolume(FMRadio *fm);
void fmradio_setfrequency(FMRadio *fm, uint16_t newfreq);
uint16_t fmradio_getfrequency(FMRadio *fm);

#define fmradio_seekup(fm)      fmradio_seek((fm), 1)
#define fmradio_seekdown(fm)    fmradio_seek((fm), 0)
void fmradio_seek(FMRadio *fm, uint8_t is_seekup); 

void fmradio_setmono(FMRadio *fm, uint8_t state);
void fmradio_setmute(FMRadio *fm, uint8_t state);
void fmradio_setsoftmute(FMRadio *fm, uint8_t state);
void fmradio_setrdsverbose(FMRadio *fm, uint8_t state);
uint8_t fmradio_getrdsstate(FMRadio *fm);
uint8_t fmradio_getrssi(FMRadio *fm);

typedef struct {
    char psname[10];
    char psname1[10];
    char psname2[10];
    char rdstext[66];
    uint8_t lasttextab;
    uint8_t lasttextidx;
    uint8_t lastRDSMinutes;
    
    void (*rdspsname_callback)(char *name);
    void (*rdstext_callback)(char *name);
    void (*rdstime_callback)(uint8_t hour, uint8_t minute);
} RDSRadioData;

uint8_t rds_check(FMRadio *fm, RDSRadioData *rds);
void rds_init(RDSRadioData *rdsdata);
void rds_empty(RDSRadioData *rdsdata);
void rds_calladd_psname(RDSRadioData *rdsdata, void (*callback)(char *));
void rds_calladd_text(RDSRadioData *rdsdata, void (*callback)(char *));
void rds_calladd_time(RDSRadioData *rdsdata, void (*callback)(uint8_t, uint8_t));
void rds_process(RDSRadioData *rds, 
                 uint16_t block1, uint16_t block2, uint16_t block3, uint16_t block4);




#endif
