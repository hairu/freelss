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
#include "NeutralFileReader.h"
#include "StlWriter.h"
#include "Camera.h"
#include "Laser.h"

namespace freelss
{

static real32 DistanceSquared(const ColoredPoint& a, const ColoredPoint&b)
{
	real32 dx = a.x - b.x;
	real32 dy = a.y - b.y;
	real32 dz = a.z - b.z;

	return dx * dx + dy * dy + dz * dz;
}


StlWriter::StlWriter() :
	m_maxEdgeDistMmSq(12.4 * 12.2),  // TODO: Make this a setting or autodetect it based off the distance to table and the detail level
	m_attribute(0),
	m_imageWidth(0)
{
	m_normal[0] = 0;
	m_normal[1] = 0;
	m_normal[2] = 0;
}


void StlWriter::write(const std::string& baseFilename, const std::vector<NeutralFileRecord>& results, bool connectLastFrameToFirst)
{
	std::string stlFilename = baseFilename + ".stl";
	std::ofstream fout (stlFilename.c_str());
	if (!fout.is_open())
	{
		throw Exception("Error opening STL file for writing: " + stlFilename);
	}

	uint32 maxNumRows = Camera::getInstance()->getImageHeight(); // TODO: Make this use the image height that generated the result and not the current Camera
	m_imageWidth = Camera::getInstance()->getImageWidth();
	uint32 numRowBins = 400; // TODO: Autodetect this or have it in Database

	// Write the STL header
	writeHeader(fout);

	uint32 numTriangles = 0;

	try
	{
		int iStep = 0;

		std::vector<NeutralFileRecord> firstFrame;
		std::vector<NeutralFileRecord> frameA;
		std::vector<NeutralFileRecord> frameB;
		std::vector<NeutralFileRecord> frameC;

		std::vector<NeutralFileRecord> * currentFrame = &frameA;
		std::vector<NeutralFileRecord> * lastFrame = &frameB;

		size_t resultIndex = 0;
		size_t totNumPoints = 0;
		while (NeutralFileRecord::readNextFrame(frameC, results, resultIndex))
		{
			// Reduce the number of result rows and filter out some of the noise
			NeutralFileRecord::lowpassFilter(* currentFrame, frameC, maxNumRows, numRowBins);

			totNumPoints += currentFrame->size();

			// Sort the results in decreasing Y
			//std::sort(currentFrame->begin(), currentFrame->end(), SortRecordByYValue);

			if (iStep > 0)
			{
				writeTrianglesForColumn(* currentFrame, * lastFrame, fout, numTriangles);
			}
			else if (connectLastFrameToFirst)
			{
				// Store the first frame for usage later on
				firstFrame = frameC;
			}

			// Swap to the other buffer to populate it
			std::vector<NeutralFileRecord> * tmpFrame = lastFrame;
			lastFrame = currentFrame;
			currentFrame = tmpFrame;

			iStep++;
		}

		// If this was a full scan close the loop
		if (connectLastFrameToFirst && iStep > 0)
		{
			totNumPoints += firstFrame.size();
			writeTrianglesForColumn(firstFrame, * lastFrame, fout, numTriangles);
		}

		fout.close();

		std::cout << "Meshed " << (iStep + 1) << " scans with a total of " << totNumPoints << " points." << std::endl;

		int filePos = 80;  // The file position where we should write the triangle count
		std::fstream fp (stlFilename.c_str());
		if (!fp.is_open())
		{
			throw Exception(std::string("Error updating triangle count in STL file: ") + stlFilename);
		}

		fp.seekp(filePos, std::ios_base::beg);
		fp.write((const char *)&numTriangles, sizeof(uint32));
		fp.close();
	}
	catch (...)
	{
		fout.close();
		throw;
	}

	fout.close();
}

void StlWriter::writeHeader(std::ofstream& fout)
{
	unsigned char header[80];
	memset(header, 0, sizeof(header));
	strcpy((char *)header, "2014 Uriah Liggett");

	// Write header string
	fout.write((const char *)header, sizeof(unsigned char) * 80);

	// Write number of triangles
	uint32 numTriangles = 0;
	fout.write((const char *)&numTriangles, sizeof(uint32));
}

bool StlWriter::isInwardFacingFace(const NeutralFileRecord& p1, const NeutralFileRecord& p2, const NeutralFileRecord& p3)
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
void StlWriter::writeTrianglesForColumn(const std::vector<NeutralFileRecord>& currentFrame, const std::vector<NeutralFileRecord>& lastFrame, std::ofstream& fout, uint32& numTriangles)
{
	size_t iCur = 0;

	for (size_t iLst = 0; iLst + 1 < lastFrame.size(); iLst++)
	{
		const NeutralFileRecord & l1 = lastFrame[iLst];
		const NeutralFileRecord & l2 = lastFrame[iLst + 1];

		// If the current point is in range
		while (iCur + 1 < currentFrame.size())
		{
			const NeutralFileRecord & c1 = currentFrame[iCur];
			const NeutralFileRecord & c2 = currentFrame[iCur + 1];

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
					writeTriangle(l1.point, c1.point, c2.point, flipNormal, fout);
					numTriangles++;
				}
				else
				{
					// Connect to this triangle
					if (isValidTriangle(l2.point, l1.point, c1.point))
					{
						bool flipNormal = isInwardFacingFace(l2, l1, c1);
						writeTriangle(l2.point, l1.point, c1.point, flipNormal, fout);
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

bool StlWriter::isValidTriangle(const ColoredPoint& pt1, const ColoredPoint& pt2, const ColoredPoint& pt3)
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

void StlWriter::writeTriangle(const ColoredPoint& pt1, const ColoredPoint& pt2, const ColoredPoint& pt3, bool flipNormal, std::ofstream& fout)
{
	real32 pt1x, pt1y, pt1z;
	real32 pt2x, pt2y, pt2z;
	real32 pt3x, pt3y, pt3z;


	if (!flipNormal)
	{
		pt1x = pt1.x;
		pt1y = pt1.y;
		pt1z = pt1.z;
		pt2x = pt2.x;
		pt2y = pt2.y;
		pt2z = pt2.z;
		pt3x = pt3.x;
		pt3y = pt3.y;
		pt3z = pt3.z;
	}
	else
	{
		pt1x = pt3.x;
		pt1y = pt3.y;
		pt1z = pt3.z;
		pt2x = pt2.x;
		pt2y = pt2.y;
		pt2z = pt2.z;
		pt3x = pt1.x;
		pt3y = pt1.y;
		pt3z = pt1.z;
	}


	fout.write((const char *)&m_normal, sizeof(real32) * 3);
	fout.write((const char *)&pt1x, sizeof(real32));
	fout.write((const char *)&pt1y, sizeof(real32));
	fout.write((const char *)&pt1z, sizeof(real32));
	fout.write((const char *)&pt2x, sizeof(real32));
	fout.write((const char *)&pt2y, sizeof(real32));
	fout.write((const char *)&pt2z, sizeof(real32));
	fout.write((const char *)&pt3x, sizeof(real32));
	fout.write((const char *)&pt3y, sizeof(real32));
	fout.write((const char *)&pt3z, sizeof(real32));
	fout.write((const char *)&m_attribute, sizeof(uint16));
}

void StlWriter::populateBuffer(NeutralFileRecord * records, int numRecords, NeutralFileRecord ** buffer, int maxNumRecords)
{
	// Set all the pointers in buffer to NULL
	memset(buffer, 0, sizeof(NeutralFileRecord *) * maxNumRecords);

	// Expand the records so that they sit at the same index as their pixel row
	for (int i = 0; i < numRecords; i++)
	{
		int row = ROUND(records[i].pixel.y);
		if (row >= maxNumRecords)
		{
			throw Exception("Pixel value is outside the maximum number of records");
		}

		// Point to the record
		buffer[row] = &records[i];
	}
}

}
