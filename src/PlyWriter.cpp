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
#include "PlyWriter.h"
#include "Preset.h"
#include "IWriter.h"

namespace freelss
{

PlyWriter::PlyWriter() :
		m_writer(NULL),
		m_totalNumPoints(0),
		m_totalNumFaces(0),
		m_numPointsWritten(0),
		m_dataFormat(PLY_BINARY)
{
	// Do nothing
}

void PlyWriter::setTotalNumPoints(int totalNumPoints)
{
	m_totalNumPoints = totalNumPoints;
}

void PlyWriter::setTotalNumFacesFromFaceMap(const FaceMap& faces)
{
	m_totalNumFaces = faces.triangles.size() / 3;
}

void PlyWriter::setDataFormat(PlyDataFormat dataRepresentation)
{
	m_dataFormat = dataRepresentation;
}

void PlyWriter::begin(IWriter * writer)
{
	m_writer = writer;

	std::cout << "Writing to PLY file..." << std::endl;

	std::stringstream sstr;

	sstr << "ply" << std::endl;
	sstr << "format ";

	if (m_dataFormat == PLY_ASCII)
	{
		sstr << "ascii";
	}
	else if (m_dataFormat == PLY_BINARY)
	{
		sstr << "binary_little_endian";
	}
	else
	{
		throw Exception("Unsupported PLY data format");
	}

	sstr << " 1.0" << std::endl;
	sstr << "element vertex " << m_totalNumPoints << std::endl;  // Leave space for updating the vertex count
	sstr << "property float x" << std::endl;
	sstr << "property float y" << std::endl;
	sstr << "property float z" << std::endl;
	sstr << "property float nx" << std::endl;
	sstr << "property float ny" << std::endl;
	sstr << "property float nz" << std::endl;
	sstr << "property uchar red" << std::endl;
	sstr << "property uchar green" << std::endl;
	sstr << "property uchar blue" << std::endl;
	sstr << "element face " << m_totalNumFaces << std::endl;
	sstr << "property list uchar int vertex_indices" << std::endl;
	sstr << "end_header" << std::endl;

	std::string header = sstr.str();
	m_writer->write(header.c_str(), header.size());
}

void PlyWriter::writeFaces(const FaceMap& faces)
{
	if (m_dataFormat == PLY_ASCII)
	{
		writeAsciiFaces(faces);
	}
	else if (m_dataFormat == PLY_BINARY)
	{
		writeBinaryFaces(faces);
	}
	else
	{
		throw Exception("Unsupported PLY data format");
	}
}

void PlyWriter::writeAsciiFaces(const FaceMap& faces)
{
	const std::vector<unsigned>& triangles = faces.triangles;

	for (size_t idx = 0; idx < triangles.size(); idx += 3)
	{
		std::stringstream sstr;
		sstr << "3 " << triangles[idx] << " " << triangles[idx + 1] << " " << triangles[idx + 2] << std::endl;

		std::string data = sstr.str();
		m_writer->write(data.c_str(), data.size());
	}
}

void PlyWriter::writeBinaryFaces(const FaceMap& faces)
{
	const std::vector<unsigned>& triangles = faces.triangles;

	for (size_t idx = 0; idx < triangles.size(); idx += 3)
	{
		uint8 numVert = 3;
		uint32 v1 = triangles[idx];
		uint32 v2 = triangles[idx + 1];
		uint32 v3 = triangles[idx + 2];

		m_writer->write((const char *)&numVert, sizeof(numVert));
		m_writer->write((const char *)&v1, sizeof(v1));
		m_writer->write((const char *)&v2, sizeof(v2));
		m_writer->write((const char *)&v3, sizeof(v3));
	}
}

void PlyWriter::writePoints(ColoredPoint * points, int numPoints)
{
	// Check the number of points
	if (m_numPointsWritten + numPoints > m_totalNumPoints)
	{
		throw Exception("Attempt to write more PLY points than the indicated max");
	}

	if (m_dataFormat == PLY_ASCII)
	{
		writeAsciiPoints(points, numPoints);
	}
	else if (m_dataFormat == PLY_BINARY)
	{
		writeBinaryPoints(points, numPoints);
	}
	else
	{
		throw Exception("Unsupported PLY data format");
	}

	m_numPointsWritten += numPoints;
}

void PlyWriter::writeAsciiPoints(ColoredPoint * points, int numPoints)
{
	for (int iPt = 0; iPt < numPoints; iPt++)
	{
		const ColoredPoint & point = points[iPt];

		std::stringstream sstr;
		sstr << point.x << " " << point.y << " " << point.z << " "
			 << point.normal.x << " " << point.normal.y << " " << point.normal.z << " "
			 << (int)point.r << " " << (int)point.g << " " << (int)point.b << std::endl;

		std::string data = sstr.str();
		m_writer->write(data.c_str(), data.size());
	}
}

void PlyWriter::writeBinaryPoints(ColoredPoint * points, int numPoints)
{
	for (int iPt = 0; iPt < numPoints; iPt++)
	{
		const ColoredPoint & point = points[iPt];

		//
		// Make sure we have the proper data types/sizes
		//
		real32 x = point.x;
		real32 y = point.y;
		real32 z = point.z;
		real32 nx = point.normal.x;
		real32 ny = point.normal.y;
		real32 nz = point.normal.z;
		unsigned char r = point.r;
		unsigned char g = point.g;
		unsigned char b = point.b;

		//
		// Write the record
		//
		m_writer->write((const char *)&x, sizeof(x));
		m_writer->write((const char *)&y, sizeof(y));
		m_writer->write((const char *)&z, sizeof(z));
		m_writer->write((const char *)&nx, sizeof(nx));
		m_writer->write((const char *)&ny, sizeof(ny));
		m_writer->write((const char *)&nz, sizeof(nz));
		m_writer->write((const char *)&r, sizeof(r));
		m_writer->write((const char *)&g, sizeof(g));
		m_writer->write((const char *)&b, sizeof(b));
	}
}

void PlyWriter::end()
{
	// Ensure that all of the points were written
	if (m_totalNumPoints != m_numPointsWritten)
	{
		std::stringstream sstr;
		sstr << "Only " << m_numPointsWritten << " of the " << m_totalNumPoints << " expected PLY points were written.";
		throw Exception(sstr.str());
	}
}

}
