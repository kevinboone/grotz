# This is the Makefile for grotz. It is intended to work on modern-ish 
# Linux systems, and on Windows with MinGW and Msys installed. 
# To build a debug version, set the environment variable DEBUG. Otherwise
# you'll get a stripped version with no console output
#
# NB: The actually dependencies are in dependencies.mak.
#
# To build:
# make
#
# To build an installable bundle:
# make bundle
# (output is writtern to deploy/[platform]/grotz)

UNAME := $(shell uname -o)
BINDIR=/usr/bin
DESKTOPDIR=/usr/share/applications
PIXMAPDIR=/usr/share/pixmaps

.SUFFIXES: .o .c

APPNAME=grotz
PROJNAME=$(APPNAME)

PLATFORM=dummy
ifeq ($(UNAME),Msys)
        PLATFORM=win32
	APPBIN=$(APPNAME).exe
else  
        PLATFORM=linux
	APPBIN=$(APPNAME)
endif

OBJS=main.o MainWindow.o Settings.o SettingsDialog.o fileutils.o kbcomboboxtext.o StoryReader.o Interpreter.o StoryTerminal.o ZMachine.o blorbreader.o Picture.o MetaData.o ZTerminal.o charutils.o colourutils.o frotz_main.o frotz_buffer.o frotz_err.o frotz_sound.o frotz_process.o frotz_fastmem.o frotz_files.o frotz_hotkey.o frotz_input.o frotz_math.o frotz_object.o frotz_quetzal.o frotz_random.o frotz_redirect.o frotz_screen.o frotz_stream.o frotz_table.o frotz_text.o frotz_variable.o dialogs.o Sound.o MediaPlayer.o


APPS=$(APPBIN)
VERSION=0.2c

ifeq ($(DEBUG),1)
	DEBUG_CFLAGS=-g -DDEBUG=1
else
  ifeq ($(PLATFORM),win32)
        PROD_LDFLAGS=-s -mwindows
  else
        PROD_LDFLAGS=-s
  endif
endif


all: $(APPS)

ifeq ($(PLATFORM),win32)
  PLATFORM_LIBS=-L winstuff/lib -l gtk-win32-2.0 -lglib-2.0-0 -lgobject-2.0-0 -lgdk-win32-2.0 -lpango-1.0 -lgdk_pixbuf-2.0 -lpangocairo-1.0 -lgio-2.0
  PLATFORM_INCLUDES=-I winstuff/include/gtk-2.0 -I winstuff/include/glib-2.0 -I winstuff/include/cairo -I winstuff/include/pango-1.0 -I winstuff/include/atk-1.0
  PLATFORM_CFLAGS=-mms-bitfields
  OBJS += res.o 
else
  PLATFORM_LIBS=$(shell pkg-config --libs gtk+-2.0)
  PLATFORM_INCLUDES=$(shell pkg-config --cflags gtk+-2.0)
endif

include dependencies.mak

CFLAGS=-Wall $(DEBUG_CFLAGS) $(PLATFORM_CFLAGS) -DVERSION=\"$(VERSION)\"
INCLUDES=$(PLATFORM_INCLUDES) 
LIBS=$(PLATFORM_LIBS)


res.o: app.rc
	windres app.rc -o res.o

.c.o:
	gcc $(CFLAGS) $(INCLUDES) -DPIXMAPDIR=\"$(PIXMAPDIR)\" -DAPPNAME=\"$(APPNAME)\" -c $*.c -o $*.o

$(APPBIN): $(OBJS) 
	gcc $(PROD_LDFLAGS) $(DEBUG_LDFLAGS) $(LDFLAGS) -o $(APPNAME) $(OBJS) $(LIBS)

winbundle: all
	mkdir -p deploy/win32/$(PROJNAME)
	cp -pru winstuff/lib/* deploy/win32/$(PROJNAME)
	cp -p $(APPBIN) deploy/win32/$(PROJNAME)/
	cp -p scripts/*.bat deploy/win32/$(PROJNAME)/

linbundle: all
	mkdir -p deploy/linux/$(PROJNAME)
	cp -p $(APPBIN) deploy/linux/$(PROJNAME)/
	cp -p scripts/*.sh deploy/linux/$(PROJNAME)/

wininstall:
	echo Please run the program from the 'deploy\win32' directory

lininstall: linbundle
	cp -p deploy/linux/$(PROJNAME)/* $(BINDIR)
	cp grotz.desktop $(DESKTOPDIR)
	cp icons/grotz.png $(PIXMAPDIR)

ifeq ($(PLATFORM),win32)
bundle: all winbundle
else
bundle: all linbundle
endif

ifeq ($(PLATFORM),win32)
install: winbundle wininstall 
else
install: all lininstall
endif

clean:
	rm -f dump grotz grotz.exe *.o

veryclean: clean
	rm -rf deploy/*

