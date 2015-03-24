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


class StlWriter
{
public:
	StlWriter();

	void write(const std::string& filename, const std::vector<NeutralFileRecord>& results, bool connectLastFrameToFirst);

private:
	void writeHeader(std::ofstream& fout);
	void populateBuffer(NeutralFileRecord * records, int numRecords, NeutralFileRecord ** buffer, int maxNumRecords);
	void writeTrianglesForColumn(const std::vector<NeutralFileRecord>& currentFrame, const std::vector<NeutralFileRecord>& lastFrame, std::ofstream& fout, uint32& numTriangles);
	void writeTriangle(const ColoredPoint& pt1, const ColoredPoint& pt2, const ColoredPoint& pt3, bool flipNormal, std::ofstream& fout);

	bool isValidTriangle(const ColoredPoint& pt1, const ColoredPoint& pt2, const ColoredPoint& pt3);

	/**
	 * Indicates if the face is oriented point into the model and the normal needs to get flipped.
	 * This method assumes that the camera is looking straight down -z and x = 0.  The y doesn't matter.
	 * This method assumes that all points were taken from the same laser.
	 */
	bool isInwardFacingFace(const NeutralFileRecord& p1, const NeutralFileRecord& p2, const NeutralFileRecord& p3);


	/** The max triangle edge distance in mm sq */
	real32 m_maxEdgeDistMmSq;

	real32 m_normal[3];
	uint16 m_attribute;

	/** The image width of the current camera */
	int m_imageWidth;
};

}
