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
#include "PresetManager.h"
#include "HttpServer.h"
#include "PresetManager.h"
#include "PropertyReaderWriter.h"
#include "Setup.h"

void ReleaseSingletons()
{
	freelss::HttpServer::release();
	freelss::Laser::release();
	freelss::Camera::release();
}

std::string GetPropertiesFile()
{
	// Get the users home directory
	char * home = getenv("HOME");
	if (home == NULL)
	{
		std::cerr << "Could not detect the user's home directory";
		return "";
	}

	return home + std::string("/.freelss.properties");

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

		freelss::LoadProperties();
		freelss::A4988TurnTable::initialize();
		freelss::RelayLaser::initialize();

		int port = freelss::Setup::get()->httpPort;

		std::cout << "Running on port " << port << "..." << std::endl;
		freelss::HttpServer::get()->start(port);

		while (true)
		{
			freelss::Thread::usleep(100000);
		}


		ReleaseSingletons();
	}
	catch (freelss::Exception& ex)
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

namespace freelss
{

const std::string SCAN_OUTPUT_DIR = "/scans";
const std::string DEBUG_OUTPUT_DIR = "/debug";
const std::string PROPERTIES_FILE = GetPropertiesFile();

time_t ScanResult::getScanDate() const
{
	if (files.empty())
	{
		return 0;
	}

	return files.front().creationTime;
}

void LoadProperties()
{
	PropertyReaderWriter reader;

	std::vector<Property> properties;

	// Read the presets from the file
	if (access(PROPERTIES_FILE.c_str(), F_OK) != -1)
	{
		properties = reader.readProperties(PROPERTIES_FILE);
	}

	PresetManager * presetMgr = PresetManager::get();

	// Read the presets
	presetMgr->decodeProperties(properties);

	// Create the default Preset object if there are no presets
	if (presetMgr->getPresets().empty())
	{
		Preset defaultPreset;
		defaultPreset.name = "Default";

		defaultPreset.id = presetMgr->addPreset(defaultPreset);
		presetMgr->setActivePreset(defaultPreset.id);
	}

	// Load the hardware setup information
	Setup::get()->decodeProperties(properties);
}

void SaveProperties()
{
	// Encode the info as Property objects
	std::vector<Property> properties;
	PresetManager::get()->encodeProperties(properties);
	Setup::get()->encodeProperties(properties);

	PropertyReaderWriter writer;
	writer.writeProperties(properties, PROPERTIES_FILE);
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

std::string ToString(real value)
{
	std::stringstream sstr;
	sstr << value;

	return sstr.str();
}

std::string ToString(int value)
{
	std::stringstream sstr;
	sstr << value;

	return sstr.str();
}

real ToReal(const std::string& str)
{
	return atof(str.c_str());
}

int ToInt(const std::string& str)
{
	return atoi(str.c_str());
}

bool EndsWith(const std::string& str, const std::string& ending)
{
	if (str.length() >= ending.length())
	{
		return (0 == str.compare (str.length() - ending.length(), ending.length(), ending));
	}

	return false;
}

void HtmlEncode(std::string& str)
{
    std::string buffer;
    buffer.reserve(str.size());
    for(size_t iPos = 0; iPos != str.size(); ++iPos)
    {
        switch(str[iPos])
        {
            case '\"': buffer.append("&quot;");      break;
            case '\'': buffer.append("&apos;");      break;
            case '<':  buffer.append("&lt;");        break;
            case '>':  buffer.append("&gt;");        break;
            case '.':  buffer.append("&#46;");       break;
            case '=':  buffer.append("&#61;");       break;
            default:   buffer.append(&str[iPos], 1); break;
        }
    }

    str.swap(buffer);
}

} // ns scanner


