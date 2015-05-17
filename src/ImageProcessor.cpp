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
#include "ImageProcessor.h"
#include "PresetManager.h"
#include "Camera.h"

// Only one of these filters should be defined
#define CENTERMASS_FILTER 1
#define GAUSS_FILTER      0
#define PEAK_FILTER       0
#define INV_SQRT_2PI      0.3989422804014327

#define MAX_NUM_LASER_RANGES 3 // TODO: Place this in the settings

#if GUASS_FILTER
#include <mpfit.h>
#endif

namespace freelss
{

const real ImageProcessor::RED_HUE_LOWER_THRESHOLD = 30;
const real ImageProcessor::RED_HUE_UPPER_THRESHOLD = 329;
const unsigned ImageProcessor::RANGE_DISTANCE_THRESHOLD = 5;

struct GaussPrivate
{
	/** The data that is being fitted to gauss */
	double actual[4098]; // TODO: Make this the proper size
};

struct vars_struct
{
	double *x;
	double *y;
	double *ey;
};

int gaussfunc(int m, int n, double *p, double *dy, double **dvec, void *vars)
{
	int i;
	struct vars_struct *v = (struct vars_struct *) vars;
	double *x, *y, *ey;
	double xc, sig2;
	x = v->x;
	y = v->y;
	ey = v->ey;
	sig2 = p[3]*p[3];
	for (i=0; i<m; i++) {
		xc = x[i]-p[2];
		dy[i] = (y[i] - p[1]*exp(-0.5*xc*xc/sig2) - p[0])/ey[i];
	}
	return 0;
}

double gauss_compute(double x, double m, double s)
{
	double a = (x - m) / s;
	return INV_SQRT_2PI / s * exp(-0.5 * a * a);
}

int mpfit_gauss(int m, int n, double *p, double *deviates, double **derivs, void * priv)
{
	double p_mean   = p[0];
	double p_stddev = p[1];
	double p_scale  = p[2];
	GaussPrivate * gpriv = (GaussPrivate *) priv;
	double * input = gpriv->actual;

	double errorSum = 0;

	// Compute function deviates
	for (int i = 0; i < m; i++)
	{
		double error = (gauss_compute(i, p_mean, p_stddev) * p_scale) - input[i];
		deviates[i] = error;

		errorSum += error;
	}

	std::cout << "mpfit_gauss mean=" << p_mean << ", stddev=" << p_stddev << ", scale=" << p_scale << ", errorSum=" << errorSum << std::endl;
	return 0;
}

ImageProcessor::ImageProcessor()
{
	m_laserRanges = new ImageProcessor::LaserRange[Camera::getInstance()->getImageWidth() + 1];

	Preset& preset = PresetManager::get()->getActivePreset();
	m_laserMagnitudeThreshold = preset.laserThreshold;
	m_maxLaserWidth = preset.maxLaserWidth;
	m_minLaserWidth = preset.minLaserWidth;
}

ImageProcessor::~ImageProcessor()
{
	delete [] m_laserRanges;
}

int ImageProcessor::process(const Image& before, const Image& after, Image * debuggingImage, PixelLocation * laserLocations,
		int maxNumLocations, int& firstRowLaserCol, int& numRowsBadFromColor, int& numRowsBadFromNumRanges, const char * debuggingCsvFile)
{	
	const real MAX_MAGNITUDE_SQ = 255 * 255 * 3; // The maximum pixel magnitude sq we can see
	const real INV_MAX_MAGNITUDE_SQ = 1.0f / MAX_MAGNITUDE_SQ;

	unsigned char * a = before.getPixels();
	unsigned char * b = after.getPixels();
	unsigned char * d = NULL;

	std::fstream rowOut;

	const bool writeDebugImage = debuggingImage != NULL;
	if (writeDebugImage)
	{
		d = debuggingImage->getPixels();
	}

	if (debuggingCsvFile != NULL)
	{
		rowOut.open(debuggingCsvFile, std::ios::out);
	}
	
	const unsigned width = before.getWidth();
	const unsigned height = before.getHeight();
	unsigned components = before.getNumComponents();
	unsigned rowStep = width * components;

	int numLocations = 0;
	int numMerged = 0;

	// The location that we last detected a laser line
	int prevLaserCol = firstRowLaserCol;

	unsigned char * ar = a;
	unsigned char * br = b;
	unsigned char * dr = d;
	for (unsigned iRow = 0; iRow < height && numLocations < maxNumLocations; iRow++)
	{
		// The column that the laser started and ended on
		int numLaserRanges = 0;
		m_laserRanges[numLaserRanges].startCol = -1;
		m_laserRanges[numLaserRanges].endCol = -1;

		int numRowOut = 0;
		int imageColumn = 0;
		for (unsigned iCol = 0; iCol < rowStep; iCol += components)
		{
			// Perform image subtraction

			// Use general magnitude
			const int r = (int)br[iCol + 0] - (int)ar[iCol + 0];
			const int g = (int)br[iCol + 1] - (int)ar[iCol + 1];
			const int b = (int)br[iCol + 2] - (int)ar[iCol + 2];
			const int magSq = r * r + g * g + b * b;
			real mag = 255.0f * (magSq * INV_MAX_MAGNITUDE_SQ);

			if (writeDebugImage)
			{
				if (mag > m_laserMagnitudeThreshold)
				{
					dr[iCol + 0] = mag;
					dr[iCol + 1] = mag;
					dr[iCol + 2] = mag;
				}
				else
				{
					dr[iCol + 0] = mag;
					dr[iCol + 1] = mag;
					dr[iCol + 2] = 0;
				}
			}

			// Compare it against the threshold
			if (mag > m_laserMagnitudeThreshold)
			{
				// The start of pixels with laser in them
				if (m_laserRanges[numLaserRanges].startCol == -1)
				{
					m_laserRanges[numLaserRanges].startCol = imageColumn;
				}

				if (debuggingCsvFile != NULL)
				{
					rowOut << mag << ", ";
					numRowOut++;
				}
			}
			// The end of pixels with laser in them
			else if (m_laserRanges[numLaserRanges].startCol != -1)
			{
				int laserWidth = imageColumn - m_laserRanges[numLaserRanges].startCol;
				if (laserWidth <= m_maxLaserWidth && laserWidth >= m_minLaserWidth)
				{
					// If this range was real close to the previous one, merge them instead of creating a new one
					bool wasMerged = false;
					if (numLaserRanges > 0)
					{
						unsigned rangeDistance =  m_laserRanges[numLaserRanges].startCol - m_laserRanges[numLaserRanges - 1].endCol;
						if (rangeDistance < RANGE_DISTANCE_THRESHOLD)
						{
							 m_laserRanges[numLaserRanges - 1].endCol =  imageColumn;
							 m_laserRanges[numLaserRanges - 1].centerCol = round((m_laserRanges[numLaserRanges - 1].startCol + m_laserRanges[numLaserRanges - 1].endCol) / 2);
							 wasMerged = true;
							 numMerged++;
						}
					}

					// Proceed to the next laser range
					if (!wasMerged)
					{
						// Add this range as a candidate
						m_laserRanges[numLaserRanges].endCol = imageColumn;
						m_laserRanges[numLaserRanges].centerCol = round((m_laserRanges[numLaserRanges].startCol + m_laserRanges[numLaserRanges].endCol) / 2);

						numLaserRanges++;
					}

					// Reinitialize the range
					m_laserRanges[numLaserRanges].startCol = -1;
					m_laserRanges[numLaserRanges].endCol = -1;
				}
				// There was a false positive
				else
				{
					m_laserRanges[numLaserRanges].startCol = -1;
				}
			}

			// Go from image components back to image pixels
			imageColumn++;

		} // foreach column

		if (debuggingCsvFile != NULL && numRowOut > 0)
		{
			rowOut << std::endl;
		}

		// Removes the ranges that on closer inspection don't appear to be caused by the laser
		if (numLaserRanges < MAX_NUM_LASER_RANGES)
		{
			if (numLaserRanges > 0)
			{
				//numLaserRanges = removeInvalidLaserRanges(m_laserRanges, width, numLaserRanges, br);

				// If we have a valid laser region
				if (numLaserRanges > 0)
				{
					int rangeChoice = detectBestLaserRange(m_laserRanges, numLaserRanges, prevLaserCol);
					prevLaserCol = m_laserRanges[rangeChoice].centerCol;

					real centerCol = detectLaserRangeCenter(m_laserRanges[rangeChoice], ar, br);

					laserLocations[numLocations].x = centerCol;
					laserLocations[numLocations].y = iRow;

					// If this is the first row that a laser is detected in, set the firstRowLaserCol member
					if (numLocations == 0)
					{
						firstRowLaserCol = m_laserRanges[rangeChoice].startCol;
					}

					numLocations++;
				}
				else
				{
					numRowsBadFromColor++;
				}
			}
		}
		else
		{
			numRowsBadFromNumRanges++;
		}

		// Increment to the next row
		ar += rowStep;
		br += rowStep;

		if (writeDebugImage)
		{
			dr += rowStep;
		}
	} // foreach row

	if (numMerged > 0)
	{
		std::cout << "Merged " << numMerged << " laser ranges." << std::endl;
	}

	if (numRowsBadFromColor > 0)
	{
		std::cout << "Dropped " << numRowsBadFromColor << " laser rows because the color wasn't red enough. " << std::endl;
	}

	if (numRowsBadFromNumRanges > 0)
	{
		std::cout << numRowsBadFromNumRanges << " laser rows contained too many ranges. " << std::endl;
	}

	if (debuggingCsvFile != NULL)
	{
		rowOut.close();
	}

	return numLocations;
}

int ImageProcessor::removeInvalidLaserRanges(ImageProcessor::LaserRange * ranges, int imageWidth, int numLaserRanges, unsigned char * br)
{
	// Look for a ramp up and ramp down of the red component centered around the laser range to differentiate
	// a laser from things moving in the background
	const real MIN_RED_VALUE = 255.0f * 0.85f;

	int components = 3;
	std::vector<ImageProcessor::LaserRange> goodRanges;  // TODO: Check if this is a bottleneck

	for (int iRng = 0; iRng < numLaserRanges; iRng++)
	{
		const ImageProcessor::LaserRange& range = ranges[iRng];
		int startCol = range.startCol;
		int endCol = range.endCol;
		int rangeWidth = endCol - startCol + 1;

		/*
		int rampWidth = rangeWidth;

		// Determine where to start computing the median of the "pre" portion of the laser line
		int preStart = MAX(0, startCol - rampWidth);
		int preWidth = MAX(0, startCol - preStart);

		// Determine where to start computing the median of the "post" portion of the laser line
		int postStart = MIN(imageWidth, endCol + rampWidth);
		int postWidth = rampWidth;

		if (postStart + postWidth >= imageWidth)
		{
			postWidth = imageWidth - postStart;
		}

		real preAvg = computeMeanAverage(br + preStart * components, preWidth, components);
		real postAvg = computeMeanAverage(br + postStart * components, postWidth, components);
		*/

		real rangeAvg = computeMeanAverage(br + components * startCol, rangeWidth, components);


		//std::cout << (int)preAvg << "  " << (int)rangeAvg << "  " << (int)postAvg << std::endl;

		// If there is a ramp up and ramp down then include this range
		if (rangeAvg > MIN_RED_VALUE)
		{
			goodRanges.push_back(range);
		}
	}

	// Update the ranges
	if ((int)goodRanges.size() != numLaserRanges)
	{
		for (size_t iRng = 0; iRng < goodRanges.size(); iRng++)
		{
			ranges[iRng] = goodRanges[iRng];
		}
	}

	return (int) goodRanges.size();
}

real ImageProcessor::computeMeanAverage(unsigned char * br, int numSteps, int stepSize)
{
	real avg = 0;
	real invStep = 255.0f / numSteps;

	Hsv hsv;

	// Red is the first component of an RGB triplet so we can use br directly
	for (int iStep = 0; iStep < numSteps; iStep++)
	{
		float r = br[iStep * stepSize];
		float g = br[iStep * stepSize + 1];
		float b = br[iStep * stepSize + 2];

		toHsv(r, g, b, &hsv);

		 // Red wraps HSV colorspace so check both sides
		real red = hsv.h <= 180 ? hsv.h : 360 - hsv.h;

		//std::cout << r << "," << g << "," << b << " " << h << " " << red << std::endl;

		// The smaller the value "red" then the closer to the color red it is

		// normalize 0-1 and scale to 0-255
		avg += ((180 - red) / 180) * invStep;
	}

	return avg;
}

real ImageProcessor::detectLaserRangeCenter(const ImageProcessor::LaserRange& range, unsigned char * ar, unsigned char * br)
{
	int startCol = range.startCol;
	real centerCol = startCol;
	int endCol = range.endCol;
	int components = 3;

#if CENTERMASS_FILTER
	float totalSum = 0.0;
	float weightedSum = 0.0;
	int cCol = 0;
	for (int bCol = startCol; bCol < endCol; bCol++)
	{
		int iCol = bCol * components;
		const int r = (int)br[iCol + 0] - (int)ar[iCol + 0];
		const int g = (int)br[iCol + 1] - (int)ar[iCol + 1];
		const int b = (int)br[iCol + 2] - (int)ar[iCol + 2];

		float mag = r * r + g * g + b * b;
		totalSum += mag;
		weightedSum += mag * cCol;

		cCol++;
	}

	// Compute the center of mass
	centerCol = startCol + ROUND(weightedSum / totalSum);
#endif

#if GAUSS_FILTER
	GaussPrivate gaussPriv;
	double * actual = gaussPriv.actual;
	int m = endCol - startCol;
	double p[4];
	p[0] = 3000;
	p[1] = 45000;
	p[2] = m_laserRanges[rangeChoice].centerCol - startCol;
	p[3] = 5;

	mp_par pars[3];
	memset(pars,0,sizeof(pars));

	pars[0].limited[0] = 1;
	pars[0].limits[0] = 0;

	pars[1].limited[0] = 1;
	pars[1].limits[0] = 1;

	pars[2].limited[0] = 1;
	pars[2].limited[1] = 1;
	pars[2].limits[0] = 0;
	pars[2].limits[1] = m;

	// Populate the input data
	int iAct = 0;
	double * x = new double[m];
	double * ey = new double[m];
	for (int i = 0; i < m; i++)
	{
		x[i] = i + 1;
		ey[i] = 0.5;
	}

	for (int bCol = startCol; bCol < endCol; bCol++)
	{
		int iCol = bCol * components;
		const int r = (int)br[iCol + 0] - (int)ar[iCol + 0];
		const int g = (int)br[iCol + 1] - (int)ar[iCol + 1];
		const int b = (int)br[iCol + 2] - (int)ar[iCol + 2];
		actual[iAct++] = r * r + g * g + b * b;
	}

	vars_struct v;
	v.y = actual;
	v.x = x;
	v.ey = ey;

	mp_result result;
	memset(&result, 0, sizeof(result));
	int status = mpfit(gaussfunc, m, 4, p, pars, 0, (void *)&v, &result);
	//std::cout << "mpfit retCode=" << status << ", p[0]=" << p[0] << ", p[1]=" << p[1] << ", p[2]=" << p[2] << ", p[3]=" << p[3] <<", m=" << m << std::endl;

	delete [] x;
	delete [] ey;

	if (status == 1)
	{
		centerCol = startCol + p[2];
	}
	else
	{
		centerCol = m_laserRanges[rangeChoice].centerCol;
		std::cerr << "mpfit abnormal staus=" << status << std::endl;
	}
#endif


#if PEAK_FILTER  // Half-width for Center of full laser range
	int maxMagSq = 0;
	int numSameMax = 0;
	for (int bCol = startCol; bCol <= endCol; bCol++)
	{
		int iCol = bCol * components;
		const int r = (int)br[iCol + 0] - (int)ar[iCol + 0];
		const int g = (int)br[iCol + 1] - (int)ar[iCol + 1];
		const int b = (int)br[iCol + 2] - (int)ar[iCol + 2];
		const int magSq = r * r + g * g + b * b;

		if (magSq > maxMagSq || bCol == 0)
		{
			maxMagSq = magSq;
			centerCol = bCol;
			numSameMax = 0;
		}
		else if (magSq == maxMagSq)
		{
			numSameMax++;
		}
	}

	centerCol += numSameMax / 2.0f;
#endif

	return centerCol;
}

int ImageProcessor::detectBestLaserRange(ImageProcessor::LaserRange * ranges, int numRanges, int prevLaserCol)
{
	int bestRange = 0;
	int distanceOfBest = ABS(ranges[0].centerCol - prevLaserCol);

	// TODO: instead of just looking at the last laser position, this should probably be a sliding window
	// Select based off of minimum distance to last laser position
	for (int i = 1; i < numRanges; i++)
	{
		int dist = ABS(ranges[i].centerCol - prevLaserCol);
		if (dist < distanceOfBest)
		{
			bestRange = i;
			distanceOfBest = dist;
		}
	}

	return bestRange;
}

void ImageProcessor::toHsv(real r, real g, real b, Hsv * hsv)
{
	r /= 255;
	g /= 255;
	b /= 255;
	real h;

	real M = MAX3(r,g,b);
	real m = MIN3(r,g,b);
	real C = M - m;

	if (C == 0)
	{
		h = 0;
	}
	else if (M == r)
	{
		h = fmodf ((g - b) / C, 6);
	}
	else if(M == g)
	{
		h = (b - r) / C + 2;
	}
	else
	{
		h = (r - g) / C + 4;
	}

	h *= 60;

	if (h < 0)
	{
		h += 360;
	}

	real s;
	real v = M;
	if(v == 0)
	{
		s = 0;
	}
	else
	{
		s = C / v;
	}

	s *= 100;
	v *= 100;
	hsv->h = h;
	hsv->s = s;
	hsv->v = v;

}

} // ns scanner
