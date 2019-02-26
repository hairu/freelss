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
#include "MountManager.h"
#include "Logger.h"

namespace freelss
{

MountManager * MountManager::m_instance = NULL;
const std::string MountManager::MTAB_PATH = "/etc/mtab";
const std::string MountManager::MEDIA_PATH = "/media";
const std::string MountManager::MNT_PATH = "/mnt";

MountManager * MountManager::get()
{
	if (MountManager::m_instance == NULL)
	{
		MountManager::m_instance = new MountManager();
	}

	return MountManager::m_instance;
}

void MountManager::release()
{
	delete MountManager::m_instance;
	MountManager::m_instance = NULL;
}

MountManager::MountManager()
{
	mkdir(MTAB_PATH.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	mkdir(MEDIA_PATH.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	mkdir(MNT_PATH.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

	std::vector<std::string> paths = getValidMounts();
	for (size_t i = 0; i < paths.size(); i++)
	{
		InfoLog << "Mount Path: " << paths[i] << Logger::ENDL;
	}
}

std::string MountManager::mount(const std::string& serverPath, const std::string& username,
		                        const std::string& password, const std::string& workgroup)
{
	if (TrimString(serverPath).empty() || TrimString(username).empty())
	{
		throw Exception("Invalid mount parameters");
	}

	std::string target = serverPath;
	for (size_t i = 0; i < target.size(); i++)
	{
		if (!isalnum(target[i]))
		{
			target[i] = '_';
		}
	}

	target = MNT_PATH + "/" + target;

	if (isMounted(target))
	{
		throw Exception(target + " is already mounted.");
	}

	mkdir(target.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

	std::string data = "username=" + username;

	if (!password.empty())
	{
		data += ",password=" + password;
	}

	if (!workgroup.empty())
	{
		data += ",workgroup=" + workgroup;
	}

	int retVal = ::mount(serverPath.c_str(), target.c_str(), "cifs", 0, data.c_str());
	if (retVal != 0)
	{
		throw Exception("Mount failed from " + serverPath + " to " + target);
	}
	else
	{
		InfoLog << "Mounted " << serverPath << " to " << target << Logger::ENDL;
	}

	return target;
}

bool MountManager::isMounted(const std::string& path)
{
	std::vector<std::string> mounts = getMounts();
	bool found = false;
	for (size_t i = 0; i < mounts.size(); i++)
	{
		if (path == mounts[i])
		{
			found = true;
			break;
		}
	}
	return found;
}

std::string MountManager::parseMtabLine(const std::string& line)
{
	std::string path;
	std::string device;

	std::istringstream iss(line);
	iss >> device >> path;

	return TrimString(path);
}

std::vector<std::string> MountManager::getValidMounts()
{
	std::vector<std::string> paths = getMounts();
	std::vector<std::string> valids;

	for (size_t i = 0; i < paths.size(); i++)
	{
		std::string path = paths[i];
		if (path.find("/media/") == 0 || path.find("/mnt/") == 0)
		{
			valids.push_back(path);
		}
	}

	return valids;
}
std::vector<std::string> MountManager::getMounts()
{
	std::vector<std::string> mounts;

	std::ifstream fin(MountManager::MTAB_PATH.c_str());
	if (!fin.is_open())
	{
		throw Exception("Error opening file: " + MountManager::MTAB_PATH);
	}

	std::string line;
	for (size_t iLn = 0; std::getline(fin, line); iLn++)
	{
		// Skip the first 2 header lines
		if (iLn >= 2)
		{
			std::string path = parseMtabLine(line);
			if (!path.empty())
			{
				mounts.push_back(path);
			}
		}
	}

	fin.close();

	return mounts;
}

}
