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
#include "UpdateManager.h"
#include "Setup.h"
#include <rapidxml.hpp>
#include <curl/curl.h>
#include <iomanip>

using namespace rapidxml;

namespace freelss
{

UpdateManager * UpdateManager::m_instance = NULL;
const std::string UpdateManager::UPDATE_URL = "https://www.murobo.com/atlas3d/updates.xml";
const std::string UpdateManager::UPDATE_DIR = "/updates";

static bool SortByVersion(const SoftwareUpdate& a, const SoftwareUpdate& b)
{
	if (a.majorVersion != b.majorVersion)
	{
		return a.majorVersion < b.majorVersion;
	}

	return a.minorVersion < b.minorVersion;
}

UpdateManager::UpdateManager() :
	m_cs()
{
	// Create the update directory if it doesn't exist
	mkdir(UpdateManager::UPDATE_DIR.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}

UpdateManager * UpdateManager::get()
{
	if (UpdateManager::m_instance == NULL)
	{
		UpdateManager::m_instance = new UpdateManager();
	}

	return UpdateManager::m_instance;
}

void UpdateManager::release()
{
	delete UpdateManager::m_instance;
	m_instance = NULL;
}

void UpdateManager::downloadFile(const std::string& url, const std::string& destination, const std::string& postData)
{
	std::cout << destination << " <-- " << url << " " << postData << std::endl;

	CURL * curl = curl_easy_init();
	if (curl == NULL)
	{
		throw Exception("Error initializing CURL");
	}

	FILE * fp = fopen(destination.c_str(), "w");
	if (fp == NULL)
	{
		throw Exception("Error opening file for writing: " + destination);
	}

	try
	{
		// POST
		if (!postData.empty())
		{
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
		}

		curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		CURLcode curlCode = curl_easy_perform(curl);

		// Read back the return code
		long httpCode = 0;
		curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &httpCode);
		if (curlCode == CURLE_ABORTED_BY_CALLBACK)
		{
			throw Exception("Error downloading file");
		}
		else if (httpCode != 200)
		{
			// Close the output file
			fclose(fp);
			fp = NULL;

			std::ifstream fin (destination.c_str(), std::ios::binary);
			if (fin.is_open())
			{
				// Read the file the long way in case it's binary we need to terminate the string
				fin.seekg(0, std::ios::end);
				int fileSize = fin.tellg();
				fin.seekg(0, std::ios::beg);

				std::vector<char> text (fileSize + 1, 0);
				fin.read(&text.front(), fileSize);
				fin.close();

				std::stringstream sstr;
				sstr << httpCode << " Error: " << &text.front();
				throw Exception(sstr.str());
			}
			else
			{
				std::stringstream sstr;
				sstr << httpCode << " Error";
				throw Exception(sstr.str());
			}
		}
	}
	catch (...)
	{
		curl_easy_cleanup(curl);

		if (fp != NULL)
		{
			fclose(fp);
		}

		throw;
	}

	curl_easy_cleanup(curl);

	if (fp != NULL)
	{
		fclose(fp);
	}
}

void UpdateManager::downloadUpdate(const std::string& url, const std::string& destination, const std::string& serialNumber, int majorVersion, int minorVersion)
{
	// Build the post data
	std::stringstream sstr;
	sstr << "serial=" << serialNumber << "&majorVersion=" << majorVersion << "&minorVersion=" << minorVersion;
	downloadFile(url, destination, sstr.str());

	std::cout << "Retrieved file" << std::endl;
}

void UpdateManager::checkForUpdates()
{
	m_cs.enter();
	try
	{
		// Build a destination filename
		std::stringstream sstr;
		sstr << UpdateManager::UPDATE_DIR << "/" << std::setprecision(100) << GetTimeInSeconds() << ".xml";

		// Download the updates file
		downloadFile(UpdateManager::UPDATE_URL, sstr.str());

		// Read the updates
		m_updates = readUpdates(sstr.str());

		// Sort the updates by version
		std::sort(m_updates.begin(), m_updates.end(), SortByVersion);
	}
	catch (...)
	{
		m_cs.leave();
		throw;
	}

	m_cs.leave();
}

/**
<?xml version="1.0" encoding="utf-8"?>
<updates>
<update name="FreeLSS 1.2" url="https://www.murobo.com/freelss-1-2.tar.gz" description="New features" releaseDate="2014-03-05" majorVersion="1" minorVersion="2" />
</updates>
 */
std::vector<SoftwareUpdate> UpdateManager::readUpdates(const std::string& filename)
{
	std::cout << "Reading updates from " << filename << std::endl;

	//
	// Read the XML file into memory
	//
	std::ifstream fin (filename.c_str(), std::ios::binary);
	if (! fin.is_open())
	{
		throw Exception("Error opening file: " + filename);
	}

	std::stringstream buffer;
	buffer << fin.rdbuf();
	fin.close();

	std::string contents(buffer.str());

	std::vector<SoftwareUpdate> updates;

	//
	// Parse the file with RapidXML
	//
	xml_document<> doc;
	doc.parse<0>(&contents[0]);

	std::cout << "Done parsing " << filename << std::endl;

	xml_node<> * rootNode = doc.first_node();
	if (rootNode == NULL || rootNode->name() == NULL || std::string(rootNode->name()) != "updates")
	{
		throw Exception("Error finding updates section in updates file: " + filename);
	}

	xml_node<> * node = rootNode->first_node();
	while (node != NULL)
	{
		// Build the update
		if (node->name() != NULL && std::string(node->name()) == "update")
		{
			std::cout << "Found Update" << std::endl;

			SoftwareUpdate update;
			update.majorVersion = -1;
			update.minorVersion = -1;

			// Read the data
			for (xml_attribute<> *attr = node->first_attribute(); attr != NULL; attr = attr->next_attribute())
			{
				std::string name = "";
				if (attr->name() != NULL)
				{
					name = attr->name();
				}

				std::string value = "";
				if (attr->value() != NULL)
				{
					value = attr->value();
				}

				if ("name" == name)
				{
					update.name = value;
				}
				else if ("url" == name)
				{
					update.url = value;
				}
				else if ("description" == name)
				{
					update.description = value;
				}
				else if ("releaseDate" == name)
				{
					update.releaseDate = value;
				}
				else if ("majorVersion" == name)
				{
					update.majorVersion = ToInt(value);
				}
				else if ("minorVersion" == name)
				{
					update.minorVersion = ToInt(value);
				}
			}

			// Add this update to the output
			updates.push_back(update);
		}

		node = node->next_sibling();
	}

	std::cout << "Found " << updates.size() << " update records." << std::endl;

	return updates;
}

void UpdateManager::applyUpdate(SoftwareUpdate * update)
{
	if (update == NULL)
	{
		throw Exception("NULL Update");
	}

	Setup * setup = Setup::get();

	std::stringstream archivePath;
	archivePath << UpdateManager::UPDATE_DIR << "/freelss-" << update->majorVersion << "-" << update->minorVersion << ".tar.gz";

	// Download the update
	std::cout << "Downloading " << update->url << " to " << archivePath.str() << std::endl;
	downloadUpdate(update->url, archivePath.str(), setup->serialNumber, update->majorVersion, update->minorVersion);

	// Extract the update
	std::stringstream extractCmd;
	extractCmd << "tar xvzf " << archivePath.str() << " -C " << UpdateManager::UPDATE_DIR;

	std::cout << "Extracting with " << extractCmd.str() << std::endl;
	if (system(extractCmd.str().c_str()) == -1)
	{
		throw Exception("Error extracting update to " + UpdateManager::UPDATE_DIR);
	}

	// Run the update
	std::stringstream updateCmd;
	updateCmd << UpdateManager::UPDATE_DIR << "/freelss-" << update->majorVersion << "-" << update->minorVersion << "/update.sh";

	std::cout << "Applying with " << updateCmd.str() << std::endl;
	if (system(updateCmd.str().c_str()) == -1)
	{
		throw Exception("Error applying update with " + updateCmd.str());
	}

	std::cout << "Updated to "  << update->majorVersion << "." << update->minorVersion << "." << std::endl;
	std::cout << "Rebooting..." << std::endl;
	system("reboot&");
}

SoftwareUpdate * UpdateManager::getLatestUpdate()
{
	std::auto_ptr<SoftwareUpdate> update(NULL);

	m_cs.enter();
	try
	{
		if (!m_updates.empty())
		{
			const SoftwareUpdate & latest = m_updates.back();

			// If this update is newer than what we are currently running
			if (latest.majorVersion > FREELSS_VERSION_MAJOR || (latest.majorVersion == FREELSS_VERSION_MAJOR && latest.minorVersion > FREELSS_VERSION_MINOR))
			{
				update.reset(new SoftwareUpdate(latest));
			}
		}
	}
	catch (...)
	{
		m_cs.leave();
		throw;
	}

	m_cs.leave();

	return update.release();
}

}
