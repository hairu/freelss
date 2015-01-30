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
#include "HttpServer.h"
#include "WebContent.h"
#include "TurnTable.h"
#include "Settings.h"
#include "Laser.h"
#include "Scanner.h"
#include "Camera.h"
#include "Calibrator.h"

#define POSTBUFFERSIZE 2048
#define MAX_PIN 7

namespace scanner
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

static void SaveSettings(RequestInfo * reqInfo)
{
	Settings * settings = Settings::get();

	std::string laserSelection = reqInfo->arguments[Settings::LASER_SELECTION];
	if (!laserSelection.empty())
	{
		settings->writeInt(Settings::GENERAL_SETTINGS, Settings::LASER_SELECTION, atoi(laserSelection.c_str()));
	}

	std::string cameraY = reqInfo->arguments[Settings::CAMERA_Y];
	if (!cameraY.empty())
	{
		settings->writeReal(Settings::GENERAL_SETTINGS, Settings::CAMERA_Y, 25.4 * atof(cameraY.c_str()));
		settings->writeReal(Settings::GENERAL_SETTINGS, Settings::RIGHT_LASER_Y, 25.4 * atof(cameraY.c_str()));
		settings->writeReal(Settings::GENERAL_SETTINGS, Settings::LEFT_LASER_Y, 25.4 * atof(cameraY.c_str()));
	}

	std::string cameraZ = reqInfo->arguments[Settings::CAMERA_Z];
	if (!cameraZ.empty())
	{
		settings->writeReal(Settings::GENERAL_SETTINGS, Settings::CAMERA_Z, 25.4 * atof(cameraZ.c_str()));
		settings->writeReal(Settings::GENERAL_SETTINGS, Settings::RIGHT_LASER_Z, 25.4 * atof(cameraZ.c_str()));
		settings->writeReal(Settings::GENERAL_SETTINGS, Settings::LEFT_LASER_Z, 25.4 * atof(cameraZ.c_str()));
	}

	std::string cameraType = reqInfo->arguments[Settings::CAMERA_MODE];
	if (!cameraType.empty())
	{
		settings->writeInt(Settings::GENERAL_SETTINGS, Settings::CAMERA_MODE, atoi(cameraType.c_str()));
	}

	std::string rightLaserX = reqInfo->arguments[Settings::RIGHT_LASER_X];
	if (!rightLaserX.empty())
	{
		settings->writeReal(Settings::GENERAL_SETTINGS, Settings::RIGHT_LASER_X, 25.4 * atof(rightLaserX.c_str()));
	}

	std::string leftLaserX = reqInfo->arguments[Settings::LEFT_LASER_X];
	if (!leftLaserX.empty())
	{
		settings->writeReal(Settings::GENERAL_SETTINGS, Settings::LEFT_LASER_X, 25.4 * atof(leftLaserX.c_str()));
	}

	std::string laserThreshold = reqInfo->arguments[Settings::LASER_MAGNITUDE_THRESHOLD];
	if (!laserThreshold.empty())
	{
		settings->writeReal(Settings::GENERAL_SETTINGS, Settings::LASER_MAGNITUDE_THRESHOLD, atof(laserThreshold.c_str()));
	}

	std::string laserDelay = reqInfo->arguments[Settings::LASER_DELAY];
	if (!laserDelay.empty())
	{
		settings->writeInt(Settings::GENERAL_SETTINGS, Settings::LASER_DELAY, atoi(laserDelay.c_str()));
	}

	std::string rightLaserPin = reqInfo->arguments[Settings::RIGHT_LASER_PIN];
	if (!rightLaserPin.empty())
	{
		int pin = atoi(rightLaserPin.c_str());
		if (pin > MAX_PIN)
		{
			throw Exception("Invalid Laser Pin Setting");
		}

		settings->writeInt(Settings::GENERAL_SETTINGS, Settings::RIGHT_LASER_PIN, pin);
	}

	std::string leftLaserPin = reqInfo->arguments[Settings::LEFT_LASER_PIN];
	if (!leftLaserPin.empty())
	{
		int pin = atoi(leftLaserPin.c_str());
		if (pin > MAX_PIN)
		{
			throw Exception("Invalid Laser Pin Setting");
		}

		settings->writeInt(Settings::GENERAL_SETTINGS, Settings::LEFT_LASER_PIN, pin);
	}

	std::string laserOnValue = reqInfo->arguments[Settings::LASER_ON_VALUE];
	if (!laserOnValue.empty())
	{
		settings->writeInt(Settings::GENERAL_SETTINGS, Settings::LASER_ON_VALUE, atoi(laserOnValue.c_str()));
	}

	std::string stabilityDelay = reqInfo->arguments[Settings::STABILITY_DELAY];
	if (!stabilityDelay.empty())
	{
		settings->writeInt(Settings::GENERAL_SETTINGS, Settings::STABILITY_DELAY, atoi(stabilityDelay.c_str()));
	}

	std::string maxLaserWidth = reqInfo->arguments[Settings::MAX_LASER_WIDTH];
	if (!maxLaserWidth.empty())
	{
		settings->writeInt(Settings::GENERAL_SETTINGS, Settings::MAX_LASER_WIDTH, atoi(maxLaserWidth.c_str()));
	}

	std::string minLaserWidth = reqInfo->arguments[Settings::MIN_LASER_WIDTH];
	if (!minLaserWidth.empty())
	{
		settings->writeInt(Settings::GENERAL_SETTINGS, Settings::MIN_LASER_WIDTH, atoi(minLaserWidth.c_str()));
	}

	std::string stepsPerRev = reqInfo->arguments[Settings::STEPS_PER_REVOLUTION];
	if (!stepsPerRev.empty())
	{
		settings->writeInt(Settings::GENERAL_SETTINGS, Settings::STEPS_PER_REVOLUTION, atoi(stepsPerRev.c_str()));
	}

	std::string enablePin = reqInfo->arguments[Settings::ENABLE_PIN];
	if (!enablePin.empty())
	{
		int pin = atoi(enablePin.c_str());
		if (pin > MAX_PIN)
		{
			throw Exception("Invalid Enable Pin Setting");
		}

		settings->writeInt(Settings::A4988_SETTINGS, Settings::ENABLE_PIN, pin);
	}

	std::string stepPin = reqInfo->arguments[Settings::STEP_PIN];
	if (!stepPin.empty())
	{
		int pin = atoi(stepPin.c_str());
		if (pin > MAX_PIN)
		{
			throw Exception("Invalid Step Pin Setting");
		}

		settings->writeInt(Settings::A4988_SETTINGS, Settings::STEP_PIN, pin);
	}

	std::string stepDelay = reqInfo->arguments[Settings::STEP_DELAY];
	if (!stepDelay.empty())
	{
		settings->writeInt(Settings::A4988_SETTINGS, Settings::STEP_DELAY, atoi(stepDelay.c_str()));
	}

	std::string dirPin = reqInfo->arguments[Settings::DIRECTION_PIN];
	if (!dirPin.empty())
	{
		int pin = atoi(dirPin.c_str());
		if (pin > MAX_PIN)
		{
			throw Exception("Invalid Direction Pin Setting");
		}

		settings->writeInt(Settings::A4988_SETTINGS, Settings::DIRECTION_PIN, pin);
	}

	std::string responseDelay = reqInfo->arguments[Settings::RESPONSE_DELAY];
	if (!responseDelay.empty())
	{
		settings->writeInt(Settings::A4988_SETTINGS, Settings::RESPONSE_DELAY, atoi(responseDelay.c_str()));
	}

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
	real progress = scanner->getProgress() * 100.0;
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

static std::string AutoCalibrate(RequestInfo * reqInfo)
{
	Laser * laser = Laser::getInstance();
	Camera * camera = Camera::getInstance();
	Settings * settings = Settings::get();
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
		settings->writeReal(Settings::GENERAL_SETTINGS, Settings::CAMERA_Z, cameraZ);
		settings->writeReal(Settings::GENERAL_SETTINGS, Settings::LEFT_LASER_Z, cameraZ);
		settings->writeReal(Settings::GENERAL_SETTINGS, Settings::RIGHT_LASER_Z, cameraZ);
	}
	else
	{
		message += "Error detecting the center of the turn table!!! ";
	}

	if (Calibrator::detectLaserX(&leftLaserX, leftTop, leftBottom, laser, Laser::LEFT_LASER))
	{
		settings->writeReal(Settings::GENERAL_SETTINGS, Settings::LEFT_LASER_X, leftLaserX);
		message += "Detected LEFT laser location. ";
	}
	else
	{
		message += "Error detecting LEFT laser location!!! ";
	}


	if (Calibrator::detectLaserX(&rightLaserX, rightTop, rightBottom, laser, Laser::RIGHT_LASER))
	{
		settings->writeReal(Settings::GENERAL_SETTINGS, Settings::RIGHT_LASER_X, rightLaserX);
		message += "Detected RIGHT laser location. ";
	}
	else
	{
		message += "Error detecting RIGHT laser location!!! ";
	}

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
			std::string rotationDegrees = "360";
			if (reqInfo->method == RequestInfo::POST && cmd == "save")
			{
				SaveSettings(reqInfo);
				reqInfo->server->reinitialize();
				message = "Settings saved.";
			}
			else if (reqInfo->method == RequestInfo::POST && cmd == "shutdown")
			{
				system("shutdown -h now&");
				message = "Shutting down....";
			}
			// Toggle the right laser
			else if (reqInfo->method == RequestInfo::POST && cmd == "toggleRightLaser")
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
					real32 degrees = atof(degreesStr.c_str());
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

				Laser::LaserSide laserSide = (Laser::LaserSide) Settings::get()->readInt(Settings::GENERAL_SETTINGS, Settings::LASER_SELECTION);
				if (laserSide != Laser::LEFT_LASER && laserSide != Laser::RIGHT_LASER)
				{
					laserSide = Laser::RIGHT_LASER;
					std::cout <<"Defaulting laser side to right laser." << std::endl;
				}

				scanner->generateDebugInfo(laserSide);
				message = "Generated debug info.";
			}

			std::string page = WebContent::settings(message, rotationDegrees);
			MHD_Response *response = MHD_create_response_from_buffer (page.size(), (void *) page.c_str(), MHD_RESPMEM_MUST_COPY);
			MHD_add_response_header (response, "Content-Type", "text/html");
			ret = MHD_queue_response (reqInfo->connection, MHD_HTTP_OK, response);
			MHD_destroy_response (response);
		}
		else if (reqInfo->url.find("/cal1") == 0)
		{
			std::string page = WebContent::cal1();
			MHD_Response *response = MHD_create_response_from_buffer (page.size(), (void *) page.c_str(), MHD_RESPMEM_MUST_COPY);
			MHD_add_response_header (response, "Content-Type", "text/html");
			ret = MHD_queue_response (reqInfo->connection, MHD_HTTP_OK, response);
			MHD_destroy_response (response);
		}
		else if (reqInfo->url.find("/dl/") == 0 || reqInfo->url.find("/dbg/") == 0)
		{
			std::string filename;

			if (reqInfo->url.find("/dl/") == 0)
			{
				filename = reqInfo->url.substr(4);
			}
			else
			{
				filename = reqInfo->url.substr(5);
			}

			size_t dotPos = filename.find(".");
			std::string id = "";
			std::string ext = "";

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
				else if (ext == "ply")
				{
					mimeType = "model/ply";
				}
				else
				{
					mimeType = "application/octet-stream";
				}

				// Construct the filename
				const char * directory = (reqInfo->url.find("/dl/") == 0) ? Settings::SCAN_OUTPUT_DIR : Settings::DEBUG_OUTPUT_DIR;
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
		}
		else if (reqInfo->url == "/")
		{
			std::string cmd = reqInfo->arguments["cmd"];

			if (reqInfo->method == RequestInfo::POST && cmd == "startScan")
			{
				// Start the scan

				std::string detail = reqInfo->arguments["detail"];
				std::string degrees = reqInfo->arguments["degrees"];

				if (detail.empty() || degrees.empty())
				{
					throw Exception("Missing required input parameters");
				}

				scanner->setDetail(atoi(detail.c_str()));
				scanner->setRange(atof(degrees.c_str()));
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
				cmd << "rm -f " << Settings::SCAN_OUTPUT_DIR << "/" << atoi(id.c_str()) << ".*";

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
		else
		{
			std::string message = std::string("Not Found: ") + reqInfo->url;
			ret = BuildError(reqInfo->connection, message, MHD_HTTP_NOT_FOUND);
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
		HttpServer * server = reqInfo->server;
		Scanner * scanner = server->getScanner();

		// Process the request
		ProcessPageRequest(reqInfo);

		// Show the scan progress
		if (scanner->isRunning())
		{
			ShowScanProgress(reqInfo);
		}
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
	                             &ConnectionHandler, this, MHD_OPTION_NOTIFY_COMPLETED, RequestCompleted, NULL, MHD_OPTION_END);
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
	mkdir(Settings::SCAN_OUTPUT_DIR, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	mkdir(Settings::DEBUG_OUTPUT_DIR, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}

Scanner * HttpServer::getScanner()
{
	return  m_scanner;
}

}
