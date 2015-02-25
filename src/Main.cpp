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
#include <algorithm>

static bool SortRecordByRow(const freelss::NeutralFileRecord& a, const freelss::NeutralFileRecord& b)
{
	return a.pixel.y < b.pixel.y;
}

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
	
	pid_t pid = fork();
	if (pid < 0)
	{
		std::cerr << "Error forking process!!!" << std::endl;
		return 1;
	}
	else if (pid != 0)
	{
		return 0;
	}

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
			freelss::Thread::usleep(500000);  // Sleep 500ms
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

bool NeutralFileRecord::readNextFrame(std::vector<NeutralFileRecord>& out, const std::vector<NeutralFileRecord>& results, size_t & resultIndex)
{
	out.clear();

	if (resultIndex >= results.size() || results.empty())
	{
		return false;
	}

	int pseudoStep = results[resultIndex].pseudoFrame;
	while (pseudoStep == results[resultIndex].pseudoFrame && resultIndex < results.size())
	{
		out.push_back(results[resultIndex]);
		resultIndex++;
	}

	return true;
}

void NeutralFileRecord::computeAverage(const std::vector<NeutralFileRecord>& bin, NeutralFileRecord& out)
{
	out = bin.front();

	real32 invSize = 1.0f / bin.size();

	real32 rotation = 0;
	real32 pixelLocationX = 0;
	real32 pixelLocationY = 0;
	real32 ptX = 0;
	real32 ptY = 0;
	real32 ptZ = 0;
	real32 ptR = 0;
	real32 ptG = 0;
	real32 ptB = 0;

	for (size_t iBin = 0; iBin < bin.size(); iBin++)
	{
		const NeutralFileRecord& br = bin[iBin];

		rotation += invSize * br.rotation;
		pixelLocationX += invSize * br.pixel.x;
		pixelLocationY += invSize * br.pixel.y;
		ptX += invSize * br.point.x;
		ptY += invSize * br.point.y;
		ptZ += invSize * br.point.z;
		ptR += invSize * br.point.r;
		ptG += invSize * br.point.g;
		ptB += invSize * br.point.b;
	}

	out.rotation = rotation;
	out.pixel.x = pixelLocationX;  // TODO: We should probably round these values
	out.pixel.y = pixelLocationY;
	out.point.x = ptX;
	out.point.y = ptY;
	out.point.z = ptZ;
	out.point.r = ptR;
	out.point.g = ptG;
	out.point.b = ptB;
}

void NeutralFileRecord::lowpassFilter(std::vector<NeutralFileRecord>& output, std::vector<NeutralFileRecord>& frame, unsigned maxNumRows, unsigned numRowBins)
{
	output.clear();

	// Sanity check
	if (frame.empty())
	{
		return;
	}

	// Sort by image row
	std::sort(frame.begin(), frame.end(), SortRecordByRow);

	unsigned binSize = maxNumRows / numRowBins;

	// Holds the current contents of the bin
	std::vector<NeutralFileRecord> bin;
	unsigned nextBinY = frame.front().pixel.y + binSize;

	// unsigned bin = frame.front().pixel.y / numRowBins;
	for (size_t iFr = 0; iFr < frame.size(); iFr++)
	{
		NeutralFileRecord& record = frame[iFr];

		if (record.pixel.y < nextBinY)
		{
			bin.push_back(record);
		}
		else
		{
			// Average the bin results and add it to the output
			if (!bin.empty())
			{
				NeutralFileRecord out;
				computeAverage(bin, out);

				output.push_back(out);
				bin.clear();
			}

			nextBinY = record.pixel.y + binSize;
			bin.push_back(record);
		}
	}

	// Process any results still left in the bin
	if (!bin.empty())
	{
		NeutralFileRecord out;
		computeAverage(bin, out);

		output.push_back(out);
		bin.clear();
	}
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

std::string ToString(bool value)
{
	return value ? "1" : "0";
}

real ToReal(const std::string& str)
{
	return atof(str.c_str());
}

int ToInt(const std::string& str)
{
	return atoi(str.c_str());
}

bool ToBool(const std::string& str)
{
	return str == "1" || str == "true";
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


