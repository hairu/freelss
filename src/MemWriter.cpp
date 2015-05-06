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
#include "MemWriter.h"

namespace freelss
{

void MemWriter::write(const char * inData, size_t len)
{
	const unsigned char * data = reinterpret_cast<const unsigned char *>(inData);
	for (size_t i = 0; i < len; i++)
	{
		m_data.push_back(data[i]);
	}
}

const std::vector<unsigned char>& MemWriter::getData() const
{
	return  m_data;
}

}
