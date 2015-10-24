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
#include "Main.h"
#include "ObjectBaseCreator.h"

namespace freelss
{

ObjectBaseCreator::ObjectBaseCreator()
{

}


void ObjectBaseCreator::createBase(FaceMap & outFaces, std::vector<DataPoint>& results, real32 groundPlaneHeight, int numSubdivisions, Progress& progress)
{
	int maxNumBasePoints = 100;
	real maxBasePointY = 5.0f;  // Points greater than this are not considered

	// Sanity check
	if (results.empty())
	{
		return;
	}

	progress.setLabel("Creating object base");

	uint16 lastFrame = results.front().pseudoFrame;

	std::vector<DataPoint> basePoints;

	// Find the lowest points in each frame
	for (size_t iRes = 1; iRes < results.size(); iRes++)
	{
		const DataPoint& point = results[iRes];

		// If this is a frame switch
		if (point.pseudoFrame != lastFrame || iRes + 1 == results.size())
		{
			DataPoint lastPoint = results[iRes - 1];

			if (lastPoint.point.y <= maxBasePointY)
			{
				basePoints.push_back(lastPoint);
				lastFrame = point.pseudoFrame;
			}
		}
	}

	DataPoint center = results.back();
	center.point.x = 0;
	center.point.y = 0;
	center.point.z = 0;

	// Reduce the number of base points if there are too many
	if (basePoints.size() > (size_t) maxNumBasePoints)
	{
		std::vector<DataPoint> pointSubset;
		real stepSize = basePoints.size() / (real) maxNumBasePoints;
		for (real pos = 0; pos < basePoints.size(); pos += stepSize)
		{
			unsigned idx = (unsigned) pos;

			DataPoint pt = basePoints[idx];

			center.point.x += pt.point.x;
			center.point.z += pt.point.z;

			pointSubset.push_back(pt);
		}

		basePoints = pointSubset;
	}

	// Stop here if we don't have the minimum number of points
	if (basePoints.size() < 3)
	{
		return;
	}

	// Create a point for the center
	center.point.x /= (real) basePoints.size();
	center.point.z /= (real) basePoints.size();

	basePoints.push_back(center);


	// Set base points properties
	for (size_t iPt = 0; iPt < basePoints.size(); iPt++)
	{
		DataPoint& pt = basePoints[iPt];

		pt.point.y = 0;
		pt.point.r = 0;
		pt.point.g = 0;
		pt.point.b = 0;
		pt.point.normal.x = 0;
		pt.point.normal.y = -1;
		pt.point.normal.z = 0;
		pt.index = (unsigned) iPt;
	}

	center = basePoints.back();

	//
	// Create faces
	//
	std::vector<unsigned> baseTris;
	for (unsigned iPt = 0; iPt + 2 < (unsigned) basePoints.size(); iPt++)
	{
		baseTris.push_back(iPt);
		baseTris.push_back(center.index);
		baseTris.push_back(iPt + 1);
	}

	// Connect last face to first face
	baseTris.push_back(basePoints.size() - 2);
	baseTris.push_back(center.index);
	baseTris.push_back(0);

	std::cout << "Subdividing base " << numSubdivisions << " times..." << std::endl;

	// Subdivide the results
	for (int iSub = 0; iSub < numSubdivisions; iSub++)
	{
		subdivide(baseTris, basePoints);
	}

	//
	// Merge the base with the results
	//

	// Update the indices
	unsigned indexOffset = (unsigned) results.size();
	for (size_t idx = 0; idx < baseTris.size(); idx++)
	{
		baseTris[idx] += indexOffset;
	}

	for (size_t iPt = 0; iPt < basePoints.size(); iPt++)
	{
		basePoints[iPt].index += indexOffset;
	}

	// Add the indices to the FaceMap
	outFaces.triangles.insert(outFaces.triangles.end(), baseTris.begin(), baseTris.end());

	// Add the points
	results.insert(results.end(), basePoints.begin(), basePoints.end());
}

DataPoint ObjectBaseCreator::splitEdge(const DataPoint& p1, const DataPoint& p2, std::vector<DataPoint>& results)
{
	DataPoint out = p1;

	out.point.x = (p1.point.x + p2.point.x) * 0.5;
	out.point.y = 0;
	out.point.z = (p1.point.z + p2.point.z) * 0.5;
	out.index = (unsigned) results.size();

	results.push_back(out);

	return out;
}

void ObjectBaseCreator::subdivide(std::vector<unsigned>& triangles, std::vector<DataPoint>& results)
{
	size_t numTriIndices = triangles.size();
	for (size_t idx = 0; idx + 2 < numTriIndices; idx += 3)
	{
		unsigned i1 = triangles[idx];
		unsigned i2 = triangles[idx + 1];
		unsigned i3 = triangles[idx + 2];
		if (i1 < results.size() && i2 < results.size() && i3 < results.size())
		{
			DataPoint p1 = results[i1];
			DataPoint p2 = results[i2];
			DataPoint p3 = results[i3];
			DataPoint a = splitEdge(p1, p2, results);
			DataPoint b = splitEdge(p1, p3, results);
			DataPoint c = splitEdge(p2, p3, results);

			// Create a point in the center of the triangle

			// Create triangles from this new point
			updateTriangle(idx, p1, b, a, triangles);
			addTriangle(a, b, c, triangles);
			addTriangle(b, p3, c, triangles);
			addTriangle(a, c, p2, triangles);
		}
		else
		{
			std::cerr << "!! Invalid triangle index from set of " << results.size() << "(" << i1 << "," << i2 << "," << i3 << ")" << std::endl;
		}
	}
}

void ObjectBaseCreator::updateTriangle(unsigned idx1, const DataPoint& pt1, const DataPoint& pt2, const DataPoint& pt3, std::vector<unsigned>& triangles)
{
	triangles[idx1] = pt1.index;
	triangles[idx1 + 1] = pt2.index;
	triangles[idx1 + 2] = pt3.index;
}

void ObjectBaseCreator::addTriangle(const DataPoint& pt1, const DataPoint& pt2, const DataPoint& pt3, std::vector<unsigned>& triangles)
{
	triangles.push_back(pt1.index);
	triangles.push_back(pt2.index);
	triangles.push_back(pt3.index);
}
}
