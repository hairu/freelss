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

namespace freelss
{

/**
 * Removes noisy points based off of average distance to neighboring points.
 */
class NoiseRemover
{
public:

	/** How aggressive the noise removal algorithm should be */
	enum Setting { NRS_DISABLED, NRS_LOW, NRS_MEDIUM, NRS_HIGH };

	/**
	 * Default Constructor
	 */
	NoiseRemover();

	/**
	 * Removes the noisy points from the points and laserLocations arrays.
	 */
	void removeNoise(PixelLocation * laserLocations, ColoredPoint * points,
					int numLocations, int & outNumLocations);
private:
	NoiseRemover::Setting m_setting;
	real m_distanceThreshold;
	real m_halfWindowSize;
};

}
