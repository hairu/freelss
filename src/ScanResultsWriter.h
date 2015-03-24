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

#pragma once
#include "Thread.h"
#include "CriticalSection.h"

namespace freelss
{

/**
 * Writes the scan results in a separate thread so that the scan doesn't have to wait
 * on the I/O to complete.
 */
class ScanResultsWriter : public Thread
{
public:

	/** Constructor */
	ScanResultsWriter();
	void setBaseFilePath(const std::string& baseFilePath);
	void write(const NeutralFileRecord& record);

	/** The number of records not written yet */
	size_t getNumPendingRecords();

protected:

	/** The thread function */
	void run();

private:

	CriticalSection m_cs;
	std::list<NeutralFileRecord> m_records;
	std::string m_nfFilename;
	std::string m_plyFilename;
};
}
