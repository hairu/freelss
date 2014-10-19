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

#pragma once

#include "ImageProcessor.h"
#include "Thread.h"
#include "CriticalSection.h"
#include "ScanResultsWriter.h"
#include "Laser.h"

namespace scanner
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

	void setDetail(real detail);
	void setRange(real range);

	/** Indicates if a scan is running or not */
	bool isRunning();

	/** Returns the progress of the current scan from 0 to 1 */
	real getProgress();

	/** Get the remaining time for the scan in seconds */
	real getRemainingTime();

	/** Returns the name of the current operation taking place */
	std::string getCurrentOperationName();

	/** Generate debugging images and information */
	void generateDebugInfo(Laser::LaserSide laserSide);

private:
	struct TimingStats
	{
		double imageAcquisitionTime;
		double imageProcessingTime;
		double pointMappingTime;
		double rotationTime;
		double startTime;
		double pointProcessingTime;
		double fileWritingTime;
		double meshBuildTime;
		double laserTime;
		int numScanRetries;
		int numImageProcessingRetries;
		int numScans;
	};

	void singleScan(std::vector<NeutralFileRecord> & leftLaserResults,
			        std::vector<NeutralFileRecord> & rightLaserResults,
					int step,
			        float rotation,
			        float stepRotation,
			        LocationMapper& leftLocMapper,
			        LocationMapper& rightLocMapper,
			        TimingStats * timingStats);
	void logTimingStats(const TimingStats& stats);

	/** Free the points data */
	void clearPoints();

	void finishWritingToOutput();

	void processScan(std::vector<NeutralFileRecord> & results, int step, float rotation, LocationMapper& locMapper, Laser::LaserSide laserSide, int & firstRowLaserCol, TimingStats * timingStats);

	void writeRangePoints(ColoredPoint * points, int numLocationsMapped,Laser::LaserSide laserSide);

	void mergeLaserResults(std::vector<NeutralFileRecord> & results, std::vector<NeutralFileRecord> & leftLaserResults, std::vector<NeutralFileRecord> & rightLaserResults);

private:
	/** Unowned objects */
	Laser * m_laser;
	Camera * m_camera;
	TurnTable * m_turnTable;
private:

	/**
	 *  Prepares the object for scanning.
	 */
	void prepareScan();

	/**
	 * Acquires an image from the camera.
	 */
	void acquireImage(Image * image);

	/** The maximum number of laser scans per 360 revolution */
	static const int MAX_SAMPLES_PER_FULL_REVOLUTION;

	/** Array of laser locations */
	PixelLocation * m_laserLocations;

	ImageProcessor m_imageProcessor;
	
	/** The image with the laser off */
	Image m_image1;
	
	/** The image with the laser on */
	Image m_image2;

	/** Indicates if a scan is running or not */
	bool m_running;

	/** The detail level */
	real m_detail;

	/** The degrees to scan */
	real m_range;

	/** The output filename */
	std::string m_filename;

	/** The progress of the current scan */
	real m_progress;

	/** Protection for the running, progress, and any other status parameters */
	CriticalSection m_status;

	/** The maximum number of times to try a scan (at a particular rotation) before giving up */
	const int m_maxNumScanTries;

	/** The threshold for number of suspected bad laser locations before a rescan is required */
	const int m_badLaserLocationThreshold;

	// Diagnostic info
	int m_numSuspectedBadLaserLocations;

	/** The image points for every column */
	ColoredPoint * m_columnPoints;

	/** The start time in seconds */
	real m_startTimeSec;

	/** The amount of time remaining in the scan */
	real m_remainingTime;

	/** Location of the first right laser line detected in the last image */
	int m_firstRowRightLaserCol;

	/** Location of the first left laser line detected in the last image */
	int m_firstRowLeftLaserCol;

	/** Writes the results to a neutral file, PLY file, or other output file */
	ScanResultsWriter m_scanResultsWriter;

	/** Max number of pixel locations */
	unsigned m_maxNumLocations;

	/** The angle between the left and right laser planes */
	real m_radiansBetweenLaserPlanes;

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

	/** The number of radians advanced from a single step */
	float m_radiansPerStep;

	/** The number of frames between the laser planes */
	float m_numScansBetweenLaserPlanes;

	/** The laser to scan with */
	Laser::LaserSide m_laserSelection;

	/** The name of current operation taking place */
	std::string m_currentOperationName;

	/** The task to perform */
	Scanner::Task m_task;
};

}
