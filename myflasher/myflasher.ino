#include "DataFlash.h"
#include <SPI.h>

#define LED_HB 9 // 13 is already taken by a pin !
#define LED_W 6 // DESACTIVE

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

DataFlash dflash; 

void setup()
{
  
  uint8_t status;
  DataFlash::ID id;

  analogReference(DEFAULT); // restore analog reference (to have 5V on it!)
  
  // status LEDS  
  pinMode(LED_HB, OUTPUT);
  pinMode(LED_W, OUTPUT);
  pulse(LED_HB, 2);
  pulse(LED_W, 2);

  Serial.begin(9600);
  SPI.begin();
    
  dflash.setup(10,6,7); // csPin, resetPin, wpPin
  
  delay(10);
  dflash.begin();
  
  status = dflash.status();
  
//  /* Display status register */
//  Serial.print("Status register :");
//  Serial.print(status, BIN);
//  Serial.print('\n');
//
//  /* Display manufacturer and device ID */
//  Serial.print("Manufacturer ID :\n");  // Should be 00011111 (1F)
//  Serial.print(id.manufacturer, HEX);
//  Serial.print('\n');
//
//  Serial.print("Device ID (part 1) :\n"); // Should be 00011111 (1F)
//  Serial.print(id.device[0], HEX);
//  Serial.print('\n');
//
//  Serial.print("Device ID (part 2)  :\n"); // Should be 00000000 (0)
//  Serial.print(id.device[1], HEX);
//  Serial.print('\n');
//
//  Serial.print("Extended Device Information String Length  :\n"); // 00000000 (0)
//  Serial.print(id.extendedInfoLength, HEX);
//  Serial.print('\n');
 
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
  
  if (getch()!=CRC_EOP) { Serial.write((char)STK_NOSYNC); return;}
  unsigned int page_id = ((unsigned int)getch()<<8) + getch();

  if (getch()!=CRC_EOP) { Serial.write((char)STK_NOSYNC2); return;}
  dflash.pageToBuffer(page_id,1); 


  // envoie OK
  Serial.write((char)STK_OK);
  
  // envoie buffer
  for (unsigned int i=0;i<PAGE_LEN;i++) {
    dflash.bufferRead(1,i);
    Serial.write((char) SPI.transfer(0xff));
  }  
}


void write_page() {
  // W_HiLo_(PAGE_LEN bytes)_ -> OK/KO
  
  uint8_t c;

  if (getch()!=CRC_EOP) { Serial.write((char)STK_NOSYNC); return ;}

  unsigned int page_id = (getch()<<8) + getch();
  
  if (getch()!=CRC_EOP) { Serial.write((char)STK_NOSYNC2); return ;}
  
  
  for (unsigned int i=0;i<PAGE_LEN;i++) {
    c = getch();
    dflash.bufferWrite(0,i); // write to buffer 1
    SPI.transfer(c);
  }
  

  // ecrit effectivement la page
  dflash.bufferToPage(0, page_id); //write the buffer to the memory on page: here
  //pulse(LED_W,1); // ralentit trop

  Serial.write((char)STK_OK);
}


int flasher() { 
  uint8_t ch = getch();
  switch (ch) {
  case 'H': // Hello
    if (getch()!=CRC_EOP) {
      Serial.write((char)STK_NOSYNC5);
    } else {
      char *welcome = "Salut, envoyez la puree!\n";
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


