#!/usr/bin/python
# encoding: utf-8
import sys,time,struct,wave, os.path
import serial # requires pySerial to be installed

tty = serial.Serial('/dev/tty.usbmodem1d1131', 9600, timeout=1)

print tty.name

time.sleep(4.0) # this sometimes fails with 2.0, but seems to work with 3.0 (wtf)

tty.flush()

tty.write(" H ")

print tty.readline()

print "Done."

