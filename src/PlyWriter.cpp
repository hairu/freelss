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

namespace freelss
{

PlyWriter::PlyWriter() :
		m_filename(),
		m_totalNumPoints(0)
{
	// Do nothing
}

PlyWriter::~PlyWriter()
{
	if (m_fout.is_open())
	{
		m_fout.close();
	}
}

void PlyWriter::begin(const char * filename)
{
	m_filename = filename;
	m_totalNumPoints = 0;
	m_fout.open(filename);
	if (!m_fout.is_open())
	{
		throw Exception(std::string("Error opening points file, ") + filename);
	}
	
	m_fout << "ply" << std::endl;
	m_fout << "format ascii 1.0" << std::endl;
	m_fout << "element vertex             " << std::endl;  // Leave space for updating the vertex count
	m_fout << "property float x" << std::endl;
	m_fout << "property float y" << std::endl;
	m_fout << "property float z" << std::endl;
	m_fout << "property float nx" << std::endl;
	m_fout << "property float ny" << std::endl;
	m_fout << "property float nz" << std::endl;
	m_fout << "property uchar red" << std::endl;
	m_fout << "property uchar green" << std::endl;
	m_fout << "property uchar blue" << std::endl;
	m_fout << "element face 0" << std::endl;
	m_fout << "property list uchar int vertex_indices" << std::endl;
	m_fout << "end_header" << std::endl;
}

void PlyWriter::writePoints(ColoredPoint * points, int numPoints)
{
	m_totalNumPoints += numPoints;
	for (int iPt = 0; iPt < numPoints; iPt++)
	{
		const ColoredPoint & point = points[iPt];
		m_fout << point.x << " " << point.y << " " << point.z << " "
			   << point.normal.x << " " << point.normal.y << " " << point.normal.z << " "
			   << (int)point.r << " " << (int)point.g << " " << (int)point.b << std::endl;
	}
}

void PlyWriter::end()
{
	// Close the file
	m_fout.close();

	// Re-open the file and update the point count
	if (m_totalNumPoints > 0)
	{
		int filePos = 36;  // The file position where we should write the point count
		std::fstream fout (m_filename.c_str());
		if (!fout.is_open())
		{
			throw Exception(std::string("Error updating vertex count in PLY file: ") + m_filename);
		}

		fout.seekp(filePos, std::ios_base::beg);
		std::stringstream sstr;
		sstr << m_totalNumPoints;

		std::string countStr = sstr.str();
		fout.write(countStr.c_str(), countStr.size());
		fout.close();
	}
}

}
