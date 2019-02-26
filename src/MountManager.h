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
 * This class handles network mounts.
 */
class MountManager
{
public:
	MountManager();

	/** Returns the singleton instance */
	static MountManager * get();

	/** Releases the singleton instance */
	static void release();

	std::string mount(const std::string& serverPath, const std::string& username,
			          const std::string& password, const std::string& workgroup);

	bool isMounted(const std::string& path);

	std::vector<std::string> getValidMounts();
	std::vector<std::string> getMounts();
private:

	std::string parseMtabLine(const std::string& line);

	/** Singleton instance */
	static MountManager * m_instance;
	static const std::string MTAB_PATH;
	static const std::string MEDIA_PATH;
	static const std::string MNT_PATH;
};

}
