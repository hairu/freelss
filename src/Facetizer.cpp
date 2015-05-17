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
#include "Facetizer.h"
#include "Camera.h"

namespace freelss
{

static real32 DistanceSquared(const ColoredPoint& a, const ColoredPoint&b)
{
	real32 dx = a.x - b.x;
	real32 dy = a.y - b.y;
	real32 dz = a.z - b.z;

	return dx * dx + dy * dy + dz * dz;
}

Facetizer::Facetizer() :
	m_maxEdgeDistMmSq(12.4 * 12.2),  // TODO: Make this a setting or autodetect it based off the distance to table and the detail level
	m_imageWidth(0)
{

}

void Facetizer::facetize(FaceMap & faces, std::vector<DataPoint>& results, bool connectLastFrameToFirst, Progress& progress, bool updateVertexNormals)
{
	progress.setPercent(0);

	m_imageWidth = Camera::getInstance()->getImageWidth();


	uint32 numTriangles = 0;

	int iStep = 0;

	std::vector<DataPoint> firstFrame;
	std::vector<DataPoint> frameA;
	std::vector<DataPoint> frameB;
	std::vector<DataPoint> frameC;

	std::vector<DataPoint> * currentFrame = &frameA;
	std::vector<DataPoint> * lastFrame = &frameB;

	size_t resultIndex = 0;
	size_t totNumPoints = 0;

	// Set the index of the data points
	for (size_t iPt = 0; iPt < results.size(); iPt++)
	{
		results[iPt].index = (uint32) iPt;
	}

	real percent = 0;
	while (DataPoint::readNextFrame(frameC, results, resultIndex))
	{
		real newPct = 100.0f * resultIndex / results.size();
		if (newPct - percent > 0.1)
		{
			progress.setPercent(newPct);
			percent = newPct;
		}

		* currentFrame = frameC;
		totNumPoints += currentFrame->size();

		// Sort the results in decreasing Y
		//std::sort(currentFrame->begin(), currentFrame->end(), SortRecordByYValue);

		if (iStep > 0)
		{
			addFacesForColumn(* currentFrame, * lastFrame, faces, numTriangles);
		}
		else if (connectLastFrameToFirst)
		{
			// Store the first frame for usage later on
			firstFrame = frameC;
		}

		// Swap to the other buffer to populate it
		std::vector<DataPoint> * tmpFrame = lastFrame;
		lastFrame = currentFrame;
		currentFrame = tmpFrame;

		iStep++;
	}

	// If this was a full scan close the loop
	if (connectLastFrameToFirst && iStep > 0)
	{
		totNumPoints += firstFrame.size();
		addFacesForColumn(firstFrame, * lastFrame, faces, numTriangles);
	}

	std::cout << "Meshed " << (iStep + 1) << " scans with a total of " << totNumPoints << " points." << std::endl;

	std::cout << "Computing vertex normals..." << std::endl;

	const std::vector<unsigned>& triangles = faces.triangles;

	Vector3 normal;
	for (size_t idx = 0; idx < triangles.size(); idx += 3)
	{
		ColoredPoint& pt1 = results[triangles[idx]].point;
		ColoredPoint& pt2 = results[triangles[idx + 1]].point;
		ColoredPoint& pt3 = results[triangles[idx + 2]].point;

		Vector3 v1 (pt2.x - pt1.x, pt2.y - pt1.y, pt2.z - pt1.z);
		Vector3 v2 (pt3.x - pt2.x, pt3.y - pt2.y, pt3.z - pt2.z);

		v1.cross(normal, v2);
		normal.normalize();

		pt1.normal = normal;
		pt2.normal = normal;
		pt3.normal = normal;
	}

	std::cout << "Vertex normals computed." << std::endl;
}

bool Facetizer::isInwardFacingFace(const DataPoint& p1, const DataPoint& p2, const DataPoint& p3)
{
	int laserSide = p1.laserSide;
	bool inwardFacing = false;
	float halfImageWidth = m_imageWidth * 0.5f; // If it's past the centerline then it will result in an inward facing triangle

	if (laserSide == (int)Laser::LEFT_LASER)
	{
		inwardFacing = p1.pixel.x > halfImageWidth || p2.pixel.x > halfImageWidth || p3.pixel.x > halfImageWidth;
	}
	else if (laserSide == (int)Laser::RIGHT_LASER)
	{
		inwardFacing = p1.pixel.x < halfImageWidth || p2.pixel.x < halfImageWidth || p3.pixel.x < halfImageWidth;
	}
	else
	{
		throw Exception("Unsupported LaserSide in StlWriter::isInwardFacingFace");
	}

	return inwardFacing;
}


/*
foreach column
	set row to 0
	foreach pt in prev_column
		// Fan connect to the row
		while cell[row] is within range and cell[row + 1] is within range and closer to this point than the one below
			createTriangle(pt, cell[row], cell[row + 1])
			row++

		// Extend mesh down to start processing the next point
		if nextPt is within range
			createTriangle(pt, nextPt, cell[row])


 */
void Facetizer::addFacesForColumn(const std::vector<DataPoint>& currentFrame, const std::vector<DataPoint>& lastFrame, FaceMap& faces, uint32& numTriangles)
{
	size_t iCur = 0;

	for (size_t iLst = 0; iLst + 1 < lastFrame.size(); iLst++)
	{
		const DataPoint & l1 = lastFrame[iLst];
		const DataPoint & l2 = lastFrame[iLst + 1];

		// If the current point is in range
		while (iCur + 1 < currentFrame.size())
		{
			const DataPoint & c1 = currentFrame[iCur];
			const DataPoint & c2 = currentFrame[iCur + 1];

			// If there is a hole in the model, skip along until there isn't one
			if (isValidTriangle(l1.point, c1.point, c2.point))
			{
				real32 distSq1 = DistanceSquared(l1.point, c2.point);
				real32 distSq2 = DistanceSquared(l2.point, c2.point);

				// While this C point is closer to this L point than the next L point, create a triangle fan with it
				// Don't generate triangles bigger than our maximum allowable edge distance
				// if (distSq1 < distSq2 && distSq1 <= m_maxEdgeDistMmSq)
				if (distSq1 < distSq2)
				{
					// Determine if we need to flip the normal or not
					bool flipNormal = isInwardFacingFace(l1, c1, c2);
					addTriangle(l1, c1, c2, flipNormal, faces);
					numTriangles++;
				}
				else
				{
					// Connect to this triangle
					if (isValidTriangle(l2.point, l1.point, c1.point))
					{
						bool flipNormal = isInwardFacingFace(l2, l1, c1);
						addTriangle(l2, l1, c1, flipNormal, faces);
						numTriangles++;
					}
					else
					{
						iCur++;
					}

					// It's closer to the next L point, so advance to it
					break;
				}
			}
			else
			{
				iCur++;
				break;
			}

			// Advance to the next row
			iCur++;
		}
	}
}

bool Facetizer::isValidTriangle(const ColoredPoint& pt1, const ColoredPoint& pt2, const ColoredPoint& pt3)
{
	if (DistanceSquared(pt1, pt2) > m_maxEdgeDistMmSq)
	{
		return false;
	}
	else if (DistanceSquared(pt1, pt3) > m_maxEdgeDistMmSq)
	{
		return false;
	}
	else if (DistanceSquared(pt2, pt3) > m_maxEdgeDistMmSq)
	{
		return false;
	}

	return true;
}

void Facetizer::addTriangle(const DataPoint& pt1, const DataPoint& pt2, const DataPoint& pt3, bool flipNormal, FaceMap& faces)
{
	if (!flipNormal)
	{
		faces.triangles.push_back(pt1.index);
		faces.triangles.push_back(pt2.index);
		faces.triangles.push_back(pt3.index);
	}
	else
	{
		faces.triangles.push_back(pt3.index);
		faces.triangles.push_back(pt2.index);
		faces.triangles.push_back(pt1.index);
	}
}

}
