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

#include "Laser.h"
#include "Progress.h"

namespace freelss
{

class Facetizer
{
public:
	Facetizer();

	void facetize(FaceMap & outFaces, std::vector<DataPoint>& results, bool connectLastFrameToFirst, Progress& progress, bool updateVertexNormals);

private:
	/**
	 * Indicates if the face is oriented point into the model and the normal needs to get flipped.
	 * This method assumes that the camera is looking straight down -z and x = 0.  The y doesn't matter.
	 * This method assumes that all points were taken from the same laser.
	 */
	bool isInwardFacingFace(const DataPoint& p1, const DataPoint& p2, const DataPoint& p3);
	bool isValidTriangle(const ColoredPoint& pt1, const ColoredPoint& pt2, const ColoredPoint& pt3);
	void addFacesForColumn(const std::vector<DataPoint>& currentFrame, const std::vector<DataPoint>& lastFrame, FaceMap& fout, uint32& numTriangles);
	void addTriangle(const DataPoint& pt1, const DataPoint& pt2, const DataPoint& pt3, bool flipNormal, FaceMap& fout);

	/** The max triangle edge distance in mm sq */
	real32 m_maxEdgeDistMmSq;

	/** The image width of the current camera */
	int m_imageWidth;
};

}
