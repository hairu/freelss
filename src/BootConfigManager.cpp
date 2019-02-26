/*
 ****************************************************************************
 *  Copyright (c) 2016 Uriah Liggett <freelaserscanner@gmail.com>           *
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
#include "BootConfigManager.h"
#include "Logger.h"

namespace freelss
{

std::string BootConfigManager::GPU_MEM = "gpu_mem";
std::string BootConfigManager::DISABLE_CAMERA_LED = "disable_camera_led";
std::string BootConfigManager::BOOT_CONFIG = "/boot/config.txt";

BootConfigManager * BootConfigManager::m_instance = NULL;

bool BootConfigManager::Settings::operator == (const Settings& a) const
{
	return gpuMemoryMb == a.gpuMemoryMb && ledDisabled == a.ledDisabled;
}

bool BootConfigManager::Settings::operator != (const Settings& a) const
{
	return ! (*this == a);
}

BootConfigManager::BootConfigManager()
{
	// Do nothing
}

BootConfigManager::Settings BootConfigManager::readSettings()
{
	BootConfigManager::Settings settings;
	settings.gpuMemoryMb = 128;
	settings.ledDisabled = false;

	std::vector<std::string> lines = readFileLines(BootConfigManager::BOOT_CONFIG);

	for (size_t iLn = 0; iLn < lines.size(); iLn++)
	{
		std::string line = lines[iLn];

		std::string value;
		if (parseLine(BootConfigManager::GPU_MEM, line, value))
		{
			settings.gpuMemoryMb = ToInt(value);
		}

		if (parseLine(BootConfigManager::DISABLE_CAMERA_LED, line, value))
		{
			settings.ledDisabled = ToBool(value);
		}
	}

	return settings;
}

bool BootConfigManager::parseLine(const std::string& search, const std::string& inLine, std::string& out)
{
	std::string line = TrimString(inLine);
	bool found = false;

	if (line.find(search) == 0 && line.size() > search.size() + 1)
	{
		std::string s2 = TrimString(line.substr(search.size()));
		if (s2.size() > 1 && s2[0] == '=')
		{
			out = TrimString(s2.substr(1));
			found = true;
		}
	}

	return found;
}

void BootConfigManager::writeSettings(BootConfigManager::Settings& settings)
{
	std::vector<std::string> lines = readFileLines(BootConfigManager::BOOT_CONFIG);
	std::ofstream fout(BootConfigManager::BOOT_CONFIG.c_str());
	if (!fout.is_open())
	{
		throw Exception("Error opening file for writing: " + BootConfigManager::BOOT_CONFIG);
	}

	bool foundGpuLine = false;
	bool foundLedLine = false;
	for (size_t iLn = 0; iLn < lines.size(); iLn++)
	{
		std::string line = lines[iLn];
		std::string value;

		if (parseLine(BootConfigManager::GPU_MEM, line, value))
		{
			foundGpuLine = true;
			fout << BootConfigManager::GPU_MEM << "=" << settings.gpuMemoryMb << std::endl;
		}
		else if (parseLine(BootConfigManager::DISABLE_CAMERA_LED, line, value))
		{
			foundLedLine = true;
			fout << BootConfigManager::DISABLE_CAMERA_LED << "=" << (settings.ledDisabled ? "1" : "0") << std::endl;
		}
		else
		{
			fout << line << std::endl;
		}
	}

	if (!foundGpuLine)
	{
		fout << BootConfigManager::GPU_MEM << "=" << settings.gpuMemoryMb << std::endl;
	}

	if (!foundLedLine)
	{
		fout << BootConfigManager::DISABLE_CAMERA_LED << "=" << (settings.ledDisabled ? "1" : "0") << std::endl;
	}

	fout.close();
}

BootConfigManager * BootConfigManager::get()
{
	if (BootConfigManager::m_instance == NULL)
	{
		BootConfigManager::m_instance = new BootConfigManager();
	}

	return BootConfigManager::m_instance;
}

std::vector<std::string> BootConfigManager::readFileLines(const std::string& filename)
{
	std::ifstream fin(filename.c_str());
	if (!fin.is_open())
	{
		throw Exception("Error opening file for reading: " + filename);
	}

	std::vector<std::string> lines;
	std::stringstream line;
	char c;

	while (fin.get(c))
	{
		if (c == '\n')
		{
			lines.push_back(line.str());
			line.str("");
		}
		else
		{
			line << c;
		}
	}

	// Get the last line
	if (!line.str().empty())
	{
		lines.push_back(line.str());
	}

	return lines;
}

void BootConfigManager::release()
{
	delete m_instance;
	m_instance = NULL;
}

}
