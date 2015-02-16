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

#include "Main.h"
#include "Scanner.h"
#include "Settings.h"
#include "TurnTable.h"
#include "LocationMapper.h"
#include "Laser.h"
#include "Camera.h"
#include "PixelLocationWriter.h"
#include "NeutralFileReader.h"
#include "StlWriter.h"

namespace scanner
{

static bool CompareScanResults(const ScanResult& a, const ScanResult& b)
{
	return a.getScanDate() > b.getScanDate();
}
static bool CompareScanResultFiles(const ScanResultFile& a, const ScanResultFile& b)
{
	return a.extension > b.extension;
}

const int Scanner::MAX_SAMPLES_PER_FULL_REVOLUTION = 800;  // Note: This should be in Settings

Scanner::Scanner() :
	m_laser(NULL),
	m_camera(NULL),
	m_turnTable(NULL),
	m_laserLocations(NULL),
	m_running(false),
	m_detail(800),
	m_range(360),
	m_filename(""),
	m_progress(0.0),
	m_status(),
	m_maxNumScanTries(3),                    // TODO: Place this in Settings
	m_badLaserLocationThreshold(15),           // TODO: Place this in Settings
	m_numSuspectedBadLaserLocations(0),
	m_columnPoints(NULL),
	m_startTimeSec(0),
	m_remainingTime(0),
	m_firstRowRightLaserCol(0),
	m_firstRowLeftLaserCol(0),
	m_maxNumLocations(0),
	m_radiansBetweenLaserPlanes(0),
	m_rightLaserLoc(),
	m_leftLaserLoc(),
	m_cameraLoc(),
	m_writeRangeCsvEnabled(false),
	m_rangeFout(),
	m_radiansPerStep(0),
	m_numScansBetweenLaserPlanes(0),
	m_laserSelection(Laser::ALL_LASERS),
	m_currentOperationName(""),
	m_task(GENERATE_SCAN)
{
	// Do nothing
}

Scanner::~Scanner()
{
	// Disable the turn table motor
	if (m_turnTable != NULL)
	{
		m_turnTable->setMotorEnabled(false);
	}

	delete [] m_laserLocations;
	delete [] m_columnPoints;
}

void Scanner::setTask(Scanner::Task task)
{
	m_task = task;
}

bool Scanner::isRunning()
{
	bool running;

	m_status.enter();
	running = m_running;
	m_status.leave();

	return running;
}

real Scanner::getProgress()
{
	real progress;

	m_status.enter();
	progress = m_progress;
	m_status.leave();

	return progress;
}

real Scanner::getRemainingTime()
{
	real remainingTime;

	m_status.enter();
	remainingTime = m_remainingTime;
	m_status.leave();

	return remainingTime;
}

void Scanner::setDetail(int detail)
{
	int stepsPerRevolution = Settings::get()->readInt(Settings::GENERAL_SETTINGS, Settings::STEPS_PER_REVOLUTION);
	if (detail <= stepsPerRevolution && detail > 0)
	{
		m_detail = detail;
	}
}

void Scanner::setRange(real range)
{
	if (range > 360)
	{
		m_range = 360;
	}
	else if (range < 0.01)
	{
		m_range = 0.01;
	}
	else
	{
		m_range = range;
	}
}

void Scanner::prepareScan()
{
	m_status.enter();
	m_running = true;
	m_progress = 0.0;
	m_startTimeSec = GetTimeInSeconds();
	m_remainingTime = 0;

	if (m_task == Scanner::GENERATE_SCAN)
	{
		m_currentOperationName = "Scanning";
	}
	else if (m_task == Scanner::GENERATE_DEBUG)
	{
		m_currentOperationName = "Generating debug info";
	}
	else
	{
		m_currentOperationName = "Unknown";
	}

	// Get handles to our hardware devices
	m_camera = Camera::getInstance();
	m_laser = Laser::getInstance();
	m_turnTable = TurnTable::getInstance();

	// Initialize data structures
	m_maxNumLocations = m_camera->getImageHeight();

	delete [] m_laserLocations;
	m_laserLocations = new PixelLocation[m_maxNumLocations];

	delete [] m_columnPoints;
	m_columnPoints = new ColoredPoint[m_camera->getImageWidth()];

	m_status.leave();
}

void Scanner::run()
{
	// Prepare to scan
	prepareScan();

	// Set the base output file
	std::stringstream sstr;
	sstr << Settings::SCAN_OUTPUT_DIR << std::string("/") << time(NULL);

	m_filename = sstr.str();

	m_numSuspectedBadLaserLocations = 0;

	m_firstRowRightLaserCol = m_camera->getImageWidth() * 0.5;
	m_firstRowLeftLaserCol = m_camera->getImageWidth() * 0.5;

	Scanner::TimingStats timingStats;
	memset(&timingStats, 0, sizeof(Scanner::TimingStats));
	timingStats.startTime = GetTimeInSeconds();
	double time1 = 0;

	Settings * settings = Settings::get();

	// Read the laser selection
	m_laserSelection = (Laser::LaserSide) settings->readInt(Settings::GENERAL_SETTINGS, Settings::LASER_SELECTION);

	// Read the location of the lasers
	m_rightLaserLoc.x = settings->readReal(Settings::GENERAL_SETTINGS, Settings::RIGHT_LASER_X);
	m_rightLaserLoc.y = settings->readReal(Settings::GENERAL_SETTINGS, Settings::RIGHT_LASER_Y);
	m_rightLaserLoc.z = settings->readReal(Settings::GENERAL_SETTINGS, Settings::RIGHT_LASER_Z);

	m_leftLaserLoc.x = settings->readReal(Settings::GENERAL_SETTINGS, Settings::LEFT_LASER_X);
	m_leftLaserLoc.y = settings->readReal(Settings::GENERAL_SETTINGS, Settings::LEFT_LASER_Y);
	m_leftLaserLoc.z = settings->readReal(Settings::GENERAL_SETTINGS, Settings::LEFT_LASER_Z);

	// Read the location of the camera
	m_cameraLoc.x = settings->readReal(Settings::GENERAL_SETTINGS, Settings::CAMERA_X);
	m_cameraLoc.y = settings->readReal(Settings::GENERAL_SETTINGS, Settings::CAMERA_Y);
	m_cameraLoc.z = settings->readReal(Settings::GENERAL_SETTINGS, Settings::CAMERA_Z);

	LocationMapper leftLocMapper(m_leftLaserLoc, m_cameraLoc);
	LocationMapper rightLocMapper(m_rightLaserLoc, m_cameraLoc);

	// Compute the angle between the two laser planes
	real leftLaserX = ABS(m_leftLaserLoc.x);
	real rightLaserX = ABS(m_rightLaserLoc.x);
	real camZ = ABS(m_cameraLoc.z);

	// Sanity check to prevent divide by 0.  In reality the laser should never be this close to the camera
	if (leftLaserX < 0.001)
	{
		leftLaserX = 0.001;
	}

	if (rightLaserX < 0.001)
	{
		rightLaserX = 0.001;
	}

	//
	// tan(theta) = ABS(laserX) / camZ
	// theta = atan(ABS(laserX) / camZ)
	//
	real leftLaserAngle = atan(leftLaserX / camZ);
	real rightLaserAngle = atan(rightLaserX / camZ);

	m_radiansBetweenLaserPlanes = leftLaserAngle + rightLaserAngle;

	// Write the range CSV
	if (m_writeRangeCsvEnabled)
	{
		m_rangeFout.open((m_filename + ".csv").c_str());
		if (!m_rangeFout.is_open())
		{
			throw Exception("Error opening range CSV file");
		}
	}

	// The results vector
	std::vector<NeutralFileRecord> leftLaserResults, rightLaserResults;

	try
	{
		// Enable the turn table motor
		m_turnTable->setMotorEnabled(true);
		std::cout << "Enabled motor" << std::endl;

		if (m_laser == NULL)
		{
			throw Exception("Laser object is NULL");
		}

		if (m_turnTable == NULL)
		{
			throw Exception("Turntable object is NULL");
		}

		float rotation = 0;

		// Read the number of motor steps per revolution
		int stepsPerRevolution = Settings::get()->readInt(Settings::GENERAL_SETTINGS, Settings::STEPS_PER_REVOLUTION);

		float rangeRadians =  (m_range / 360) * (2 * PI);

		int maxSamplesPerRevolution = m_detail;
		if (maxSamplesPerRevolution < 1)
		{
			maxSamplesPerRevolution = 1;
		}

		// The number of steps for a single scan
		int stepsPerScan = ceil(stepsPerRevolution / (float)maxSamplesPerRevolution);
		if (stepsPerScan < 1)
		{
			stepsPerScan = 1;
		}

		// The number of radians a single step takes you
		m_radiansPerStep = ((2 * PI) / (float) stepsPerRevolution);

		// The radians to move for a single scan
		float scanRadians = stepsPerScan * m_radiansPerStep;

		int numSteps = ceil(rangeRadians / scanRadians);

		// Start the output thread
		m_scanResultsWriter.setBaseFilePath(m_filename);
		std::cout << "Starting output thread..." << std::endl;
		time1 = GetTimeInSeconds();
		m_scanResultsWriter.execute();
		timingStats.fileWritingTime += GetTimeInSeconds() - time1;
		std::cout << "Output thread started" << std::endl;

		m_numScansBetweenLaserPlanes = rangeRadians / m_radiansBetweenLaserPlanes;

		std::cout << "Angle between laser planes: " << RADIANS_TO_DEGREES(m_radiansBetweenLaserPlanes)
				  << " degrees, radiansPerStep=" << m_radiansPerStep
				  << ", numScansBetweenLaserPlanes=" << m_numScansBetweenLaserPlanes
				  << ", numSamples=" << numSteps << std::endl;

		for (int iStep = 0; iStep < numSteps; iStep++)
		{
			timingStats.numScans++;

			// Stop if the user asked us to
			if (m_stopRequested)
			{
				break;
			}

			singleScan(leftLaserResults, rightLaserResults, iStep, rotation, scanRadians, leftLocMapper, rightLocMapper, &timingStats);

			rotation += scanRadians;

			// Update the progress
			double progress = (iStep + 1.0) / numSteps;
			double timeElapsed = GetTimeInSeconds() - m_startTimeSec;
			double percentComplete = 100.0 * progress;
			double percentPerSecond = percentComplete / timeElapsed;
			double fullTimeSec = 100.0 / percentPerSecond;
			double remainingSec = fullTimeSec - timeElapsed;

			m_status.enter();
			m_progress = progress;
			m_remainingTime = remainingSec;
			m_status.leave();

			logTimingStats(timingStats);
			std::cout << percentComplete << "% Complete, " << (remainingSec / 60) << " minutes remaining." << std::endl;
		}
	}
	catch (...)
	{	
		m_turnTable->setMotorEnabled(false);

		m_status.enter();
		m_running = false;
		m_status.leave();
		finishWritingToOutput();

		if (m_writeRangeCsvEnabled)
		{
			m_rangeFout.close();
		}

		throw;
	}
	
	m_rangeFout.close();
	std::cout << m_numSuspectedBadLaserLocations << " laser locations are suspected as being bad." << std::endl;

	m_turnTable->setMotorEnabled(false);

	// Finish writing to the output files
	std::cout << "Finishing output..." << std::endl;
	time1 = GetTimeInSeconds();
	finishWritingToOutput();
	timingStats.fileWritingTime += GetTimeInSeconds() - time1;

	// Build the mesh
	m_status.enter();
	m_progress = 0.5;
	m_currentOperationName = "Building mesh";
	m_status.leave();

	std::cout << "Merging laser results..." << std::endl;

	time1 = GetTimeInSeconds();

	// Average the left and right lasers and sort the results
	std::vector<NeutralFileRecord> results;
	mergeLaserResults(results, leftLaserResults, rightLaserResults);

	std::cout << "Merged " << leftLaserResults.size() << " left laser and " << rightLaserResults.size() << " right laser results into " << results.size() << " results." << std::endl;
	std::cout << "Constructing mesh..." << std::endl;

	// Mesh the contents
	StlWriter stlWriter;
	stlWriter.write(m_filename, results, m_range > 359);
	timingStats.meshBuildTime = GetTimeInSeconds() - time1;

	logTimingStats(timingStats);

	m_status.enter();
	m_running = false;
	m_progress = 1.0;
	m_status.leave();

	std::cout << "Done." << std::endl;
}

void Scanner::mergeLaserResults(std::vector<NeutralFileRecord> & out, std::vector<NeutralFileRecord> & leftLaserResults, std::vector<NeutralFileRecord> & rightLaserResults)
{
	// Handle the cases of single laser scans
	if (leftLaserResults.empty())
	{
		out = rightLaserResults;
		for (size_t iOut = 0; iOut < out.size(); iOut++)
		{
			out[iOut].pseudoStep = out[iOut].step;
		}
	}
	else if (rightLaserResults.empty())
	{
		out = leftLaserResults;
		for (size_t iOut = 0; iOut < out.size(); iOut++)
		{
			out[iOut].pseudoStep = out[iOut].step;
		}
	}
	else
	{
		// Merge the results
		std::cout << "Detected " << m_numScansBetweenLaserPlanes << " scans between the lasers." << std::endl;

		size_t iLeft = 0;
		for (size_t iRight = 0; iRight < rightLaserResults.size(); iRight++)
		{
			NeutralFileRecord right = rightLaserResults[iRight];
			right.pseudoStep = right.step;

			// Add all the left lasers at this portion of the pseudo step
			NeutralFileRecord left = leftLaserResults[iLeft];
			left.pseudoStep = left.step + m_numScansBetweenLaserPlanes;

			// TODO: Match up the left with the closest right and average these columns
			while (iLeft < leftLaserResults.size() && left.pseudoStep <= right.pseudoStep)
			{
				out.push_back(left);
				iLeft++;

				left = leftLaserResults[iLeft];
				left.pseudoStep = left.step + m_numScansBetweenLaserPlanes;
			}

			out.push_back(right);
		}

		// Add the remaining left lasers
		while (iLeft < leftLaserResults.size())
		{
			out.push_back(leftLaserResults[iLeft]);
			iLeft++;
		}
	}
}

void Scanner::finishWritingToOutput()
{
	// Wait for all the I/O writing to complete
	while (m_scanResultsWriter.getNumPendingRecords() > 0)
	{
		Thread::usleep(100000);
	}

	// Wait for the thread to stop
	m_scanResultsWriter.stop();
	m_scanResultsWriter.join();
}

void Scanner::generateDebugInfo(Laser::LaserSide laserSide)
{
	// Prepare for scanning
	prepareScan();

	m_laser->turnOff(Laser::ALL_LASERS);
	acquireImage(&m_image1);

	m_laser->turnOn(laserSide);
	acquireImage(&m_image2);

	m_laser->turnOff(laserSide);

	Image debuggingImage;

	std::string debuggingCsv = std::string(Settings::DEBUG_OUTPUT_DIR) + "/0.csv";

	int firstRowLaserCol = m_camera->getImageWidth() * 0.5;
	int numSuspectedBadLaserLocations = 0;
	int numImageProcessingRetries = 0;
	int numLocations = m_imageProcessor.process(m_image1,
											    m_image2,
												&debuggingImage,
												m_laserLocations,
												m_maxNumLocations,
												firstRowLaserCol,
												numSuspectedBadLaserLocations,
												numImageProcessingRetries,
												debuggingCsv.c_str());

	std::string baseFilename = std::string(Settings::DEBUG_OUTPUT_DIR) + "/";

	// Write the laser off image
	Image::writeJpeg(m_image1, baseFilename + "1.jpg");

	// Write the laser on image
	Image::writeJpeg(m_image2, baseFilename + "2.jpg");

	PixelLocationWriter locWriter;

	// Write the difference image
	locWriter.writeImage(debuggingImage, debuggingImage.getWidth(), debuggingImage.getHeight(), baseFilename + "3.png");

	// Write the pixel image
	locWriter.writePixels(m_laserLocations, numLocations, m_image1.getWidth(), m_image1.getHeight(), baseFilename + "4.png");

	// Overlay the pixels onto the debug image and write that as a new image
	Image::overlayPixels(debuggingImage, m_laserLocations, numLocations);
	locWriter.writeImage(debuggingImage, debuggingImage.getWidth(), debuggingImage.getHeight(), baseFilename + "5.png");

	m_status.enter();
	m_running = false;
	m_progress = 1.0;
	m_status.leave();
	std::cout << "Done." << std::endl;
}

void Scanner::singleScan(std::vector<NeutralFileRecord> & leftLaserResults, std::vector<NeutralFileRecord> & rightLaserResults, int step, float rotation, float stepRotation,
		                 LocationMapper& leftLocMapper, LocationMapper& rightLocMapper, TimingStats * timingStats)
{
	double time1 = GetTimeInSeconds();
	m_turnTable->rotate(stepRotation);
	timingStats->rotationTime += GetTimeInSeconds() - time1;

	// Make sure the laser is off
	m_laser->turnOff(Laser::ALL_LASERS);

	// Take a picture with the laser off
	time1 = GetTimeInSeconds();
	acquireImage(&m_image1);
	timingStats->imageAcquisitionTime += GetTimeInSeconds() - time1;

	// If this is the first image, save it as a thumbnail
	if (step == 0)
	{
		std::string thumbnail = m_filename + ".png";

		PixelLocationWriter imageWriter;
		imageWriter.writeImage(m_image1, 128, 96, thumbnail.c_str());
	}

	// Scan with the Right laser
	if (m_laserSelection == Laser::RIGHT_LASER || m_laserSelection == Laser::ALL_LASERS)
	{
		// Turn on the right laser
		time1 = GetTimeInSeconds();
		m_laser->turnOn(Laser::RIGHT_LASER);
		timingStats->laserTime += GetTimeInSeconds() - time1;

		// Take a picture with the right laser on
		time1 = GetTimeInSeconds();
		acquireImage(&m_image2);
		timingStats->imageAcquisitionTime += GetTimeInSeconds() - time1;

		// Turn off the right laser
		time1 = GetTimeInSeconds();
		m_laser->turnOff(Laser::RIGHT_LASER);
		timingStats->laserTime += GetTimeInSeconds() - time1;

		// Process the right laser results
		processScan(rightLaserResults, step, rotation, rightLocMapper, Laser::RIGHT_LASER, m_firstRowRightLaserCol, timingStats);
	}

	// Scan with the Left laser
	if (m_laserSelection == Laser::LEFT_LASER || m_laserSelection == Laser::ALL_LASERS)
	{
		// Turn on the left laser
		time1 = GetTimeInSeconds();
		m_laser->turnOn(Laser::LEFT_LASER);
		timingStats->laserTime += GetTimeInSeconds() - time1;

		// Take a picture with the left laser on
		time1 = GetTimeInSeconds();
		acquireImage(&m_image2);
		timingStats->imageAcquisitionTime += GetTimeInSeconds() - time1;

		// Turn off the left laser
		time1 = GetTimeInSeconds();
		m_laser->turnOff(Laser::LEFT_LASER);
		timingStats->laserTime += GetTimeInSeconds() - time1;

		// Process the left laser results
		processScan(leftLaserResults, step, rotation, leftLocMapper, Laser::LEFT_LASER, m_firstRowLeftLaserCol, timingStats);
	}
}

void Scanner::processScan(std::vector<NeutralFileRecord> & results, int step, float rotation, LocationMapper& locMapper, Laser::LaserSide laserSide, int & firstRowLaserCol, TimingStats * timingStats)
{
	int numLocationsMapped = 0;
	int numSuspectedBadLaserLocations = 0;

	// Send the pictures off for processing
	double time1 = GetTimeInSeconds();
	int numLocations = m_imageProcessor.process(m_image1,
												m_image2,
												NULL,
												m_laserLocations,
												m_maxNumLocations,
												firstRowLaserCol,
												numSuspectedBadLaserLocations,
												timingStats->numImageProcessingRetries,
												NULL);


	timingStats->imageProcessingTime += GetTimeInSeconds() - time1;

	std::cout << "Detected " << numLocations << " laser pixels." << std::endl;

	// Lookup the 3D locations for the laser points
	numLocationsMapped = 0;

	if (numLocations > 0)
	{
		time1 = GetTimeInSeconds();
		locMapper.mapPoints(m_laserLocations, &m_image1, m_columnPoints, numLocations, numLocationsMapped);
		timingStats->pointMappingTime += GetTimeInSeconds() - time1;

		if (numLocations != numLocationsMapped)
		{
			std::cout << "Discarded " << numLocations - numLocationsMapped << " points." << std::endl;
		}

		if (numSuspectedBadLaserLocations > m_badLaserLocationThreshold)
		{
			std::cout << "!! Many bad laser locations suspected, num=" << numSuspectedBadLaserLocations << std::endl;
		}
	}
	else
	{
		// Stop here if we didn't detect the laser at all
		std::cerr << "!!! Could not detect laser at all" << std::endl;
	}


	// Map the points if there was something to map
	if (numLocationsMapped > 0)
	{
		time1 = GetTimeInSeconds();
		m_numSuspectedBadLaserLocations += numSuspectedBadLaserLocations;

		if (m_writeRangeCsvEnabled)
		{
			writeRangePoints(m_columnPoints, numLocationsMapped, laserSide);
		}

		// Rotate the points
		rotatePoints(m_columnPoints, rotation, numLocationsMapped);

		timingStats->pointProcessingTime += GetTimeInSeconds() - time1;

		// Write to the neutral file
		time1 = GetTimeInSeconds();
		for (int iLoc = 0; iLoc < numLocationsMapped; iLoc++)
		{
			NeutralFileRecord record;
			record.pixel = m_laserLocations[iLoc];
			record.point = m_columnPoints[iLoc];
			record.rotation = laserSide == Laser::RIGHT_LASER ? rotation : rotation + m_radiansBetweenLaserPlanes;
			record.step = step;
			record.laserSide = (int) laserSide;
			results.push_back(record);

			m_scanResultsWriter.write(record);
		}
		timingStats->fileWritingTime += GetTimeInSeconds() - time1;
	}
}

void Scanner::rotatePoints(ColoredPoint * points, float theta, int numPoints)
{
	// Build the 2D rotation matrix to rotate in the XZ plane
	real c = cos(theta);
	real s = sin(theta);
	
	// Rotate
	for (int iPt = 0; iPt < numPoints; iPt++)
	{
		// Location
		real x = points[iPt].x * c + points[iPt].z * -s;
		real z = points[iPt].x * s + points[iPt].z * c;
		points[iPt].x = x;
		points[iPt].z = z;

		// Normal
		real nx = points[iPt].normal.x * c + points[iPt].normal.z * -s;
		real nz = points[iPt].normal.x * s + points[iPt].normal.z * c;
		points[iPt].normal.x = nx;
		points[iPt].normal.z = nz;
	}
}

void Scanner::writeRangePoints(ColoredPoint * points, int numLocationsMapped, Laser::LaserSide laserSide)
{
	Vector3 laserLoc;

	if (Laser::LEFT_LASER == laserSide)
	{
		laserLoc = m_leftLaserLoc;
	}
	else if (Laser::RIGHT_LASER == laserSide)
	{
		laserLoc = m_rightLaserLoc;
	}
	else
	{
		throw Exception("Invalid LaserSide given");
	}

	int iLoc = 0;
	while (iLoc < numLocationsMapped)
	{
		const ColoredPoint& point = points[iLoc];

		real32 dx = laserLoc.x - point.x;
		real32 dy = laserLoc.y - point.y;
		real32 dz = laserLoc.z - point.z;

		real32 distSq = dx * dx + dy * dy + dz * dz;

		if (iLoc > 0)
		{
			m_rangeFout << ",";
		}

		m_rangeFout << distSq;
		iLoc++;
	}

	Camera * camera = Camera::getInstance();
	int imageHeight = camera->getImageHeight();
	while (iLoc < imageHeight)
	{
		m_rangeFout << ",";
		iLoc++;
	}

	m_rangeFout << std::endl;
}

void Scanner::logTimingStats(const Scanner::TimingStats& stats)
{
	// Prevent divide by zero
	if (stats.numScans == 0)
	{
		return;
	}

	double now = GetTimeInSeconds();
	double totalTime = now - stats.startTime;

	double accountedTime = stats.meshBuildTime + stats.imageAcquisitionTime + stats.imageProcessingTime + stats.pointMappingTime
			+ stats.pointProcessingTime + stats.rotationTime + stats.fileWritingTime + stats.laserTime;

	double unaccountedTime = totalTime - accountedTime;
	double rate = totalTime / stats.numScans;

	std::cout << "Total Seconds per frame:\t" << rate << std::endl;
	std::cout << "Unaccounted time:\t" << (100.0 * unaccountedTime / totalTime) << "%" << std::endl;
	std::cout << "Image Acquisition:\t" << (100.0 * stats.imageAcquisitionTime / totalTime) << "%, " << (stats.imageAcquisitionTime / stats.numScans) << " seconds per frame." << std::endl;
	std::cout << "Image Processing:\t" << (100.0 * stats.imageProcessingTime / totalTime) << "%, " << (stats.imageProcessingTime / stats.numScans) << " seconds per frame." << std::endl;
	std::cout << "Laser Time:\t" << (100.0 * stats.laserTime / totalTime) << "%, " << (stats.laserTime / stats.numScans) << " seconds per frame." << std::endl;
	std::cout << "Point Mapping:\t" << (100.0 * stats.pointMappingTime / totalTime) << "%" << std::endl;
	std::cout << "Point Rotating:\t" << (100.0 * stats.pointProcessingTime / totalTime) << "%" << std::endl;
	std::cout << "Table Rotation:\t" << (100.0 * stats.rotationTime / totalTime) << "%" << std::endl;
	std::cout << "File Writing:\t" << (100.0 * stats.fileWritingTime / totalTime) << "%" << std::endl;
	std::cout << "Mesh Construction:\t" << (100.0 * stats.meshBuildTime / totalTime) << "%" << std::endl;
	std::cout << "Num Scan Retries:\t" << stats.numScanRetries << std::endl;
	std::cout << "Num Image Processing Retries:\t" << stats.numImageProcessingRetries << std::endl;
	std::cout << "Num Frames:\t" << stats.numScans << std::endl;
	std::cout << "Total Time (min):\t" << (totalTime / 60.0) << std::endl << std::endl;
}


std::vector<ScanResult> Scanner::getPastScanResults()
{
	DIR * dirp = opendir(Settings::SCAN_OUTPUT_DIR);
	if (dirp == NULL)
	{
		throw Exception("Error opening scan directory");
	}

	struct dirent *dp = readdir(dirp);

	std::map<std::string, ScanResult> scanResultMap;

	while (dp != NULL)
	{
		std::string name = dp->d_name;
		size_t dotPos = name.find_last_of(".");

		if (name != "." && name != ".." && dotPos != std::string::npos)
		{
			std::string extension = name.substr(dotPos + 1);
			std::string base = name.substr(0, dotPos);

			std::string fullPath = std::string(Settings::SCAN_OUTPUT_DIR) + "/" + name;
			ScanResultFile file;

			struct stat st;
			if (stat(fullPath.c_str(), &st) != 0)
			{
				throw Exception("Error obtaining stats on file: " + fullPath);
			}

			file.extension = extension;
			file.creationTime = atol(base.c_str());
			file.fileSize = st.st_size;

			scanResultMap[base].files.push_back(file);
		}

		dp = readdir(dirp);
	}

	closedir(dirp);

	//
	// Turn the map into a vector
	//
	std::vector<ScanResult> results;

	std::map<std::string, ScanResult>::iterator it = scanResultMap.begin();
	while (it != scanResultMap.end())
	{
		std::sort(it->second.files.begin(), it->second.files.end(), CompareScanResultFiles);
		results.push_back(it->second);
		it++;
	}

	// Sort the results
	std::sort(results.begin(), results.end(), CompareScanResults);

	return results;
}

std::string Scanner::getCurrentOperationName()
{
	std::string operation;

	m_status.enter();
	operation = m_currentOperationName;
	m_status.leave();

	return operation;
}

void Scanner::acquireImage(Image * image)
{
	m_camera->acquireImage(image);
}

} // ns sdl

