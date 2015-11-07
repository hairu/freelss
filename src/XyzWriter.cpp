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
#include "Progress.h"

namespace freelss
{

void XyzWriter::write(const std::string& baseFilename, const std::vector<DataPoint>& results, Progress& progress)
{
	// Sanity check
	if (results.empty())
	{
		return;
	}

	progress.setLabel("Generating XYZ file");
	progress.setPercent(0);

	std::string xyzFilename = baseFilename + ".xyz";
	std::ofstream xyz (xyzFilename.c_str());
	if (!xyz.is_open())
	{
		throw Exception("Error opening STL file for writing: " + xyzFilename);
	}

	xyz << std::fixed;

	real percent = 0;
	for (size_t iRec = 0; iRec < results.size(); iRec++)
	{
		real newPct = 100.0f * (iRec + 1) / results.size();
		if (newPct - percent > 0.1)
		{
			progress.setPercent(newPct);
			percent = newPct;
		}

		const DataPoint& rec = results[iRec];
		const ColoredPoint & pt = rec.point;

		xyz << std::setprecision(2) << pt.x << " "
			<< std::setprecision(2) << pt.y << " "
			<< std::setprecision(2) << pt.z << " "
			<< std::setprecision(2) << pt.normal.x << " "
			<< std::setprecision(2) << pt.normal.y << " "
			<< std::setprecision(2) << pt.normal.z
			<< std::endl;
	}

	xyz.close();
}


} // ns freelss
