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
#include "XyzWriter.h"
#include "Camera.h"

namespace freelss
{

void XyzWriter::write(const std::string& baseFilename, const std::vector<NeutralFileRecord>& results)
{
	std::string xyzFilename = baseFilename + ".xyz";
	std::ofstream xyz (xyzFilename.c_str());
	if (!xyz.is_open())
	{
		throw Exception("Error opening STL file for writing: " + xyzFilename);
	}

	uint32 maxNumRows = Camera::getInstance()->getImageHeight(); // TODO: Make this use the image height that generated the result and not the current Camera
	uint32 numRowBins = 400; // TODO: Autodetect this or have it in Database

	try
	{
		int iFrame = 0;

		std::vector<NeutralFileRecord> frameA;
		std::vector<NeutralFileRecord> currentFrame;

		size_t resultIndex = 0;
		while (NeutralFileRecord::readNextFrame(frameA, results, resultIndex))
		{
			// Reduce the number of result rows and filter out some of the noise
			NeutralFileRecord::lowpassFilter(currentFrame, frameA, maxNumRows, numRowBins);

			// Write the filtered results to the XYZ file
			for (size_t iRec = 0; iRec < currentFrame.size(); iRec++)
			{
				const NeutralFileRecord& rec = currentFrame[iRec];
				const ColoredPoint & pt = rec.point;

				xyz << pt.x        << " " << pt.y        << " " << pt.z        << " "
				    << pt.normal.x << " " << pt.normal.y << " " << pt.normal.z
				    << std::endl;
			}
		}
	}
	catch (...)
	{
		xyz.close();
		throw;
	}

	xyz.close();
}


} // ns freelss
