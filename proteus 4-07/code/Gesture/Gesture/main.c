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
	//int result;
	int i;
	int fingerInSight = 0;
	int totalFingers = 0;
	timeElapsed = 0;
	//char val[5];
	//int full;
	//int floating;
	overflowCount = 0;
	char numberArr[5];
	int input = 1;
	int fdig=0, sdig=0, tdig=0;
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
		// start conversion, loop until end
		ADCSRA |= (1 << ADSC);
		while (ADCSRA & (1 << ADSC)) {;}
		
		// voltage calculation from ADC output
		//result = ADC;
		//voltage = (result * 5.0) / 1024;
		voltage = read_adc();
		
		// object detection
		if (!fingerInSight && voltage < 2) {
			if(input==1)
			{
				fdig+=1;
			}
			else if(input==2)
			{
				sdig+=1;
			}
			else if(input==3)
			{
				tdig+=1;
			}
			totalFingers += 1;
			timeElapsed = 0;
			fingerInSight = 1;
		}
		if (fingerInSight && voltage > 2){
			timeElapsed = 0;
			fingerInSight = 0;
		}
		
		//full = (int)voltage;
		//voltage = (voltage-full)*100;
		//floating = (int)voltage;
		
		//Lcd4_Set_Cursor(2, 1);
		//Lcd4_Write_String("Voltage: ");
		
		//itoa(full, val, 10);
		//Lcd4_Write_String(val);
		
		//Lcd4_Write_String(".");
		
		//itoa(floating, val, 10);
		//Lcd4_Write_String(val);
		
		// display in LCD
		Lcd4_Set_Cursor(1, 1);
		if (timeElapsed > 6 && fingerInSight) {
			// halt
			
			input=1;
			fdig = 0;
			sdig = 0;
			tdig = 0;
			ADMUX = 0b01100111;
			
		}
		
		else if (timeElapsed > 6 && !fingerInSight) {
			timeElapsed=0;
			if(input==3)
			{
				totalFingers = 100*tdig+10*sdig+fdig;
				
				//Lcd4_Write_String("RESET");
				input=1;
				fdig = 0;
				sdig = 0;
				tdig = 0;
				ADMUX = 0b01100111;
				
				itoa(totalFingers, numberArr, 10);
				for (i=0; numberArr[i] != '\0'; i++)
				UART_send(numberArr[i]);
				totalFingers = 0;
			}
			else
			{
				//Lcd4_Write_String("Give The Next Digit");
				input+=1;
				ADMUX-=1;
			}
		}
		
		
		
		
			itoa(input, numberArr, 10);
			Lcd4_Write_String("Digit: ");
			Lcd4_Write_String(numberArr);
			
			Lcd4_Set_Cursor(2, 1);
			
			Lcd4_Write_String("Floor: ");
			
			itoa(tdig, numberArr, 10);
			Lcd4_Write_String(numberArr);
			
			itoa(sdig, numberArr, 10);
			Lcd4_Write_String(numberArr);
			
			itoa(fdig, numberArr, 10);
			Lcd4_Write_String(numberArr);
			
		
	}
}

