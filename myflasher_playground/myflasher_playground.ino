/*****
myflasher_playground.ino

based on https://github.com/makapuf/2313goose
with dataflash.c from Arduino playground http://playground.arduino.cc/Code/Dataflash

DataFlash AT45DB161D <-----> Arduino Uno
1 <--- black -----> 11   (MISO)
2 <--- yellow ----> 13   (SCK)
3 <--- blue ------> 6    (RESET)
4 <--- white -----> 10   (CS aka SS)
5 <--- yellow ----> 7    (WP)
6 <--- green -----> 3.3V (POWER)
7 <--- white -----> GND  (GND)
8 <--- red -------> 12   (MOSI)

17.nov.2013

set arduino serial terminal to "newline" mode

confirmed that basic operation is correct
  H + space (+ newline) = Hello, this thing is working!
but flasher.py (as modified today) is still not working
format command gives this result:

$ python flasher.py format
Hello, this thing is working!
synced
Are you sure to erase chip ? [y/N]y
writing table to chip
writing page 0  ...Traceback (most recent call last):
  File "flasher.py", line 315, in <module>
    p.format()
  File "flasher.py", line 223, in format
    self.write_table()
  File "flasher.py", line 191, in write_table
    self.write_page(0,buf+TRAIL_CHAR*(PAGELEN-len(buf)))
  File "flasher.py", line 89, in write_page
    assert r=='$','expected $, got %s'%repr(r) # ok
AssertionError: expected $, got '#'

can't find any combination of inputs in terminal that cause arduino to respond 
with a $ (ok) response. only get !, #, *, or ? (or "Hello, this thing is working!")

*****/
#include "dataflash.h"

#define LED_HB 9 // 13 is already taken by a pin !
#define LED_W 6 // off

#define PAGE_LEN 264 

// STK definitions
#define STK_OK '$'
#define STK_FAILED 0x11
#define STK_UNKNOWN '?'
#define STK_INSYNC '+'
#define STK_NOSYNC '!'
#define STK_NOSYNC2 '%'
#define STK_NOSYNC3 '*'
#define STK_NOSYNC4 '#'
#define STK_NOSYNC5 '%'
#define CRC_EOP ' ' 

Dataflash dflash; 

void setup()
{
  Serial.begin(9600);
    
  dflash.init(); //initialize the memory (pins are defined in dataflash.cpp)
  analogReference(DEFAULT); // restore analog reference (to have 5V on it!)
  
  // status LEDS  
  pinMode(LED_HB, OUTPUT);
  pinMode(LED_W, OUTPUT);
  pulse(LED_HB, 2);
  pulse(LED_W, 2);

}

// this provides a heartbeat on pin 9 (not 13 because already taken) , so you can tell the software is running.
uint8_t hbval=128;
int8_t hbdelta=8;
void heartbeat() {
  if (hbval > 192) hbdelta = -hbdelta;
  if (hbval < 32) hbdelta = -hbdelta;
  hbval += hbdelta;
  analogWrite(LED_HB, hbval);
  analogWrite(LED_W, 255-hbval);
  delay(40);
}

#define PTIME 200
void pulse(int pin, int times) {
  do {
    digitalWrite(pin, HIGH);
    delay(PTIME);
    digitalWrite(pin, LOW);
    delay(PTIME);
  } while (--times);

}


void loop(void) {
  // light the heartbeat LED
  heartbeat();
  if (Serial.available()) {
    flasher();
  }
}


uint8_t getch() {
  while(!(Serial.available()>0));
  return (uint8_t) Serial.read();
}


void read_page() {
  // R_hilo_ -> OK+page
  uint8_t c;
  
  if (getch()!=CRC_EOP) { Serial.print((char)STK_NOSYNC); return;}
  unsigned int page_id = ((unsigned int)getch()<<8) + getch();

  if (getch()!=CRC_EOP) { Serial.print((char)STK_NOSYNC2); return;}
  dflash.Page_To_Buffer(page_id,2); 


  // send OK
  Serial.print((char)STK_OK);
  
  // send buffer
  for (unsigned int i=0;i<PAGE_LEN;i++) {
    Serial.print((char) dflash.Buffer_Read_Byte(2,i));
  }  
}


void write_page() {
  // W_HiLo_(PAGE_LEN bytes)_ -> OK/KO
  
  uint8_t c;

  if (getch()!=CRC_EOP) { Serial.print((char)STK_NOSYNC); return ;}

  unsigned int page_id = (getch()<<8) + getch();
  
  if (getch()!=CRC_EOP) { Serial.print((char)STK_NOSYNC2); return ;}
  
  for (unsigned int i=0;i<PAGE_LEN;i++) {
    c = getch(); 
    dflash.Buffer_Write_Byte(1,i,c);  //write to buffer 1, 1 byte at a time
  }
  

  // actually written page
  dflash.Buffer_To_Page(1, page_id); //write the buffer to the memory on page: here
  //pulse(LED_W,1); // too slow

  Serial.print((char)STK_OK);
}


int flasher() { 
  uint8_t ch = getch();
  switch (ch) {
    
  case 'H': // Hello
    if (getch()!=CRC_EOP) {
      Serial.print((char)STK_NOSYNC5);
    } else {
      char *welcome = "Hello, this thing is working!\n";
      for (char *c = welcome;*c != '\0';c++) {
        Serial.write(*c);
      }
    }
    break;
  
  case 'R': // Read Page X
    read_page(); 
    break;

  case 'W': //STK_PROG_PAGE
    write_page();
    break;
    
  // expecting a command, not CRC_EOP
  // this is how we can get back in sync
  case CRC_EOP:
    Serial.write((char) STK_NOSYNC3);
    break;

    // anything else we will return STK_UNKNOWN
  default:
    if (CRC_EOP == getch()) 
      Serial.write((char)STK_UNKNOWN);
    else
      Serial.write((char)STK_NOSYNC4);
  }
}


