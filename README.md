tinywav
=======

tinywav is a cheap hardware platform to play sounds with a dataflash + attiny 2313 AVR microcontroller.

More info about it on : http://s3-eu-west-1.amazonaws.com/makapufprojects/oie/doc.html

enjoy ! -- makapuf

cbonsig fork
============
* 0_makapuf_original: The original 2313 code by @makapuf
* 1_myflasher_github: Arduino code to manage communication from computer to Dataflash
* 2_myflasher_python: Python code to format and write 8-bit WAV files to Dataflash
* 3_atmelstudio61_test2313: Atmel Studio 6.1 project, with C code for the ATTiny2313 to play 8-bit WAV sounds
* revised Arduino code for compatibility with 1.0.5
* ported to the Dataflash library by @BlockoS
* revised Python to work with these changes
* Python flasher + Arduino code confirmed to work using Arduino Uno, AT45DB161D, Python 2.7.5, Arduino 1.0.5, Mac OSX 10.9



