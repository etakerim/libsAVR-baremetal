BIN=exec
SRC= test_uart.c uart.c 
OBJS=${SRC:.c=.o}

CC=avr-gcc
OBJCOPY=avr-objcopy
CFLAGS=-Os -std=c99 -Wall -Wextra -pedantic 
CFLAGS+=-DF_CPU=16000000UL -mmcu=atmega328p
PORT=/dev/ttyUSB0

${BIN}.hex: ${BIN}.elf
	${OBJCOPY} -O ihex -R .eeprom $< $@

${BIN}.elf: ${OBJS}
	${CC} ${CFLAGS} -o $@ $^

.PHONY:
install: ${BIN}.hex
	avrdude -F -V -c arduino -p ATMEGA328P -P ${PORT} -b 115200 -U flash:w:$<

.PHONY:
clean:
	rm -f ${BIN}.elf ${BIN}.hex ${OBJS}
