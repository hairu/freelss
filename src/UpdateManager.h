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

#include "CriticalSection.h"

namespace freelss
{

/**
 * This class handles the downloading and applying of updates.
 */
class UpdateManager
{
public:

	/** Returns the singleton instance */
	static UpdateManager * get();

	/** Releases the singleton instance */
	static void release();

	/** Returns info about the latest software update.  Ownership of returned is transferred to caller. */
	SoftwareUpdate * getLatestUpdate();

	/** Checks for updates */
	void checkForUpdates();

	/** Applies the given software update */
	void applyUpdate(SoftwareUpdate * update);

protected:

	/** Default Constructor */
	UpdateManager();

private:

	/** Downloads the file to the given destination */
	void downloadFile(const std::string& url,
			          const std::string& destination,
					  const std::string& postData = "");

	/** Downloads the file to the given destination */
	void downloadUpdate(const std::string& url,
			            const std::string& destination,
			            const std::string& serialNumber,
			            int majorVersion,
			            int minorVersion);

	/** Reads the updates from the given update file */
	std::vector<SoftwareUpdate> readUpdates(const std::string& filename);

	/** Singleton instance */
	static UpdateManager * m_instance;

	/** The url to check for updates */
	static const std::string UPDATE_URL;

	/** The directory where updates are temporarily stored */
	static const std::string UPDATE_DIR;

	/** The critical section for this instance */
	CriticalSection m_cs;

	/** The updates */
	std::vector<SoftwareUpdate> m_updates;
};

}
