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


matcher myflasher data speed (9600) : 
$ stty -f /dev/tty.usbmodem1d1131 9600

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

DEBUG = True

PAGELEN = 264
TRAIL_CHAR = '\0'
SIGNATURE = 'YEAH'

class Programmer : 

	print "2a. i am now in the class Programmer"

	def __init__(self,port) : 
		
		print "2b. i am now in the class Programmer __init__ definition"

		self.contenttable = [] # tuples (page_start,last_byte)

		print "2c. i have defined contenttable"

		#self.tty = open(port,'w+')
		#self.tty = open('/dev/tty.usbmodem1d1131','w+')
		#self.tty = serial.Serial('/dev/tty.usbmodem1d1131')
		#self.tty = open('/dev/tty.usbmodem1d1131','w', 1)
		
		try:
			print "2d1. i am about to try to open self.tty"
			self.tty = serial.Serial('/dev/tty.usbmodem1d1131', 9600, timeout=1)
			print "2d2. it appears that i have opened self.tty"
		except:
			print "2e. i have failed to open self.tty"

		time.sleep(1.5)

		print "2d. i have slept for 1.5s and made it past the __init__ try/except"

		# self.tty.write(' ') # resync

		print "2e. i skipped writing a blank to the tty"

		# read intro screen
		self.tty.write('H ')
		print "2f. i have written H+(blank) to the tty"
		self.tty.write('\n')

		print "2g. i have written a newline to the tty"

		r='' # readline ?
		while r!='\n' : 
			r=self.getch() 
			sys.stderr.write(r)
		# print >>sys.stderr,"synced"
		print "synced"
		

	def getch(self) :
		r=''
		print "2h. i am about to try to read one character"
		while r=='': 
			r=self.tty.read(1)
			if not r : time.sleep(0.020)
		return r

	def write_page(self,pageid,buf) : 
		assert len(buf) == PAGELEN,'wrong size:%d'%len(buf)
		if DEBUG : 
			print >>sys.stderr,'writing page %d'%pageid,
		s='W '+chr(pageid>>8)+chr(pageid&0xff)+' '+buf
		#print >>sys.stderr,'sending',repr(s)
		self.tty.write(s)
		print >>sys.stderr,' ...',
		r=self.getch()
		assert r=='$','expected $, got %s'%repr(r) # ok
		print >>sys.stderr,"ok, done"

	def read_page(self,pageid) :
		if DEBUG : 
			print >>sys.stderr,'reading page',pageid,':',
		s='R '+chr(pageid>>8)+chr(pageid&0xff)+' '
		#print >>sys.stderr,'sending',repr(s)

		self.tty.write(s)

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
		while (pg+1)*PAGELEN<len(str) : # il reste un buffer
			buf = str[pg*PAGELEN:(pg+1)*PAGELEN]
			self.write_page(start_page+pg+1,buf)
			if read : 
				buf2 = self.read_page(start_page+pg)
				if buf != buf2 : 
					print >>sys.stderr,"Values read different from written :"
					print >>sys.stderr,repr(buf)
					print >>sys.stderr,repr(buf2)
			pg+=1

		# dernier buffer
		a1=str[pg*PAGELEN:]
		a2=TRAIL_CHAR*(PAGELEN-len(a1))
		self.write_page(start_page+pg+1,a1+a2)
		pg += 1

		# table des matieres
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
			if r[2:4] == '\0\0' : break # mais inclut quand même le dernier
			x += 4

	def write_table(self) : 
		assert self.contenttable,"No content table read yet!"
		# test croissant ?
		# test aucun offset a zero sauf le dernier
		print >>sys.stderr,"writing table to chip"
		buf=SIGNATURE+''.join(chr(page>>8)+chr(page&0xff)+chr(offset>>8)+chr(offset&0xff) for page,offset in self.contenttable)
		assert len(buf)<PAGELEN,'table too big to be written !'
		self.write_page(0,buf+TRAIL_CHAR*(PAGELEN-len(buf)))
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
	
	print "1. hello i am here"

	try:
		print "2. i am about to set p"
		p=Programmer('/dev/tty.usbmodem1d1131')
		print "3. i have now set p"
	except:
		print "4. i have failed to open usb"

	print "5. i am now past the try/except"

	if len(sys.argv)==1 : 
		help()
	if sys.argv[1]=='ls' : 
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



	