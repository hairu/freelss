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
#include "Main.h"
#include "PlyReader.h"

namespace freelss
{

PlyReader::PlyReader()
{

}

void PlyReader::read(const std::string& filename, std::vector<ColoredPoint>& points)
{
	std::ifstream fin (filename.c_str());
	if (!fin.is_open())
	{
		throw Exception("Error opening file for reading: " + filename);
	}

	std::string line;
	if (!std::getline(fin, line) || line != "ply")
	{
		throw Exception("File is not a PLY: " + filename);
	}

	PlyDataFormat dataFormat;
	if (!std::getline(fin, line))
	{
		throw Exception("Invalid PLY format");
	}

	if (line == "format ascii 1.0")
	{
		dataFormat = PLY_ASCII;
	}
	else if (line == "format binary_little_endian 1.0")
	{
		dataFormat = PLY_BINARY;
	}
	else
	{
		throw Exception("Invalid PLY format.  Neither ASCII nor little endian binary detected.");
	}

	int numPts = -1;
	bool foundEndHeader = false;
	while (std::getline(fin, line))
	{
		if (line == "end_header")
		{
			foundEndHeader = true;
			break;
		}

		std::string elementVertexStart = "element vertex ";

		if (StartsWith(line, elementVertexStart))
		{
			numPts = ToInt(line.substr(elementVertexStart.size()));
		}
	}

	if (numPts == -1)
	{
		throw Exception("Could not detect vertex count");
	}

	if (!foundEndHeader)
	{
		throw Exception("Could not find end of header");
	}

	if (dataFormat == PLY_ASCII)
	{
		readAsciiPoints(points, fin, numPts);
	}
	else if (dataFormat == PLY_BINARY)
	{
		readBinaryPoints(points, fin, numPts);
	}
	else
	{
		throw Exception("Unsupported PlyDataFormat");
	}
}

void PlyReader::readBinaryPoints(std::vector<ColoredPoint>& points, std::ifstream& fin, int numPoints)
{
	points.resize(numPoints);

	for (int iPt = 0; iPt < numPoints; iPt++)
	{
		ColoredPoint & point = points[iPt];

		//
		// Make sure we have the proper data types/sizes
		//
		real32 x;
		real32 y;
		real32 z;
		real32 nx;
		real32 ny;
		real32 nz;
		unsigned char r;
		unsigned char g;
		unsigned char b;

		//
		// Read the record
		//
		fin.read((char *)&x, sizeof(x));
		fin.read((char *)&y, sizeof(y));
		fin.read((char *)&z, sizeof(z));
		fin.read((char *)&nx, sizeof(nx));
		fin.read((char *)&ny, sizeof(ny));
		fin.read((char *)&nz, sizeof(nz));
		fin.read((char *)&r, sizeof(r));
		fin.read((char *)&g, sizeof(g));
		fin.read((char *)&b, sizeof(b));

		//
		// Assign to the ColoredPoint
		//
		point.x = x;
		point.y = y;
		point.z = z;
		point.normal.x = nx;
		point.normal.y = ny;
		point.normal.z = nz;
		point.r = r;
		point.g = g;
		point.b = b;
	}
}

void PlyReader::readAsciiPoints(std::vector<ColoredPoint>& points, std::ifstream& fin, int numPoints)
{
	points.resize(numPoints);
	for (int iPt = 0; iPt < numPoints; iPt++)
	{
		ColoredPoint & point = points[iPt];

		fin >> point.x >> point.y >> point.z
			>> point.normal.x >> point.normal.y >> point.normal.z
			>> point.r >> point.g >> point.b;
	}
}

} // ns freelss
