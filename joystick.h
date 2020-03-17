/*
 * joystick.h
 *
 * Created: 3/3/2020 4:14:29 PM
 *  Author: rizel
 */ 


#ifndef JOYSTICK_H_
#define JOYSTICK_H_

unsigned short coords[3];
#define REF_AVCC (1 << REFS0)
#define REF_INT (1 << REFS0) | (1 << REFS1)

void ADC_init() {
	ADCSRA |= (1 << ADEN) | (1 << ADSC) | (1 << ADATE);
	// ADEN: setting this bit enables analog-to-digital conversion.
	// ADSC: setting this bit starts the first conversion.
	// ADATE: setting this bit enables auto-triggering. Since we are
	// in Free Running Mode, a new conversion will trigger whenever
	// the previous conversion completes.
}

void wait(long num) {
	for( long i = 0; i < num; i++)
	asm("nop");
}

void fetchStick() {
	unsigned short x;
	unsigned short y;
	
	ADMUX = REF_AVCC | 0x00;
	wait(300);
	x = ADC;
	ADMUX = REF_AVCC | 0x01;
	wait(300);
	y = ADC;
	
	coords[0] = x;
	coords[1] = y;
}




#endif /* JOYSTICK_H_ */