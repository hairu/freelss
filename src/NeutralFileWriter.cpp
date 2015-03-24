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
#include "NeutralFileWriter.h"

namespace freelss
{

const char * NeutralFileWriter::CREATE_DATABASE_SQL = "\
CREATE TABLE RESULTS (RED INTEGER, GREEN INTEGER, BLUE INTEGER, X REAL, Y REAL, Z REAL, COLUMN REAL, ROW REAL, ROTATION REAL, STEP INTEGER);\n";

NeutralFileWriter::NeutralFileWriter() :
	m_db(NULL),
	m_stmt(NULL)
{
	// Do nothing
}

NeutralFileWriter::~NeutralFileWriter()
{
	close();
}

void NeutralFileWriter::open(const std::string& filename)
{
	if (m_db != NULL)
	{
		return;
	}

	// If the file exists, delete it
	if (access(filename.c_str(), F_OK) == 0)
	{
		if (remove(filename.c_str()) == -1)
		{
			throw Exception("Error removing file: " + filename);
		}
	}

	if (sqlite3_open(filename.c_str(), &m_db) != 0)
	{
		throw Exception(std::string("Could not open sqlite database: ") + filename);
	}

	char * errMsg = 0;
	int rc = sqlite3_exec(m_db, NeutralFileWriter::CREATE_DATABASE_SQL, NULL, NULL, &errMsg);
	if (rc != SQLITE_OK)
	{
		std::string error = errMsg;
		sqlite3_free(errMsg);
		throw Exception("SQL Error: " + error);
	}

	// Disable wait on write
	errMsg = 0;
	rc = sqlite3_exec(m_db, "PRAGMA synchronous = OFF", NULL, NULL, &errMsg);
	if (rc != SQLITE_OK)
	{
		std::string error = errMsg;
		sqlite3_free(errMsg);
		throw Exception("Error setting synchronous mode: " + error);
	}

	// In-memory journal only
	errMsg = 0;
	rc = sqlite3_exec(m_db, "PRAGMA journal_mode = MEMORY", NULL, NULL, &errMsg);
	if (rc != SQLITE_OK)
	{
		std::string error = errMsg;
		sqlite3_free(errMsg);
		throw Exception("Error setting journal_mode: " + error);
	}
}

void NeutralFileWriter::close()
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

void NeutralFileWriter::write(const NeutralFileRecord& record)
{
	if (m_db == NULL)
	{
		throw Exception("The database is not open");
	}

	sqlite3_bind_int(m_stmt, 1, (int)record.point.r);
	sqlite3_bind_int(m_stmt, 2, (int)record.point.g);
	sqlite3_bind_int(m_stmt, 3, (int)record.point.b);
	sqlite3_bind_double(m_stmt, 4, record.point.x);
	sqlite3_bind_double(m_stmt, 5, record.point.y);
	sqlite3_bind_double(m_stmt, 6, record.point.z);
	sqlite3_bind_double(m_stmt, 7, record.pixel.x);
	sqlite3_bind_double(m_stmt, 8, record.pixel.y);
	sqlite3_bind_double(m_stmt, 9, record.rotation);
	sqlite3_bind_int(m_stmt, 10, record.frame);

	sqlite3_step(m_stmt);
	sqlite3_clear_bindings(m_stmt);
	sqlite3_reset(m_stmt);
}

void NeutralFileWriter::beginBatch()
{
	// Create the prepared statement
	prepareStatement();

	char * errMsg = 0;
	int rc = sqlite3_exec(m_db, "BEGIN TRANSACTION", NULL, NULL, &errMsg);
	if(rc != SQLITE_OK)
	{
		std::string error = errMsg;
		sqlite3_free(errMsg);
		throw Exception(std::string("Error beginning transaction: ") + error);
	}
}

void NeutralFileWriter::prepareStatement()
{
	if (m_stmt != NULL)
	{
		sqlite3_finalize(m_stmt);
		m_stmt = NULL;
	}

	// Create the prepared statement
	char * errMsg = 0;
	const char * sql = "INSERT INTO RESULTS (RED, GREEN, BLUE, X, Y, Z, COLUMN, ROW, ROTATION, STEP) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
	int rc = sqlite3_prepare_v2(m_db, sql, -1, &m_stmt, NULL);
	if (rc != SQLITE_OK)
	{
		std::string error = errMsg;
		sqlite3_free(errMsg);
		throw Exception(std::string("Error calling sqlite3_prepare_v2: ") + error);
	}
}

void NeutralFileWriter::commit()
{
	if (m_stmt == NULL)
	{
		return;
	}

	char * errMsg = 0;
	int rc = sqlite3_exec(m_db, "END TRANSACTION", NULL, NULL, &errMsg);
	if(rc != SQLITE_OK)
	{
		std::string error = errMsg;
		sqlite3_free(errMsg);
		throw Exception(std::string("Error committing transaction: ") + error);
	}

	sqlite3_finalize(m_stmt);
	m_stmt = NULL;
}

}
