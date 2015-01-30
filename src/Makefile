CC=g++
# Set target to "pi" or "linux" depending on target system's camera hardware
TARGET=linux
ifeq ($(TARGET),pi)
	LFLAGS=-lraspicam -lmmal -lmmal_core -lmmal_util -lvcos -lpthread -lpng -ljpeg -lsqlite3 -lwiringPi -lmicrohttpd -lbcm_host -L/usr/local/lib -L/opt/vc/lib
else
	LFLAGS=-lpthread -lpng -ljpeg -lsqlite3 -lmicrohttpd -L/usr/local/lib -L/opt/vc/lib
	HW_DEF=-DUSE_LINUX_HARDWARE
endif

CFLAGS=-O3 -Wall -Winvalid-pch -fexceptions $(HW_DEF) -I/usr/local/include -I/opt/vc/include \
	-I/opt/vc/include/interface/vcos/pthreads \
	-I/opt/vc/include/interface/vmcs_host/linux \
	-I/opt/vc/include/interface\
	-I/opt/vc/include/interface/mmal/util

CORE_OBJECTS=WebContent.o Settings.o Main.o \
	HttpServer.o CriticalSection.o Scanner.o Image.o ImageProcessor.o \
	Thread.o LocationMapper.o \
	PlyWriter.o Calibrator.o RaspistillCamera.o \
	NeutralFileWriter.o NeutralFileReader.o ArduinoLaser.o \
	ArduinoTurnTable.o Camera.o ScanResultsWriter.o \
	PixelLocationWriter.o StlWriter.o TurnTable.o \
	Laser.o ArduinoSerial.o

PI_OBJECTS=MmalStillCamera.o MmalVideoCamera.o RaspicamCamera.o A4988TurnTable.o RelayLaser.o

ifeq ($(TARGET),pi) 
OBJECTS=$(CORE_OBJECTS) $(PI_OBJECTS)
else
OBJECTS=$(CORE_OBJECTS)
endif

all: freelss

freelss: Main.h.gch $(OBJECTS)
	$(CC) $(OBJECTS) -o freelss $(LFLAGS)
	
Main.h.gch: Main.h
	$(CC) -c $(CFLAGS) Main.h -o Main.h.gch

Main.o: Main.cpp Main.h Thread.h ArduinoSerial.h
	$(CC) -c $(CFLAGS) Main.cpp

# Pi only objects
ifeq ($(TARGET),pi)
MmalStillCamera.o: MmalStillCamera.cpp MmalStillCamera.h Main.h
	$(CC) -c $(CFLAGS) MmalStillCamera.cpp
	
MmalVideoCamera.o: MmalVideoCamera.cpp MmalVideoCamera.h Main.h
	$(CC) -c $(CFLAGS) MmalVideoCamera.cpp

RaspicamCamera.o: RaspicamCamera.cpp RaspicamCamera.h Main.h
	$(CC) -c $(CFLAGS) RaspicamCamera.cpp

A4988TurnTable.o: A4988TurnTable.cpp A4988TurnTable.h TurnTable.h Main.h
	$(CC) -c $(CFLAGS) A4988TurnTable.cpp

RelayLaser.o: RelayLaser.cpp RelayLaser.h Laser.h Main.h
	$(CC) -c $(CFLAGS) RelayLaser.cpp
endif

CriticalSection.o: CriticalSection.cpp CriticalSection.h Main.h
	$(CC) -c $(CFLAGS) CriticalSection.cpp

Scanner.o: Scanner.cpp Scanner.h Main.h
	$(CC) -c $(CFLAGS) Scanner.cpp

HttpServer.o: HttpServer.cpp HttpServer.h Main.h
	$(CC) -c $(CFLAGS) HttpServer.cpp

Settings.o: Settings.cpp Settings.h Main.h
	$(CC) -c $(CFLAGS) Settings.cpp

Image.o: Image.cpp Image.h
	$(CC) -c $(CFLAGS) Image.cpp

ImageProcessor.o: ImageProcessor.cpp ImageProcessor.h Main.h
	$(CC) -c $(CFLAGS) ImageProcessor.cpp

RaspistillCamera.o: RaspistillCamera.cpp RaspistillCamera.h Main.h
	$(CC) -c $(CFLAGS) RaspistillCamera.cpp

ArduinoTurnTable.o: ArduinoTurnTable.cpp ArduinoTurnTable.h ArduinoSerial.h fsdefines.h TurnTable.h Main.h
	$(CC) -c $(CFLAGS) ArduinoTurnTable.cpp

ArduinoLaser.o: ArduinoLaser.cpp ArduinoLaser.h Laser.h Main.h fsdefines.h ArduinoSerial.h
	$(CC) -c $(CFLAGS) ArduinoLaser.cpp

ArduinoSerial.o: ArduinoSerial.cpp ArduinoSerial.h Main.h
	$(CC) -c $(CFLAGS) ArduinoSerial.cpp

Thread.o: Thread.cpp Thread.h Main.h
	$(CC) -c $(CFLAGS) Thread.cpp

LocationMapper.o: LocationMapper.cpp LocationMapper.h Main.h
	$(CC) -c $(CFLAGS) LocationMapper.cpp

PlyWriter.o: PlyWriter.cpp PlyWriter.h Main.h
	$(CC) -c $(CFLAGS) PlyWriter.cpp

PixelLocationWriter.o: PixelLocationWriter.cpp PixelLocationWriter.h Main.h
	$(CC) -c $(CFLAGS) PixelLocationWriter.cpp

WebContent.o: WebContent.cpp WebContent.h Main.h
	$(CC) -c $(CFLAGS) WebContent.cpp

Calibrator.o: Calibrator.cpp Calibrator.h Main.h
	$(CC) -c $(CFLAGS) Calibrator.cpp

NeutralFileWriter.o: NeutralFileWriter.cpp NeutralFileWriter.h Main.h
	$(CC) -c $(CFLAGS) NeutralFileWriter.cpp

NeutralFileReader.o: NeutralFileReader.cpp NeutralFileReader.h Main.h
	$(CC) -c $(CFLAGS) NeutralFileReader.cpp

Camera.o: Camera.cpp Camera.h Main.h
	$(CC) -c $(CFLAGS) Camera.cpp

ScanResultsWriter.o: ScanResultsWriter.cpp ScanResultsWriter.h Main.h
	$(CC) -c $(CFLAGS) ScanResultsWriter.cpp

StlWriter.o: StlWriter.cpp StlWriter.h Main.h
	$(CC) -c $(CFLAGS) StlWriter.cpp

TurnTable.o: TurnTable.cpp TurnTable.h Main.h
	$(CC) -c $(CFLAGS) TurnTable.cpp

Laser.o: Laser.cpp Laser.h Main.h
	$(CC) -c $(CFLAGS) Laser.cpp

github:
	mkdir -p ../github
	mkdir -p ../github/src
	cp -f *.h *.cpp Makefile ../github/src

run: freelss
	sudo ./freelss

dist: clean
	tar -cvzf freelss.tar.gz ../../Scanner --exclude=".svn"

clean:
	rm -f *.o *.gch *~ freelss
