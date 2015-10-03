/*
 ****************************************************************************
 *  Copyright (c) 2015 Uriah Liggett <freelaserscanner@gmail.com>           *
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

#include "Laser.h"
#include "Camera.h"
#include "PlyWriter.h"

namespace freelss
{

/**
 * Handles Wi-Fi Configuration management.
 */
class WifiConfig
{
public:

	/** Represents a Wifi access point */
	struct AccessPoint
	{
		/** Name of the AP */
		std::string essid;

		/** Name of the interface */
		std::string interface;

		/** Encryption status */
		bool encrypted;
	};

	/** Returns the singleton instance */
	static WifiConfig * get();

	/** Releases the singleton instance */
	static void release();

	/** Constructor */
	WifiConfig();

	/** Returns the names of the detected access points */
	std::vector<WifiConfig::AccessPoint> getAccessPoints();

	/** Returns the stored access point name */
	std::string getStoredAccessPointName();

	/** Returns a list of all the network interfaces */
	std::vector<std::string> getAllInterfaces();

	/** Returns a list of all the WiFi interfaces */
	std::vector<std::string> getWifiInterfaces();

	/** Returns the IP address of the interface or empty string if it has none */
	std::string getIpAddress(const std::string& interface);

	/** Finds all of the access points in range */
	void scan();

	/** Connects to the given access point and stores the credentials. */
	void connect(const std::string& accessPointName, const std::string& password);

private:

	/** The scanned access points */
	std::vector<AccessPoint> m_accessPoints;

	/** Returns a list of all the interfaces in the file */
	std::vector<std::string> getInterfaces(const std::string& filename);

	/** Parse the interface name from the proc line */
	std::string parseInterface(const std::string& line);

	/** The singleton instance */
	static WifiConfig * m_instance;
	static const std::string PROC_NET_WIRELESS;
	static const std::string PROC_NET_DEV;
	static const std::string WPA_SUPPLICANT;
};

}
