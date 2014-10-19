/*
 ****************************************************************************
 *  Copyright (c) 2014 Uriah Liggett <hairu526@gmail.com>                   *
 *	This file is part of FreeLSS.                                           *
 *                                                                          *
 *  FreeLSS is free software: you can redistribute it and/or modify         *
 *  it under the terms of the GNU General Public License as published by    *
 *  the Free Software Foundation, either version 3 of the License, or       *
 *  (at your option) any later version.                                     *
 *                                                                          *
 *  FreeLSS is distributed in the hope that it will be useful,              *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *  GNU General Public License for more details.                            *
 *                                                                          *
 *   You should have received a copy of the GNU General Public License      *
 *   along with FreeLSS.  If not, see <http://www.gnu.org/licenses/>.       *
 ****************************************************************************
*/

#include "Main.h"
#include "Camera.h"
#include "Scanner.h"
#include "A4988TurnTable.h"
#include "RelayLaser.h"
#include "Settings.h"
#include "HttpServer.h"


void ReleaseSingletons()
{
	scanner::HttpServer::release();
	scanner::Laser::release();
	scanner::Camera::release();
	scanner::Settings::release();
}

int main(int argc, char **argv)
{
	int retVal = 0;
	
	// Initialize the Raspberry Pi hardware
	bcm_host_init();

	std::cout << "Initialized BCM HOST" << std::endl;

	try
	{
		// Setup wiring pi
		wiringPiSetup();
		
		// Get the users home directory
		char * home = getenv("HOME");
		if (home == NULL)
		{
			throw scanner::Exception("Could not detect the user's home directory");
		}

		std::string settingsDb = home + std::string("/.scannerdb");

		scanner::Settings::initialize(settingsDb.c_str());
		scanner::A4988TurnTable::initialize();
		scanner::RelayLaser::initialize();

		int port = 80;
		scanner::HttpServer::get()->start(port);

		std::cout << "Running on port " << port << "..." << std::endl;
		char input;
		while (true)
		{
			std::cin >> input;
			if (input == 'Q')
			{
				break;
			}
		}
		std::cout << "Received: " << input << std::endl;

		ReleaseSingletons();
	}
	catch (scanner::Exception& ex)
	{
		std::cerr << "Exception: " << ex << std::endl;
		ReleaseSingletons();
		retVal = -1;
	}
	catch (std::exception& ex)
	{
		std::cerr << "Exception: " << ex.what() << std::endl;
		ReleaseSingletons();
		retVal = -1;
	}
	catch (...)
	{
		std::cerr << "Unknown Exception Occurred" << std::endl;
		ReleaseSingletons();
		retVal = -1;
	}


	return retVal;
}

namespace scanner
{

time_t ScanResult::getScanDate() const
{
	if (files.empty())
	{
		return 0;
	}

	return files.front().creationTime;
}

double GetTimeInSeconds()
{
	struct timeval tv;
	if (gettimeofday(&tv, NULL) != 0)
	{
		std::cerr << "Error calling gettimeofday()" << std::endl;
	}

	double sec = tv.tv_sec;

	if (tv.tv_usec != 0)
	{
		sec += tv.tv_usec / 1000000.0;  // US to SEC
	}

	return sec;
}

} // ns scanner


