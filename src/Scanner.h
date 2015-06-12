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

#include "ImageProcessor.h"
#include "Thread.h"
#include "CriticalSection.h"
#include "PlyWriter.h"
#include "Laser.h"
#include "Progress.h"

namespace freelss
{

class TurnTable;
class Camera;
class Laser;
class LocationMapper;

class Scanner : public Thread
{
public:
	Scanner();
	~Scanner();

	/**
	 * The overridden thread method.  This method runs in a separate thread once execute() is called.
	 */
	void run();	
	
	/** The operation that the scanner should perform */
	enum Task { GENERATE_SCAN /**< Perform a normal scan */,
		        GENERATE_DEBUG /**< Generate debug images instead of scanning */
	          };

	/** 
	 * Rotate the points clockwise (when looking down)
	 * about the center of the turn table 
	 */
	static void rotatePoints(ColoredPoint * points, float rotation, int numPoints);
	
	/** Sets whether it should scan or generate debug info */
	void setTask(Scanner::Task task);

	/**
	 * Returns the past scan results.
	 */
	std::vector<ScanResult> getPastScanResults();

	void setRange(real range);

	/** Indicates if a scan is running or not */
	bool isRunning();

	/** Returns the progress of the current scan from 0 to 1 */
	Progress& getProgress();

	/** Get the remaining time for the scan in seconds */
	real getRemainingTime();

	/** Generate debugging images and information */
	void generateDebugInfo(Laser::LaserSide laserSide);

	/** The live data from the scanner */
	struct LiveData
	{
		std::vector<DataPoint> * leftLaserResults;
		std::vector<DataPoint> * rightLaserResults;
	};

	/** Returns the data being scanned and locks all other access to it */
	Scanner::LiveData getLiveDataLock();

	/** Releases the lock on the data */
	void releaseLiveDataLock();

private:
	struct TimingStats
	{
		double imageAcquisitionTime;
		double imageProcessingTime;
		double pointMappingTime;
		double rotationTime;
		double startTime;
		double pointProcessingTime;
		double plyWritingTime;
		double stlWritingTime;
		double xyzWritingTime;
		double facetizationTime;
		double laserTime;
		double laserMergeTime;
		int numFrameRetries;
		int numFrames;
	};

	void singleScan(int frame,
			        float rotation,
			        float stepRotation,
			        LocationMapper& leftLocMapper,
			        LocationMapper& rightLocMapper,
			        TimingStats * timingStats);
	void logTimingStats(const TimingStats& stats);

	/** Free the points data */
	void clearPoints();

	/**
	 * Returns true if the scan was processed successfully and false if there was a problem and the frame needs to be again.
	 */
	bool processScan(Image * image1, Image * image2, std::vector<DataPoint> & results, int frame, float rotation, LocationMapper& locMapper, Laser::LaserSide laserSide, int & firstRowLaserCol, TimingStats * timingStats);

	void writeRangePoints(ColoredPoint * points, int numLocationsMapped,Laser::LaserSide laserSide);

	void mergeDebuggingImages(Image& outImage, Image& leftDebuggingImage, Image& rightDebuggingImage, Laser::LaserSide laserSide);

	/**
	 *  Prepares the object for scanning.
	 */
	void prepareScan();

	/**
	 * Acquires an image from the camera.
	 */
	Image * acquireImage();

	/**
	 *  Releases an image back to the camera.
	 */
	void releaseImage(Image * image);

	/**
	 * Ensures that the "laser delay" amount of time has passed before the next
	 * picture will be taken.
	 */
	void delayAcquisitionForLaser();

	/**
	 * Draws a cross hair around pixel
	 */
	void highlightPixel(const PixelLocation& inPixel, std::vector<PixelLocation>& outPixels);

private:
	/** Unowned objects */
	Laser * m_laser;
	Camera * m_camera;
	TurnTable * m_turnTable;

private:

	/** Array of laser locations */
	PixelLocation * m_laserLocations;

	ImageProcessor m_imageProcessor;

	/** Indicates if a scan is running or not */
	bool m_running;

	/** The degrees to scan */
	real m_range;

	/** The output filename */
	std::string m_filename;

	/** The progress of the current scan */
	Progress m_progress;

	/** Protection for the running, progress, and any other status parameters */
	CriticalSection m_status;

	/** The maximum number of times to try a frame (at a particular rotation) before giving up */
	const int m_maxNumFrameRetries;

	/** The maximum number of failed laser detection rows before */
	const real m_maxNumFailedRows;

	/** The image points for every column */
	ColoredPoint * m_columnPoints;

	/** The amount of time remaining in the scan */
	real m_remainingTime;

	/** Location of the first right laser line detected in the last image */
	int m_firstRowRightLaserCol;

	/** Location of the first left laser line detected in the last image */
	int m_firstRowLeftLaserCol;

	/** Max number of pixel locations */
	unsigned m_maxNumLocations;

	/** The max number of frames per revolution */
	int m_maxFramesPerRevolution;

	/** The angle between the left and right laser planes */
	real m_radiansBetweenLaserPlanes;

	/** The number of radians between each frame */
	real m_radiansPerFrame;

	/** Location of the right laser */
	Vector3 m_rightLaserLoc;

	/** Location of the left laser */
	Vector3 m_leftLaserLoc;

	/** Location of the camera */
	Vector3 m_cameraLoc;

	/** Indicates if we wan't to write out a CSV of the range values */
	bool m_writeRangeCsvEnabled;

	/** The range CSV output */
	std::ofstream m_rangeFout;

	/** The number of frames between the laser planes */
	float m_numFramesBetweenLaserPlanes;

	/** The laser to scan with */
	Laser::LaserSide m_laserSelection;

	/** The task to perform */
	Scanner::Task m_task;

	/** Left laser results */
	std::vector<DataPoint> m_leftLaserResults;

	/** Right laser results */
	std::vector<DataPoint> m_rightLaserResults;

	/** Protection for the the 3D result data */
	CriticalSection m_results;

	/** The laser delay from the preset */
	double m_laserDelaySec;
};

}
