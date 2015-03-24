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
#include "NeutralFileReader.h"


namespace freelss
{

const char * NeutralFileReader::SELECT_SQL = "SELECT RED, GREEN, BLUE, X, Y, Z, COLUMN, ROW, ROTATION, STEP FROM RESULTS ORDER BY STEP ASC";

NeutralFileReader::NeutralFileReader() :
	m_db(NULL),
	m_stmt(NULL),
	m_recordReady(false),
	m_moreResults(false)
{
	// Do nothing
}


NeutralFileReader::~NeutralFileReader()
{
	close();
}

void NeutralFileReader::open(const std::string& filename)
{
	if (m_db != NULL)
	{
		return;
	}

	if (sqlite3_open(filename.c_str(), &m_db) != 0)
	{
		throw Exception(std::string("Could not open sqlite database: ") + filename);
	}
}

void NeutralFileReader::close()
{
	if (m_stmt != NULL)
	{
		sqlite3_finalize(m_stmt);
		m_stmt = NULL;
	}

	if (m_db != NULL)
	{
		sqlite3_close(m_db);
		m_db = NULL;
	}
}

void NeutralFileReader::start()
{
	if (m_stmt != NULL)
	{
		sqlite3_finalize(m_stmt);
		m_stmt = NULL;
	}

	// Create the prepared statement
	char * errMsg = 0;
	int rc = sqlite3_prepare_v2(m_db, NeutralFileReader::SELECT_SQL, -1, &m_stmt, NULL);
	if (rc != SQLITE_OK)
	{
		std::string error = errMsg;
		sqlite3_free(errMsg);
		throw Exception("Error calling sqlite3_prepare_v2: " + error);
	}

	m_recordReady = false;
	m_moreResults = true;
}

bool NeutralFileReader::readNextStep(std::vector<NeutralFileRecord>& records)
{
	if (m_stmt == NULL)
	{
		throw Exception("Prepared statement is NULL");
	}

	if (!m_moreResults)
	{
		return false;
	}

	// Clear any previous results
	records.clear();

	int lastStep = -1;

	// Copy the previously read record
	if (m_recordReady)
	{
		records.push_back(m_record);
		lastStep = m_record.frame;
		m_recordReady = false;
	}

	bool firstRow = true;
	while (true)
	{
		if (sqlite3_step(m_stmt) == SQLITE_ROW)
		{
			NeutralFileRecord record;

			record.point.r = sqlite3_column_int(m_stmt, 0);
			record.point.g = sqlite3_column_int(m_stmt, 1);
			record.point.b = sqlite3_column_int(m_stmt, 2);
			record.point.x = sqlite3_column_double(m_stmt, 3);
			record.point.y = sqlite3_column_double(m_stmt, 4);
			record.point.z = sqlite3_column_double(m_stmt, 5);
			record.pixel.x = sqlite3_column_double(m_stmt, 6);
			record.pixel.y = sqlite3_column_double(m_stmt, 7);
			record.rotation = sqlite3_column_double(m_stmt, 8);
			record.frame = sqlite3_column_int(m_stmt, 9);

			// Check if we are moving to the next scan frame
			if (!firstRow && record.frame != lastStep)
			{
				m_record = record;
				m_recordReady = true;
				lastStep = record.frame;
				break;
			}

			firstRow = false;
			lastStep = record.frame;
			records.push_back(record);
		}
		else
		{
			m_moreResults = false;
		}
	}

	return m_moreResults;
}

} // ns sdl
