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
#include "Main.h"
#include "WifiConfig.h"

namespace freelss
{

WifiConfig * WifiConfig::m_instance = NULL;
const std::string WifiConfig::PROC_NET_WIRELESS = "/proc/net/wireless";
const std::string WifiConfig::PROC_NET_DEV = "/proc/net/dev";
const std::string WifiConfig::WPA_SUPPLICANT = "/etc/wpa_supplicant/wpa_supplicant.conf";

WifiConfig * WifiConfig::get()
{
	if (WifiConfig::m_instance == NULL)
	{
		WifiConfig::m_instance = new WifiConfig();
	}

	return WifiConfig::m_instance;
}

void WifiConfig::release()
{
	delete WifiConfig::m_instance;
	WifiConfig::m_instance = NULL;
}

WifiConfig::WifiConfig()
{
	// Do nothing
}

std::vector<WifiConfig::AccessPoint> WifiConfig::getAccessPoints()
{
	return m_accessPoints;
}

std::string WifiConfig::getStoredAccessPointName()
{
	std::string essid;
	const std::string search = "ssid=\"";
	std::ifstream fin(WPA_SUPPLICANT.c_str());
	if (!fin.is_open())
	{
		throw Exception("Error opening file for reading: " + WPA_SUPPLICANT);
	}

	std::string line;
	while(std::getline(fin, line))
	{
		size_t pos1 = line.find(search);
		size_t pos2 = line.find_last_of('"');
		if (pos1 != std::string::npos &&
			pos2 != std::string::npos &&
			pos2 > pos1 + search.size() + 1 &&
			line.find("#") == std::string::npos)
		{
			size_t pos3 = pos1 + search.size();
			essid = line.substr(pos3, pos2 - pos3);
			break;
		}
	}

	fin.close();

	return essid;
}

std::string WifiConfig::parseInterface(const std::string& line)
{
	std::string interface = "";

	size_t colonPos = line.find_first_of(":");
	if (colonPos != std::string::npos && colonPos > 1)
	{
		interface = line.substr(1, colonPos - 1);
	}

	// Trim Left
	size_t pos = 0;
	for (pos = 0; pos < interface.size() && (interface[pos] == ' ' || interface[pos] == '\t'); pos++)
	{
		// Do nothing
	}

	if (pos > 0)
	{
		interface = interface.substr(pos);
	}

	return interface;
}

void WifiConfig::scan()
{
	m_accessPoints.clear();

	int sk = iw_sockets_open();
	if (sk < 0)
	{
		throw Exception("Error opening wireless socket");
	}

	try
	{
		std::ifstream fin(PROC_NET_WIRELESS.c_str());
		if (!fin.is_open())
		{
			throw Exception("Error opening file: " + PROC_NET_WIRELESS);
		}

		std::string line;
		for (size_t iLn = 0; std::getline(fin, line); iLn++)
		{
			// Skip the first 2 header lines
			if (iLn >= 2)
			{
				std::string interface = parseInterface(line);
				if (!interface.empty())
				{
					std::cout << "Scanning wireless device: " << interface << std::endl;

					std::vector<char> chars(interface.c_str(), interface.c_str() + interface.size() + 1u);
					struct wireless_scan_head context;
					int ret = iw_scan(sk, &chars.front(), iw_get_kernel_we_version(), &context);
					if (ret < 0)
					{
						throw Exception("Wireless scanning failed");
					}

					while(context.result)
					{
						WifiConfig::AccessPoint accessPoint;

						if(context.result->b.has_essid && context.result->b.essid_on)
						{
							accessPoint.essid = context.result->b.essid;
							if (!accessPoint.essid.empty())
							{
								accessPoint.encrypted = context.result->b.has_key;
								accessPoint.interface = interface;
								std::cout << "ESSID: " << accessPoint.essid << ", encrypted=" << accessPoint.encrypted << ", interface=" << accessPoint.interface << std::endl;

								m_accessPoints.push_back(accessPoint);
							}
						}

						context.result = context.result->next;
					}
				}
			}
		}
		fin.close();
	}
	catch (...)
	{
		iw_sockets_close(sk);
		throw;
	}

	iw_sockets_close(sk);
}

std::vector<std::string> WifiConfig::getInterfaces(const std::string& filename)
{
	std::vector<std::string> interfaces;

	std::ifstream fin(filename.c_str());
	if (!fin.is_open())
	{
		throw Exception("Error opening file: " + filename);
	}

	std::string line;
	for (size_t iLn = 0; std::getline(fin, line); iLn++)
	{
		// Skip the first 2 header lines
		if (iLn >= 2)
		{
			std::string interface = parseInterface(line);
			if (!interface.empty())
			{
				interfaces.push_back(interface);
			}
		}
	}

	fin.close();

	return interfaces;
}

std::vector<std::string>  WifiConfig::getAllInterfaces()
{
	return getInterfaces(PROC_NET_DEV);
}

std::vector<std::string>  WifiConfig::getWifiInterfaces()
{
	return getInterfaces(PROC_NET_WIRELESS);
}

std::string WifiConfig::getIpAddress(const std::string& interface)
{
	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd == -1)
	{
		throw Exception("Error creating datagram socket");
	}

	struct ifreq ifr;
	memset(&ifr, 0, sizeof(ifreq));

	// Get IPV4 Address
	ifr.ifr_addr.sa_family = AF_INET;

	strncpy(ifr.ifr_name, interface.c_str(), IFNAMSIZ-1);

	ioctl(fd, SIOCGIFADDR, &ifr);

	close(fd);

	std::string address;
	const char * str = inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
	if (str != NULL)
	{
		address = str;
	}

	return address;
}

void WifiConfig::connect(const std::string& essid, const std::string& password)
{
	//
	// Read the WPA supplicant file
	//
	std::ifstream fin(WPA_SUPPLICANT.c_str());
	if (!fin.is_open())
	{
		throw Exception("Error opening file for reading: " + WPA_SUPPLICANT);
	}

	std::vector<std::string> lines;
	std::string line;
	while(std::getline(fin, line))
	{
		lines.push_back(line);
	}

	fin.close();

	std::ofstream fout(WPA_SUPPLICANT.c_str());
	if (!fout.is_open())
	{
		throw Exception("Error opening file for writing: " + WPA_SUPPLICANT);
	}

	for (size_t iLn = 0; iLn < lines.size(); iLn++)
	{
		line = lines[iLn];
		if (line.find("ssid=\"") != std::string::npos && line.find("#") == std::string::npos)
		{
			line = "ssid=\"" + essid + "\"";
		}
		else if (line.find("psk=\"") != std::string::npos && line.find("#") == std::string::npos)
		{
			line = "psk=\"" + password + "\"";
		}

		fout << line << std::endl;
	}

	fout.close();

	std::string interface;
	for (size_t iInt = 0; iInt < m_accessPoints.size(); iInt++)
	{
		if (essid == m_accessPoints[iInt].essid)
		{
			interface = m_accessPoints[iInt].interface;
			break;
		}
	}

	if (!interface.empty())
	{
		// Restart the wifi connection
		std::string cmd = "ifdown " + interface;
		std::cout << "Executing " << cmd << std::endl;
		if (system(cmd.c_str()) == -1)
		{
			std::cerr << "Error calling: " << cmd << std::endl;
		}

		cmd = "ifup " + interface;
		std::cout << "Executing " << cmd << std::endl;
		if (system(cmd.c_str()) == -1)
		{
			std::cerr << "Error calling: " << cmd << std::endl;
		}
	}
	else
	{
		std::cerr << "Error finding information for wireless interface" << std::endl;
	}
}

}
