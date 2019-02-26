/*
 ****************************************************************************
 *  Copyright (c) 2016 Uriah Liggett <freelaserscanner@gmail.com>           *
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

/**
 * Reads PLY files as ColoredPoints.  This is not a general purpose PLY file reader
 * and is solely focused on reading the PLY files outputted by ATLAS 3D.
 */
class PlyReader
{
public:
	PlyReader();
	void read(const std::string& filename, std::vector<ColoredPoint>& points);
private:
	/** Reads points from an ASCII PLY file */
	void readAsciiPoints(std::vector<ColoredPoint>& points, std::ifstream& fin, int numPts);

	/** Reads points from a binary PLY file */
	void readBinaryPoints(std::vector<ColoredPoint>& points, std::ifstream& fin, int numPts);
};
}
