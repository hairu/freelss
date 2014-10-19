/*
 ****************************************************************************
 *  Copyright (c) 2014 Uriah Liggett <hairu526@gmail.com>                   *
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

namespace scanner
{

static real32 DistanceSquared(const ColoredPoint& a, const ColoredPoint&b)
{
	real32 dx = a.x - b.x;
	real32 dy = a.y - b.y;
	real32 dz = a.z - b.z;

	return dx * dx + dy * dy + dz * dz;
}

static bool SortRecordByRow(const NeutralFileRecord& a, const NeutralFileRecord& b)
{
	return a.pixel.y < b.pixel.y;
}


StlWriter::StlWriter() :
	m_maxEdgeDistMmSq(12.4 * 12.2),  // TODO: Make this a setting or autodetect it based off the distance to table and the detail level
	m_attribute(0)
{
	m_normal[0] = 0;
	m_normal[1] = 0;
	m_normal[2] = 0;
}

void StlWriter::computeAverage(const std::vector<NeutralFileRecord>& bin, NeutralFileRecord& out)
{
	out = bin.front();

	real32 invSize = 1.0f / bin.size();

	real32 rotation = 0;
	real32 pixelLocationX = 0;
	real32 pixelLocationY = 0;
	real32 ptX = 0;
	real32 ptY = 0;
	real32 ptZ = 0;
	real32 ptR = 0;
	real32 ptG = 0;
	real32 ptB = 0;

	for (size_t iBin = 0; iBin < bin.size(); iBin++)
	{
		const NeutralFileRecord& br = bin[iBin];

		rotation += invSize * br.rotation;
		pixelLocationX += invSize * br.pixel.x;
		pixelLocationY += invSize * br.pixel.y;
		ptX += invSize * br.point.x;
		ptY += invSize * br.point.y;
		ptZ += invSize * br.point.z;
		ptR += invSize * br.point.r;
		ptG += invSize * br.point.g;
		ptB += invSize * br.point.b;
	}

	out.rotation = rotation;
	out.pixel.x = pixelLocationX;  // TODO: We should probably round these values
	out.pixel.y = pixelLocationY;
	out.point.x = ptX;
	out.point.y = ptY;
	out.point.z = ptZ;
	out.point.r = ptR;
	out.point.g = ptG;
	out.point.b = ptB;
}

void StlWriter::lowpassFilter(std::vector<NeutralFileRecord>& output, std::vector<NeutralFileRecord>& frame, unsigned maxNumRows, unsigned numRowBins)
{
	output.clear();

	// Sanity check
	if (frame.empty())
	{
		return;
	}

	// Sort by image row
	std::sort(frame.begin(), frame.end(), SortRecordByRow);

	unsigned binSize = maxNumRows / numRowBins;

	// Holds the current contents of the bin
	std::vector<NeutralFileRecord> bin;
	unsigned nextBinY = frame.front().pixel.y + binSize;

	// unsigned bin = frame.front().pixel.y / numRowBins;
	for (size_t iFr = 0; iFr < frame.size(); iFr++)
	{
		NeutralFileRecord& record = frame[iFr];

		if (record.pixel.y < nextBinY)
		{
			bin.push_back(record);
		}
		else
		{
			// Average the bin results and add it to the output
			if (!bin.empty())
			{
				NeutralFileRecord out;
				computeAverage(bin, out);

				output.push_back(out);
				bin.clear();
			}

			nextBinY = record.pixel.y + binSize;
			bin.push_back(record);
		}
	}

	// Process any results still left in the bin
	if (!bin.empty())
	{
		NeutralFileRecord out;
		computeAverage(bin, out);

		output.push_back(out);
		bin.clear();
	}
}

bool StlWriter::readNextStep(std::vector<NeutralFileRecord>& out, const std::vector<NeutralFileRecord>& results, size_t & resultIndex)
{
	out.clear();

	if (resultIndex >= results.size() || results.empty())
	{
		return false;
	}

	int pseudoStep = results[resultIndex].pseudoStep;
	while (pseudoStep == results[resultIndex].pseudoStep && resultIndex < results.size())
	{
		out.push_back(results[resultIndex]);
		resultIndex++;
	}

	return true;
}

void StlWriter::write(const std::string& baseFilename, const std::vector<NeutralFileRecord>& results, bool connectLastFrameToFirst)
{
	std::string stlFilename = baseFilename + ".stl";
	std::ofstream fout (stlFilename.c_str());
	if (!fout.is_open())
	{
		throw Exception("Error opening STL file for writing: " + stlFilename);
	}

	std::string xyzFilename = baseFilename + ".xyz";
	std::ofstream xyz (xyzFilename.c_str());
	if (!xyz.is_open())
	{
		throw Exception("Error opening STL file for writing: " + xyzFilename);
	}

	uint32 maxNumRows = Camera::getInstance()->getImageHeight(); // TODO: Make this use the image height that generated the result and not the current Camera
	uint32 numRowBins = 400; // TODO: Autodetect this or have it in Settings

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
		while (readNextStep(frameC, results, resultIndex))
		{
			// Reduce the number of result rows and filter out some of the noise
			lowpassFilter(* currentFrame, frameC, maxNumRows, numRowBins);

			totNumPoints += currentFrame->size();

			// Sort the results in decreasing Y
			//std::sort(currentFrame->begin(), currentFrame->end(), SortRecordByYValue);

			// Write the filtered results to the XYZ file
			for (size_t iRec = 0; iRec < currentFrame->size(); iRec++)
			{
				const NeutralFileRecord& rec = currentFrame->at(iRec);
				const ColoredPoint & pt = rec.point;

				xyz << pt.x        << " " << pt.y        << " " << pt.z        << " "
				    << pt.normal.x << " " << pt.normal.y << " " << pt.normal.z
				    << std::endl;
			}

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
		xyz.close();
		throw;
	}

	fout.close();
	xyz.close();
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
					writeTriangle(l1.point, c1.point, c2.point, fout);
					numTriangles++;
				}
				else
				{
					// Connect to this triangle
					if (isValidTriangle(l2.point, l1.point, c1.point))
					{
						writeTriangle(l2.point, l1.point, c1.point, fout);
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

void StlWriter::writeTriangle(const ColoredPoint& pt1, const ColoredPoint& pt2, const ColoredPoint& pt3, std::ofstream& fout)
{
	real32 pt1x = pt1.x;
	real32 pt1y = pt1.y;
	real32 pt1z = pt1.z;
	real32 pt2x = pt2.x;
	real32 pt2y = pt2.y;
	real32 pt2z = pt2.z;
	real32 pt3x = pt3.x;
	real32 pt3y = pt3.y;
	real32 pt3z = pt3.z;


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
