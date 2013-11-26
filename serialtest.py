#!/usr/bin/python
# encoding: utf-8
import sys,time,struct,wave, os.path
import serial # requires pySerial to be installed


def getch(serialObject) :
		r=''
		while r=='': 
			serialObject.read(1)  
			if not r :
				time.sleep(0.020)
		return r


tty = serial.Serial('/dev/tty.usbmodem1d1131', 9600)

print tty.name

time.sleep(2.0)

tty.flush()

tty.write(" H ")

print tty.readline()

print "Done."

