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

static unsigned char mode = 0;
unsigned char digits[4];
const unsigned char seg7[21] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F,
								0xBF, 0x86, 0xDB, 0xCF, 0xE6, 0xED, 0xFD, 0x87, 0x8F, 0xEF, 0x0};
volatile unsigned char button_disable;
static unsigned char index = 0;
unsigned char blink = 255;

unsigned char minute;
unsigned char hour;

void setupTimer1() {
	//setup TIMER1
	TCCR1A |= 0;	//CTC mode
	TCCR1B |= (1<<WGM12)|(1<<CS12)|(1<<CS10);	// prescaler 1024
		
	OCR1A = F_CPU/1024;		// compare value of CTC mode
	TIMSK|=(1<<OCIE1A);		//enable interrupt
}

void setupTimer0() {
	TCCR0 |= (1 << CS01);
	TCNT0 = 0;
	TIMSK |= (1 << TOIE0);
}

bool increaseMinute() {
	bool carry = FALSE;
	minute ++;
	if (minute > 60) {
		minute = 0;
		carry = TRUE;
	}
	
	digits[2] = minute/10;
	digits[3] = minute - 10*digits[2];

	return carry;
}

void increaseHour() {
	hour ++;
	if (hour >= 24) {
		hour = 0;
	}
	
	digits[0] = hour/10;
	digits[1] = hour - 10* digits[0];
	
	if (hour >=12) {
		digits[1] += 10;
		digits[0] --;
	}
	
	if (digits[0] == 0) {
		digits[0] = 20;
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
	
	digits[0] = 1;
	digits[1] = 1;
	digits[2] = 5;
	digits[3] = 5;
    
	DDRC = 0xFF;
	DDRA = 0xFF;		// Port A for the multiplex
	
	mode = 0;
	index = 0;
	blink = 255;
	while(1) {};
	return 0;
}

ISR(TIMER0_OVF_vect) {
	if (button_disable > 0)
		button_disable --;
	index ++;
	index &= (Ndigits - 1);
	PORTA = 1 << index;
	if (mode == 0) {
		PORTC = seg7[digits[index]];
	} 
	else if (mode == 1) {
		if (blink > 100) {
			PORTC = seg7[digits[index]];
		}
		else {
			PORTC = 0x0;
		}
		if (blink > 0) {
			blink --;
		} else {
			blink = 255;
		}
	}
}

ISR(INT0_vect) {
	if (button_disable == 0) {
		if (mode == 1) {
			increaseHour();
		}
		button_disable = 100;
	}
}

ISR(INT1_vect) {
	if (button_disable == 0) {
		if (mode == 1) {
			increaseMinute();
		}
		button_disable = 100;
	}
}

ISR(INT2_vect) {
	if (button_disable == 0) {
		button_disable = 100;
		mode ++;
		mode &= 1;
	}
}

ISR(TIMER1_COMPA_vect) {
	if (mode == 0) {
		if (increaseMinute()) {
			increaseHour();
		}
	}
}
