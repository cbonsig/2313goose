#!/usr/bin/python
# encoding: utf-8
import sys,time,struct,wave, os.path
import serial # requires pySerial to be installed


def getch(serialObject) :
		r=''
		while r=='': 
			# print "before tty.read"
			# self.tty.read(1)
			serialObject.read()                  # getting stuck here! why?? never reads characters
			print "past read()"
			# print "after tty.read"
			# print r
			if not r :
				print "not r" 
				time.sleep(0.020)
		return r


tty = serial.Serial('/dev/tty.usbmodem1d1131', 9600, timeout=0)

print tty.name

time.sleep(2.0)

tty.flush()

# tty.write(' H \n')

tty.write(' ')
print tty.read()

tty.write('H')
print tty.read()

tty.write(' ')
print tty.readline()

print "Done."

