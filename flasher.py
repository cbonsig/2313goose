#!/usr/bin/python
# encoding: utf-8
import sys,time,struct,wave, os.path
import serial # requires pySerial to be installed
'''
Flash data / wav files to at45DB dataflash file, from wav files.
This program communicates with the flasher arduino sketch. (myflasher + dataflash library)

wav files should be provided in commandline

1) prepare all files
2) put all files to Flash
3) prepare a small .h file with all addresses for program use

(be careful not to go too fast with serial, 9600 is OK)

'''

"""
format the memory:
264 bytes Sectors

zero-page = SIGNATURE + FAT = list (HiPagestart, LoPagestart, HiLastBytes, LoLastBytes)
with last record = xx, xx, where xx is the 00.00 first free page.
# Do not often write too much!

- Page N: padde file name with spaces
- Page 1 ... N M: data
- M in the last page, just lastbytes bytes on the page. (never null)
(next page Free M +1)

- 264/4 = 66 files max (ca ira)
- The size is deducted from the num next page (under 1 page)
"""

"""
cbonsig_notes

25.nov.2013
flasher.py initializes properly. attempts to format the flash fail as shown below:

Craigs-MacBook-Pro:2313goose cbonsignore$ python flasher.py format
/dev/tty.usbmodem1d1131
*Hello, this thing is working!
synced
Are you sure to erase chip ? [y/N]y
writing table to chip
writing page 0  ...  ok, done
content table written successfully with 0 files
reading content table from chip : 
reading page 0 : done reading page:0
Error : unformatted flash (should start with YEAH)
(debug) Dump of the first sector : 
...

Changing SIGNATURE to something other than 'YEAH' has same outcome.
Changing SIGNATURE to '' allows format to (appear to) succeed
add_wav appears to be successful -- but after successfully transferring the file,
it does not appear in the content table, nor does it appear when using ls. Sample output below:

Craigs-MacBook-Pro:2313goose cbonsignore$ python flasher.py add_wav 1.wav 
/dev/tty.usbmodem1d1131
*Hello, this thing is working!
synced
1 channels
1 byte width
8000 samples per second
reading page 0 : done reading page:0
Adding  1.wav ...
writing page 0  ...  ok, done
Will write 20 pages
writing page 1  ...  ok, done
writing page 2  ...  ok, done
writing page 3  ...  ok, done
writing page 4  ...  ok, done
writing page 5  ...  ok, done
writing page 6  ...  ok, done
writing page 7  ...  ok, done
writing page 8  ...  ok, done
writing page 9  ...  ok, done
writing page 10  ...  ok, done
writing page 11  ...  ok, done
writing page 12  ...  ok, done
writing page 13  ...  ok, done
writing page 14  ...  ok, done
writing page 15  ...  ok, done
writing page 16  ...  ok, done
writing page 17  ...  ok, done
writing page 18  ...  ok, done
writing page 19  ...  ok, done
writing page 20  ...  ok, done
writing page 21  ...  ok, done
writing table to chip
writing page 0  ...  ok, done
content table written successfully with 1 files
reading content table from chip : 
reading page 0 : done reading page:0
content table: [(0, 0)]
 -- content of chip : 
 --
 - next free page (over 2048) :  0  - 528 kbytes left
reading page 0 : done reading page:0
content table: [(0, 0)]
 -- content of chip : 
 --
 - next free page (over 2048) :  0  - 528 kbytes left
Craigs-MacBook-Pro:2313goose cbonsignore$ python flasher.py ls
/dev/tty.usbmodem1d1131
*Hello, this thing is working!
synced
reading page 0 : done reading page:0
content table: [(0, 0)]
 -- content of chip : 
 --
 - next free page (over 2048) :  0  - 528 kbytes left


12/1 status
* sketch_dec01a_dataflash.ino confirms that read and write work as intended
* 32(E) chip responds with buffer size of 512. switched back to 16(D) chip and buffer responds as 528.
* flasher.py modified to shift chr's and confirm that read and write commands are going to serial as intended
* problem: write appears to work properly, read appears to work properly, BUT read != write.
* page_read 7 confirms that previously written "hello world" can be read. but o in hello reads as /x00 (???)
* signs point to myflasher_github.ino as culprit. review, compare with sketch_dec01a_dataflash.ino, find problems
* issue is probably with write ... but read is suspicious because of strange 5th character


"""

USBPORT = '/dev/tty.usbmodem1a1231'

DEBUG = True

#PAGELEN = 264   /// per DataFlashSizes.h: DF_45DB161_PAGESIZE     528
PAGELEN = 528

TRAIL_CHAR = '\0'
# SIGNATURE = 'YEAH'                # formatting fails with -- Error : unformatted flash (should start with YEAH)
SIGNATURE = 'YIPY'

class Programmer : 

	def __init__(self,port) : 
		
		self.contenttable = [] # tuples (page_start,last_byte)

		self.tty = serial.Serial(USBPORT, 9600, timeout=1)

		print self.tty.name

		time.sleep(4.0) # this fails with 1.0, 2.0, sometimes 3.0
		self.tty.flush()

		self.tty.write(" H ")

		r='' # readline ?

		while r!='\n' : 
			r=self.getch()                   
			sys.stderr.write(r)
		# print >>sys.stderr,"synced"
		print "synced"
		

	def getch(self) :
		r=''
		while r=='': 
			r = self.tty.read(1)
			if not r : time.sleep(0.020)
		return r

	def write_page(self,pageid,buf) : 
		assert len(buf) == PAGELEN,'wrong size:%d'%len(buf)

		if DEBUG : 
			print >>sys.stderr,'writing page %d'%pageid,
			print "*** begin print repr(buf) ***"
			print repr(buf)
			print "*** end print repr(buf) ***"
		
		# ORIGINAL CODE HERE
		#s='W '+chr(pageid>>8)+chr(pageid&0xff)+' '+buf
		#self.tty.write(s)

		
		# TROUBLESHOOTING CODE STARTS HERE --------------------

		b = bytearray()

		b.append('W')
		b.append(' ')
		b.append(chr((0>>8)+48))
		b.append(chr((0&0xff)+48))
		b.append(' ')
		for c in buf:
			b.append(c)


		if DEBUG:
			hex_string = "".join("\\x%02x" % j for j in b)
			print "*** begin print hex_string ***"
			print hex_string
			print "*** end print hex_string ***"

		#self.tty.write(bytes(s))

		#self.tty.write(writebytes)  # this command is accepted, but fails the desired data doesn't make it to the chip

		#self.tty.write(hex_string)  # sending a hex string is not accepted... seems like it sends a string

		#for b in writebytes:
		#	self.tty.write(b)

		#self.tty.write(bytes(writebytes))
		#self.tty.write(writebytes)

		#for b in writebytes: self.tty.write(b)   #FAIL

		# CLUE: pySerial write does not accept bytearray!

		import array
		s = array.array('B',b).tostring()
		#self.tty.write(s)

		i=0
		for c in s:
			if i<11:
				print i,c
			i=i+1
			self.tty.write(c)

		print ""
		print "*** write - begin print pageid"
		print pageid
		print "*** write - end print pageid"
		print ""
		print "*** write - begin print s"
		print s
		print 's[0]='+s[0]+'.'
		print 's[1]='+s[1]+'.'
		print 's[2]='+s[2]+'.'
		print 's[3]='+s[3]+'.'
		print 's[4]='+s[4]+'.'
		print 's[5]='+s[5]+'...'
		print "*** write - end print s"


		# TROUBLESHOOTING CODE ENDS HERE --------------------

		print >>sys.stderr,' ... ',
		r=self.getch()
		assert r=='$','expected $, got %s'%repr(r) # ok
		print >>sys.stderr,"ok, done ... this worked"

	def read_page(self,pageid) :
		if DEBUG : 
			print >>sys.stderr,'reading page',pageid,':',
		#s='R '+chr(pageid>>8)+chr(pageid&0xff)+' ' # lets try to rewrite this as above
		#self.tty.write(s)

		# SIMILAR TO ABOVE, CONSTRUCT COMMAND ONE BYTE AT A TIME, CONFIRM SEND AS BYTE
		b = bytearray()
		b.append('R')
		b.append(' ')
		#b.append(chr(pageid>>8))
		#b.append(chr(pageid&0xff))
		b.append(chr((pageid>>8) + 48))		# this hack shifts it int value of digit to ascii equiv
		b.append(chr((pageid&0xff) + 48))
		b.append(' ')

		#self.tty.write(bytes(s))
		#self.tty.write(s)

		import array
		s = array.array('B',b).tostring()

		# 12/30 debugging...
		#s='R '+(pageid>>8)+(pageid&0xff)+' '

		self.tty.write(s)

		print ""
		print "*** read - begin print pageid"
		print pageid
		print "*** read - end print pageid"
		print ""
		print "*** read - begin print s"
		print s
		print 's[0]='+s[0]+'.'
		print 's[1]='+s[1]+'.'
		print 's[2]='+s[2]+'.'
		print 's[3]='+s[3]+'.'
		print 's[4]='+s[4]+'.'
		print "*** read - end print s"

		r=self.getch()
		assert r=='$','expected $, got %s'%repr(r) # ok
		s=''
		while len(s)<PAGELEN : 
			s+=self.tty.read()

		if DEBUG : 
			print >>sys.stderr,"done reading page:%d"%pageid
		
		return s

	def write(self,filename,str,read=False) : 
		'''wrote a full buffer after the last record, with zeros
		file trailer filled with TRAIL_CHAR
		do not write the table of contents but prepare the structure in memory.
		the tdm is written last. writing the name of the first file
		'''

		start_page = self.contenttable[-1][0] # fail if not formatted ie contenttable is empty

		# write filename padded with spaces
		base = os.path.basename(filename)
		self.write_page(start_page,base+' '*(PAGELEN-len(base)))
		# write data
		print >>sys.stderr,"Will write %d pages"%(len(str)/PAGELEN)
		pg = 0 # written pages
		while (pg+1)*PAGELEN<len(str) : # there is a buffer
			buf = str[pg*PAGELEN:(pg+1)*PAGELEN]
			self.write_page(start_page+pg+1,buf)
			if read : 
				buf2 = self.read_page(start_page+pg)
				if buf != buf2 : 
					print >>sys.stderr,"Values read different from written :"
					print >>sys.stderr,repr(buf)
					print >>sys.stderr,repr(buf2)
			pg+=1

		# last buffer
		a1=str[pg*PAGELEN:]
		a2=TRAIL_CHAR*(PAGELEN-len(a1))
		self.write_page(start_page+pg+1,a1+a2)
		pg += 1

		# table of contents
		self.contenttable[-1]=(self.contenttable[-1][0],len(a1)) # replace with new end buffer offset
		self.contenttable.append((start_page+pg+1,0))

	def read(self,number) :
		self.read_table()
		assert number < len(self.contenttable),'unknown file number %d'%number
		page_start,offset = self.contenttable[number]
		page_end = self.contenttable[number+1][0]-1
		# read page name
		filename = self.read_page(page_start).rstrip()

		# read data
		buf = ""
		for page in range(page_start,page_end) : 
			buf += self.read_page(page+1)
		buf += self.read_page(page_end)[:offset]
		return filename,buf



	def read_table(self) :
		"read content table from chip"
		self.contenttable=[]
		# read page zero 0
		buf=self.read_page(0) 
		#print >>sys.stderr,repr(buf)
		if not buf.startswith(SIGNATURE) : 
			print >>sys.stderr,'Error : unformatted flash (should start with %s)'%SIGNATURE
			if DEBUG  : 
				print >>sys.stderr,'(debug) Dump of the first sector : '
				print repr(buf)
			sys.exit(0)
		x=4
		while x<264:
			r = buf[x:x+4]
			# read entry
			self.contenttable.append(((ord(r[0])<<8)+ord(r[1]),(ord(r[2])<<8)+ord(r[3])))
			if r[2:4] == '\0\0' : break # but still includes the last
			x += 4

	def write_table(self) : 
		assert self.contenttable,"No content table read yet!"
		# increasing test ?
		# test offsets are zero except the last
		print >>sys.stderr,"writing table to chip"

		buf=SIGNATURE+''.join(chr(page>>8)+chr(page&0xff)+chr(offset>>8)+chr(offset&0xff) for page,offset in self.contenttable)
		print "about to print buf"
		print buf
		print "done printing buf"
		assert len(buf)<PAGELEN,'table too big to be written !'

		buf_to_write = buf+TRAIL_CHAR*(PAGELEN-len(buf))
		print "about to print buf_to_write"
		print buf_to_write
		print "done printing buf_to_write"


		self.write_page(0,buf_to_write)
		print >>sys.stderr,'content table written successfully with %d files'%(len(self.contenttable)-1)
		print >>sys.stderr,'reading content table from chip : '
		self.ls()


	def ls(self) : 
		try : 
			self.read_table()
			print >>sys.stderr,"content table:",self.contenttable
			print >>sys.stderr," -- content of chip : "
			for i,(page,offset) in enumerate(self.contenttable): 
				if offset != 0 : # last one does not count
					print >>sys.stderr,' id:%d, file:%s, length : %d, start page : %d, end page : %d'%(
						i,
						self.read_page(page).rstrip(),
						(self.contenttable[i+1][0]-page)*PAGELEN+offset,
						page,
						self.contenttable[i+1][0],
						)

				else : 
					print >>sys.stderr,' --'
					print >>sys.stderr," - next free page (over 2048) : ",page," - %d kbytes left"%((2048-(page-1))*PAGELEN/1024)
		except ValueError,e : 
			print >>sys.stderr,"cannot read content table : ",e
			raise



	def format(self) : 
		self.contenttable=[(1,0)] # reset content table
		self.write_table()


	def add_wav(self,files,read=False) : 
		"files : filename, filename , ...]"
		# read all files in memory (not too big) and append to chip
		assert files,'nothing to write'
		for f in files : # check all BEFORE actual writing
			fp_in = wave.open(f)
			print >>sys.stderr,fp_in.getnchannels(), "channels"
			assert fp_in.getnchannels()!='1',"mono sound file only !"
			print >>sys.stderr,fp_in.getsampwidth(), "byte width"
			assert fp_in.getsampwidth()==1,'only 8 bits input !'
			print >>sys.stderr,fp_in.getframerate(), "samples per second"
			assert fp_in.getframerate()==8000,'only 8khz samplerate !'

		self.read_table()
		for f in files : 
			print >>sys.stderr,'Adding ',f,'...'
			# read input entirely into memory
			fp_in = wave.open(f, "r")
			frameraw = fp_in.readframes(fp_in.getnframes())

			# append / prepend ramping sound to avoid clicks
			pre  = ''.join([chr(i) for i in range(0,ord(frameraw[0]),16)])
			post = ''.join([chr(i) for i in range(ord(frameraw[-1]),0,-16)])

			self.write(f,pre+frameraw+post,read)

		self.write_table()

	def read_wav(self,number) : 

		# read buffer
		filename,buf = self.read(number)

		# check overwrite
		while os.path.exists(filename) : filename+='_new'

		# get that to wav
		wav = wave.open(filename,'w')
		wav.setnchannels(1)
		wav.setsampwidth(1)
		wav.setframerate(8000)
		wav.writeframes(buf)
		print >>sys.stderr,"file %s written."%filename

	def add_raw(self,files,read=False) : 
		# read all files in memory (not too big) and append to chip
		assert files,'nothing to write'

		for f in files : # check all BEFORE actual writing
			fp_in = open(f)

		self.read_table()
		for f in files : 
			print >>sys.stderr,'Adding ',f,'...'
			# read input entirely into memory
			self.write(f,open(f).read(),read)
		self.write_table()

	def read_raw(self,number) : 
		# read buffer
		filename,buf = self.read(number)
		while os.path.exists(filename) : filename+='_new'
		else : 
			open(filename,'w').write(buf)
		print >>sys.stderr,"file %s written."%filename

def help() :
	print >>sys.stderr,"""%s help : flasher.py <option>
	with <option> = 
	  ls
	  format
	  add_wav <filenames> (to chip)
	  read_wav <N>  
	  add_raw <filenames>
	  read_raw <N> 
	  read_page <N>
	"""%sys.argv[0]
if __name__=='__main__' :

	p=Programmer(USBPORT)

	if len(sys.argv)==1 : 
		help()
		exit
	elif sys.argv[1]=='ls' : 
		p.ls()
	elif sys.argv[1]=='format' :
		sure=raw_input('Are you sure to erase chip ? [y/N]')
		if sure == 'y' : 
			p.format()
	elif sys.argv[1]=='read_page' : 
		pgid = int(sys.argv[2])
		buf = p.read_page(pgid)
		print >>sys.stderr,"read page %d"%pgid
		print >>sys.stderr,repr(buf)

	# raw files 
	elif sys.argv[1]=='add_raw' : 
		files = sys.argv[2:]
		p.add_raw(files)
		p.ls()
	elif sys.argv[1]=='read_raw' : 
		n = int(sys.argv[2])
		p.read_raw(n)
	# 8Khz wav files
	elif sys.argv[1]=='add_wav' : 
		files = sys.argv[2:]
		p.add_wav(files)
		p.ls()
	elif sys.argv[1]=='read_wav' : 
		n = int(sys.argv[2])
		p.read_wav(n)
	else : 
		help()



	