#define F_CPU 1000000
#define D4 eS_PORTD4
#define D5 eS_PORTD5
#define D6 eS_PORTD6
#define D7 eS_PORTD7
#define RS eS_PORTC6
#define EN eS_PORTC7

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <util/delay.h>
#include "lcd.h"

volatile int overflowCount;
volatile int timeElapsed = 0;

ISR(TIMER1_OVF_vect) {
	overflowCount++;
	if (overflowCount >= 16) {
		timeElapsed += 1;
		overflowCount = 0;
	}
}

void USART_init(void) {
	UCSRA = 0b00000010; // double speed (/8)
	UCSRB = 0b00001000; // only tx
	UCSRC = 0b10000110; // async, no parity, 1 stop, 8 data
	
	UBRRH = 0;
	UBRRL = 12; // baud rate 9600 bps
}

double read_adc()
{
	int v_adch=0, v_adcl=0, temp;
	double step_size = 0.0048828125, value;
	
	v_adcl = (int) ADCL;
	v_adch = (int) ADCH;
	
	PORTB = ADCH;
	
	value = v_adch*4 + v_adcl/64;
	value = value*step_size;
	
	temp = (int)(value * 100 + .5);
	value = (double) temp/100;
	
	return value;
}

void UART_send(unsigned char data) {
	while ((UCSRA & (1 << UDRE)) == 0x00);
	UDR = data;
}

int main(void)
{
	float voltage;
	int i,j;
	int fingerInSight[3];
	int inSightSum=0;
	int totalFingers = 0;
	timeElapsed = 0;
	overflowCount = 0;
	char numberArr[5];
	int input = 1;
	int digits[3];
	
	for(i=0; i<3; i++) digits[i]=0;
	
	for(i=0; i<3; i++) fingerInSight[i]=1;
	
	// configure I/O
	DDRB = 0xFF;
	DDRD = 0xFF;
	DDRC = 0xFF;
	// configure ADC
	ADMUX = 0b01100111; // 3.0, Right, ADC7
	ADCSRA = 0b10000110; // 64
	Lcd4_Init();
	// configure timer
	TCCR1A = 0b00000000; // normal mode
	TCCR1B = 0b00000001; // no prescaler
	TIMSK = 0b00000100; // timer 1
	sei();
	// set comm
	USART_init();
	
	while(1)
	{
		ADMUX = 0b01100111;
		
		for (j=0; j<3; j++)
		{
			// ================ ADC - read voltage ================
			ADCSRA |= (1 << ADSC);
			while (ADCSRA & (1 << ADSC)) {;}
			voltage = read_adc();
			ADMUX-=1;
			
			// ================ Gesture - finger detection ================
			
			if (!fingerInSight[j] && voltage < 2) {
				// light to dark, finger
				digits[j] += 1;
				timeElapsed = 0;
				fingerInSight[j] = 1;
			}
			else if (fingerInSight[j] && voltage > 2) {
				// dark to light, no finger
				timeElapsed = 0;
				fingerInSight[j] = 0;
			}
			
		}
		
		inSightSum=0;
		
		for(i=0; i<3; i++) inSightSum+=fingerInSight[i];
		
		
		// ================ Control - reset, change digit, final result ================
		
		if (timeElapsed > 6 && !inSightSum) {
			// reset input, set to digit 1 (LSB)
			timeElapsed = 0;
			input = 1;
			for(i=0; i<3; i++) digits[i]=0;
			for(i=0; i<3; i++) fingerInSight[i]=1; // prevent initial 1
			ADMUX = 0b01100111;
		}
		else if (timeElapsed > 6 && inSightSum) {
			// change input digit
			timeElapsed=0;
			
				// send final result to PC
				totalFingers = (100* digits[2]) + (10* digits[1]) +  digits[0];
				itoa(totalFingers, numberArr, 10);
				for (i=0; numberArr[i] != '\0'; i++)
				UART_send(numberArr[i]);
				
				// set to digit 1 (LSB)
				input = 1;
				for(i=0; i<3; i++) digits[i]=0;
				ADMUX = 0b01100111;
				totalFingers = 0;
			
			for(i=0; i<3; i++) fingerInSight[i]=1; // prevent initial 1
		}
		
		// ================ Output - printing to LCD ================
		
		Lcd4_Set_Cursor(1, 1); // line 1
		itoa(input, numberArr, 10);
		Lcd4_Write_String("Digit:   ");
		Lcd4_Write_String(numberArr);
		
		Lcd4_Set_Cursor(2, 1); // line 2
		Lcd4_Write_String("Floor: ");
		itoa(digits[2], numberArr, 10);
		Lcd4_Write_String(numberArr); // X00
		itoa(digits[1], numberArr, 10);
		Lcd4_Write_String(numberArr); // 0X0
		itoa(digits[0], numberArr, 10);
		Lcd4_Write_String(numberArr); // 00X
	}
}

