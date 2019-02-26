/*
 ****************************************************************************
 *  Copyright (c) 2014 Uriah Liggett <freelaserscanner@gmail.com>           *
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
#include "UpdateManager.h"
#include "Lighting.h"
#include "WifiConfig.h"
#include "Logger.h"
#include "MountManager.h"
#include "BootConfigManager.h"
#include "MmalUtil.h"
#include <curl/curl.h>
#include <algorithm>

static std::string FREELSS_HOME_DIR = "/var/lib/freelss";

static bool SortRecordByRow(const freelss::DataPoint& a, const freelss::DataPoint& b)
{
	return a.pixel.y < b.pixel.y;
}

/** Initializes and destroys libcurl */
struct InitCurl
{
	InitCurl()
	{
		curl_global_init(CURL_GLOBAL_ALL);
	}

	~InitCurl()
	{
		curl_global_cleanup();
	}
};

/** Initializes and destroys the singletons */
struct InitSingletons
{
	InitSingletons()
	{
		freelss::PresetManager::get();
		freelss::A4988TurnTable::initialize();
		freelss::RelayLaser::initialize();
		freelss::UpdateManager::get();
		freelss::Setup::get();
		freelss::Lighting::get();
		freelss::WifiConfig::get();
		freelss::HttpServer::get();
		freelss::MountManager::get();
		freelss::BootConfigManager::get();

#ifndef MOCK
		freelss::MmalUtil::get();
#endif
	}

	~InitSingletons()
	{
		freelss::HttpServer::release();
		freelss::Laser::release();
		freelss::Camera::release();
		freelss::TurnTable::release();
		freelss::UpdateManager::release();
		freelss::PresetManager::release();
		freelss::Lighting::release();
		freelss::WifiConfig::release();
		freelss::Setup::release();
		freelss::MountManager::release();
		freelss::BootConfigManager::release();

#ifndef MOCK
		freelss::MmalUtil::release();
#endif

	}
};

/** Initializes and destroys the Raspberry Pi subsystem */
struct InitBcmHost
{
	InitBcmHost()
	{
		bcm_host_init();
	}

	~InitBcmHost()
	{
		bcm_host_deinit();
	}
};
namespace freelss
{
	void add(std::vector<freelss::DataPoint>& pts, real x, real y, real z, unsigned char r = 0, unsigned char g = 0, unsigned char b = 0)
	{
		DataPoint pt;
		pt.point.x = x;
		pt.point.y = y;
		pt.point.z = z;
		pt.point.r = r;
		pt.point.g = g;
		pt.point.b = b;
		pts.push_back(pt);
	}
}

int main(int argc, char **argv)
{
	int retVal = 0;
#ifndef MOCK
	pid_t pid = fork();
	if (pid < 0)
	{
		freelss::ErrorLog << "Error forking process!!!" << freelss::Logger::ENDL;
		return 1;
	}
	else if (pid != 0)
	{
		return 0;
	}
#endif

	// Create the output directories if they don't exist
	std::string homeDir = freelss::GetAppHomeDir();
	mkdir(homeDir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

	// Migrate to the new directory structure
	freelss::MigrateHome();

	std::string debugOutputDir = freelss::GetDebugOutputDir();
	mkdir(debugOutputDir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

	std::string updateDir = freelss::GetUpdateDir();
	mkdir(updateDir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

	std::string scanOutputDir = freelss::GetScanOutputDir();
	mkdir(scanOutputDir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

	// Initialize the Raspberry Pi hardware
	InitBcmHost bcmHost;

	try
	{
		// Setup wiring pi
		wiringPiSetup();

		// Initialize Curl
		InitCurl curl();

		// Load the properties
		freelss::LoadProperties();

		// Initialize the singletons
		InitSingletons singletons;

		int port = freelss::Setup::get()->httpPort;

		freelss::InfoLog << "Running on port " << port << "..." << freelss::Logger::ENDL;
		freelss::HttpServer::get()->start(port);

		while (true)
		{
			freelss::Thread::usleep(500000);  // Sleep 500ms
		}
	}
	catch (freelss::Exception& ex)
	{
		freelss::ErrorLog << "Exception: " << ex << freelss::Logger::ENDL;
		retVal = -1;
	}
	catch (std::exception& ex)
	{
		freelss::ErrorLog << "Exception: " << ex.what() << freelss::Logger::ENDL;
		retVal = -1;
	}
	catch (...)
	{
		freelss::ErrorLog << "Unknown Exception Occurred" << freelss::Logger::ENDL;
		retVal = -1;
	}

	return retVal;
}

namespace freelss
{

void MigrateHome()
{
	std::string oldPropertiesFile = "/.freelss.properties";
	std::string oldScanDir = "/scans";

	// Move the properties file
	std::string propertiesFile = GetPropertiesFile();
	if (access(propertiesFile.c_str(), F_OK) == -1 &&
		access(oldPropertiesFile.c_str(), F_OK) != -1)
	{
		if (rename(oldPropertiesFile.c_str(), propertiesFile.c_str()) == -1)
		{
			ErrorLog << "!! Error moving " << oldPropertiesFile << " to " << propertiesFile << Logger::ENDL;
		}
		else
		{
			ErrorLog << "Moved " << oldPropertiesFile << " to " << propertiesFile << Logger::ENDL;
		}
	}

	// Move the scans directory
	std::string scanOutputDir = GetScanOutputDir();
	if (access(scanOutputDir.c_str(), F_OK) == -1 &&
		access(oldScanDir.c_str(), F_OK) != -1)
	{
		if (rename(oldScanDir.c_str(), scanOutputDir.c_str()) == -1)
		{
			ErrorLog << "!! Error moving " << oldScanDir << " to " << scanOutputDir << Logger::ENDL;
		}
		else
		{
			ErrorLog << "Moved " << oldScanDir << " to " << scanOutputDir << Logger::ENDL;
		}
	}
}

std::string GetAppHomeDir()
{
	return FREELSS_HOME_DIR;
}

std::string GetScanOutputDir()
{
	return GetAppHomeDir() + "/scans";
}

std::string GetDebugOutputDir()
{
	return GetAppHomeDir() + "/debug";
}
std::string GetPropertiesFile()
{
	return GetAppHomeDir() + "/freelss.properties";
}

std::string GetUpdateDir()
{
	return GetAppHomeDir() + "/updates";
}

time_t ScanResult::getScanDate() const
{
	if (files.empty())
	{
		return 0;
	}

	return files.front().creationTime;
}

bool DataPoint::readNextFrame(std::vector<DataPoint>& out, const std::vector<DataPoint>& results, size_t & resultIndex)
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

bool DataPoint::readNextFrame(const std::vector<DataPoint>& results, size_t & resultIndex)
{
	if (resultIndex >= results.size() || results.empty())
	{
		return false;
	}

	int pseudoStep = results[resultIndex].pseudoFrame;
	while (pseudoStep == results[resultIndex].pseudoFrame && resultIndex < results.size())
	{
		resultIndex++;
	}

	return true;
}

void DataPoint::computeAverage(const std::vector<DataPoint>& bin, DataPoint& out)
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
		const DataPoint& br = bin[iBin];

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

void DataPoint::lowpassFilter(std::vector<DataPoint>& output, std::vector<DataPoint>& frame, unsigned maxNumRows, unsigned numRowBins)
{
	// Sanity check
	if (frame.empty())
	{
		return;
	}

	// No binning can occur, so copy
	if (numRowBins >= maxNumRows)
	{
		output.insert(output.end(), frame.begin(), frame.end());
		return;
	}

	// Sort by image row
	std::sort(frame.begin(), frame.end(), SortRecordByRow);

	unsigned binSize = maxNumRows / numRowBins;

	// Holds the current contents of the bin
	std::vector<DataPoint> bin;
	unsigned nextBinY = frame.front().pixel.y + binSize;

	// unsigned bin = frame.front().pixel.y / numRowBins;
	for (size_t iFr = 0; iFr < frame.size(); iFr++)
	{
		DataPoint& record = frame[iFr];

		if (record.pixel.y < nextBinY)
		{
			bin.push_back(record);
		}
		else
		{
			// Average the bin results and add it to the output
			if (!bin.empty())
			{
				DataPoint out;
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
		DataPoint out;
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
	std::string propertiesFile = GetPropertiesFile();
	if (access(propertiesFile.c_str(), F_OK) != -1)
	{
		properties = reader.readProperties(propertiesFile);
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

	properties.push_back(Property("freelss.majorVersion", ToString(FREELSS_VERSION_MAJOR)));
	properties.push_back(Property("freelss.minorVersion", ToString(FREELSS_VERSION_MINOR)));

	PropertyReaderWriter writer;
	writer.writeProperties(properties, GetPropertiesFile());
}

real ConvertUnitOfLength(real value, UnitOfLength srcUnits, UnitOfLength dstUnits)
{
	// Shortcut
	if (srcUnits == dstUnits)
	{
		return value;
	}

	//
	// Convert to millimeters
	//
	real mmValue;
	if (srcUnits == UL_MILLIMETERS)
	{
		mmValue = value;
	}
	else if (srcUnits == UL_CENTIMETERS)
	{
		mmValue = value * 10.0;
	}
	else if (srcUnits == UL_INCHES )
	{
		mmValue = value * 25.4;
	}
	else
	{
		throw Exception("Unsupported Unit of Length");
	}

	//
	// Convert to destination units
	//
	real out;
	if (dstUnits == UL_MILLIMETERS)
	{
		out = mmValue;
	}
	else if (dstUnits == UL_CENTIMETERS)
	{
		out = mmValue / 10.0;
	}
	else if (dstUnits == UL_INCHES)
	{
		out = mmValue / 25.4;
	}
	else
	{
		throw Exception("Unsupported Unit of Length");
	}

	return out;
}

double GetTimeInSeconds()
{
	struct timeval tv;
	if (gettimeofday(&tv, NULL) != 0)
	{
		ErrorLog << "Error calling gettimeofday()" << Logger::ENDL;
	}

	double sec = tv.tv_sec;

	if (tv.tv_usec != 0)
	{
		sec += tv.tv_usec / 1000000.0;  // US to SEC
	}

	return sec;
}

int GetFreeSpaceMb()
{
	int freeSpaceMb = 0;
	struct statvfs fileSystemInfo;
	std::string scanOutputDir = GetScanOutputDir();
	if (statvfs(scanOutputDir.c_str(), &fileSystemInfo) == 0)
	{
		real freeSpaceBytes = (real)fileSystemInfo.f_bsize * (real)fileSystemInfo.f_bfree;
		freeSpaceMb = (int)(freeSpaceBytes / 1024.0f / 1024.0f);
	}

	return freeSpaceMb;
}

std::string ToString(UnitOfLength unit)
{
	std::string out;

	switch(unit)
	{
	case UL_MILLIMETERS:
		out = "mm";
		break;

	case UL_CENTIMETERS:
		out = "cm";
		break;

	case UL_INCHES:
		out = "in";
		break;

	default:
		throw Exception("Unsupported unit of measure");
	}

	return out;
}

std::string ToHexString(unsigned char * data, size_t dataLength)
{
	std::stringstream sstr;
	for (size_t iDat = 0; iDat < dataLength; iDat++)
	{
		sstr << std::setbase(16) << (unsigned) data[iDat];
	}

	return sstr.str();
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

bool StartsWith(const std::string& str, const std::string& start)
{
	if (str.length() >= start.length())
	{
		return (0 == str.compare (0, start.length(), start));
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

static int ishex(int x)
{
	return	(x >= '0' && x <= '9')	||
		(x >= 'a' && x <= 'f')	||
		(x >= 'A' && x <= 'F');
}

std::string UrlDecode(const std::string& in)
{
	std::stringstream out;

	for (size_t idx = 0; idx < in.length(); idx++)
	{
		int a = in[idx];

		if (a == '+')
		{
			a = ' ';
		}
		else if (a == '%' && (idx + 2 < in.length()))
		{
			int b = in[idx + 1];
			int c = in[idx + 2];

			if (ishex(b) && ishex(c))
			{
				sscanf(&in[idx + 1], "%2x", &a);
			}
			else
			{
				ErrorLog << "!! Invalid URL encoded string" << Logger::ENDL;
				break;
			}
		}

		out << (char) a;
	}

	return out.str();
}

std::string TrimString(const std::string& str)
{
	if (str.empty())
	{
		return "";
	}
	
	size_t start = 0;
	for (size_t i = 0; i < str.size(); i++)
	{
		char c = str[i];
		if (c == '\r' || c == '\n' || c == ' ' || c == '\t')
		{
			start++;
		}
		else
		{
			break;
		}
	}
	
	if (start == str.size())
	{
		return "";
	}
	
	size_t end = str.size() - 1;
	while (end > start)
	{
		char c = str[end];
		if (c != '\r' && c != '\n' && c != ' ' && c != '\t')
		{
			break;
		}
		
		end--;
	}	
	
	return str.substr(start, (end - start + 1));
}
} // ns scanner


