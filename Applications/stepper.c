#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdbool.h>
#include <stdint.h>

#define INPUT_DDR       DDRD
#define INPUT_PORT      PORTD
#define INPUT_IN1       PD2
#define INPUT_IN2       PD3

#define MOTOR_DDR       DDRB
#define MOTOR_PORT      PORTB
#define low(P, B)       ((P) &= ~_BV(B))
#define high(P, B)      ((P) |= _BV(B))

/******************** NEMA 17 *****************************/
#define MOTOR_DIR       PB1
#define MOTOR_STEP      PB2
#define PULSE_DELAY_US  600

void motor_init(void)
{
    MOTOR_DDR |= _BV(MOTOR_DIR) | _BV(MOTOR_STEP);
    MOTOR_PORT &= ~(_BV(MOTOR_DIR) | _BV(MOTOR_STEP));
}

void motor_step(uint16_t movesteps, bool direction, uint16_t rotsteps)
{
    cli();
    if (direction) 
        low(MOTOR_PORT, MOTOR_DIR);
    else 
        high(MOTOR_PORT, MOTOR_DIR);
    
    while (movesteps-- > 0) {
        high(MOTOR_PORT, MOTOR_STEP);
        _delay_us(PULSE_DELAY_US);
        //_delay_ms(1);
        low(MOTOR_PORT, MOTOR_STEP);
        //_delay_ms(1);
        _delay_us(PULSE_DELAY_US);
    }
    sei();
}


/******************** 28BYJ-48 *************************
#define MOTOR_IN1       PB1  
#define MOTOR_IN2       PB2
#define MOTOR_IN3       PB3
#define MOTOR_IN4       PB4 
#define PULSE_DELAYUS   2500  //2500 pri 5V / 1500 pri 12V(krátko)
#define MOTOR_STEPS     64     

void motor_init(void)
{
     // OUT => 4-pinové pripojenie motora 
    MOTOR_DDR |= _BV(MOTOR_IN1) | _BV(MOTOR_IN2) 
                 | _BV(MOTOR_IN3) | _BV(MOTOR_IN4);
}

void motor_step(uint16_t movesteps, bool direction, uint16_t rotsteps)
{
    static int16_t stepnum = 0; 

    while (movesteps-- > 0) {
        _delay_us(PULSE_DELAYUS);
      
        if (direction) {
            stepnum++;
            if (stepnum == rotsteps) 
                stepnum = 0;
        } else {
            if (stepnum == 0) 
                stepnum = rotsteps;
            stepnum--;
        }
      
        switch (stepnum % 4) {
            case 0:  // 1010
                high(MOTOR_PORT, MOTOR_IN1);
                low(MOTOR_PORT, MOTOR_IN2);
                high(MOTOR_PORT, MOTOR_IN3);
                low(MOTOR_PORT, MOTOR_IN4);
                break;
            case 1:  // 0110
                low(MOTOR_PORT, MOTOR_IN1);
                high(MOTOR_PORT, MOTOR_IN2);
                high(MOTOR_PORT, MOTOR_IN3);
                low(MOTOR_PORT, MOTOR_IN4);
                break;
            case 2:  //0101
                low(MOTOR_PORT, MOTOR_IN1);
                high(MOTOR_PORT, MOTOR_IN2);
                low(MOTOR_PORT, MOTOR_IN3);
                high(MOTOR_PORT, MOTOR_IN4);
                break;
            case 3:  //1001
                high(MOTOR_PORT, MOTOR_IN1);
                low(MOTOR_PORT, MOTOR_IN2);
                low(MOTOR_PORT, MOTOR_IN3);
                high(MOTOR_PORT, MOTOR_IN4);
                break;
            }
    }
}

**************************************************************/
volatile bool isrunning = true;
/*
ISR(INT0_vect)
{
    if (isrunning) 
        isrunning = false;
}
*/
ISR(INT1_vect)
{
    if (!isrunning) 
        isrunning = true;
}

int main(void)
{
    motor_init();

    /* IN + pullup => Vstupné senzory ovládajúce motor */
    INPUT_DDR &= ~(_BV(INPUT_IN1) | _BV(INPUT_IN2)); 
    INPUT_PORT |= _BV(INPUT_IN1) | _BV(INPUT_IN2);

    /* Nastavenie prerušenia pri falling edge */
    //EICRA |= _BV(ISC11) | _BV(ISC01);        
    //EIMSK |= _BV(INT1)  | _BV(INT0);
    
    sei();
    isrunning = true;
    while (1) {
    
        if (isrunning) {
            motor_step(100, false, 0);//MOTOR_STEPS);
            _delay_ms(100);
        }
    }
}

