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

class Progress;

class StlWriter
{
public:
	StlWriter();

	void write(const std::string& filename, const std::vector<DataPoint>& results, const FaceMap& faces, Progress& progress);

private:
	void writeHeader(std::ofstream& fout, const FaceMap& faces);
	void writeTriangle(const ColoredPoint& pt1, const ColoredPoint& pt2, const ColoredPoint& pt3, std::ofstream& fout);

	real32 m_normal[3];
	uint16 m_attribute;
};

}
