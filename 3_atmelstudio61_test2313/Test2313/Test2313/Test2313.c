/*
 * Test2313.c
 *
 * Created: 2/16/2014 10:29:08 PM
 *  Author: cbonsignore
 */ 


#include <avr/io.h>
#include <stdint.h>
#include <stdlib.h>
// hardware functions
#include "hw.h"

enum sounds {  // list of sounds
	DONOTUSE, //0
	ONE, //1
	TWO, //2
	THREE,//3
	FOUR, //4
	FIVE, //5
	SIX, //6
	SEVEN, //7
	EIGHT, //8
	NINE, //9
	CHIME, //10
	QTR1, //11
	QTR2, //12
	QTR3, //13
	QTR4, //14
	QTRFULL, //15
	OH, //16
};


int main()
{
	setup();
	wait_but(1);
	while(1)
	{
		play(ONE);
		play(TWO);
		play(THREE);
		play(FOUR);
		play(FIVE);
		play(SIX);
		play(SEVEN);
		play(EIGHT);
		blink_led(1);
		play(ONE);
		blink_led(2);
		play(TWO);
		blink_led(3);
		play(THREE);
		blink_led(4);
		play(FOUR);
		blink_led(5);
		play(FIVE);
		blink_led(6);
		play(SIX);
		blink_led(7);
		play(SEVEN);
		blink_led(8);
		play(EIGHT);
		blink_led(9);
		play(NINE);
		blink_led(1);
		play(OH);
		blink_led(1);
		wait();
		blink_led(5);
		wait();
		blink_led(1);
		play(QTR1);
		wait();
		blink_led(2);
		play(QTR2);
		wait();
		blink_led(3);
		play(QTR3);
		wait();
		blink_led(4);
		play(QTR4);
		blink_led(5);
		play(CHIME);
	} // never returns
}