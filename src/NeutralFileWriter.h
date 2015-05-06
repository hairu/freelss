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

namespace freelss
{

class NeutralFileWriter
{
public:
	NeutralFileWriter();
	~NeutralFileWriter();

	void beginBatch();
	void commit();
	void open(const std::string& filename);
	void close();
	void write(const NeutralFileRecord& record);
private:

	void prepareStatement();

	/** The SQL for creating the database */
	static const char * CREATE_DATABASE_SQL;

	/** The Sqlite database */
	sqlite3 * m_db;

	/** The prepared statement */
	sqlite3_stmt * m_stmt;
};

}
