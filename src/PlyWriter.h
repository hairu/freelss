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

enum PlyDataFormat { PLY_ASCII, PLY_BINARY };

class IWriter;

class PlyWriter
{
public:
	PlyWriter();

	void setDataFormat(PlyDataFormat dataRepresentation);
	void setTotalNumPoints(int totalNumPoints);
	void setTotalNumFacesFromFaceMap(const FaceMap& faces);
	void begin(IWriter * writer);
	void writePoints(ColoredPoint * points, int numPoints);
	void writeFaces(const FaceMap& faces);
	void end();
private:

	void writeAsciiPoints(ColoredPoint * points, int numPoints);
	void writeBinaryPoints(ColoredPoint * points, int numPoints);
	void writeAsciiFaces(const FaceMap& faces);
	void writeBinaryFaces(const FaceMap& faces);

	IWriter * m_writer;
	int m_totalNumPoints;
	int m_totalNumFaces;
	int m_numPointsWritten;
	PlyDataFormat m_dataFormat;
};

}
