/*****
myflasher_github.ino

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

24.nov.2013

attempt to port from "playground" dataflash library to github dataflash library
arduino code seems to work as intended
now getting $ response, looks all good

problem now is with the python code, which now mysteriously
fails to read characters back from serial
(though it works fine in serial monitor, or direct tty session)

next:
try all this again when not tired (cycle power?)
first try to get serialtest.py to work
then fix flasher.py

*****/
#include <SPI.h>
#include "DataFlash.h"

#define LED_HB 9 // 13 is already taken by a pin !
#define LED_W 6 // off

#define PAGE_LEN DF_45DB161_PAGESIZE // 32(E) chip defaults to 512, not 528

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

DataFlash dataflash;
uint8_t   loop_cnt;
uint16_t  page;

void setup()
{
  uint8_t status;
  DataFlash::ID id;

  /* Initialize SPI */
  SPI.begin();

  /* Wait for 1 second */
  delay(1000);

  /* Initialize DataFlash CS, Reset, WP */
  dataflash.setup(10,6,7);
     
  delay(10);
  
  dataflash.begin();
  
  /* Read status register */
  status = dataflash.status();
  
  /* Read manufacturer and device ID */
  dataflash.readID(id); 

  Serial.begin(9600);
     
  analogReference(DEFAULT); // restore analog reference (to have 5V on it!)
  
  // status LEDS  
  pinMode(LED_HB, OUTPUT);
  pinMode(LED_W, OUTPUT);
  pulse(LED_HB, 2);
  pulse(LED_W, 2);

  loop_cnt = 0;
  page     = 0;
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

  // what i think is happening here...
  // the read command should be in this format: "R_##_"
  // ...where the '_' is actually a CRC_EOP (space) and ## is the page number in two characters
  // so, in this function...
  // an 'R' character has been received, and now the next characters are expected to be
  // a CRC_EOP (a.k.a. a space character) followed by two characters defining the page value
  // if the first character isn't a space, return a '!' and get outta here
  // define the page_id using the next two characters
  // then if the next character isn't a space, return a '%' and get outta here
  // move the requested page to buffer 2
  // send an OK ('$')
  // print out the value of the buffer

  if (getch()!=CRC_EOP) { Serial.print((char)STK_NOSYNC); return;}  // STK_NOSYNC  = '!'
  //uint16_t page_id = ((uint16_t)getch()<<8) + getch();      // first char shifted left by 8 + next char

  // old library wanted pageid in funky format; new library wants simple int (i think)
  // get next two ASCII characters, and convert them to a uint16_t integar
  char buffer[2] = {getch(),getch()};
  uint16_t page_id = atoi(buffer);

  if (getch()!=CRC_EOP) { Serial.print((char)STK_NOSYNC2); return;} // STK_NOSYNC2  = '%'
  
  // debug: print the page_id if we get this far
  // Serial.print(page_id,DEC);
  // the command "R 10 " results in "12337" --- i think chars get converted in the Page_To_Buffer function
  // with atoi() code above, this now returns the expected results

  // i think the code is hanging here ... we never get to the '$'
  dataflash.pageToBuffer(page_id,2); 
  // #FAIL on Page_To_Buffer ---- now this is fine


  // send OK
  // TEMP DEBUG 12/30
  // Serial.print(page_id); //debug -- print page_id

  Serial.print((char)STK_OK);                                        // STK_OK  = '$'
  
  // send buffer
  for (uint16_t i=0;i<PAGE_LEN;i++) {
    dataflash.bufferRead(2,i);
    //Serial.print((char) SPI.transfer(0xff));
    
    // TEMP DEBUG 11/30 confirms using human readable output that buffer looks correct
    // uint8_t in_byte[1] = {SPI.transfer(0xff)};
    // PrintHex8(in_byte,1);  // this successfully and clearly prints out each byte

    // TEMP COMMENT OUT 11/30
    uint8_t in_byte = SPI.transfer(0xff);
    Serial.write(in_byte);  // use Serial.write instead of Serial.print for bytes
    
    //Serial.print(in_byte); // use print for debugging

    // PROBLEM as of 11/29
    // Sending command "R 00 " repeatedly yields different results every time. WTF?
    // There should be no change to the memory, so there's no reason for R 00 to be differnt.

    // 11/30: I think I fixed this with bit masking in write_page

    // 11/30: used atoi to make sure BlockoS is getting the pageID as an integer instead
    //        of some weird char. confirmed wuth PrintHex8

    }  
}


void write_page() {
  // W_HiLo_(PAGE_LEN bytes)_ -> OK/KO
  
  uint8_t c;

  if (getch()!=CRC_EOP) { Serial.print((char)STK_NOSYNC); return ;}  // STK_NOSYNC  = '!'

  //unsigned int page_id = (getch()<<8) + getch();
  //uint16_t page_id = (getch()<<8) + getch();

  //same funky issue here with page_id
  char buffer[2] = {getch(),getch()};
  uint16_t page_id = atoi(buffer);
  
  if (getch()!=CRC_EOP) { Serial.print((char)STK_NOSYNC2); return ;} // STK_NOSYNC2  = '%'
  
  
  // playground --- void Buffer_Write_Byte (unsigned char BufferNo, unsigned int IntPageAdr, unsigned char Data);
  // github/BlokoS --- void bufferWrite(uint8_t bufferNum, uint16_t offset);

  for (uint16_t i=0;i<PAGE_LEN;i++) {
    c = getch(); 
    //dflash.Buffer_Write_Byte(1,i,c);  //write to buffer 1, 1 byte at a time
    dataflash.bufferWrite(1,i);
    SPI.transfer(c & 0xff); // lets try adding bit masking here, as seen in github dataflash example
  }
  

  // actually written page
  //from playground --- void Buffer_To_Page (unsigned char BufferNo, unsigned int PageAdr);
  //from github/BlokoS -- void bufferToPage(uint8_t bufferNum, uint16_t page);
  //dflash.Buffer_To_Page(1, page_id); //write the buffer to the memory on page: here
  dataflash.bufferToPage(1, page_id);
  //pulse(LED_W,1); // too slow

  Serial.print((char)STK_OK);                                        // STK_OK  = '$'

  // TEMP DEBUG 11/30, confirming that read and write page values are as expected
  // Serial.print((char)STK_OK);
  // Serial.print(page_id); //debug -- print page_id  
  // Serial.print((char)STK_OK);
}


int flasher() { 
  uint8_t ch = getch();
  switch (ch) {
    
  case 'H': // Hello
    if (getch()!=CRC_EOP) {
      Serial.print((char)STK_NOSYNC5);                               // STK_NOSYNC5 = '%'
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
    Serial.write((char) STK_NOSYNC3);                                 // STK_NOSYNC3 = '*'
    break;

    // anything else we will return STK_UNKNOWN
  default:
    if (CRC_EOP == getch()) 
      Serial.write((char)STK_UNKNOWN);                                // STK_UNKNOWN = '?'
    else
      Serial.write((char)STK_NOSYNC4);                                // STK_NOSYNC4 = '#'
  }
}

/*
  PrintHex routines for Arduino: to print byte or word data in hex with
  leading zeroes.
  Copyright (C) 2010 Kairama Inc

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
void PrintHex8(uint8_t *data, uint8_t length) // prints 8-bit data in hex with leading zeroes
{
        Serial.print("0x"); 
        for (int i=0; i<length; i++) { 
          if (data[i]<0x10) {Serial.print("0");} 
          Serial.print(data[i],HEX); 
          Serial.print(" "); 
        }
}
