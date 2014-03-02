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
	QTR8, //0
	CHIME8, //1
};


int main()
{
	setup();
	wait_but(1);
	while(1)
	{
		play(1);
	} // never returns
}