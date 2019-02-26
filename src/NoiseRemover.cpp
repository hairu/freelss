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
#include "NoiseRemover.h"
#include "PresetManager.h"

namespace freelss
{

NoiseRemover::NoiseRemover()
{
	m_setting = PresetManager::get()->getActivePreset().noiseRemovalSetting;

	switch (m_setting)
	{
	case NoiseRemover::NRS_DISABLED:
		m_distanceThreshold = -1;
		m_halfWindowSize = -1;
		break;

	case NoiseRemover::NRS_LOW:
		m_distanceThreshold = 1.5;
		m_halfWindowSize = 3;
		break;

	case NoiseRemover::NRS_MEDIUM:
		m_distanceThreshold = 0.5;
		m_halfWindowSize = 3;
		break;

	case NoiseRemover::NRS_HIGH:
		m_distanceThreshold = 0.25;
		m_halfWindowSize = 3;
		break;

	default:
		throw Exception("Unsupported NoiseRemover setting");
		break;
	}
}

void NoiseRemover::removeNoise(PixelLocation * laserLocations, ColoredPoint * points,
							   int numLocations, int & outNumLocations)
{
	// Stop here if noise removal is disabled
	if (m_setting == NoiseRemover::NRS_DISABLED)
	{
		outNumLocations = numLocations;
		return;
	}

	std::vector<bool> goodLocs;
	for (int iLoc = 0; iLoc < outNumLocations; iLoc++)
	{
		real distSum = 0;
		int start = MAX(0, iLoc - m_halfWindowSize);
		int end = MIN(outNumLocations - 1, iLoc + m_halfWindowSize);

		for (int iWin = start; iWin < end; iWin++)
		{
			ColoredPoint * a = &points[iWin];
			ColoredPoint * b = &points[iWin + 1];
			real dx = a->x - b->x;
			real dy = a->y - b->y;
			real dz = a->z - b->z;
			distSum += sqrt((dx * dx) + (dy * dy) + (dz * dz));
		}

		int cnt = (end - start);

		// Prevent a divide by zero
		if (cnt > 0)
		{
			real meanDist = distSum / cnt;

			// Remove the noise
			goodLocs.push_back(meanDist < m_distanceThreshold);
		}
		else
		{
			goodLocs.push_back(false);
		}
	}

	int numLocationsNoise = outNumLocations;
	outNumLocations = 0;
	for (int iLoc = 0; iLoc < numLocationsNoise; iLoc++)
	{
		if (goodLocs[iLoc])
		{
			laserLocations[outNumLocations] = laserLocations[iLoc];
			points[outNumLocations] = points[iLoc];
			outNumLocations++;
		}
	}
}

} // ns freelss
