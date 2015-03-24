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
#include "Scanner.h"
#include "PresetManager.h"
#include "Setup.h"
#include "TurnTable.h"
#include "LocationMapper.h"
#include "Laser.h"
#include "Camera.h"
#include "PixelLocationWriter.h"
#include "NeutralFileReader.h"
#include "StlWriter.h"
#include "XyzWriter.h"
#include "LaserResultsMerger.h"

namespace freelss
{

static bool CompareScanResults(const ScanResult& a, const ScanResult& b)
{
	return a.getScanDate() > b.getScanDate();
}
static bool CompareScanResultFiles(const ScanResultFile& a, const ScanResultFile& b)
{
	return a.extension > b.extension;
}

static bool ComparePseudoSteps(const NeutralFileRecord& a, const NeutralFileRecord& b)
{
	if (a.pseudoFrame != b.pseudoFrame)
	{
		return a.pseudoFrame < b.pseudoFrame;
	}

	return a.pixel.y < b.pixel.y;
}

Scanner::Scanner() :
	m_laser(NULL),
	m_camera(NULL),
	m_turnTable(NULL),
	m_laserLocations(NULL),
	m_running(false),
	m_range(360),
	m_filename(""),
	m_progress(0.0),
	m_status(),
	m_maxNumFrameRetries(5),                    // TODO: Place this in Database
	m_maxPercentPixelsOverThreshold(3),           // TODO: Place this in Database
	m_columnPoints(NULL),
	m_startTimeSec(0),
	m_remainingTime(0),
	m_firstRowRightLaserCol(0),
	m_firstRowLeftLaserCol(0),
	m_maxNumLocations(0),
	m_radiansBetweenLaserPlanes(0),
	m_radiansPerFrame(0),
	m_rightLaserLoc(),
	m_leftLaserLoc(),
	m_cameraLoc(),
	m_writeRangeCsvEnabled(false),
	m_rangeFout(),
	m_numFramesBetweenLaserPlanes(0),
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
	sstr << SCAN_OUTPUT_DIR << std::string("/") << time(NULL);

	m_filename = sstr.str();

	m_firstRowRightLaserCol = m_camera->getImageWidth() * 0.5;
	m_firstRowLeftLaserCol = m_camera->getImageWidth() * 0.5;

	Scanner::TimingStats timingStats;
	memset(&timingStats, 0, sizeof(Scanner::TimingStats));
	timingStats.startTime = GetTimeInSeconds();
	double time1 = 0;

	Setup * setup = Setup::get();
	Preset preset = PresetManager::get()->getActivePreset();

	// Read the laser selection
	m_laserSelection = preset.laserSide;

	// Read the location of the lasers and camera
	m_rightLaserLoc = setup->rightLaserLocation;
	m_leftLaserLoc = setup->leftLaserLocation;
	m_cameraLoc = setup->cameraLocation;

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
	int numFrames = 0;

	int maxFramesPerRevolution = preset.framesPerRevolution;
	if (maxFramesPerRevolution < 1)
	{
		maxFramesPerRevolution = 1;
	}

	int stepsPerRevolution = Setup::get()->stepsPerRevolution;
	if (maxFramesPerRevolution > stepsPerRevolution)
	{
		maxFramesPerRevolution = stepsPerRevolution;
	}

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
			throw Exception("Laser object is NULL");
		}

		float rotation = 0;

		// Read the number of motor steps per revolution
		int stepsPerRevolution = setup->stepsPerRevolution;

		float rangeRadians =  (m_range / 360) * (2 * PI);

		// The number of steps for a single frame
		int stepsPerFrame = ceil(stepsPerRevolution / (float)maxFramesPerRevolution);
		if (stepsPerFrame < 1)
		{
			stepsPerFrame = 1;
		}

		// The number of radians a single step takes you
		m_radiansPerFrame = ((2 * PI) / (float) stepsPerRevolution);

		// The radians to move for a single frame
		float frameRadians = stepsPerFrame * m_radiansPerFrame;

		numFrames = ceil(rangeRadians / frameRadians);

		m_numFramesBetweenLaserPlanes = m_radiansBetweenLaserPlanes / frameRadians;

		std::cout << "Angle between laser planes: " << RADIANS_TO_DEGREES(m_radiansBetweenLaserPlanes)
				  << " degrees, radiansPerFrame=" << m_radiansPerFrame
				  << ", numFramesBetweenLaserPlanes=" << m_numFramesBetweenLaserPlanes
				  << ", numFrames=" << numFrames << std::endl;

		for (int iFrame = 0; iFrame < numFrames; iFrame++)
		{
			timingStats.numFrames++;

			// Stop if the user asked us to
			if (m_stopRequested)
			{
				break;
			}

			singleScan(leftLaserResults, rightLaserResults, iFrame, rotation, frameRadians, leftLocMapper, rightLocMapper, &timingStats);

			rotation += frameRadians;

			// Update the progress
			double progress = (iFrame + 1.0) / numFrames;
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

	m_turnTable->setMotorEnabled(false);

	// Build the mesh
	m_status.enter();
	m_progress = 0.5;
	m_currentOperationName = "Building mesh";
	m_status.leave();

	std::cout << "Merging laser results..." << std::endl;

	time1 = GetTimeInSeconds();

	// Merge the left and right lasers and sort the results
	std::vector<NeutralFileRecord> results;
	LaserResultsMerger merger;
	merger.merge(results, leftLaserResults, rightLaserResults, maxFramesPerRevolution,
			     m_numFramesBetweenLaserPlanes, Camera::getInstance()->getImageHeight(), preset.laserMergeAction);

	// Sort by pseudo-step and row
	std::cout << "Sort 2... " << std::endl;
	std::sort(results.begin(), results.end(), ComparePseudoSteps);
	std::cout << "End Sort 2... " << std::endl;

	std::cout << "Merged " << leftLaserResults.size() << " left laser and " << rightLaserResults.size() << " right laser results into " << results.size() << " results." << std::endl;
	std::cout << "Constructing mesh..." << std::endl;

	timingStats.laserMergeTime += GetTimeInSeconds() - time1;

	// Write the PLY file
	std::cout << "Starting output thread..." << std::endl;
	if (preset.generatePly)
	{
		std::cout << "Writing PLY file..." << std::endl;
		time1 = GetTimeInSeconds();

		m_scanResultsWriter.setBaseFilePath(m_filename);
		m_scanResultsWriter.execute();

		for (size_t iRec = 0; iRec < results.size(); iRec++)
		{
			m_scanResultsWriter.write(results[iRec]);
		}

		finishWritingToOutput();
		timingStats.fileWritingTime += GetTimeInSeconds() - time1;
	}

	// Generate the XYZ file
	if (preset.generateXyz)
	{
		std::cout << "Generating XYZ file..." << std::endl;
		time1 = GetTimeInSeconds();
		XyzWriter xyzWriter;
		xyzWriter.write(m_filename, results);
		timingStats.fileWritingTime += GetTimeInSeconds() - time1;
	}

	// Generate the STL file
	if (preset.generateStl)
	{
		std::cout << "Generating STL mesh..." << std::endl;
		time1 = GetTimeInSeconds();
		StlWriter stlWriter;
		stlWriter.write(m_filename, results, m_range > 359);
		timingStats.meshBuildTime = GetTimeInSeconds() - time1;
	}

	logTimingStats(timingStats);

	m_status.enter();
	m_running = false;
	m_progress = 1.0;
	m_status.leave();

	std::cout << "Done." << std::endl;
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

	std::string debuggingCsv = std::string(DEBUG_OUTPUT_DIR) + "/0.csv";

	int firstRowLaserCol = m_camera->getImageWidth() * 0.5;
	real percentPixelsOverThreshold = 0;
	int numLocations = m_imageProcessor.process(m_image1,
											    m_image2,
												&debuggingImage,
												m_laserLocations,
												m_maxNumLocations,
												firstRowLaserCol,
												percentPixelsOverThreshold,
												debuggingCsv.c_str());

	std::string baseFilename = std::string(DEBUG_OUTPUT_DIR) + "/";

	// Overlay the pixels onto the debug image and write that as a new image
	PixelLocationWriter locWriter;
	Image::overlayPixels(debuggingImage, m_laserLocations, numLocations);
	locWriter.writeImage(debuggingImage, debuggingImage.getWidth(), debuggingImage.getHeight(), baseFilename + "5.png");

	m_status.enter();
	m_running = false;
	m_progress = 1.0;
	m_status.leave();
	std::cout << "Done." << std::endl;
}

void Scanner::singleScan(std::vector<NeutralFileRecord> & leftLaserResults, std::vector<NeutralFileRecord> & rightLaserResults, int frame, float rotation, float frameRotation,
		                 LocationMapper& leftLocMapper, LocationMapper& rightLocMapper, TimingStats * timingStats)
{
	double time1 = GetTimeInSeconds();
	m_turnTable->rotate(frameRotation);
	timingStats->rotationTime += GetTimeInSeconds() - time1;

	// Make sure the laser is off
	m_laser->turnOff(Laser::ALL_LASERS);

	// Take a picture with the laser off
	time1 = GetTimeInSeconds();
	acquireImage(&m_image1);
	timingStats->imageAcquisitionTime += GetTimeInSeconds() - time1;

	// If this is the first image, save it as a thumbnail
	if (frame == 0)
	{
		std::string thumbnail = m_filename + ".png";

		PixelLocationWriter imageWriter;
		imageWriter.writeImage(m_image1, 128, 96, thumbnail.c_str());
	}

	// Scan with the Right laser
	if (m_laserSelection == Laser::RIGHT_LASER || m_laserSelection == Laser::ALL_LASERS)
	{
		bool goodResult = false;

		for (int iTry = 0; iTry < m_maxNumFrameRetries && ! goodResult; iTry++)
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
			goodResult = processScan(rightLaserResults, frame, rotation, rightLocMapper, Laser::RIGHT_LASER, m_firstRowRightLaserCol, timingStats);

			// If it didn't succeed, take the first image again
			if (!goodResult)
			{
				// Take a picture with the laser off
				time1 = GetTimeInSeconds();
				acquireImage(&m_image1);
				timingStats->imageAcquisitionTime += GetTimeInSeconds() - time1;
			}
		}
	}

	// Scan with the Left laser
	if (m_laserSelection == Laser::LEFT_LASER || m_laserSelection == Laser::ALL_LASERS)
	{
		bool goodResult = false;

		for (int iTry = 0; iTry < m_maxNumFrameRetries && ! goodResult; iTry++)
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
			goodResult = processScan(leftLaserResults, frame, rotation, leftLocMapper, Laser::LEFT_LASER, m_firstRowLeftLaserCol, timingStats);

			// If it didn't succeed, take the first image again
			if (!goodResult)
			{
				// Take a picture with the laser off
				time1 = GetTimeInSeconds();
				acquireImage(&m_image1);
				timingStats->imageAcquisitionTime += GetTimeInSeconds() - time1;
			}
		}
	}
}

bool Scanner::processScan(std::vector<NeutralFileRecord> & results, int frame, float rotation, LocationMapper& locMapper, Laser::LaserSide laserSide, int & firstRowLaserCol, TimingStats * timingStats)
{
	int numLocationsMapped = 0;
	real percentPixelsOverThreshold = 0;

	// Send the pictures off for processing
	double time1 = GetTimeInSeconds();
	int numLocations = m_imageProcessor.process(m_image1,
												m_image2,
												NULL,
												m_laserLocations,
												m_maxNumLocations,
												firstRowLaserCol,
												percentPixelsOverThreshold,
												NULL);

	timingStats->imageProcessingTime += GetTimeInSeconds() - time1;

	// If we had major problems with this frame, try it again
	std::cout << "percentPixelsOverThreshold: " << percentPixelsOverThreshold << std::endl;
	if (percentPixelsOverThreshold > m_maxPercentPixelsOverThreshold)
	{
		std::cout << "!! Many bad laser locations suspected, pctOverThreshold=" << percentPixelsOverThreshold
				  << ", maxPct=" << m_maxPercentPixelsOverThreshold << ", rescanning..." << std::endl;

		timingStats->numFrameRetries++;

		return false;
	}

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
			record.frame = frame;
			record.laserSide = (int) laserSide;
			results.push_back(record);
		}
		timingStats->fileWritingTime += GetTimeInSeconds() - time1;
	}

	return true;
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
	if (stats.numFrames == 0)
	{
		return;
	}

	double now = GetTimeInSeconds();
	double totalTime = now - stats.startTime;

	double accountedTime = stats.meshBuildTime + stats.imageAcquisitionTime + stats.imageProcessingTime + stats.pointMappingTime
			+ stats.pointProcessingTime + stats.rotationTime + stats.fileWritingTime + stats.laserTime + stats.laserMergeTime;

	double unaccountedTime = totalTime - accountedTime;
	double rate = totalTime / stats.numFrames;

	std::cout << "Total Seconds per frame:\t" << rate << std::endl;
	std::cout << "Unaccounted time:\t" << (100.0 * unaccountedTime / totalTime) << "%" << std::endl;
	std::cout << "Image Acquisition:\t" << (100.0 * stats.imageAcquisitionTime / totalTime) << "%, " << (stats.imageAcquisitionTime / stats.numFrames) << " seconds per frame." << std::endl;
	std::cout << "Image Processing:\t" << (100.0 * stats.imageProcessingTime / totalTime) << "%, " << (stats.imageProcessingTime / stats.numFrames) << " seconds per frame." << std::endl;
	std::cout << "Laser Time:\t" << (100.0 * stats.laserTime / totalTime) << "%, " << (stats.laserTime / stats.numFrames) << " seconds per frame." << std::endl;
	std::cout << "Point Mapping:\t" << (100.0 * stats.pointMappingTime / totalTime) << "%" << std::endl;
	std::cout << "Point Rotating:\t" << (100.0 * stats.pointProcessingTime / totalTime) << "%" << std::endl;
	std::cout << "Table Rotation:\t" << (100.0 * stats.rotationTime / totalTime) << "%" << std::endl;
	std::cout << "Laser Merging:\t" << (100.0 * stats.laserMergeTime / totalTime) << "%" << std::endl;
	std::cout << "File Writing:\t" << (100.0 * stats.fileWritingTime / totalTime) << "%" << std::endl;
	std::cout << "Mesh Construction:\t" << (100.0 * stats.meshBuildTime / totalTime) << "%" << std::endl;
	std::cout << "Num Frame Retries:\t" << stats.numFrameRetries << std::endl;
	std::cout << "Num Frames:\t" << stats.numFrames << std::endl;
	std::cout << "Total Time (min):\t" << (totalTime / 60.0) << std::endl << std::endl;
}


std::vector<ScanResult> Scanner::getPastScanResults()
{
	DIR * dirp = opendir(SCAN_OUTPUT_DIR.c_str());
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

			std::string fullPath = std::string(SCAN_OUTPUT_DIR) + "/" + name;
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

