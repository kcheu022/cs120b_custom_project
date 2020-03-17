#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include "nokia5110.c"
#include "joystick.h"
#include <time.h> 

#define tmpA (~PINA & 0x0C)



volatile unsigned char TimerFlag = 0;
void TimerISR() { TimerFlag = 1; }

unsigned long _avr_timer_M = 1;
unsigned long _avr_timer_cntcurr = 0;

void TimerOn() {
	TCCR1B = 0x0B;
	OCR1A = 125;
	TIMSK1 = 0x02;
	TCNT1 = 0;
	_avr_timer_cntcurr = _avr_timer_M;
	SREG |= 0x80;
}

void TimerOff() { TCCR1B = 0x00; }

ISR(TIMER1_COMPA_vect) {
	_avr_timer_cntcurr--;
	if (_avr_timer_cntcurr == 0) {
		TimerISR();
		_avr_timer_cntcurr = _avr_timer_M;
	}
}

void TimerSet(unsigned long M) {
	_avr_timer_M = M;
	_avr_timer_cntcurr = _avr_timer_M;
}

void swap(int *xp, int *yp)
{
	int temp = *xp;
	*xp = *yp;
	*yp = temp;
}

void randomize (int arr[], int n)
{
	srand(time(NULL));
	for (int i = n-1; i > 0 ; i--)
	{
		int j = rand() %(i+1);
		swap(&arr[i], &arr[j]);
		
	}
	
}

unsigned char col = 2, row = 2;		//offset the game piece
unsigned char timer = 9;
unsigned char life = 3;
unsigned char score = 0;
static unsigned char joyflag = 0;
unsigned char flag = 0;		//flag to trigger blinking cursor
char cstr[2];	//store cstr for translate from int to string vice versa
int gamearr[2][3];
int randomarr[6];
int index = 0;	//use to index thru random array
int open_or_not[2][3] = {0};
enum initGame_State { initGame_Start, exp_col, exp_row, countdown, coverup_col, coverup_row, ready, countdown2, end, win } initGame_State;
void initGameTick() {
	switch (initGame_State) {		//transition
		case initGame_Start:
			initGame_State = exp_col;
			break;
		case exp_col:
			if (col < 5)
				initGame_State = exp_col;
			else
				initGame_State = exp_row;
			break;
		case exp_row:
			if (row < 4)
				initGame_State = exp_col;
			else
				initGame_State = countdown;
			break;
		case countdown:
			if (timer > 0)
				timer--;
			else
				initGame_State = coverup_col;
			break;
		case coverup_col:
			if (col < 5)
				initGame_State = coverup_col;
			else
				initGame_State = coverup_row;
			break;
		case coverup_row:
			if (row < 4)
				initGame_State = coverup_col;
			else
				initGame_State = ready;
			break;
		case ready:
			initGame_State = countdown2;
			joyflag = 1;
			break;			
		case countdown2:		//this is when you have to find the card
			if (timer > 0)
				timer--;
			else if (index == 6){
				initGame_State = win;
			}else
				initGame_State = end;
			break;
		case end:
			break;
		case win:
			break;
		default:
			break;
	}
	switch(initGame_State) {		//action
		case initGame_Start:
			break;
		case exp_col:
			nokia_lcd_set_cursor(col * 8, row * 8);
			int num = rand() % 10;
			gamearr[row - 2][col - 2] = num;  //save game pieces
			randomarr[row + col - 4] = num;		//use this to randomize pieces to find
			itoa(num, cstr, 10);
			nokia_lcd_write_string(cstr, 1);
			nokia_lcd_render();
			col++;
			break;
		case exp_row:
			row++;
			col = 2;
			break;
		case countdown:
			col = 2; //set back to 2
			row = 2;
			nokia_lcd_set_cursor(0,0);
			itoa(timer, cstr, 10);
			nokia_lcd_write_string("Timer: ", 1);
			nokia_lcd_write_string(cstr, 1);
			nokia_lcd_render();
			break;
		case coverup_col:
			timer = 54;
			nokia_lcd_set_cursor(col * 8, row * 8);
			nokia_lcd_write_string("@", 1);
			nokia_lcd_render();
			col++;
			break;
		case coverup_row:
			row++;
			col = 2;
			break;
		case ready:
			nokia_lcd_set_cursor(0,8);
			nokia_lcd_write_string("Life: ", 1);
			nokia_lcd_write_string("~", 1);
			nokia_lcd_write_string("~", 1);
			nokia_lcd_write_string("~", 1);
			nokia_lcd_render();
			randomize(randomarr, 6);
			col = 2; //set back to 2
			row = 2;
		case countdown2:
			nokia_lcd_set_cursor(0,0);
			itoa(timer, cstr, 10);
			nokia_lcd_write_string("Timer: ", 1);
			nokia_lcd_write_string(cstr, 1);
			nokia_lcd_set_cursor(0,32);
			nokia_lcd_write_string("Find: ", 1);
			itoa(randomarr[index], cstr, 10);
			nokia_lcd_write_string( cstr, 1);		//display what to find
			nokia_lcd_render();
			break;
		case end:
			nokia_lcd_clear();
			nokia_lcd_set_cursor(0,0);
			nokia_lcd_write_string("GAME OVER!", 1);
			nokia_lcd_set_cursor(0,8);
			nokia_lcd_write_string("SCORE: ", 1);
			itoa(score, cstr, 10);
			nokia_lcd_write_string(cstr, 1);
			nokia_lcd_render();
			break;
		case win:
			nokia_lcd_clear();
			nokia_lcd_set_cursor(0,0);
			nokia_lcd_write_string("YOU WIN!", 1);
			nokia_lcd_set_cursor(0,8);
			nokia_lcd_write_string("SCORE: ", 1);
			itoa(score, cstr, 10);
			nokia_lcd_write_string(cstr, 1);
			nokia_lcd_render();
			break;
		default:
			break;
	}
}

enum Joy_States {flagcheck, j_init, up, down, left, right, press, check, hit, miss, gameover } J_State;
void J_tick() {			//transition
	fetchStick();
	switch(J_State) {
		case flagcheck:
			if (joyflag == 1)
				J_State = j_init;
			break;
		case j_init:
			if (coords[0] < 200) {
				if (col < 4)
					J_State = right;
			}
			else if (coords[0] > 800) {
				if (col > 2)
					J_State = left;
			}
			else if (coords[1] < 200) {
				if (row < 3)
					J_State = down;
			}
			else if (coords[1] > 800) {
				if (row > 2)
					J_State = up;
			}
			else if (timer == 0) {
				J_State = end;
			}
			else if (tmpA == 0x04) {
				J_State = press;
			}
			else if (life == 0) {
				J_State = gameover;
			}
			else {
				J_State = j_init;
			}
			break;
		case up:
			if (coords[0] < 200) {
				if (col < 4)
					J_State = right;
			}
			else if (coords[0] > 800) {
				if (col > 2)
					J_State = left;
			}
			else if (coords[1] < 200) {
				if (row < 3)
					J_State = down;
			}
			else if (coords[1] > 800) {
				if (row > 2)
					J_State = up;
			}
			else if (timer == 0) {
				J_State = end;
			}
			else {
				J_State = j_init;
			}
			break;
		case down:
			if (coords[0] < 200) {
				if (col < 4)
					J_State = right;
			}
			else if (coords[0] > 800) {
				if (col > 2)
					J_State = left;
			}
			else if (coords[1] < 200) {
				if (row < 3)
					J_State = down;
			}
			else if (coords[1] > 800) {
				if (row > 2)
					J_State = up;
			}
			else if (timer == 0) {
				J_State = end;
			}
			else {
				J_State = j_init;
			}
			break;
		case left:
			if (coords[0] < 200) {
				if (col < 4)
					J_State = right;
			}
			else if (coords[0] > 800) {
				if (col > 2)
					J_State = left;
			}
			else if (coords[1] < 200) {
				if (row < 3)
					J_State = down;
			}
			else if (coords[1] > 800) {
				if (row > 2)
					J_State = up;
			}
			else if (timer == 0) {
				J_State = end;
			}
			else {
				J_State = j_init;
			}
			break;
		case right:
			if (coords[0] < 200) {
				if (col < 4)
					J_State = right;
			}
			else if (coords[0] > 800) {
				if (col > 2)
					J_State = left;
			}
			else if (coords[1] < 200) {
				if (row < 3)
					J_State = down;
			}
			else if (coords[1] > 800) {
				if (row > 2)
					J_State = up;
			}
			else {
				J_State = j_init;
			}
			break;
		case press:
			J_State = check;
			break;
		case check:
			PORTC = 0x01;
			if (gamearr[row - 2][col - 2] == randomarr[index]){
				J_State = hit;
			}else {
				J_State = miss;
			}
			break;
		case hit:
			J_State = j_init;
			break;
		case miss:
			if(life == 0){
				J_State = gameover;
			}else{
				J_State = j_init;
			}
			break;
		case gameover:
			break;
	}
	
	switch(J_State) {			//action
		case j_init:
			PORTC = 0x00;
			nokia_lcd_set_cursor(col * 8, row * 8);
			if (open_or_not[row - 2][col - 2] == 0){
				if (flag == 0){
					nokia_lcd_write_string("_", 1);
					nokia_lcd_render();
					flag = 1;
				}else{
					nokia_lcd_write_string("@", 1);
					nokia_lcd_render();
					flag = 0;
				}
				
			}else{
				if (flag == 0){
					nokia_lcd_write_string("_", 1);
					nokia_lcd_render();
					flag = 1;
				}else{
					int num;
					num = gamearr[row - 2][col - 2];
					itoa(num, cstr, 10);
					nokia_lcd_write_string(cstr, 1);
					nokia_lcd_render();
					flag = 0;
				}
				
			}
			break;
		case right:
			PORTC = 0x01;
			nokia_lcd_set_cursor(col * 8, row * 8);
			if (open_or_not[row - 2][col - 2] == 0){
				nokia_lcd_write_string("@", 1);
				nokia_lcd_render();
			}else{
				int num;
				num = gamearr[row - 2][col - 2];
				itoa(num, cstr, 10);
				nokia_lcd_write_string(cstr, 1);
				nokia_lcd_render();
			}
			col++;
			break;
		case left:
			PORTC = 0x02;
			nokia_lcd_set_cursor(col * 8, row * 8);
			if (open_or_not[row - 2][col - 2] == 0){
				nokia_lcd_write_string("@", 1);
				nokia_lcd_render();
			}else{
				int num;
				num = gamearr[row - 2][col - 2];
				itoa(num, cstr, 10);
				nokia_lcd_write_string(cstr, 1);
				nokia_lcd_render();
			}
			col--;
			break;
		case up:
			PORTC = 0x80;
			nokia_lcd_set_cursor(col * 8, row * 8);
			if (open_or_not[row - 2][col - 2] == 0){
				nokia_lcd_write_string("@", 1);
				nokia_lcd_render();
			}else{
				int num;
				num = gamearr[row - 2][col - 2];
				itoa(num, cstr, 10);
				nokia_lcd_write_string(cstr, 1);
				nokia_lcd_render();
			}
			row--;
			break;
		case down:
			PORTC = 0x40;
			nokia_lcd_set_cursor(col * 8, row * 8);
			if (open_or_not[row - 2][col - 2] == 0){
				nokia_lcd_write_string("@", 1);
				nokia_lcd_render();
			}else{
				int num;
				num = gamearr[row - 2][col - 2];
				itoa(num, cstr, 10);
				nokia_lcd_write_string(cstr, 1);
				nokia_lcd_render();
			}
			row++;
			break;
		case press:
			PORTC = 0xFF;
			open_or_not[row - 2][col - 2] = 1;
			break;
		case check:
			//PORTC = 0xF0;
			break;
		case hit:
			PORTC = 0x0F;
			index++;
			score++;
			break;
		case miss:
			if (life == 3) {
				life--;
				nokia_lcd_set_cursor(0,8);
				nokia_lcd_write_string("Life: ", 1);
				nokia_lcd_write_string("~", 1);
				nokia_lcd_write_string("~", 1);
				nokia_lcd_write_string(" ", 1);
			} else if (life == 2) {
				life--;
				nokia_lcd_set_cursor(0,8);
				nokia_lcd_write_string("Life: ", 1);
				nokia_lcd_write_string("~", 1);
				nokia_lcd_write_string(" ", 1);
				nokia_lcd_write_string(" ", 1);
			} else if (life == 1) {
				life--;
				nokia_lcd_set_cursor(0,8);
				nokia_lcd_write_string("Life: ", 1);
				nokia_lcd_write_string(" ", 1);
				nokia_lcd_write_string(" ", 1);
				nokia_lcd_write_string(" ", 1);
			}
			break;
		case gameover:
			timer = 0;
			break;
		default:
			break;
	}
}


int main(void)
{
	DDRA = 0x00; PORTA = 0xFF;
	DDRC = 0xFF; PORTC = 0x00;
	
	unsigned long initGame_elapsedTime = 1000;
	unsigned long j_tick_elapsedTime = 300;
	const unsigned long timerPeriod = 100;
	
    nokia_lcd_init();
    nokia_lcd_clear();
	
	TimerSet(timerPeriod);
	TimerOn();
	ADC_init();
	initGame_State = initGame_Start;
	J_State = flagcheck;
	

    while (1) {
		
		if (j_tick_elapsedTime >= 300) {
			J_tick();
			j_tick_elapsedTime = 0;
		}
        if (initGame_elapsedTime >= 1000) {
	        initGameTick();
	        initGame_elapsedTime = 0;
        }
		
		
		while (!TimerFlag) {}
		TimerFlag = 0;
		j_tick_elapsedTime += timerPeriod;
		initGame_elapsedTime += timerPeriod;
    }
}
