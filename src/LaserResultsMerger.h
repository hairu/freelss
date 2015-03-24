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

#include "Preset.h"

namespace freelss
{

/**
 * This class merges the left and right laser results into a single result set.
 * It uses the results from the left laser in places where the right laser does
 * not have information.  It utilizes a volumetric masking technique to determine
 * if the right laser already has information in a given area.
 */
class LaserResultsMerger
{
public:
	LaserResultsMerger();

	void merge(std::vector<NeutralFileRecord> & out,
            std::vector<NeutralFileRecord> & leftLaserResults,
            std::vector<NeutralFileRecord> & rightLaserResults,
            int numFramesPerRevolution,
            int numFramesBetweenLaserPlanes,
            int maxPointY,
            Preset::LaserMergeAction mergeAction);

private:

	int getIndex(const NeutralFileRecord& record);

	int m_numFramesBetweenLaserPlanes;
	real m_numFramesPerRevolution;
	real m_maxPointY;
};

}
