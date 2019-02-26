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
#include "Thread.h"
#include "Image.h"

namespace freelss
{

class ImageProcessor
{

public:
	ImageProcessor();
	~ImageProcessor();
	
	/** The mode and amount of thresholding */
	enum ThresholdMode { THM_STATIC, THM_LOW, THM_MEDIUM, THM_HIGH };

	/**
	 * Detects the laser in x, y pixel coordinates.
	 * @param debuggingImage - If non-NULL, it will be populated with the processed image that was used to detect the laser locations.
	 *     This can be helpful when debugging laser detection issues.
	 * @param laserLocations - Output variable to store the laser locations.
	 * @param maxNumLocations - The maximum number of locations to store in @p laserLocations.
	 * @param percentPixelsOverThreshold - The percentage of pixels that were over the threshold amount.
	 * @return Returns the number of locations written to @p laserLocations.
	 */
	int process(Image& before, Image& after, Image * debuggingImage, PixelLocation * laserLocations, int maxNumLocations,
			    int& firstRowLaserCol, int& numRowsBadFromColor, int& numRowsBadFromNumRanges, const char * debuggingCsvFile);

	static void toHsv(real r, real g, real b, Hsv * hsv);

private:

	/** The starting and ending column for a detected laser line in the image */
	struct LaserRange
	{
		int startCol;
		int endCol;
		int centerCol;
		real energy;
	};

	/**  Removes the ranges that on closer inspection don't appear to be caused by the laser */
	int removeInvalidLaserRanges(ImageProcessor::LaserRange * ranges, int imageWidth, int numRanges, unsigned char * laserOnPixels);

	int detectBestLaserRange(ImageProcessor::LaserRange * ranges, int numRanges, int prevLaserCol);

	real detectLaserRangeCenter(const ImageProcessor::LaserRange& range, unsigned char * ar, unsigned char * br);

	real computeMeanAverage(unsigned char * br, int numSteps, int stepSize);

	/** Converts the RGB color to HSV */
	static const unsigned RANGE_DISTANCE_THRESHOLD;

	/** The LaserRanges for each column */
	LaserRange * m_laserRanges;
	ImageProcessor::ThresholdMode m_thresholdMode;
	real m_laserMagnitudeThreshold;
	real m_laserThresholdFactor;
	int m_maxLaserWidth;
	int m_minLaserWidth;
	std::vector<real> m_magnitudes;
};

}
