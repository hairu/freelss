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

#pragma once

namespace freelss
{

/**
 * This class interacts with /boot/config.txt file.
 */
class BootConfigManager
{
public:

	struct Settings
	{
		bool operator == (const Settings& a) const;
		bool operator != (const Settings& a) const;

		int gpuMemoryMb;
		bool ledDisabled;
	};

	BootConfigManager();

	BootConfigManager::Settings readSettings();
	void writeSettings(BootConfigManager::Settings& settings);

	static BootConfigManager * get();
	static void release();
private:
	std::vector<std::string> readFileLines(const std::string& filename);
	bool parseLine(const std::string& search, const std::string& line, std::string& out);
private:
	static std::string GPU_MEM;
	static std::string DISABLE_CAMERA_LED;
	static std::string BOOT_CONFIG;
	static BootConfigManager * m_instance;
};
}
