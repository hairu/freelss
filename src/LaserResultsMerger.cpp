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
#include "LaserResultsMerger.h"
#include "PresetManager.h"

#define MASK_DIM_1 1000 // The size of the frame dimension
#define MASK_DIM_2 1000 // The size of the Y dimension

namespace freelss
{

LaserResultsMerger::LaserResultsMerger() :
	m_numFramesBetweenLaserPlanes(0),
	m_numFramesPerRevolution(0),
	m_maxPointY(0)
{
	// Do nothing
}

int LaserResultsMerger::getIndex(const DataPoint& record)
{
	real pct1 = record.pseudoFrame / m_numFramesPerRevolution;
	real pct2 = record.point.y / m_maxPointY;

	int dim1 = pct1 * (MASK_DIM_1 - 1);
	int dim2 = pct2 * (MASK_DIM_2 - 1);

	return dim1 * MASK_DIM_2 + dim2;
}

void LaserResultsMerger::merge(std::vector<DataPoint> & out,
		                       std::vector<DataPoint> & leftLaserResults,
		                       std::vector<DataPoint> & rightLaserResults,
		                       int numFramesPerRevolution,
		                       int numFramesBetweenLaserPlanes,
		                       int maxPointY,
		                       Preset::LaserMergeAction mergeAction,
		                       Progress& progress)
{
	progress.setLabel("Merging laser results");
	progress.setPercent(0);

	// Sanity check
	if (mergeAction != Preset::LMA_PREFER_RIGHT_LASER && mergeAction != Preset::LMA_SEPARATE_BY_COLOR)
	{
		throw Exception("Unsupported Laser Merge Action");
	}

	// Sanity check
	if (leftLaserResults.empty() &&  rightLaserResults.empty())
	{
		return;
	}

	// Handle the cases of single laser scans
	if (leftLaserResults.empty())
	{
		out = rightLaserResults;
		for (size_t iOut = 0; iOut < out.size(); iOut++)
		{
			out[iOut].pseudoFrame = out[iOut].frame;
		}
	}
	else if (rightLaserResults.empty())
	{
		out = leftLaserResults;
		for (size_t iOut = 0; iOut < out.size(); iOut++)
		{
			out[iOut].pseudoFrame = out[iOut].frame;
		}
	}
	else
	{
		m_numFramesBetweenLaserPlanes = numFramesBetweenLaserPlanes;
		m_numFramesPerRevolution = (real)numFramesPerRevolution;
		m_maxPointY = (real)maxPointY;

		std::vector<byte> mask;
		mask.resize(MASK_DIM_1 * MASK_DIM_2, 0);

		// Merge the results
		std::cout << "Detected " << numFramesBetweenLaserPlanes << " frames between the lasers." << std::endl;

		for (size_t iRight = 0; iRight < rightLaserResults.size(); iRight++)
		{
			DataPoint& right = rightLaserResults[iRight];
			right.pseudoFrame = right.frame;

			// Make the Right laser black
			if (mergeAction == Preset::LMA_SEPARATE_BY_COLOR)
			{
				right.point.r = 0;
				right.point.g = 0;
				right.point.b = 0;
			}
		}

		progress.setPercent(10);
		for (size_t iLeft = 0; iLeft < leftLaserResults.size(); iLeft++)
		{
			DataPoint& left = leftLaserResults[iLeft];

			// Align the frames of the left and right lasers
			left.pseudoFrame = left.frame + m_numFramesBetweenLaserPlanes;

			// Cause the first right laser results to overlay with the last left laser results
			if (left.pseudoFrame >= m_numFramesPerRevolution)
			{
				left.pseudoFrame -= m_numFramesPerRevolution;
			}

			// Make the Left laser red
			if (mergeAction == Preset::LMA_SEPARATE_BY_COLOR)
			{
				left.point.r = 255;
				left.point.g = 0;
				left.point.b = 0;
			}
		}

		progress.setPercent(20);
		int numCulledPoints = 0;

		//
		// Populate the mask with location information from the right laser
		//
		out = rightLaserResults;
		for (size_t iRight = 0; iRight < rightLaserResults.size(); iRight++)
		{
			DataPoint& right = rightLaserResults[iRight];

			mask[getIndex(right)] = 1;
		}

		progress.setPercent(50);

		//
		// Only add left lasers that don't map to a cube by the right laser
		//
		for (size_t iLeft = 0; iLeft < leftLaserResults.size(); iLeft++)
		{
			DataPoint& left = leftLaserResults[iLeft];

			if (mask[getIndex(left)] == 0)
			{
				out.push_back(left);
			}
			else
			{
				// Make the culled left laser results gray
				if (mergeAction == Preset::LMA_SEPARATE_BY_COLOR)
				{
					left.point.r = 175;
					left.point.g = 175;
					left.point.b = 175;
					out.push_back(left);
				}

				numCulledPoints++;
			}
		}

		std::cout << "Culled " << numCulledPoints << ", " << (100 * (real)numCulledPoints / leftLaserResults.size()) << "% of the left laser points." << std::endl;
	}

	progress.setPercent(100);
}

} // ns
