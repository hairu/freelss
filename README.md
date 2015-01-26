#FreeLSS

FreeLSS is a laser scanning program for the Raspberry Pi. It allows a Raspberry Pi to function as the core to a complete turn table laser scanning system.



###COMPILE

These instructions assume you are running the latest version of Raspbian.  Other distros will likely require changes.

First, update the firmware to the latest version and reboot.
```
$ sudo rpi-update
```

Install the dependencies that are managed by the package manager.
```
$ sudo apt-get install libpng-dev libjpeg-dev cmake vlc git-core gcc build-essential unzip sqlite3 libsqlite3-dev libmicrohttpd-dev
```
If using a standard Linux webcam also install packages for that:
```
$ sudo apt-get install fswebcam
```

Download and install wiringPi
```
$ cd
$ git clone git://git.drogon.net/wiringPi
$ cd wiringPi
$ ./build
```

Download and install Raspicam
```
$ cd
$ wget http://downloads.sourceforge.net/project/raspicam/raspicam-0.1.1.zip
$ unzip raspicam-0.1.1.zip
$ cd raspicam-0.1.1
$ mkdir build
$ cd build
$ cmake ..
$ make
$ sudo make install
$ sudo ldconfig
```
Download FreeLSS
```
$ cd
$ git clone https://github.com/hairu/freelss
$ cd freelss/src
```
Now edit Makefile with your favourite editor and set the TARGET variable to either "pi" or "linux" depending on which camera and turntable hardware you wish to use.

Finally, build FreeLSS itself
```
$ make
```
###Running FreeLSS
FreeLSS must be ran as root (or another user with access to the hardware pins).  The interface for FreeLSS is web based and by default runs on port 8080 (you can change this to 80 in Main.cpp if running as root).  When running, access FreeLSS by navigating to http://localhost:8080/ from the Raspberry Pi itself. Or access it from another machine on the network by the Raspberry Pi's IP or hostname.  For Example: http://raspberrypi:8080/

The following command starts FreeLSS as root.
```
$ sudo ./freelss
```
