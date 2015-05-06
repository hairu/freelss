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
#include "FileWriter.h"

namespace freelss
{

FileWriter::FileWriter (const char * filename) :
	m_fout(filename)
{
	// Do nothing
}

void FileWriter::write(const char * data, size_t len)
{
	m_fout.write(data, len);
}

void FileWriter::close()
{
	m_fout.close();
}

bool FileWriter::is_open() const
{
	return  m_fout.is_open();
}

}
