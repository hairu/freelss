CC=g++
CFLAGS=-O3 -Wall -Winvalid-pch -fexceptions -I/usr/local/include -I/opt/vc/include \
	-I/opt/vc/include/interface/vcos/pthreads \
	-I/opt/vc/include/interface/vmcs_host/linux \
	-I/opt/vc/include/interface\
	-I/opt/vc/include/interface/mmal/util\
	-I../contrib

LFLAGS=-lcurl -lraspicam -lmmal -lmmal_core -lmmal_util -lvcos -lpthread -lpng -ljpeg -lsqlite3 -lwiringPi -lmicrohttpd -lbcm_host -L/usr/local/lib -L/opt/vc/lib
OBJECTS=WebContent.o PropertyReaderWriter.o Main.o \
	HttpServer.o CriticalSection.o Scanner.o Image.o ImageProcessor.o \
	Thread.o A4988TurnTable.o RelayLaser.o LocationMapper.o \
	PlyWriter.o Calibrator.o RaspistillCamera.o \
	RaspicamCamera.o \
	Camera.o MmalStillCamera.o \
	MmalVideoCamera.o PixelLocationWriter.o StlWriter.o TurnTable.o \
	Laser.o Preset.o PresetManager.o Setup.o LaserResultsMerger.o\
	XyzWriter.o UpdateManager.o Progress.o FileWriter.o MemWriter.o \
	Facetizer.o

all: freelss 

freelss: Main.h.gch $(OBJECTS)
	$(CC) $(OBJECTS) -o freelss $(LFLAGS)
	
Main.h.gch: Main.h
	$(CC) -c $(CFLAGS) Main.h -o Main.h.gch

Main.o: Main.cpp Main.h Thread.h
	$(CC) -c $(CFLAGS) Main.cpp

CriticalSection.o: CriticalSection.cpp CriticalSection.h Main.h
	$(CC) -c $(CFLAGS) CriticalSection.cpp

Scanner.o: Scanner.cpp Scanner.h Main.h
	$(CC) -c $(CFLAGS) Scanner.cpp

HttpServer.o: HttpServer.cpp HttpServer.h Main.h
	$(CC) -c $(CFLAGS) HttpServer.cpp

PropertyReaderWriter.o: PropertyReaderWriter.cpp PropertyReaderWriter.h Main.h
	$(CC) -c $(CFLAGS) PropertyReaderWriter.cpp

Image.o: Image.cpp Image.h
	$(CC) -c $(CFLAGS) Image.cpp

ImageProcessor.o: ImageProcessor.cpp ImageProcessor.h Main.h
	$(CC) -c $(CFLAGS) ImageProcessor.cpp

MmalStillCamera.o: MmalStillCamera.cpp MmalStillCamera.h Main.h
	$(CC) -c $(CFLAGS) MmalStillCamera.cpp
	
MmalVideoCamera.o: MmalVideoCamera.cpp MmalVideoCamera.h Main.h
	$(CC) -c $(CFLAGS) MmalVideoCamera.cpp

RaspistillCamera.o: RaspistillCamera.cpp RaspistillCamera.h Main.h
	$(CC) -c $(CFLAGS) RaspistillCamera.cpp

RaspicamCamera.o: RaspicamCamera.cpp RaspicamCamera.h Main.h
	$(CC) -c $(CFLAGS) RaspicamCamera.cpp

A4988TurnTable.o: A4988TurnTable.cpp A4988TurnTable.h TurnTable.h Main.h
	$(CC) -c $(CFLAGS) A4988TurnTable.cpp

RelayLaser.o: RelayLaser.cpp RelayLaser.h Laser.h Main.h
	$(CC) -c $(CFLAGS) RelayLaser.cpp

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

Camera.o: Camera.cpp Camera.h Main.h
	$(CC) -c $(CFLAGS) Camera.cpp

StlWriter.o: StlWriter.cpp StlWriter.h Main.h
	$(CC) -c $(CFLAGS) StlWriter.cpp

TurnTable.o: TurnTable.cpp TurnTable.h Main.h
	$(CC) -c $(CFLAGS) TurnTable.cpp

Laser.o: Laser.cpp Laser.h Main.h
	$(CC) -c $(CFLAGS) Laser.cpp

Preset.o: Preset.cpp Preset.h Main.h
	$(CC) -c $(CFLAGS) Preset.cpp

PresetManager.o: PresetManager.cpp PresetManager.h Main.h
	$(CC) -c $(CFLAGS) PresetManager.cpp

Setup.o: Setup.cpp Setup.h Main.h
	$(CC) -c $(CFLAGS) Setup.cpp

LaserResultsMerger.o: LaserResultsMerger.cpp LaserResultsMerger.h Main.h
	$(CC) -c $(CFLAGS) LaserResultsMerger.cpp

XyzWriter.o: XyzWriter.cpp XyzWriter.h Main.h
	$(CC) -c $(CFLAGS) XyzWriter.cpp

UpdateManager.o: UpdateManager.cpp UpdateManager.h Main.h
	$(CC) -c $(CFLAGS) UpdateManager.cpp

Progress.o: Progress.cpp Progress.h Main.h
	$(CC) -c $(CFLAGS) Progress.cpp

FileWriter.o: FileWriter.cpp FileWriter.h Main.h
	$(CC) -c $(CFLAGS) FileWriter.cpp

MemWriter.o: MemWriter.cpp MemWriter.h Main.h
	$(CC) -c $(CFLAGS) MemWriter.cpp

Facetizer.o: Facetizer.cpp Facetizer.h Main.h
	$(CC) -c $(CFLAGS) Facetizer.cpp

github:
	mkdir -p ../../github
	mkdir -p ../../github/src
	mkdir -p ../../github/contrib
	cp -f *.h *.cpp Makefile ../../github/src
	cp -f ../contrib/*.h ../contrib/*.hpp ../../github/contrib

run: freelss
	sudo ./freelss

dist: clean
	tar -cvzf freelss.tar.gz ../../Scanner --exclude=".svn"

install: freelss
	sudo cp -f freelss /usr/local/bin/freelss

startup: install
	sudo cp ../config/freelss /etc/init.d/freelss
	sudo update-rc.d freelss remove
	sudo update-rc.d freelss defaults

clean:
	rm -f *.o *.gch *~ freelss
