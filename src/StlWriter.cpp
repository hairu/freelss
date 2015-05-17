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
#include "StlWriter.h"
#include "Camera.h"
#include "Laser.h"
#include "Progress.h"

namespace freelss
{



StlWriter::StlWriter() :
	m_attribute(0)
{
	m_normal[0] = 0;
	m_normal[1] = 0;
	m_normal[2] = 0;
}


void StlWriter::write(const std::string& baseFilename, const std::vector<DataPoint>& results, const FaceMap& faces, Progress& progress)
{
	progress.setLabel("Generating STL file");
	progress.setPercent(0);

	std::string stlFilename = baseFilename + ".stl";
	std::ofstream fout (stlFilename.c_str());
	if (!fout.is_open())
	{
		throw Exception("Error opening STL file for writing: " + stlFilename);
	}

	// Write the STL header
	writeHeader(fout, faces);

	const std::vector<unsigned>& triangles = faces.triangles;

	for (size_t idx = 0; idx < triangles.size(); idx += 3)
	{
		const ColoredPoint& pt1 = results[triangles[idx]].point;
		const ColoredPoint& pt2 = results[triangles[idx + 1]].point;
		const ColoredPoint& pt3 = results[triangles[idx + 2]].point;

		writeTriangle(pt1, pt2, pt3, fout);
	}

	fout.close();
}

void StlWriter::writeHeader(std::ofstream& fout, const FaceMap& faces)
{
	unsigned char header[80];
	memset(header, 0, sizeof(header));
	strcpy((char *)header, "2015 Uriah Liggett");

	// Write header string
	fout.write((const char *)header, sizeof(unsigned char) * 80);

	// Write number of triangles
	uint32 numTriangles = (uint32) (faces.triangles.size() / 3);
	fout.write((const char *)&numTriangles, sizeof(uint32));
}

void StlWriter::writeTriangle(const ColoredPoint& pt1, const ColoredPoint& pt2, const ColoredPoint& pt3, std::ofstream& fout)
{
	fout.write((const char *)&m_normal, sizeof(real32) * 3);
	fout.write((const char *)&pt1.x, sizeof(real32));
	fout.write((const char *)&pt1.y, sizeof(real32));
	fout.write((const char *)&pt1.z, sizeof(real32));
	fout.write((const char *)&pt2.x, sizeof(real32));
	fout.write((const char *)&pt2.y, sizeof(real32));
	fout.write((const char *)&pt2.z, sizeof(real32));
	fout.write((const char *)&pt3.x, sizeof(real32));
	fout.write((const char *)&pt3.y, sizeof(real32));
	fout.write((const char *)&pt3.z, sizeof(real32));
	fout.write((const char *)&m_attribute, sizeof(uint16));
}

}
