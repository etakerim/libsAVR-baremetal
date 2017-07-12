# AVR C "bare metal" libraries
Collection created enable to other occasional Arduino hackers with deep intrest
in hardware details, write application AVR C code on thin layer of abstraction. 
These drivers have been mostly rewritten and put together from various existing 
sources, so I am sorry if I have "borowed" your code without attribution. 

Most important low level protocols (UART, I2C) has been originally implemented 
by Peter Fluery <pfleury@gmx.ch> http://tinyurl.com/peterfleury. Therefore, they
are included only for completness.


### Internal Dependencies 
Every driver proper consists one *.c and one *.h file. Then there is a small
functional demonstation in test_*.c. 
Every library depends on:
`I2C/i2cmaster.h`, `I2C/twimaster.h` for functionality 
`Uart/uart.h`, `Uart/uart.c`         for testing (cause there is no JTAG on ATMEGA328P)

!!! You have to make sure that in your Makefile you configure include paths
correctly - as reference you can use one in `Applications/Makefile`


### Example Applications
Every program has been tested on AVR ATmega328p (basically Arduino) so
incompatibilies with other processors are not covered for now.

#### avrblink.c
Basic hello world program for hardware. 
- *Connections*:    
    - LED anode  ->  PB5  (Arduino: D13)


#### basicclock.c, digitalclock.c 
Display clock data from Real-time clock on 16x2 LCD. 
- *Dependencies*:  Uart, CharLCD
- *Connections*:  
    -  SCL (both)  ->   PC5   (Arduino: A5) 
    -  SDA (both)  ->   PC4   (Arduino: A4)


#### DCF77.c
Decode bits from DCF reciever input and displays then in binary form
- *Dependecies*:   Uart
- *Connections*:   
    -  Reciever    ->   PD2   (Arduino: D2)


#### ledpattern.c
On button interrupt changes timing of LED on/off cycles 
- *Connections*:  
    - 4x LED anode ->  PB2, PB3, PB4, PB5  (Arduino: 10 - 13)
    - Button       ->  PD2                 (Arduino: D2)


#### sndgenerator.c
Depending on ADC input values changes frequency of PWM signal and reports
frequency on 4x7-segment display. With button you can change sound wave (it is
simulated by square wave, so a bit off)
- *Connections*:  
    - Potenciometer middle             ->  PC0 (ADC0)  (Arduino: A0)
    - 8 ohm Speaker + min. 50 ohm R    <-  PB3         (Arduino: 11)
    - Button                           ->  PB5         (Arduino: 13)
    - LED display segments             <-  PB0, PB1, PB2, PB3 (Arduino: 8 -11)
    - LED A-G                          <-  PORTD              (Arduno: 0 - 6)

#### stopwatch.c
Stopwatch with reset and stop ability.
- *Connections*:  
    - Run/Stop button                  ->  PD3      (Arduino: 3)
    - Reset button                     ->  PC2      (Arduino: A2)
    - LED display segements            <-  PB1, PB2, PB3, PB4   (Arduino: 9 - 12)
    - LED A - H                        <-  PD0, PD1, PD2, PD4, PD5, PD6, PD7 (0 - 8, not 3)
    - LED display decimal dot          <-  PB5      (Arduino: 13)

#### voltmeter.c
On 16x2 LCD displays measured value from ADC
- *Connections*:  
    - Analog source (up to 5V with std AREF)    ->  PC0  (ADC0)
    - SDL                                       ->  PC5
    - SDA                                       ->  PC4
