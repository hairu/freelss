// ttcapture.cpp (C)2015 Vik Olliver, GPLv3 applies.
// Uses a FabScan turntable and a webcam to scan a series of images
// suitable for feeding into a 3D point cloud reconstructor like ScanDraiD

#include <errno.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <stdexcept>

#include "Main.h"
#include "ArduinoSerial.h"

namespace scanner
{

int ArduinoSerial::m_fd=0;

ArduinoSerial::ArduinoSerial()
{
    // Nothing doing
}

ArduinoSerial::~ArduinoSerial()
{
    if (m_fd != 0)
    {
        close(m_fd);
    }
}

int set_interface_attribs (int fd, int speed, int parity)
{
        struct termios tty;
        memset (&tty, 0, sizeof tty);
        if (tcgetattr (fd, &tty) != 0)
        {
                std::cout << "error %d from tcgetattr" << errno <<"\n";
                return -1;
        }

        cfsetospeed (&tty, speed);
        cfsetispeed (&tty, speed);

        tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
        // disable IGNBRK for mismatched speed tests; otherwise receive break
        // as \000 chars
        tty.c_iflag &= ~IGNBRK;         // disable break processing
        tty.c_lflag = 0;                // no signaling chars, no echo,
                                        // no canonical processing
        tty.c_oflag = 0;                // no remapping, no delays
        tty.c_cc[VMIN]  = 0;            // read doesn't block
        tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

        tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

        tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
                                        // enable reading
        tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
        tty.c_cflag |= parity;
        tty.c_cflag &= ~CSTOPB;
        tty.c_cflag &= ~CRTSCTS;

        sleep(1);       // Avoid setting attribs soon after reading

        if (tcsetattr (fd, TCSANOW, &tty) != 0)
        {
                std::cout << "error " << errno << " from tcsetattr\n";
                return -1;
        }
        return 0;
}

void set_blocking (int fd, int should_block)
{
        struct termios tty;
        memset (&tty, 0, sizeof tty);
        if (tcgetattr (fd, &tty) != 0)
        {
                std::cout << "error " << errno << " from tggetattr\n";
                return;
        }

        tty.c_cc[VMIN]  = should_block ? 1 : 0;
        tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

        if (tcsetattr (fd, TCSANOW, &tty) != 0)
                std::cout << "error " << errno << " setting term attributes/n";
}

int ArduinoSerial::sendchar(char c)
{
    char buf[2];
    buf[0]=c;
    return(write(m_fd,&buf,1));

}

int ArduinoSerial::initialize(const char* port) {
 
    m_fd = open (port, O_RDWR | O_NOCTTY | O_SYNC);
    if (m_fd < 0)
    {
            std::cout << "error " << errno << " opening " << port << ": " << strerror(errno) << "\n";
            return 0;
    }
    return -1;
}
} // ns serial
