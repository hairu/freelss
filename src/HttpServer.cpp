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
#include "HttpServer.h"
#include "WebContent.h"
#include "TurnTable.h"
#include "PresetManager.h"
#include "Setup.h"
#include "Laser.h"
#include "Scanner.h"
#include "Camera.h"
#include "Calibrator.h"
#include "UpdateManager.h"
#include "Progress.h"
#include "MemWriter.h"
#include <three.min.js.h>
#include <OrbitControls.js.h>
#include <PLYLoader.js.h>
#include <licenses.txt.h>

#define POSTBUFFERSIZE 2048
#define MAX_PIN 7

namespace freelss
{

struct RequestInfo
{
	enum HttpMethod { UNKNOWN, GET, POST };

	RequestInfo()
	{
		postProcessor = NULL;
		server = NULL;
		connection = NULL;
		uploadData = NULL;
		url = "";
		uploadDataSize = 0;
		method = UNKNOWN;
	}

	std::string url;
	MHD_PostProcessor * postProcessor;
	HttpServer * server;
	MHD_Connection * connection;
	size_t uploadDataSize;
	const char * uploadData;
	std::map<std::string, std::string> arguments;
	HttpMethod method;
};

static int BuildError(struct MHD_Connection *connection, const std::string& message, int httpCode = MHD_HTTP_INTERNAL_SERVER_ERROR)
{
	std::string errorStr = "<html><body><h1><b>ERROR</b><br>" + message + "</h1></body></html>";

	struct MHD_Response * response = MHD_create_response_from_buffer (
		errorStr.length(), (void *) errorStr.c_str(), MHD_RESPMEM_MUST_COPY);
	MHD_queue_response (connection, httpCode, response);
	MHD_destroy_response (response);

	return MHD_YES;
}

static int StorePostToMap (void *cls, enum MHD_ValueKind kind,
	      const char *key, const char *filename, const char *content_type,
          const char *transfer_encoding, const char *data,
	      uint64_t off, size_t size)
{

	if (cls != NULL && size < POSTBUFFERSIZE)
	{
		RequestInfo * reqInfo = (RequestInfo *) cls;
		if (key != NULL && data != NULL)
		{
			char value[POSTBUFFERSIZE];
			for (size_t i = 0; i < size; i++)
			{
				value[i] = data[i];
			}
			value[size] = '\0';

			reqInfo->arguments[key] = value;
		}
	}


	return MHD_YES;
}

static int StoreToMap (void *cls, enum MHD_ValueKind kind,
                         const char *key, const char *value)
{
	if (cls != NULL)
	{
		RequestInfo * reqInfo = (RequestInfo *) cls;
		if (key != NULL && value != NULL)
		{
			reqInfo->arguments[key] = value;
		}
	}

	return MHD_YES;
}

/* UNCOMMENT WHEN NEEDED
static int PrintHeaders (void *cls, enum MHD_ValueKind kind,
                           const char *key, const char *value)
{
	if (key != NULL && value != NULL)
	{
		std::cout << key << ": " << value << std::endl;
	}

	return MHD_YES;
}
*/

static void RequestCompleted (void *cls, struct MHD_Connection *connection,
                                void **con_cls, enum MHD_RequestTerminationCode toe)
{
  RequestInfo * reqInfo = (RequestInfo *)* con_cls;

  if (reqInfo == NULL)
  {
	  return;
  }

  if (reqInfo->postProcessor != NULL)
  {
      MHD_destroy_post_processor (reqInfo->postProcessor);
  }

  delete reqInfo;
  *con_cls = NULL;
}

static void RemoveActivePreset()
{
	PresetManager * profMgr = PresetManager::get();
	profMgr->removeActivePreset();

	SaveProperties();
}

static void ActivatePreset(int presetId)
{
	PresetManager * profMgr = PresetManager::get();
	profMgr->setActivePreset(presetId);
	SaveProperties();
}

static void SavePreset(RequestInfo * reqInfo)
{
	PresetManager * profMgr = PresetManager::get();
	Preset * preset = &profMgr->getActivePreset();
	Setup * setup = Setup::get();

	// Get the original units in case we need to convert
	UnitOfLength srcUnits = setup->unitOfLength;

	// If the name does not exist, create a new preset
	std::string presetName = reqInfo->arguments[WebContent::PROFILE_NAME];
	if (!presetName.empty())
	{
		HtmlEncode(presetName);

		// The name is different so the user actually created a new preset
		if (presetName != preset->name)
		{
			int profId = profMgr->addPreset(Preset());
			profMgr->setActivePreset(profId);
			preset = &profMgr->getActivePreset();
		}

		preset->name = presetName;
	}

	std::string laserSelection = reqInfo->arguments[WebContent::LASER_SELECTION];
	if (!laserSelection.empty())
	{
		preset->laserSide = (Laser::LaserSide) ToInt(laserSelection.c_str());
	}

	std::string cameraType = reqInfo->arguments[WebContent::CAMERA_MODE];
	if (!cameraType.empty())
	{
		preset->cameraMode = (Camera::CameraMode) ToInt(cameraType);
	}

	std::string laserThreshold = reqInfo->arguments[WebContent::LASER_MAGNITUDE_THRESHOLD];
	if (!laserThreshold.empty())
	{
		preset->laserThreshold = ToReal(laserThreshold.c_str());
	}

	std::string laserDelay = reqInfo->arguments[WebContent::LASER_DELAY];
	if (!laserDelay.empty())
	{
		preset->laserDelay = ToInt(laserDelay.c_str());
	}

	std::string stabilityDelay = reqInfo->arguments[WebContent::STABILITY_DELAY];
	if (!stabilityDelay.empty())
	{
		preset->stabilityDelay = ToInt(stabilityDelay.c_str());
	}

	std::string maxLaserWidth = reqInfo->arguments[WebContent::MAX_LASER_WIDTH];
	if (!maxLaserWidth.empty())
	{
		preset->maxLaserWidth = ToInt(maxLaserWidth.c_str());
	}

	std::string minLaserWidth = reqInfo->arguments[WebContent::MIN_LASER_WIDTH];
	if (!minLaserWidth.empty())
	{
		preset->minLaserWidth = ToInt(minLaserWidth.c_str());
	}

	std::string framesPerRevolution = reqInfo->arguments[WebContent::FRAMES_PER_REVOLUTION];
	if (!framesPerRevolution.empty())
	{
		preset->framesPerRevolution = ToInt(framesPerRevolution.c_str());
	}

	preset->generatePly = !reqInfo->arguments[WebContent::GENERATE_PLY].empty();

	std::string plyDataFormat = reqInfo->arguments[WebContent::PLY_DATA_FORMAT];
	if (! plyDataFormat.empty())
	{
		preset->plyDataFormat = (PlyDataFormat) ToInt(plyDataFormat);
	}

	preset->generateStl = !reqInfo->arguments[WebContent::GENERATE_STL].empty();
	preset->generateXyz = !reqInfo->arguments[WebContent::GENERATE_XYZ].empty();

	if (reqInfo->arguments[WebContent::SEPARATE_LASERS_BY_COLOR].empty())
	{
		preset->laserMergeAction = Preset::LMA_PREFER_RIGHT_LASER;
	}
	else
	{
		preset->laserMergeAction = Preset::LMA_SEPARATE_BY_COLOR;
		preset->generatePly = true;
	}

	std::string groundPlaneHeight = reqInfo->arguments[WebContent::GROUND_PLANE_HEIGHT];
	if (!groundPlaneHeight.empty())
	{
		preset->groundPlaneHeight = ConvertUnitOfLength(ToReal(groundPlaneHeight), srcUnits, UL_MILLIMETERS);
	}

	/** Save the properties */
	SaveProperties();
}

static void SaveSetup(RequestInfo * reqInfo)
{
	Setup * setup = Setup::get();

	// Get the original units in case we need to convert
	UnitOfLength srcUnits = setup->unitOfLength;

	std::string unitOfLength = reqInfo->arguments[WebContent::UNIT_OF_LENGTH];
	if (!unitOfLength.empty())
	{
		setup->unitOfLength = (UnitOfLength) ToInt(unitOfLength);
	}

	std::string cameraY = reqInfo->arguments[WebContent::CAMERA_Y];
	if (!cameraY.empty())
	{
		setup->cameraLocation.y = ConvertUnitOfLength(ToReal(cameraY), srcUnits, UL_MILLIMETERS);
		setup->rightLaserLocation.y = ConvertUnitOfLength(ToReal(cameraY), srcUnits, UL_MILLIMETERS);
		setup->leftLaserLocation.y = ConvertUnitOfLength(ToReal(cameraY), srcUnits, UL_MILLIMETERS);
	}

	std::string cameraZ = reqInfo->arguments[WebContent::CAMERA_Z];
	if (!cameraZ.empty())
	{
		setup->cameraLocation.z = ConvertUnitOfLength(ToReal(cameraZ), srcUnits, UL_MILLIMETERS);
		setup->rightLaserLocation.z = ConvertUnitOfLength(ToReal(cameraZ), srcUnits, UL_MILLIMETERS);
		setup->leftLaserLocation.z = ConvertUnitOfLength(ToReal(cameraZ), srcUnits, UL_MILLIMETERS);
	}

	std::string rightLaserX = reqInfo->arguments[WebContent::RIGHT_LASER_X];
	if (!rightLaserX.empty())
	{
		setup->rightLaserLocation.x = ConvertUnitOfLength(ToReal(rightLaserX), srcUnits, UL_MILLIMETERS);
	}

	std::string leftLaserX = reqInfo->arguments[WebContent::LEFT_LASER_X];
	if (!leftLaserX.empty())
	{
		setup->leftLaserLocation.x = ConvertUnitOfLength(ToReal(leftLaserX), srcUnits, UL_MILLIMETERS);
	}

	std::string rightLaserPin = reqInfo->arguments[WebContent::RIGHT_LASER_PIN];
	if (!rightLaserPin.empty())
	{
		int pin = ToInt(rightLaserPin);
		if (pin > MAX_PIN)
		{
			throw Exception("Invalid Laser Pin Setting");
		}

		setup->rightLaserPin = pin;
	}

	std::string leftLaserPin = reqInfo->arguments[WebContent::LEFT_LASER_PIN];
	if (!leftLaserPin.empty())
	{
		int pin = ToInt(leftLaserPin);
		if (pin > MAX_PIN)
		{
			throw Exception("Invalid Laser Pin Setting");
		}

		setup->leftLaserPin = pin;
	}

	std::string laserOnValue = reqInfo->arguments[WebContent::LASER_ON_VALUE];
	if (!laserOnValue.empty())
	{
		setup->laserOnValue = ToInt(laserOnValue);
	}

	std::string stepsPerRev = reqInfo->arguments[WebContent::STEPS_PER_REVOLUTION];
	if (!stepsPerRev.empty())
	{
		setup->stepsPerRevolution = ToInt(stepsPerRev);
	}

	std::string enablePin = reqInfo->arguments[WebContent::ENABLE_PIN];
	if (!enablePin.empty())
	{
		int pin = ToInt(enablePin);
		if (pin > MAX_PIN)
		{
			throw Exception("Invalid Enable Pin Setting");
		}

		setup->motorEnablePin = pin;
	}

	std::string stepPin = reqInfo->arguments[WebContent::STEP_PIN];
	if (!stepPin.empty())
	{
		int pin = ToInt(stepPin);
		if (pin > MAX_PIN)
		{
			throw Exception("Invalid Step Pin Setting");
		}

		setup->motorStepPin = pin;
	}

	std::string stepDelay = reqInfo->arguments[WebContent::STEP_DELAY];
	if (!stepDelay.empty())
	{
		setup->motorStepDelay = ToInt(stepDelay);
	}

	std::string dirPin = reqInfo->arguments[WebContent::DIRECTION_PIN];
	if (!dirPin.empty())
	{
		int pin = ToInt(dirPin);
		if (pin > MAX_PIN)
		{
			throw Exception("Invalid Direction Pin Setting");
		}

		setup->motorDirPin = pin;
	}

	std::string dirPinValue = reqInfo->arguments[WebContent::DIRECTION_VALUE];
	if (!dirPinValue.empty())
	{
		setup->motorDirPinValue = ToInt(dirPinValue);
	}

	std::string responseDelay = reqInfo->arguments[WebContent::RESPONSE_DELAY];
	if (!responseDelay.empty())
	{
		setup->motorResponseDelay = ToInt(responseDelay);
	}

	std::string serialNumber = reqInfo->arguments[WebContent::SERIAL_NUMBER];
	if (!serialNumber.empty())
	{
		setup->serialNumber = serialNumber;
	}

	// Save the properties
	SaveProperties();
}

static int StartRequest(RequestInfo * reqInfo)
{
	if (reqInfo->method == RequestInfo::POST)
	{
		reqInfo->postProcessor = MHD_create_post_processor (reqInfo->connection, POSTBUFFERSIZE, &StorePostToMap, (void *) reqInfo);
		if (NULL == reqInfo->postProcessor)
		{
			  delete reqInfo;
			  return MHD_NO;
		}
	}
	return MHD_YES;
}

static int ShowScanProgress(RequestInfo * reqInfo)
{
	// Show the scan progress page
	HttpServer * server = reqInfo->server;
	Scanner * scanner = server->getScanner();
	Progress& progress = scanner->getProgress();
	real remainingTime = scanner->getRemainingTime();

	std::string page = WebContent::scanRunning(progress, remainingTime);
	MHD_Response *response = MHD_create_response_from_buffer (page.size(), (void *) page.c_str(), MHD_RESPMEM_MUST_COPY);
	MHD_add_response_header (response, "Content-Type", "text/html");
	int ret = MHD_queue_response (reqInfo->connection, MHD_HTTP_OK, response);
	MHD_destroy_response (response);

	return ret;
}

static std::string ToggleLaser(RequestInfo * reqInfo, Laser::LaserSide laserSide)
{
	Laser * laser = Laser::getInstance();

	std::string message;

	if (laser->isOn(laserSide))
	{
		laser->turnOff(laserSide);
		message = "Turned off " + Laser::toString(laserSide);
	}
	else
	{
		laser->turnOn(laserSide);
		message = "Turned on "  + Laser::toString(laserSide);
	}

	std::cout << message << std::endl;

	return message;
}

static bool LineIntersect(PixelLocation * out, const PixelLocation& p1, const PixelLocation& p2, const PixelLocation& p3, const PixelLocation& p4)
{
	real d = (p1.x - p2.x) * (p3.y - p4.y) - (p1.y - p2.y) * (p3.x - p4.x);

	if (ABS(d) < 0.0001)
	{
		return false;
	}

	out->x = ((p3.x - p4.x) * (p1.x * p2.y - p1.y * p2.x) - (p1.x - p2.x) * (p3.x * p4.y - p3.y * p4.x)) / d;
	out->y = ((p3.y - p4.y) * (p1.x * p2.y - p1.y * p2.x) - (p1.y - p2.y) * (p3.x * p4.y - p3.y * p4.x)) / d;

	return true;
}

static int RetrieveFile(RequestInfo * reqInfo, const std::string& url)
{
	std::string filename;

	if (url.find("/dl/") == 0)
	{
		filename = url.substr(4);
	}
	else
	{
		filename = url.substr(5);
	}

	size_t dotPos = filename.find(".");
	std::string id = "";
	std::string ext = "";

	int ret = MHD_YES;
	if (dotPos != std::string::npos)
	{
		id = filename.substr(0, dotPos);
		ext = filename.substr(dotPos + 1);
	}

	if (ext == "ply" || ext == "jpg" || ext == "stl" || ext == "db" || ext == "png" || ext == "csv" || ext == "xyz")
	{
		std::string mimeType;

		// Get the mime type
		if (ext == "jpg")
		{
			mimeType = "image/jpeg";
		}
		else if (ext == "png")
		{
			mimeType = "image/png";
		}
		else if (ext == "csv")
		{
			mimeType = "text/csv";
		}
		else
		{
			mimeType = "application/octet-stream";
		}

		// Construct the filename
		std::string directory = (reqInfo->url.find("/dl/") == 0) ? SCAN_OUTPUT_DIR : DEBUG_OUTPUT_DIR;
		std::stringstream filename;

		filename << directory << "/" << atol(id.c_str()) << "." << ext;

		std::string contentDisposition = "attachment; filename=\"" + id + "." + ext + "\"";

		// Read the file
		FILE * fp = fopen(filename.str().c_str(), "rb");
		if (fp != NULL)
		{
			unsigned long fileSize = 0;

			fseek(fp, 0, SEEK_END);
			fileSize = ftell(fp);
			fseek(fp, 0, SEEK_SET);

			byte * fileData = (byte *) malloc(fileSize);
			if (fileData != NULL)
			{
				fread((void *)fileData, 1, fileSize, fp);
				fclose(fp);

				MHD_Response *response = MHD_create_response_from_buffer (fileSize, (void *) fileData, MHD_RESPMEM_MUST_FREE);
				MHD_add_response_header (response, "Content-Type", mimeType.c_str());
				MHD_add_response_header (response, "Content-Disposition", contentDisposition.c_str());
				MHD_add_response_header (response, "Cache-Control", "no-cache, no-store, must-revalidate");
				MHD_add_response_header (response, "Pragma", "no-cache");
				MHD_add_response_header (response, "Expires", "0");
				MHD_add_response_header (response, "Pragma-directive", "no-cache");
				MHD_add_response_header (response, "Cache-directive", "no-cache");
				ret = MHD_queue_response (reqInfo->connection, MHD_HTTP_OK, response);
				MHD_destroy_response (response);
			}
			else
			{
				fclose(fp);
				std::string message = "Out of memory";
				ret = BuildError(reqInfo->connection, message, MHD_HTTP_INTERNAL_SERVER_ERROR);
			}
		}
		else
		{
			std::string message = "Error opening file to read for download: " + filename.str();
			ret = BuildError(reqInfo->connection, message, MHD_HTTP_BAD_REQUEST);
		}
	}
	else
	{
		std::string message = "Invalid File Extension Requested";
		ret = BuildError(reqInfo->connection, message, MHD_HTTP_BAD_REQUEST);
	}

	return ret;
}

static int LivePly(RequestInfo * reqInfo)
{
	HttpServer * server = reqInfo->server;
	Scanner * scanner = server->getScanner();
	int ret = MHD_YES;
	size_t fileSize = 0;
	byte * fileData = NULL;

	Scanner::LiveData liveData = scanner->getLiveDataLock();
	try
	{
		size_t numPoints = liveData.leftLaserResults->size() + liveData.rightLaserResults->size();

		PlyWriter plyWriter;
		plyWriter.setTotalNumPoints((int)numPoints);
		plyWriter.setDataFormat(PLY_BINARY);

		MemWriter memWriter;
		plyWriter.begin(&memWriter);

		for (size_t iRec = 0; iRec < liveData.leftLaserResults->size(); iRec++)
		{
			plyWriter.writePoints(&liveData.leftLaserResults->at(iRec).point, 1);
		}

		for (size_t iRec = 0; iRec < liveData.rightLaserResults->size(); iRec++)
		{
			plyWriter.writePoints(&liveData.rightLaserResults->at(iRec).point, 1);
		}

		plyWriter.end();

		fileSize = memWriter.getData().size();
		fileData = (byte *) malloc(fileSize);
		if (fileData != NULL)
		{
			memcpy(fileData, &memWriter.getData().front(), fileSize);
		}
	}
	catch (...)
	{
		scanner->releaseLiveDataLock();
		throw;
	}

	scanner->releaseLiveDataLock();

	if (fileData != NULL)
	{
		MHD_Response *response = MHD_create_response_from_buffer (fileSize, (void *) fileData, MHD_RESPMEM_MUST_FREE);
		MHD_add_response_header (response, "Content-Type", "application/octet-stream");
		MHD_add_response_header (response, "Content-Disposition", "attachment; filename=\"live.ply\"");
		MHD_add_response_header (response, "Cache-Control", "no-cache, no-store, must-revalidate");
		MHD_add_response_header (response, "Pragma", "no-cache");
		MHD_add_response_header (response, "Expires", "0");
		MHD_add_response_header (response, "Pragma-directive", "no-cache");
		MHD_add_response_header (response, "Cache-directive", "no-cache");
		ret = MHD_queue_response (reqInfo->connection, MHD_HTTP_OK, response);
		MHD_destroy_response (response);
	}
	else
	{
		std::string message = "Out of memory";
		ret = BuildError(reqInfo->connection, message, MHD_HTTP_INTERNAL_SERVER_ERROR);
	}

	return ret;
}

static std::string AutoCalibrate(RequestInfo * reqInfo)
{
	Laser * laser = Laser::getInstance();
	Camera * camera = Camera::getInstance();
	Setup * setup = Setup::get();
	real leftLaserX, rightLaserX;
	std::string message = "";

	PixelLocation leftTop, leftBottom;
	PixelLocation rightTop, rightBottom;

	//
	// Detect the camera's Z value
	//
	real junk;
	if (!Calibrator::detectLaserX(&junk, leftTop, leftBottom, laser, Laser::LEFT_LASER))
	{
		message += "Error detecting LEFT laser line!!! ";
		return message;
	}


	if (!Calibrator::detectLaserX(&junk, rightTop, rightBottom, laser, Laser::RIGHT_LASER))
	{
		message += "Error detecting RIGHT laser location!!! ";
		return message;
	}

	// Find the center of the turn table by computing where the laser crosses the center line
	PixelLocation leftTableCenter, rightTableCenter;
	PixelLocation centerTop;
	centerTop.x = camera->getImageWidth() / 2;
	centerTop.y = 0;

	PixelLocation centerBottom;
	centerBottom.x = centerTop.x;
	centerBottom.y = camera->getImageHeight();

	if (LineIntersect(&leftTableCenter, leftTop, leftBottom, centerTop, centerBottom) &&
		LineIntersect(&rightTableCenter, rightTop, rightBottom, centerTop, centerBottom))
	{
		real tableCenter = (leftTableCenter.y + rightTableCenter.y) / 2.0f;
		real cameraZ = Calibrator::computeCameraZ(tableCenter);


		std::cout << "Detected center of turn table at row " << tableCenter << " from left row "
				  << leftTableCenter.y << " and right row " << rightTableCenter.y
				  << ". Computed camera Z as " << cameraZ / 25.4 << " in. " << std::endl;

		message += "Detected camera distance to table center. ";
		setup->cameraLocation.z = cameraZ;
		setup->leftLaserLocation.z = cameraZ;
		setup->rightLaserLocation.z = cameraZ;
	}
	else
	{
		message += "Error detecting the center of the turn table!!! ";
	}

	if (Calibrator::detectLaserX(&leftLaserX, leftTop, leftBottom, laser, Laser::LEFT_LASER))
	{
		setup->leftLaserLocation.x = leftLaserX;
		message += "Detected LEFT laser location. ";
	}
	else
	{
		message += "Error detecting LEFT laser location!!! ";
	}


	if (Calibrator::detectLaserX(&rightLaserX, rightTop, rightBottom, laser, Laser::RIGHT_LASER))
	{
		setup->rightLaserLocation.x = rightLaserX;
		message += "Detected RIGHT laser location. ";
	}
	else
	{
		message += "Error detecting RIGHT laser location!!! ";
	}

	// Write the properties to disk
	SaveProperties();

	return message;
}

static int ProcessPageRequest(RequestInfo * reqInfo)
{
	HttpServer * server = reqInfo->server;
	Scanner * scanner = server->getScanner();
	Camera * camera = Camera::getInstance();
	int ret = MHD_YES;

	MHD_get_connection_values (reqInfo->connection, MHD_GET_ARGUMENT_KIND, &StoreToMap, (void *) reqInfo);

	if (scanner->isRunning())
	{
		std::string cmd = reqInfo->arguments["cmd"];
		if (reqInfo->url == "/" && cmd == "stopScan")
		{
			std::cout << "Stopping scan..." << std::endl;
			scanner->stop();

			// Wait for the scanner to stop
			scanner->join();

			std::cout << "Scan stopped" << std::endl;
		}
	}

	if (!scanner->isRunning())
	{
		if (reqInfo->url.find("/camImage") == 0)
		{
			// Get the image from the camera
			unsigned imageSize = 0;
			byte * imageData = NULL;

			try
			{
				while (camera->acquireJpeg(imageData, &imageSize) == false)
				{
					free(imageData);
					imageData = (byte *) malloc(imageSize);
					if (imageData == NULL)
					{
						throw Exception("Out of memory");
					}
				}
			}
			catch (...)
			{
				free(imageData);
				throw;
			}

			MHD_Response *response = MHD_create_response_from_buffer (imageSize, (void *) imageData, MHD_RESPMEM_MUST_FREE);
			MHD_add_response_header (response, "Content-Type", "image/jpeg");
			MHD_add_response_header (response, "Cache-Control", "no-cache, no-store, must-revalidate");
			MHD_add_response_header (response, "Pragma", "no-cache");
			MHD_add_response_header (response, "Expires", "0");
			MHD_add_response_header (response, "Pragma-directive", "no-cache");
			MHD_add_response_header (response, "Cache-directive", "no-cache");

			ret = MHD_queue_response (reqInfo->connection, MHD_HTTP_OK, response);
			MHD_destroy_response (response);
		}
		else if (reqInfo->url.find("/settings") == 0)
		{
			// Save the settings if this was a post
			std::string cmd = reqInfo->arguments["cmd"];
			std::string message = "";

			if (reqInfo->method == RequestInfo::POST && cmd == "Save")
			{
				SavePreset(reqInfo);
				reqInfo->server->reinitialize();
				message = "Preset saved.";
			}
			else if (reqInfo->method == RequestInfo::POST && cmd == "Delete")
			{
				RemoveActivePreset();
				reqInfo->server->reinitialize();
				message = "Preset Deleted.";
			}
			else if (reqInfo->method == RequestInfo::GET && cmd == "activate")
			{
				std::string presetId = reqInfo->arguments["id"];
				if (!presetId.empty())
				{
					ActivatePreset(ToInt(presetId));
					reqInfo->server->reinitialize();
					message = "Preset Actived.";
				}
			}
			else if (reqInfo->method == RequestInfo::POST && cmd == "shutdown")
			{
				// Turn on both lasers
				Laser::getInstance()->turnOn(Laser::ALL_LASERS);

				system("shutdown -h now&");
				message = "Shutting down....";
			}

			if (!message.empty())
			{
				std::cout << message << std::endl;
			}

			std::string page = WebContent::settings(message);
			MHD_Response *response = MHD_create_response_from_buffer (page.size(), (void *) page.c_str(), MHD_RESPMEM_MUST_COPY);
			MHD_add_response_header (response, "Content-Type", "text/html");
			ret = MHD_queue_response (reqInfo->connection, MHD_HTTP_OK, response);
			MHD_destroy_response (response);

			std::cout << "Done." << std::endl;
		}
		else if (reqInfo->url.find("/setup") == 0)
		{
			// Save the settings if this was a post
			std::string cmd = reqInfo->arguments["cmd"];
			std::string message = "";

			if (reqInfo->method == RequestInfo::POST && cmd == "save")
			{
				SaveSetup(reqInfo);
				reqInfo->server->reinitialize();
				message = "Hardware setup saved.";
			}

			std::string page = WebContent::setup(message);
			MHD_Response *response = MHD_create_response_from_buffer (page.size(), (void *) page.c_str(), MHD_RESPMEM_MUST_COPY);
			MHD_add_response_header (response, "Content-Type", "text/html");
			ret = MHD_queue_response (reqInfo->connection, MHD_HTTP_OK, response);
			MHD_destroy_response (response);
		}
		else if (reqInfo->url.find("/cal1") == 0)
		{
			std::string cmd = reqInfo->arguments["cmd"];
			std::string message = "";
			bool responded = false;
			std::string rotationDegrees = "360";

			// Toggle the right laser
			if (reqInfo->method == RequestInfo::POST && cmd == "toggleRightLaser")
			{
				message = ToggleLaser(reqInfo, Laser::RIGHT_LASER);
			}
			// Toggle the left laser
			else if (reqInfo->method == RequestInfo::POST && cmd == "toggleLeftLaser")
			{
				message = ToggleLaser(reqInfo, Laser::LEFT_LASER);
			}
			// Detect the x locations for the lasers
			else if (reqInfo->method == RequestInfo::POST && cmd == "autoCalibrate")
			{
				message = AutoCalibrate(reqInfo);
			}
			// Rotate the table
			else if (reqInfo->method == RequestInfo::POST && cmd == "rotateTable")
			{
				std::string degreesStr = reqInfo->arguments["degrees"];
				if (!degreesStr.empty())
				{
					rotationDegrees = degreesStr;
					real32 degrees = ToReal(degreesStr.c_str());
					TurnTable * table = TurnTable::getInstance();
					table->setMotorEnabled(true);
					int numSteps = table->rotate(DEGREES_TO_RADIANS(degrees));
					std::cout << "Took " << numSteps << " steps." << std::endl;
				}
			}
			else if (reqInfo->method == RequestInfo::POST && cmd == "disableMotor")
			{
				TurnTable * table = TurnTable::getInstance();
				table->setMotorEnabled(false);
				message = "Motor disabled.";
			}
			else if (reqInfo->method == RequestInfo::POST && cmd == "generateDebug")
			{
				std::cout << "Generating debug info..." << std::endl;
				scanner->setTask(Scanner::GENERATE_DEBUG);

				Preset& preset = PresetManager::get()->getActivePreset();

				scanner->generateDebugInfo(preset.laserSide);
				ret = RetrieveFile(reqInfo, "/dbg/5.png");

				responded = true;
			}

			if (!responded)
			{
				std::string page = WebContent::cal1(message);
				MHD_Response *response = MHD_create_response_from_buffer (page.size(), (void *) page.c_str(), MHD_RESPMEM_MUST_COPY);
				MHD_add_response_header (response, "Content-Type", "text/html");
				ret = MHD_queue_response (reqInfo->connection, MHD_HTTP_OK, response);
				MHD_destroy_response (response);
			}
		}
		else if (reqInfo->url.find("/dl/") == 0 || reqInfo->url.find("/dbg/") == 0)
		{
			ret = RetrieveFile(reqInfo, reqInfo->url);
		}
		else if (reqInfo->url == "/" || reqInfo->url == "/preview") // We check preview here in case the scan just ended
		{
			std::string cmd = reqInfo->arguments["cmd"];

			if (reqInfo->method == RequestInfo::POST && cmd == "startScan")
			{
				// Start the scan
				std::string degrees = reqInfo->arguments["degrees"];

				if (degrees.empty())
				{
					throw Exception("Missing required input parameters");
				}

				scanner->setRange(ToReal(degrees.c_str()));
				scanner->setTask(Scanner::GENERATE_SCAN);
				scanner->execute();
				std::cout << "Starting scan..." << std::endl;

				// Give the scanner time to start
				Thread::usleep(500000);
			}
			else
			{
				std::string page = WebContent::scan(scanner->getPastScanResults());
				MHD_Response *response = MHD_create_response_from_buffer (page.size(), (void *) page.c_str(), MHD_RESPMEM_MUST_COPY);
				MHD_add_response_header (response, "Content-Type", "text/html");
				ret = MHD_queue_response (reqInfo->connection, MHD_HTTP_OK, response);
				MHD_destroy_response (response);
			}
		}
		else if (reqInfo->url == "/del")
		{
			std::string id = reqInfo->arguments["id"];

			if (reqInfo->method == RequestInfo::POST)
			{
				std::stringstream cmd;
				cmd << "rm -f " << SCAN_OUTPUT_DIR << "/" << ToInt(id.c_str()) << ".*";

				// Execute the command
				if (system(cmd.str().c_str()) != -1)
				{
					std::string page = WebContent::scan(scanner->getPastScanResults());
					MHD_Response *response = MHD_create_response_from_buffer (page.size(), (void *) page.c_str(), MHD_RESPMEM_MUST_COPY);
					MHD_add_response_header (response, "Content-Type", "text/html");
					ret = MHD_queue_response (reqInfo->connection, MHD_HTTP_OK, response);
					MHD_destroy_response (response);
				}
				else
				{
					std::string message = std::string("Error deleting scan");
					ret = BuildError(reqInfo->connection, message, MHD_HTTP_INTERNAL_SERVER_ERROR);
				}
			}
		}
		else if (reqInfo->url == "/checkUpdate")
		{
			std::string message = "";
			SoftwareUpdate * update = NULL;
			UpdateManager * updateMgr = UpdateManager::get();
			try
			{
				updateMgr->checkForUpdates();
				update = updateMgr->getLatestUpdate();
			}
			catch (Exception& ex)
			{
				message = ex;
			}

			std::string page = WebContent::showUpdate(update, message);
			MHD_Response *response = MHD_create_response_from_buffer (page.size(), (void *) page.c_str(), MHD_RESPMEM_MUST_COPY);
			MHD_add_response_header (response, "Content-Type", "text/html");
			ret = MHD_queue_response (reqInfo->connection, MHD_HTTP_OK, response);
			MHD_destroy_response (response);
		}
		else if (reqInfo->url == "/applyUpdate" && reqInfo->method == RequestInfo::POST)
		{
			int majorVersion = ToInt(reqInfo->arguments["majorVersion"]);
			int minorVersion = ToInt(reqInfo->arguments["minorVersion"]);
			std::string message;
			bool success = true;

			UpdateManager * updateMgr = UpdateManager::get();
			SoftwareUpdate * update = updateMgr->getLatestUpdate();
			if (update == NULL)
			{
				message = "Error retrieving update";
				success = false;
			}
			else if (majorVersion != update->majorVersion || minorVersion != update->minorVersion)
			{
				message = "Mismatch error with update version";
				success = false;
			}
			else
			{
				try
				{
					updateMgr->applyUpdate(update);
					message = "Update Successful.  Restarting...";
					success = true;
				}
				catch (Exception& ex)
				{
					message = ex;
					success = false;
				}
			}

			std::string page = WebContent::updateApplied(update, message, success);
			MHD_Response *response = MHD_create_response_from_buffer (page.size(), (void *) page.c_str(), MHD_RESPMEM_MUST_COPY);
			MHD_add_response_header (response, "Content-Type", "text/html");
			ret = MHD_queue_response (reqInfo->connection, MHD_HTTP_OK, response);
			MHD_destroy_response (response);
		}
		else if (reqInfo->url == "/reboot" && reqInfo->method == RequestInfo::POST)
		{
			system("reboot&");
			std::string page = "Rebooting...";
			MHD_Response *response = MHD_create_response_from_buffer (page.size(), (void *) page.c_str(), MHD_RESPMEM_MUST_COPY);
			MHD_add_response_header (response, "Content-Type", "text/html");
			ret = MHD_queue_response (reqInfo->connection, MHD_HTTP_OK, response);
			MHD_destroy_response (response);
		}
		else if (reqInfo->url == "/licenses.txt")
		{
			MHD_Response *response = MHD_create_response_from_buffer (licenses_txt_len, (void *) licenses_txt, MHD_RESPMEM_MUST_COPY);
			MHD_add_response_header (response, "Content-Type", "text/plain");
			ret = MHD_queue_response (reqInfo->connection, MHD_HTTP_OK, response);
			MHD_destroy_response (response);
		}
		else if (reqInfo->url == "/three.min.js")
		{
			MHD_Response *response = MHD_create_response_from_buffer (three_min_js_len, (void *) three_min_js, MHD_RESPMEM_MUST_COPY);
			MHD_add_response_header (response, "Content-Type", "application/javascript");
			ret = MHD_queue_response (reqInfo->connection, MHD_HTTP_OK, response);
			MHD_destroy_response (response);
		}
		else if (reqInfo->url == "/PLYLoader.js")
		{
			MHD_Response *response = MHD_create_response_from_buffer (PLYLoader_js_len, (void *) PLYLoader_js, MHD_RESPMEM_MUST_COPY);
			MHD_add_response_header (response, "Content-Type", "application/javascript");
			ret = MHD_queue_response (reqInfo->connection, MHD_HTTP_OK, response);
			MHD_destroy_response (response);
		}
		else if (reqInfo->url == "/OrbitControls.js")
		{
			MHD_Response *response = MHD_create_response_from_buffer (OrbitControls_js_len, (void *) OrbitControls_js, MHD_RESPMEM_MUST_COPY);
			MHD_add_response_header (response, "Content-Type", "application/javascript");
			ret = MHD_queue_response (reqInfo->connection, MHD_HTTP_OK, response);
			MHD_destroy_response (response);
		}
		else if (reqInfo->url == "/view")
		{
			int id = ToInt(reqInfo->arguments["id"]);
			std::stringstream filename;
			filename << "/dl/" << id << ".ply";

			std::string page = WebContent::viewScan(filename.str());
			MHD_Response *response = MHD_create_response_from_buffer (page.size(), (void *) page.c_str(), MHD_RESPMEM_MUST_COPY);
			MHD_add_response_header (response, "Content-Type", "text/html");
			ret = MHD_queue_response (reqInfo->connection, MHD_HTTP_OK, response);
			MHD_destroy_response (response);
		}
		else
		{
			std::string message = std::string("Not Found: ") + reqInfo->url;
			ret = BuildError(reqInfo->connection, message, MHD_HTTP_NOT_FOUND);
		}
	}

	// Show the scan progress
	if (scanner->isRunning())
	{
		if (reqInfo->url == "/livePly")
		{
			ret = LivePly(reqInfo);
		}
		else if (reqInfo->url == "/preview")
		{
			std::string page = WebContent::viewScan("/livePly");
			MHD_Response *response = MHD_create_response_from_buffer (page.size(), (void *) page.c_str(), MHD_RESPMEM_MUST_COPY);
			MHD_add_response_header (response, "Content-Type", "text/html");
			ret = MHD_queue_response (reqInfo->connection, MHD_HTTP_OK, response);
			MHD_destroy_response (response);
		}
		else if (reqInfo->url == "/three.min.js")
		{
			MHD_Response *response = MHD_create_response_from_buffer (three_min_js_len, (void *) three_min_js, MHD_RESPMEM_MUST_COPY);
			MHD_add_response_header (response, "Content-Type", "application/javascript");
			ret = MHD_queue_response (reqInfo->connection, MHD_HTTP_OK, response);
			MHD_destroy_response (response);
		}
		else if (reqInfo->url == "/PLYLoader.js")
		{
			MHD_Response *response = MHD_create_response_from_buffer (PLYLoader_js_len, (void *) PLYLoader_js, MHD_RESPMEM_MUST_COPY);
			MHD_add_response_header (response, "Content-Type", "application/javascript");
			ret = MHD_queue_response (reqInfo->connection, MHD_HTTP_OK, response);
			MHD_destroy_response (response);
		}
		else if (reqInfo->url == "/OrbitControls.js")
		{
			MHD_Response *response = MHD_create_response_from_buffer (OrbitControls_js_len, (void *) OrbitControls_js, MHD_RESPMEM_MUST_COPY);
			MHD_add_response_header (response, "Content-Type", "application/javascript");
			ret = MHD_queue_response (reqInfo->connection, MHD_HTTP_OK, response);
			MHD_destroy_response (response);
		}
		else
		{
			ret = ShowScanProgress(reqInfo);
		}
	}
	return ret;
}

static int ContinueRequest(RequestInfo * reqInfo)
{
	int ret = MHD_YES;

	if (reqInfo->method == RequestInfo::POST && reqInfo->uploadDataSize != 0)
	{
		MHD_post_process (reqInfo->postProcessor, reqInfo->uploadData, reqInfo->uploadDataSize);
		reqInfo->uploadDataSize = 0;
	}
	else if (reqInfo->method == RequestInfo::GET || reqInfo->uploadDataSize == 0)
	{
		// Process the request
		ret = ProcessPageRequest(reqInfo);
	}
	else
	{
		ret = BuildError(reqInfo->connection, "Bad Request", MHD_HTTP_BAD_REQUEST);
	}

	return ret;
}

static int ConnectionHandler (void *cls, struct MHD_Connection *connection,
                                const char *url,
                                const char *method, const char *version,
                                const char *upload_data,
                                size_t *upload_data_size, void **con_cls)
{
	int ret = MHD_YES;

	try
	{
		// A new request
		if (*con_cls == NULL)
		{
			RequestInfo * reqInfo = new RequestInfo();
			reqInfo->server = (HttpServer *) cls;
			reqInfo->connection = connection;
			reqInfo->url = url;

			if (strcmp(method, "GET") == 0)
			{
				reqInfo->method = RequestInfo::GET;
			}
			else if (strcmp(method, "POST") == 0)
			{
				reqInfo->method = RequestInfo::POST;
			}

			*con_cls = reqInfo;
			reqInfo->uploadData = upload_data;
			reqInfo->uploadDataSize = * upload_data_size;
			ret = StartRequest(reqInfo);
			* upload_data_size = reqInfo->uploadDataSize;
		}
		// A continued request
		else
		{
			RequestInfo * reqInfo = (RequestInfo *) *con_cls;
			reqInfo->uploadData = upload_data;
			reqInfo->uploadDataSize = * upload_data_size;
			ret = ContinueRequest(reqInfo);
			* upload_data_size = reqInfo->uploadDataSize;
		}
	}
	catch (Exception & ex)
	{
		ret = BuildError(connection, ex.c_str());
	}
	catch (...)
	{
		ret = BuildError(connection, "Unknown Error");
	}

	return ret;
}

HttpServer * HttpServer::m_instance = NULL;

HttpServer * HttpServer::get()
{
	if (m_instance == NULL)
	{
		m_instance = new HttpServer();
	}

	return m_instance;
}

void HttpServer::release()
{
	delete m_instance;
	m_instance = NULL;
}

HttpServer::HttpServer() :
	m_daemon(NULL),
	m_scanner(NULL)
{
	reinitialize();
}

HttpServer::~HttpServer()
{
	stop();
	delete m_scanner;
}

void HttpServer::start(int port)
{
	if (m_daemon != NULL)
	{
		stop();
	}

	m_daemon = MHD_start_daemon (MHD_USE_SELECT_INTERNALLY, port, NULL, NULL,
	                             &ConnectionHandler, this, MHD_OPTION_NOTIFY_COMPLETED, RequestCompleted, NULL,
	                             MHD_OPTION_THREAD_POOL_SIZE, 1, MHD_OPTION_END);
	if (m_daemon == NULL)
	{
		std::stringstream sstr;
		sstr << "Error starting server on port " << port;
		throw Exception(sstr.str());
	}
}

void HttpServer::stop()
{
	if (m_daemon != NULL)
	{
		MHD_stop_daemon (m_daemon);
		m_daemon = NULL;
	}
}

void HttpServer::reinitialize()
{
	TurnTable::release();
	TurnTable::getInstance();

	Laser::release();
	Laser::getInstance();

	Camera::reinitialize();

	delete m_scanner;
	m_scanner = new Scanner();

	// Create the output directories if they don't exist
	mkdir(SCAN_OUTPUT_DIR.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	mkdir(DEBUG_OUTPUT_DIR.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

	std::cout << "Reinitialized." << std::endl;
}

Scanner * HttpServer::getScanner()
{
	return  m_scanner;
}

}
