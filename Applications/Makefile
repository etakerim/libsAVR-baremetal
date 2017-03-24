BIN  = exec
SRC  = test.c uart.c ff.c diskio.c mmc_avr_spi.c 
OBJS = $(SRC:.c=.o)

CC      = avr-gcc
OBJCOPY = avr-objcopy
SIZE    = avr-size

DEVICE = atmega328p
CSTD   = c99
OPTIMIZE = s

CFLAGS += -O$(OPTIMIZE)
CFLAGS += -std=$(CSTD) 
CFLAGS += -Wall -Wextra -pedantic 
CFLAGS += -DF_CPU=16000000UL -DDRV_MMC=0 -mmcu=$(DEVICE)
PORT=/dev/ttyUSB0

$(BIN).hex: $(BIN).elf
	$(OBJCOPY) -O ihex -R .eeprom $< $@

$(BIN).elf: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

.PHONY:
install: $(BIN).hex
	avrdude -F -V -c arduino -p ATMEGA328P -P $(PORT) -b 57600 -U flash:w:$<

.PHONY:
size:
	@echo
	$(SIZE) -C --mcu=$(DEVICE) $(BIN).elf

.PHONY:
clean:
	rm -f $(BIN).elf $(BIN).hex $(OBJS)
