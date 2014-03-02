/*****
myflasher_github.ino

based on https://github.com/makapuf/2313goose
with Dataflash library from https://github.com/BlockoS/arduino-dataflash

DataFlash AT45DB161D <-----> Arduino Uno
1 <--- black -----> 11   (MISO)
2 <--- yellow ----> 13   (SCK)
3 <--- blue ------> 6    (RESET)
4 <--- white -----> 10   (CS aka SS)
5 <--- yellow ----> 7    (WP)
6 <--- green -----> 3.3V (POWER)
7 <--- white -----> GND  (GND)
8 <--- red -------> 12   (MOSI)

*****/

#include <SPI.h>
#include "DataFlash.h"

// #define DEBUGMODE // this mode is for debugging, for interacting use with serial monitor only (not python)

#define PAGE_LEN DF_45DB161_PAGESIZE // my 32(E) chip is factory-set for 512, not 528

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

// dataflash pin settings
static const int csPin    = 10;
static const int resetPin = 6;
static const int wpPin    = 7;

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
  dataflash.setup(csPin,resetPin,wpPin);
  dataflash.begin();
  
  /* Read status register */
  status = dataflash.status();
  
  /* Read manufacturer and device ID */
  dataflash.readID(id); 

  Serial.begin(9600);
     
  analogReference(DEFAULT); // restore analog reference (to have 5V on it!)
  
  loop_cnt = 0;
  page     = 0;
}

void loop(void) {
  // light the heartbeat LED
  // heartbeat();                // lets try getting rid of the heartbeat .. does this improve inconsistencies?
  if (Serial.available()) {
    flasher();
  }
}

// read a byte from serial line
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

  // the "R" has been read already, so now we're expecting a CRC_EOP (space).
  // if we get something else, throw an error
  if (getch()!=CRC_EOP) { Serial.print((char)STK_NOSYNC); return;}  // STK_NOSYNC  = '!'

  // page_id: first character is high portion of address; second character is low portion of address
  uint16_t page_id = ((uint16_t)getch()<<8) + getch();

  // finally, we're expecting another CRC_EOP (space), so throw an error if we don't get one
  if (getch()!=CRC_EOP) { Serial.print((char)STK_NOSYNC2); return;} // STK_NOSYNC2  = '%'
  
  // debug: print the page_id if we get this far
  #ifdef DEBUGMODE
  Serial.print(page_id,DEC);
  #endif

  // send the requested page to buffer 2
  dataflash.pageToBuffer(page_id,2); 
 
  // send STK_OK ($) to confirm a successful command
  Serial.print((char)STK_OK);                                        // STK_OK  = '$'
  
  // send content of buffer 2 out on the serial line
  for (uint16_t i=0;i<PAGE_LEN;i++) {

    // tell dataflash that we want to read a byte from buffer 2
    dataflash.bufferRead(2,i);

    #ifdef DEBUGMODE

    // read a byte, send each byte to the serial line in human readable decimal format
    //uint8_t in_byte[1] = {SPI.transfer(0xff)};
    //PrintHex8(in_byte,1);  // this successfully and clearly prints out each byte
    uint8_t in_byte = SPI.transfer(0xff);
    Serial.print(".");
    Serial.print(in_byte,DEC);

    #else

    // read a byte, and send it out on the serial line
    uint8_t in_byte = SPI.transfer(0xff);
    Serial.write(in_byte);  // use Serial.write instead of Serial.print for bytes

    #endif
    
    }  
}


void write_page() {
  // W_HiLo_(PAGE_LEN bytes)_ -> OK/KO
  
  uint8_t c;

  // after receiving a "W", we're expecting a CRC_EOP (space). if we don't get one, throw an error
  if (getch()!=CRC_EOP) { Serial.print((char)STK_NOSYNC); return ;}  // STK_NOSYNC  = '!'
  // page_id: first character is high portion of address; second character is low portion of address
  uint16_t page_id = ((uint16_t)getch()<<8) + getch();

   // the next character should be a CRC_EOP (space). if not, throw an error
  if (getch()!=CRC_EOP) { Serial.print((char)STK_NOSYNC2); return ;} // STK_NOSYNC2  = '%'
  
  // github/BlockoS --- void bufferWrite(uint8_t bufferNum, uint16_t offset);
  // from 0 to PAGE_LEN,
  // pull the next character from serial
  // set the location in buffer 1 for a buffer write
  // and send the byte down SPI to the dataflash

  #ifdef DEBUGMODE
  uint8_t temp_storage[PAGE_LEN];
  #endif
  
  for (uint16_t i=0;i<PAGE_LEN;i++) {
    c = getch(); 
    // dflash.Buffer_Write_Byte(1,i,c);  //write to buffer 1, 1 byte at a time
    dataflash.bufferWrite(1,i);
    SPI.transfer(c & 0xff); // lets try adding bit masking here, as seen in github dataflash example
    // i don't understand this bit masking stuff

    #ifdef DEBUGMODE
    temp_storage[i] = c;
    #endif
  }

  #ifdef DEBUGMODE
  for (uint16_t i=0;i<PAGE_LEN;i++) {
    Serial.print(".");
    Serial.print(temp_storage[i]);
  }

  #endif
  
  // github/BlockoS -- void bufferToPage(uint8_t bufferNum, uint16_t page);
  // send buffer 1 to the desired page
  dataflash.bufferToPage(1, page_id);

  // confirm a successful communication with STK_OK ($)
  Serial.print((char)STK_OK);                                        // STK_OK  = '$'

  #ifdef DEBUGMODE
  // confirming that read and write page values are as expected
  Serial.print((char)STK_OK);
  Serial.print(page_id); //debug -- print page_id  
  Serial.print((char)STK_OK);
  #endif
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
