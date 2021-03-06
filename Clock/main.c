/*
 * Clock.c
 *
 * Created: 5/25/2016 1:01:13 AM
 * Author : andrey
 */ 
#define F_CPU 1000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define TRUE 1
#define FALSE 0
typedef unsigned char bool;
#define Ndigits 4			// only work with power of two

volatile uint8_t mode = 0;
uint8_t digits[4];
const unsigned char seg7[21] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F,
								0xBF, 0x86, 0xDB, 0xCF, 0xE6, 0xED, 0xFD, 0x87, 0xFF, 0xEF, 0x0}; // with the dot, to show AM/PM
volatile uint8_t button_disable;
static uint8_t index = 0;
volatile uint8_t blink = 255;

uint8_t minute;
uint8_t hour;
uint8_t alarmhour;
uint8_t alarmmin;

void setupTimer1() {
	//setup TIMER1
	TCCR1A |= 0;	//CTC mode
	TCCR1B |= (1<<WGM12)|(1<<CS12)|(1<<CS10);	// prescaler 1024
		
	OCR1A = F_CPU/1024;		// compare value of CTC mode
	TIMSK|=(1<<OCIE1A);		//enable interrupt
}

void setupTimer0() {
	TCCR0 |= (1 << CS01)|(1 << CS00)|(1 << WGM01)|(1<<COM00);
	TCNT0 = 0;
	TIMSK |= (1 << OCIE0);
}



void calculateDigits(uint8_t h, uint8_t m) {

	digits[2] = m/10;
	digits[3] = m - 10*digits[2];
	
	h++;	
	unsigned char ampmhour = h;

	if (h > 12) {
		// add a dot to differentiate AM/PM
		ampmhour -= 12;
		digits[0] = ampmhour/10;
		digits[1] = ampmhour - 10 * digits[0];
		digits[1] += 10;
		} else {
		digits[0] = ampmhour/10;
		digits[1] = ampmhour - 10 * digits[0];
	}

	if (digits[0] == 0) {
		digits[0] = 20;
	}
}

void increaseHour() {
	hour ++;
	if (hour > 23) {
		hour = 0;
	}
}

bool increaseMinute() {
	bool carry = FALSE;
	minute ++;
	if (minute >= 60) {
		minute = 0;
		carry = TRUE;
	}
	return carry;
}

void increaseAlarmHour() {
	alarmhour ++;
	if (alarmhour > 23)
		alarmhour = 0;
}

void increaseAlarmMin() {
	alarmmin ++;
	if (alarmmin > 59) {
		alarmmin = 0;
	}
}

int main(void)
{
	PORTB = 1 << 2;		//pull up activated at PB2
	PORTD = 0b00001100;
	GICR = (1<<INT0)|(1<<INT1)|(1<<INT2);	//external interrupt 0, 1 and 2
	MCUCR |= (1 << ISC11)|(1 << ISC01);
	
	setupTimer1();
	setupTimer0();

	sei();
	
	minute = 15;
	hour = 2;
	alarmhour = 0;
	alarmmin = 0;
	increaseHour();
	increaseMinute();
    
	DDRC = 0xFF;
	DDRA = 0xFF;		// Port A for the multiplex
	
	mode = 0;
	index = 0;
	blink = 255;
	while(1) {};
	return 0;
}

// display interrupt
ISR(TIMER0_COMP_vect) {
	OCR0 = 100; // as large as possible so that people can't see multiplexing
	
	if (button_disable > 0)
		button_disable --;
	index ++;
	index &= (Ndigits - 1);
	PORTA = 1 << index;
	
	switch(mode) {
		case 0:
		case 2:
		// normal time display
			PORTC = seg7[digits[index]];
			break;
		case 1:
		// change times, blinking the digits
			if (blink > 25) {
				PORTC = seg7[digits[index]];
			}
			else {
				PORTC = 0x0;
			}
			if (blink > 0) {
				blink --;
				} else {
				blink = 100;
			}
	}
}

// hour setting
ISR(INT0_vect) {
	if (button_disable == 0) {
		if (mode == 1) {
			increaseHour();
			calculateDigits(hour, minute);
		} else if (mode == 2) {
			increaseAlarmHour();
			calculateDigits(alarmhour, alarmmin);
		}
		button_disable = 40;
	}
}

// time setting
ISR(INT1_vect) {
	if (button_disable == 0) {
		if (mode == 1) {
			increaseMinute();
			calculateDigits(hour, minute);
		} else if (mode == 2) {
			increaseAlarmMin();
			calculateDigits(alarmhour, alarmmin);
		}
		button_disable = 40;
	}
}

//interrupt to change modes
ISR(INT2_vect) {
	if (button_disable == 0) {
		button_disable = 100;
		mode ++;
		if (mode > 2)
			mode = 0;
			
		if (mode == 2)
			calculateDigits(alarmhour, alarmmin);
		else
			calculateDigits(hour, minute);
	}
}

// timer interrupt
ISR(TIMER1_COMPA_vect) {
	if (mode == 0 || mode == 2) {
		if (increaseMinute()) {
			increaseHour();
		}
	}
	if (mode == 0)
		calculateDigits(hour, minute);
}
