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

#pragma once


#include "Progress.h"

namespace freelss
{

class ObjectBaseCreator
{
public:
	ObjectBaseCreator();

	void createBase(FaceMap & outFaces, std::vector<DataPoint>& results, real32 groundPlaneHeight, int numSubdivisions, Progress& progress);

private:
	void subdivide(std::vector<unsigned>& triangles, std::vector<DataPoint>& results);
	DataPoint splitEdge(const DataPoint& p1, const DataPoint& p2, std::vector<DataPoint>& results);
	void addTriangle(const DataPoint& pt1, const DataPoint& pt2, const DataPoint& pt3, std::vector<unsigned>& triangles);
	void updateTriangle(unsigned idx1, const DataPoint& pt1, const DataPoint& pt2, const DataPoint& pt3, std::vector<unsigned>& triangles);
};

}
