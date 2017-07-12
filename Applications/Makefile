BIN  = exec
SRC  = src/main.c src/analog.c src/rtc.c src/bmp180.c src/twimaster.c src/uart.c 
#FatFs/diskio.c FatFs/ff.c FatFs/mmc_avr_spi.c 
OBJS = $(SRC:.c=.o)

CC      = avr-gcc
OBJCOPY = avr-objcopy
SIZE    = avr-size

DEVICE = atmega328p
FREQ   = 16000000UL
CSTD   = c99
OPTIMIZE = s

CFLAGS += -O$(OPTIMIZE)
CFLAGS += -std=$(CSTD) 
CFLAGS += -Wall -Wextra -pedantic -Iinclude/  -IFatFs/ #INCLUDE: /usr/lib/avr/include/
CFLAGS += -DF_CPU=$(FREQ) -DDRV_MMC=0 -mmcu=$(DEVICE)
PORT   = /dev/ttyUSB0
BAUD   =  57600 #uno :115200 # nano: 57600

$(BIN).hex: $(BIN).elf
	$(OBJCOPY) -O ihex -R .eeprom $< $@

$(BIN).elf: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

.PHONY:
install: $(BIN).hex
	avrdude -F -V -c arduino -p $(DEVICE) -P $(PORT) -b $(BAUD) -U flash:w:$<

.PHONY:
size:
	@echo
	$(SIZE) -C --mcu=$(DEVICE) $(BIN).elf

.PHONY:
clean:
	rm -f $(BIN).elf $(BIN).hex $(OBJS)
