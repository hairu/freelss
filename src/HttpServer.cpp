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
#include "Lighting.h"
#include "WifiConfig.h"
#include "Logger.h"
#include "MountManager.h"
#include "BootConfigManager.h"
#include "PointCloudRenderer.h"
#include "PlyReader.h"
#include <three.min.js.h>
#include <OrbitControls.js.h>
#include <PLYLoader.js.h>
#include <showScan.js.h>
#include <licenses.txt.h>
#include <CanvasRenderer.js.h>
#include <SoftwareRenderer.js.h>
#include <Projector.js.h>

#define POSTBUFFERSIZE 2048
#define MAX_PIN 40

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
		InfoLog << key << ": " << value << std::endl;
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
	std::string presetName = UrlDecode(reqInfo->arguments[WebContent::PROFILE_NAME]);
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
		preset->cameraMode = (CameraMode) ToInt(cameraType);
	}

	std::string cameraExposureTime = reqInfo->arguments[WebContent::CAMERA_EXPOSURE_TIME];
	if (!cameraExposureTime.empty())
	{
		preset->cameraExposureTime = (CameraExposureTime) ToInt(cameraExposureTime);
	}

	std::string noiseRemovalSetting = reqInfo->arguments[WebContent::NOISE_REMOVAL_SETTING];
	if (!noiseRemovalSetting.empty())
	{
		preset->noiseRemovalSetting = (NoiseRemover::Setting) ToInt(noiseRemovalSetting);
	}

	std::string imageThresholdMode = reqInfo->arguments[WebContent::IMAGE_THRESHOLD_MODE];
	if (!imageThresholdMode.empty())
	{
		preset->imageThresholdMode = (ImageProcessor::ThresholdMode) ToInt(imageThresholdMode);
	}

	std::string laserThreshold = reqInfo->arguments[WebContent::LASER_MAGNITUDE_THRESHOLD];
	if (!laserThreshold.empty())
	{
		preset->laserThreshold = ToReal(laserThreshold.c_str());
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
	preset->enableBurstModeForStillImages = !reqInfo->arguments[WebContent::ENABLE_BURST_MODE].empty();
	preset->createBaseForObject = !reqInfo->arguments[WebContent::CREATE_BASE_FOR_OBJECT].empty();

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

static std::string MountNetworkPath(RequestInfo * reqInfo)
{
	std::string msg;

	try
	{
		std::string path = MountManager::get()->mount(reqInfo->arguments[WebContent::MOUNT_SERVERPATH],
													  reqInfo->arguments[WebContent::MOUNT_USERNAME],
													  reqInfo->arguments[WebContent::MOUNT_PASSWORD],
													  reqInfo->arguments[WebContent::MOUNT_WORKGROUP]);

		msg = "Mounted network share to " + path;
	}
	catch (...)
	{
		msg = "Error mounting network share.";
	}

	return msg;
}

static std::string UnmountPath(RequestInfo * reqInfo)
{
	std::string msg;

	std::string indexStr = TrimString(reqInfo->arguments[WebContent::MOUNT_INDEX]);
	if (indexStr.empty())
	{
		return "Missing Mount Index";
	}

	int index = ToInt(indexStr);

	std::vector<std::string> paths = MountManager::get()->getValidMounts();
	if (index >= (int)paths.size())
	{
		return "Invalid Mount Index";
	}

	try
	{
		std::string path = paths[index];

		if (umount2(path.c_str(), MNT_DETACH) == 0)
		{
			msg = "Unmounted " + path;
		}
		else
		{
			msg = "Error unmounting " + path;
		}
	}
	catch (...)
	{
		msg = "Error unmounting path.";
	}

	return msg;
}

static std::string ConnectWifi(RequestInfo * reqInfo)
{
	std::string password = reqInfo->arguments[WebContent::WIFI_PASSWORD];
	if (password.empty())
	{
		return "Password is required";
	}

	std::string essid = reqInfo->arguments[WebContent::WIFI_ESSID_HIDDEN];
	int index = -1;
	if (essid.empty())
	{
		std::string essidIndexStr = reqInfo->arguments[WebContent::WIFI_ESSID];
		if (essidIndexStr.empty())
		{
			return "Missing ESSID";
		}

		index = ToInt(essidIndexStr);
	}

	std::string message;
	WifiConfig * wifi = WifiConfig::get();

	try
	{
		if (essid.empty())
		{
			std::vector<WifiConfig::AccessPoint> accessPoints = wifi->getAccessPoints();
			if (index < 0 || index >= (int)accessPoints.size())
			{
				throw Exception("Invalid access point index");
			}
			essid = accessPoints[index].essid;
		}

		// Connect via WiFi
		wifi->connect(essid, password);
		message = "Connecting to " + essid + "...";
	}
	catch (Exception& ex)
	{
		message = ex;
	}
	catch (...)
	{
		throw;
	}

	return message;
}

static std::string SaveAuthenticationSettings(RequestInfo * reqInfo)
{
	Setup * setup = Setup::get();

	std::string message;

	std::string password1 = reqInfo->arguments[WebContent::AUTH_PASSWORD1];
	std::string password2 = reqInfo->arguments[WebContent::AUTH_PASSWORD2];
	std::string enablePassword = reqInfo->arguments[WebContent::ENABLE_AUTHENTICATION];

	if (!enablePassword.empty())
	{
		if (password1 == password2)
		{
			if (!password1.empty())
			{
				// Generate hash for the password
				unsigned char hash[SHA_DIGEST_LENGTH];
				memset(hash, 0, sizeof(hash));
				SHA1((const unsigned char *)password1.c_str(), password1.size(), hash);

				setup->passwordHash = ToHexString(hash, SHA_DIGEST_LENGTH);
				setup->enableAuthentication = true;
				message = "Enabled password requirement";
				SaveProperties();
			}
			else
			{
				message = "Password must be given";
			}
		}
		else
		{
			message = "Passwords do not match";
		}
	}
	else
	{
		if (password1.empty() && password2.empty())
		{
			message = "Disabled password requirement";
			setup->enableAuthentication = false;
			SaveProperties();
		}
		else
		{
			message = "\"Require Password\" must be enable before setting a password";
		}
	}

	return message;
}

static void RestoreSetupDefaults(RequestInfo * /*reqInfo*/)
{
	Setup::get()->reinit();
	SaveProperties();

	BootConfigManager * bootConfig = BootConfigManager::get();
	BootConfigManager::Settings settings = bootConfig->readSettings();

	settings.gpuMemoryMb = 128;
	settings.ledDisabled = false;

	bootConfig->writeSettings(settings);
}

static bool IsDuplicatePins(Setup * setup)
{
	const size_t NUM_PINS_USED = 6;
	std::set<int> pins;

	pins.insert(setup->leftLaserPin);
	pins.insert(setup->lightingPin);
	pins.insert(setup->motorDirPin);
	pins.insert(setup->motorEnablePin);
	pins.insert(setup->motorStepPin);
	pins.insert(setup->rightLaserPin);

	return pins.size() != NUM_PINS_USED;
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

	std::string maxObjectSize = reqInfo->arguments[WebContent::MAX_OBJECT_SIZE];
	if (!maxObjectSize.empty())
	{
		setup->maxObjectSize = ConvertUnitOfLength(ToReal(maxObjectSize), srcUnits, UL_MILLIMETERS);
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
	setup->serialNumber = serialNumber;

	setup->enableLighting = !reqInfo->arguments[WebContent::ENABLE_LIGHTING].empty();

	std::string lightingPin = reqInfo->arguments[WebContent::LIGHTING_PIN];
	if (!lightingPin.empty())
	{
		int pin = ToInt(lightingPin);
		if (pin > MAX_PIN)
		{
			throw Exception("Invalid Lighting Pin Setting");
		}

		setup->lightingPin = pin;
	}

	if (IsDuplicatePins(setup))
	{
		throw Exception("The same pin is referenced more than once.");
	}

	// USB Network Config
	setup->enableUsbNetworkConfig = !reqInfo->arguments[WebContent::ENABLE_USB_NETWORK_CONFIG].empty();

	// Enable WebGL
	setup->enableWebGLWhenAvailable = !reqInfo->arguments[WebContent::ENABLE_WEBGL].empty();

	// Enable Point Cloud Renderer
	setup->enablePointCloudRenderer = !reqInfo->arguments[WebContent::ENABLE_POINT_CLOUD_RENDERER].empty();

	// Override focal length
	setup->overrideFocalLength = !reqInfo->arguments[WebContent::OVERRIDE_FOCAL_LENGTH].empty();

	// Overriden focal length value
	setup->overriddenFocalLength = reqInfo->arguments[WebContent::OVERRIDDEN_FOCAL_LENGTH];

	// Experimental
	setup->enableExperimental = !reqInfo->arguments[WebContent::ENABLE_EXPERIMENTAL].empty();

	// Flip Red/Blue
	setup->mmalFlipRedBlue = !reqInfo->arguments[WebContent::FLIP_RED_BLUE].empty();

	//
	// Save the properties
	//

	SaveProperties();

	//
	// Update Boot Config
	//

	BootConfigManager * bootConfig = BootConfigManager::get();
	BootConfigManager::Settings prevSettings = bootConfig->readSettings();
	BootConfigManager::Settings settings = prevSettings;

	std::string gpuMemStr = reqInfo->arguments[WebContent::GPU_MEMORY];
	if (!gpuMemStr.empty())
	{
		settings.gpuMemoryMb = ToInt(gpuMemStr);
	}

	settings.ledDisabled = !reqInfo->arguments[WebContent::DISABLE_CAMERA_LED].empty();

	if (settings != prevSettings)
	{
		bootConfig->writeSettings(settings);
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
	Progress& progress = scanner->getProgress();
	real remainingTime = scanner->getRemainingTime();
	Scanner::Task task = scanner->getTask();
	Setup * setup = Setup::get();

	std::string rotation = reqInfo->arguments[WebContent::ROTATION];
	std::string pixelRadius = reqInfo->arguments[WebContent::PIXEL_RADIUS];

	std::string page = WebContent::scanRunning(progress, remainingTime, task, reqInfo->url, setup->enablePointCloudRenderer, rotation, pixelRadius);
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

	InfoLog << message << Logger::ENDL;

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

	if (ext == "ply" || ext == "jpg" || ext == "stl" || ext == "db" || ext == "png" || ext == "csv" || ext == "xyz" || ext == "log")
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
		std::string directory = (reqInfo->url.find("/dl/") == 0) ? GetScanOutputDir() : GetDebugOutputDir();
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
				if (fread((void *)fileData, 1, fileSize, fp) == 0)
				{
					ErrorLog << "Error reading from file: " << filename.str() << Logger::ENDL;
				}

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

static int GetRenderImage(RequestInfo * reqInfo)
{
	int defaultWidth = 640;
	int minWidth = 320;
	int maxWidth = 1600;
	int defaultPixelRadius = 1;
	int maxPixelRadius = 10;

	// ID
	int id = -1;
	std::string idStr = reqInfo->arguments[WebContent::ID];
	if (!idStr.empty())
	{
		 id = ToInt(idStr);
	}

	// Width
	std::string widthStr = reqInfo->arguments[WebContent::WIDTH];
	int width = widthStr.empty() ? defaultWidth : ToInt(widthStr);
	width = MAX(minWidth, MIN(width, maxWidth));

	// Height
	int height = (width / 4) * 3;

	// Pixel Radius
	std::string pixelRadiusStr = reqInfo->arguments[WebContent::PIXEL_RADIUS];
	int pixelRadius = pixelRadiusStr.empty() ? defaultPixelRadius : ToInt(pixelRadiusStr);
	pixelRadius = MAX(0, MIN(maxPixelRadius, pixelRadius));

	// Rotation
	std::string rotationStr = reqInfo->arguments[WebContent::ROTATION];
	real rotation = rotationStr.empty() ? 0 : ToReal(rotationStr);
	rotation = MAX(0, MIN(360.0f, rotation));
	rotation = DEGREES_TO_RADIANS(rotation);

	// Render the image from the camera
	unsigned char * imageData = NULL;
	unsigned imageSize = 0;
	HttpServer * server = reqInfo->server;

	Scanner * scanner = server->getScanner();
	Scanner::LiveData liveData = scanner->getLiveDataLock();
	try
	{
		PointCloudRenderer renderer(width, height, pixelRadius, rotation);

		if (id == -1)
		{
			renderer.addPoints(* liveData.leftLaserResults);
			renderer.addPoints(* liveData.rightLaserResults);
		}
		else
		{
			// Construct the filename
			std::stringstream filename;
			filename << GetScanOutputDir() << "/" << id << ".ply";

			freelss::PlyReader reader;
			std::vector<freelss::ColoredPoint> points;
			reader.read(filename.str(), points);
			renderer.addPoints(points);
		}

		Image * image = renderer.getImage();

		imageSize = image->getPixelBufferSize();
		imageData = reinterpret_cast<unsigned char *>(malloc(imageSize));

		// Convert the image to a JPEG
		Image::convertToJpeg(* image, imageData, &imageSize);
	}
	catch (...)
	{
		scanner->releaseLiveDataLock();
		free(imageData);
		throw;
	}

	scanner->releaseLiveDataLock();

	MHD_Response *response = MHD_create_response_from_buffer (imageSize, (void *) imageData, MHD_RESPMEM_MUST_FREE);
	MHD_add_response_header (response, "Content-Type", "image/jpeg");
	MHD_add_response_header (response, "Cache-Control", "no-cache, no-store, must-revalidate");
	MHD_add_response_header (response, "Pragma", "no-cache");
	MHD_add_response_header (response, "Expires", "0");
	MHD_add_response_header (response, "Pragma-directive", "no-cache");
	MHD_add_response_header (response, "Cache-directive", "no-cache");

	int ret = MHD_queue_response (reqInfo->connection, MHD_HTTP_OK, response);
	MHD_destroy_response (response);

	return ret;
}

static int GetCameraImage(RequestInfo * reqInfo, bool addLines)
{
	// Get the image from the camera

	unsigned char * imageData = NULL;
	unsigned imageSize = 0;
	Camera * camera = Camera::getInstance();
	Image * image = NULL;

	try
	{
		image = camera->acquireImage();
		if (image == NULL)
		{
			throw Exception("NULL image in Camera::acquireJpeg");
		}

		imageSize = image->getPixelBufferSize();
		imageData = reinterpret_cast<unsigned char *>(malloc(imageSize));

		// Add the calibration lines
		if (addLines)
		{
			Calibrator::addCalibrationLines(image);
		}

		// Convert the image to a JPEG
		Image::convertToJpeg(* image, imageData, &imageSize);

		camera->releaseImage(image);
	}
	catch (...)
	{
		free(imageData);
		camera->releaseImage(image);
		throw;
	}

	MHD_Response *response = MHD_create_response_from_buffer (imageSize, (void *) imageData, MHD_RESPMEM_MUST_FREE);
	MHD_add_response_header (response, "Content-Type", "image/jpeg");
	MHD_add_response_header (response, "Cache-Control", "no-cache, no-store, must-revalidate");
	MHD_add_response_header (response, "Pragma", "no-cache");
	MHD_add_response_header (response, "Expires", "0");
	MHD_add_response_header (response, "Pragma-directive", "no-cache");
	MHD_add_response_header (response, "Cache-directive", "no-cache");

	int ret = MHD_queue_response (reqInfo->connection, MHD_HTTP_OK, response);
	MHD_destroy_response (response);

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


		InfoLog << "Detected center of turn table at row " << tableCenter << " from left row "
				  << leftTableCenter.y << " and right row " << rightTableCenter.y
				  << ". Computed camera Z as " << cameraZ / 25.4 << " in. " << Logger::ENDL;

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

static std::string AutocorrectLaserMisalignment(RequestInfo * reqInfo)
{
	std::string message;
	Setup * setup = Setup::get();
	Preset& preset = PresetManager::get()->getActivePreset();

	try
	{
		// Ensure that it is a 5MP preset
		if (preset.cameraMode != CM_STILL_5MP &&
			preset.cameraMode != CM_VIDEO_5MP &&
			preset.cameraMode != CM_STILL_8MP)
		{
			throw Exception("Laser calibration can be performed in 5MP mode or higher only.");
		}

		Plane leftPlane;
		PixelLocation leftTop, leftBottom;
		Calibrator::calculateLaserPlane(leftPlane, leftTop, leftBottom, Laser::getInstance(), Laser::LEFT_LASER, setup->leftLaserLocation);

		Plane rightPlane;
		PixelLocation rightTop, rightBottom;
		Calibrator::calculateLaserPlane(rightPlane, rightTop, rightBottom, Laser::getInstance(), Laser::RIGHT_LASER, setup->rightLaserLocation);

		Camera * camera = Camera::getInstance();

		// Update the setup
		setup->haveLaserPlaneNormals = true;
		setup->leftLaserPlaneNormal = leftPlane.normal;
		setup->rightLaserPlaneNormal = rightPlane.normal;
		setup->leftLaserCalibrationTop.x = leftTop.x / camera->getImageWidth();
		setup->leftLaserCalibrationTop.y = leftTop.y / camera->getImageHeight();
		setup->leftLaserCalibrationBottom.x = leftBottom.x / camera->getImageWidth();
		setup->leftLaserCalibrationBottom.y = leftBottom.y / camera->getImageHeight();
		setup->rightLaserCalibrationTop.x = rightTop.x / camera->getImageWidth();
		setup->rightLaserCalibrationTop.y = rightTop.y / camera->getImageHeight();
		setup->rightLaserCalibrationBottom.x = rightBottom.x / camera->getImageWidth();
		setup->rightLaserCalibrationBottom.y = rightBottom.y / camera->getImageHeight();

		SaveProperties();

		message = "Successfully auto-corrected laser alignment";
	}
	catch (Exception& ex)
	{
		message = ex;
	}
	catch (...)
	{
		message = "Unknown error occurred";
	}

	return message;
}

static std::string SetCameraExposureTime(RequestInfo * reqInfo)
{
	std::string message;

	std::string exposureValStr = reqInfo->arguments[WebContent::CAMERA_EXPOSURE_TIME];
	if (!exposureValStr.empty())
	{
		CameraExposureTime cameraExposureTime = (CameraExposureTime) ToInt(exposureValStr);

		// Apply the exposure time
		Camera::getInstance()->setExposureTime(cameraExposureTime);

		// Update the preset
		PresetManager::get()->getActivePreset().cameraExposureTime = cameraExposureTime;

		// We must reinitialize if setting to auto
		if (cameraExposureTime == CET_AUTO)
		{
			Camera::reinitialize();
		}

		SaveProperties();
	}
	else
	{
		message = "No exposure setting was given.";
	}

	return message;
}

static void WriteUsbConfigFile(const std::string& filename, const std::string& details, const std::vector<std::string>& errors,  const std::vector<WifiConfig::AccessPoint>& networks,
		                       const std::vector<std::pair<std::string, std::string> >& assignedInterfaces)
{
	const std::string ENDL = "\r\n";
	std::ofstream fout(filename.c_str());
	if (!fout.is_open())
	{
		throw Exception("Error opening file for writing: " + filename);
	}

	// Write the URL to the scanner
	int port = Setup::get()->httpPort;
    std::string portString = (port == 80) ? "" : (":" + ToString(port));
    for (size_t iInt = 0; iInt < assignedInterfaces.size(); iInt++)
	{
    	std::string interface = assignedInterfaces[iInt].first;
    	std::string ip = assignedInterfaces[iInt].second;
    	std::string interfaceStr = (assignedInterfaces.size() == 1) ? "" : " (" + interface + ")";

		std::string portString = (port == 80) ? "" : (":" + ToString(port));
		fout << "SCANNER URL" << interfaceStr << " is http://" << ip << portString << "/" << ENDL;
	}

    fout << ENDL;

	// Show any errors if there are any
	if (!errors.empty())
	{
		for (size_t iErr = 0; iErr < errors.size(); iErr++)
		{
			fout << "Error: " << errors[iErr] << ENDL;
		}

		fout << ENDL << ENDL;
	}

	if (!networks.empty())
	{
		fout << "Detected the following " << networks.size() << " wireless networks:" << ENDL;
		for (size_t iNet = 0; iNet < networks.size(); iNet++)
		{
			fout << networks[iNet].essid << ENDL;
		}
	}
	else
	{
		fout << "No wireless networks were detected." << ENDL;
	}

	fout << ENDL
		 << "Enter the name and password of your WiFi network below and then insert USB drive into the ATLAS 3D scanner." << ENDL
		 << "network=" << ENDL
		 << "password=" << ENDL;
	fout.close();
}

static void WriteUsbHtmlFile(const std::string& filename, bool writeHta, const std::string& details, const std::vector<std::string>& errors, const std::vector<WifiConfig::AccessPoint>& networks,
		                     const std::vector<std::pair<std::string, std::string> >& assignedInterfaces, bool wifiConfigVisible)
{
	std::ofstream fout (filename.c_str());
	if (!fout.is_open())
	{
		throw Exception("Error opening file for writing: " + filename);
	}

	// std::string wifiDisplay = wifiConfigVisible ? "block" : "none";
// writeHta
	fout << "\
<!DOCTYPE html>\
<html>\
  <head>\
	<style type=\"text/css\">\
		body { font-family: verdana }\
	</style>";

	if (writeHta)
	{
		fout << "<meta http-equiv=\"X-UA-Compatible\" content=\"IE=9\" />";
	}

	fout << "\
	<script language=\"javascript\">\
	window.resizeTo(550, 700);\
	function writeData()\
	{\
		var essid = document.getElementById('essid').value;\
		var password = document.getElementById('password').value;\
\
		if (essid.length == 0)\
		{\
			alert('A network must be selected.');\
			return;\
		}\
\
		if (password.length == 0)\
		{\
			alert('A password must be entered.');\
			return;\
		}\
\
		var fso = new ActiveXObject(\"Scripting.FileSystemObject\");\
\
		var s = fso.CreateTextFile('ATLAS3D_cfg.txt', true);\
		s.WriteLine('network=' + essid);\
		s.WriteLine('password=' + password);\
		s.Close();\
\
		alert('Settings saved successfully.  Insert the USB drive into the ATLAS 3D scanner to apply them.');\
	}\
	function showElement(toShow, toHide)\
	{\
		document.getElementById(toShow).style.display = 'block';\
		if (toHide)\
		{\
			document.getElementById(toHide).style.display = 'none';\
		}\
	}\
	</script>\
  </head>\
  <body style=\"width: 500px; background-color: rgb(170, 170, 170);\">";

	if (!errors.empty())
	{
		fout << "<p style=\"font-size: 24px; font-weight: bold; color: red; text-align: center; filter: dropshadow(color=black, offx=1, offy=1);\">";
		for (size_t iErr = 0; iErr < errors.size(); iErr++)
		{
			fout << errors[iErr] << "<br />";
		}
		fout << "</p>";
	}

    fout << "<img src=\"data:image/png;base64,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\" border=\"0\" />\
	<br />";

    int port = Setup::get()->httpPort;
    std::string portString = (port == 80) ? "" : (":" + ToString(port));
    for (size_t iInt = 0; iInt < assignedInterfaces.size(); iInt++)
	{
    	std::string interface = assignedInterfaces[iInt].first;
    	std::string ip = assignedInterfaces[iInt].second;
    	std::string interfaceStr = (assignedInterfaces.size() == 1) ? "" : " (" + interface + ")";

		fout << "\
    <div style=\"padding-top: 20px\">\
		<form action=\"http://" << ip << portString << "/cal1\" method=\"POST\"><input style=\"height: 40px\" type=\"submit\" value=\"Connect to ATLAS 3D" << interfaceStr << "...\"></form>\
	</div>";
	}

    if (writeHta)
    {
	fout << "\
	<div style=\"padding-top: 30px\"></div>\
	<div id=\"showWifi\">\
		<a href=\"#\" onclick=\"showElement('wifi', 'showWifi')\">Setup WiFi</a>\
	</div>\
\
	<div id=\"wifi\" style=\"display: none\">\
		<div style=\"padding-top: 5px\">\
			<div style=\"float: left; width: 125px;\">Network:</div>\
			<div>\
				<select style=\"width: 200px\" id=\"essid\" name=\"essid\">" << std::endl;

					for (size_t iNet = 0; iNet < networks.size(); iNet++)
					{
						std::string essid = networks[iNet].essid;
						fout << "<option value=\"" << essid << "\">" << essid << "</option>" << std::endl;
					}

				fout << "\
				</select>\
			</div>\
		</div>\
\
		<div style=\"padding-top: 5px\">\
			<div style=\"float: left; width: 125px\">Password:</div>\
			<div><input style=\"width: 194px\" id=\"password\" type=\"password\"></div>\
		</div>\
		<div style=\"padding-top: 10px\">\
			<button onclick=\"writeData()\">Save</button>\
		</div>\
	</div>";
    } // end if writeHta

	fout << "\
	<div style=\"padding-top: 30px\"></div>\
	<div id=\"showDetails\">\
		<a href=\"#\" onclick=\"showElement('details', 'showDetails')\">Show Details</a>\
	</div>\
	<div id=\"details\" style=\"display: none\">\
		<textarea cols=40\" rows = \"25\">" << std::endl;

		fout << details;

		fout << "\
		</textarea>\
	</div>\
  </body>\
</html>";
}

static std::string ExecuteCommand(const std::string& command)
{
	std::string tmpFile = "/tmp/freelss_cmd.txt";
	std::stringstream output;

	std::string fullCmd = command + " > " + tmpFile + " 2>&1";
	if (system(fullCmd.c_str()) != -1)
	{
		std::ifstream fin (tmpFile.c_str());
		if (fin.is_open())
		{
			std::string line;
			while (std::getline(fin, line))
			{
				output << line << std::endl;
			}

			fin.close();
		}
		else
		{
			output << "Error opening file: " << tmpFile;
		}
	}
	else
	{
		throw Exception("Error running command: " + fullCmd);
	}

	return output.str();
}

static bool IsValidIp(const std::string& ip)
{
	return ! ip.empty() && ip != "0.0.0.0" && ip != "127.0.0.1";
}

static std::string HandleUsbMount(RequestInfo * reqInfo)
{
	const unsigned long WIFI_CONNECT_TIMEOUT_SEC = 120; // Maximum time to wait until an IP is obtained in seconds

	WifiConfig * wifi = WifiConfig::get();
	std::string device = reqInfo->arguments["device"];
	if (device.empty())
	{
		throw Exception("No device given");
	}

	std::string htaFilename = TrimString(device) + "/ATLAS3D_Windows.hta";
	std::string htmlFilename = TrimString(device) + "/ATLAS3D_Connect.html";
	std::string cfgFilename = TrimString(device) + "/ATLAS3D_cfg.txt";

	Laser * laser = Laser::getInstance();
	laser->turnOn(Laser::ALL_LASERS);

	// Error messages formatted for HTML display
	std::vector<std::string> errors;
	std::stringstream details;
	std::vector<WifiConfig::AccessPoint> networks;
	std::vector<std::pair<std::string, std::string> > assignedInterfaces;

	try
	{
		//
		// Get a list of WiFi networks in range
		//
		wifi->scan();
		networks = wifi->getAccessPoints();

		//
		// Connect to the WiFi network
		//
		if (access(cfgFilename.c_str(), F_OK) != -1)
		{
			std::string essid, password;
			std::string search1 = "network=";
			std::string search2 = "password=";
			std::string line;

			std::ifstream fin (cfgFilename.c_str());
			if (fin.is_open())
			{
				while (std::getline(fin, line))
				{
					if (line.find(search1) == 0)
					{
						essid = TrimString(line.substr(search1.length()));
					}

					if (line.find(search2) == 0)
					{
						password = TrimString(line.substr(search2.length()));
					}
				}

				fin.close();
			}

			//
			// Connect to the WiFi network if we have a login and password
			//
			if (!essid.empty() && !password.empty())
			{
				WifiConfig * wifi = WifiConfig::get();
				wifi->connect(essid, password);

				// Wait up to x seconds until it connects
				double startTime = GetTimeInSeconds();
				bool timesUp = false;

				std::vector<std::string> wifiInterfaces = wifi->getWifiInterfaces();
				if (!wifiInterfaces.empty())
				{
					std::string wifiInterface = wifiInterfaces.front();
					bool hasIp = false;
					do
					{
						std::string ip = wifi->getIpAddress(wifiInterface);
						if (IsValidIp(ip))
						{
							hasIp = true;
						}
						else
						{
							Thread::sleep(2);
						}

						timesUp = GetTimeInSeconds() - startTime > WIFI_CONNECT_TIMEOUT_SEC;
					} while (!hasIp && !timesUp);

					if (timesUp && !hasIp)
					{
						errors.push_back("Error obtaining IP address for " + wifiInterface + ". The password may be incorrect.");
					}
				}
				else
				{
					errors.push_back("No WiFi devices found.");
				}
			}
		} // end if there is a cfg file

		// See if we can get the IP even if we are not creating a new connection
		std::vector<std::string> interfaces = wifi->getAllInterfaces();
		for (size_t iInt = 0; iInt < interfaces.size(); iInt++)
		{
			std::string interface = interfaces[iInt];
			std::string ip = wifi->getIpAddress(interface);
			if (IsValidIp(ip))
			{
				assignedInterfaces.push_back(std::pair<std::string, std::string>(interface, ip));
			}
		}

		details << "FreeLSS Version: " << FREELSS_VERSION_NAME << std::endl;

		details << std::endl << "Network Devices:" << std::endl;
		for (size_t iInt = 0; iInt < interfaces.size(); iInt++)
		{
			details << interfaces[iInt] << ": " << wifi->getIpAddress(interfaces[iInt]) << std::endl;
		}

		struct utsname uts;
		if (uname(&uts) == 0)
		{
			details << std::endl << "Kernel: " << uts.sysname << " " << uts.release << " " << uts.version << " " << uts.machine << std::endl;
		}

		details << std::endl << "lsusb: " << std::endl << ExecuteCommand("lsusb") << std::endl;

		details << std::endl << "lsmod: " << std::endl << ExecuteCommand("lsmod") << std::endl;

		details << std::endl << "messages: " << std::endl << ExecuteCommand("tail -n 100 /var/log/messages") << std::endl;

		details << std::endl << "date: " << std::endl << ExecuteCommand("date") << std::endl;
	}
	catch (Exception& ex)
	{
		errors.push_back(ex);
	}

	try
	{
		//
		// Write the HTA file
		//
		WriteUsbHtmlFile(htaFilename, true, details.str(), errors, networks, assignedInterfaces, assignedInterfaces.empty());

		//
		// Write the HTML file
		//
		if (!assignedInterfaces.empty())
		{
			WriteUsbHtmlFile(htmlFilename, false, details.str(), errors, networks, assignedInterfaces, assignedInterfaces.empty());
		}
		// Remove the HTML file
		else
		{
			if (access(htmlFilename.c_str(), F_OK) != -1)
			{
				if (remove(htmlFilename.c_str()) == -1)
				{
					errors.push_back("Error removing file: " + htmlFilename);
				}
			}
		}

		//
		// Write the Config File
		//
		WriteUsbConfigFile(cfgFilename, details.str(), errors, networks, assignedInterfaces);

#if 1
		sync();
#else
		if (umount(device.c_str()) == -1)
		{
			throw Exception("Error unmounting SD card");
		}
#endif
	}
	catch (Exception& ex)
	{
		ErrorLog << "!! Error: " << ex << Logger::ENDL;
	}

	laser->turnOff(Laser::ALL_LASERS);

	// Return any errors
	std::stringstream out;
	for (size_t iErr = 0; iErr < errors.size(); iErr++)
	{
		out << errors[iErr] << std::endl;
	}

	return out.str();
}

static void WritePhotoSequence(RequestInfo * reqInfo)
{
	HttpServer * server = reqInfo->server;

	std::string mountIndexStr = reqInfo->arguments[WebContent::PHOTO_MOUNT];
	int mountIndex = -1;
	if (!mountIndexStr.empty())
	{
		mountIndex = ToInt(mountIndexStr);
	}

	std::vector<std::string> mountPaths = MountManager::get()->getValidMounts();
	if ((int)mountPaths.size() <= mountIndex || mountIndex < 0)
	{
		throw Exception("Invalid mount path index");
	}

	std::string mountPath = mountPaths[mountIndex];

	bool writeLaserImages = !reqInfo->arguments[WebContent::PHOTO_WRITE_LASER_IMAGES].empty();

	server->getScanner()->setPhotoSequencePath(mountPath);
	server->getScanner()->setSavePhotoSequenceLaserImages(writeLaserImages);
	server->getScanner()->setRange(360);
	server->getScanner()->setTask(Scanner::GENERATE_PHOTOS);
	server->getScanner()->execute();
	InfoLog << "Starting photo writing..." << Logger::ENDL;

	// Give the scanner time to start
	Thread::usleep(500000);

}

static bool GetFileContents(RequestInfo * reqInfo, unsigned char *& data, unsigned int*& length)
{
	data = NULL;

	if (reqInfo->url == "/licenses.txt")
	{
		data = licenses_txt;
		length = &licenses_txt_len;
	}
	else if (reqInfo->url == "/three.min.js")
	{
		data = three_min_js;
		length = &three_min_js_len;
	}
	else if (reqInfo->url == "/PLYLoader.js")
	{
		data = PLYLoader_js;
		length = &PLYLoader_js_len;
	}
	else if (reqInfo->url == "/OrbitControls.js")
	{
		data = OrbitControls_js;
		length = &OrbitControls_js_len;
	}
	else if (reqInfo->url == "/showScan.js")
	{
		data = showScan_js;
		length = &showScan_js_len;
	}
	else if (reqInfo->url == "/CanvasRenderer.js")
	{
		data = CanvasRenderer_js;
		length = &CanvasRenderer_js_len;
	}
	else if (reqInfo->url == "/SoftwareRenderer.js")
	{
		data = SoftwareRenderer_js;
		length = &SoftwareRenderer_js_len;
	}
	else if (reqInfo->url == "/Projector.js")
	{
		data = Projector_js;
		length = &Projector_js_len;
	}

	return data != NULL;
}

static int ProcessPageRequest(RequestInfo * reqInfo)
{
	HttpServer * server = reqInfo->server;
	int ret = MHD_YES;
	unsigned char * fileData = NULL;
	unsigned int * fileSize = NULL;

	MHD_get_connection_values (reqInfo->connection, MHD_GET_ARGUMENT_KIND, &StoreToMap, (void *) reqInfo);

	if (server->getScanner()->isRunning())
	{
		std::string cmd = reqInfo->arguments["cmd"];
		if (reqInfo->url == "/" && cmd == "stopScan")
		{
			InfoLog << "Stopping scan..." << Logger::ENDL;
			server->getScanner()->stop();

			// Wait for the scanner to stop
			server->getScanner()->join();

			InfoLog << "Scan stopped" << Logger::ENDL;
		}
	}

	if (!server->getScanner()->isRunning())
	{
		if (reqInfo->url.find("/camImage") == 0)
		{
			bool addLines = reqInfo->url.find("/camImageL") == 0;

			ret = GetCameraImage(reqInfo, addLines);
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

				if (system("shutdown -h now&") == -1)
				{
					ErrorLog << "Error rebooting the system." << Logger::ENDL;
				}
				message = "Shutting down....";
			}

			if (!message.empty())
			{
				InfoLog << message << Logger::ENDL;
			}

			std::string page = WebContent::settings(message);
			MHD_Response *response = MHD_create_response_from_buffer (page.size(), (void *) page.c_str(), MHD_RESPMEM_MUST_COPY);
			MHD_add_response_header (response, "Content-Type", "text/html");
			ret = MHD_queue_response (reqInfo->connection, MHD_HTTP_OK, response);
			MHD_destroy_response (response);

			InfoLog << "Done." << Logger::ENDL;
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
			else if (reqInfo->method == RequestInfo::POST && cmd == WebContent::RESTORE_DEFAULTS)
			{
				RestoreSetupDefaults(reqInfo);
				reqInfo->server->reinitialize();
				message = "Defaults have been restored.";
			}

			std::string page = WebContent::setup(message);
			MHD_Response *response = MHD_create_response_from_buffer (page.size(), (void *) page.c_str(), MHD_RESPMEM_MUST_COPY);
			MHD_add_response_header (response, "Content-Type", "text/html");
			ret = MHD_queue_response (reqInfo->connection, MHD_HTTP_OK, response);
			MHD_destroy_response (response);
		}
		else if (reqInfo->url.find("/network") == 0)
		{
			// Save the settings if this was a post
			std::string cmd = reqInfo->arguments["cmd"];
			std::string message = "";

			if (reqInfo->method == RequestInfo::POST && cmd == "connect")
			{
				message = ConnectWifi(reqInfo);
			}

			bool hiddenEssid = ! reqInfo->arguments["hidden"].empty();
			std::string page = WebContent::network(message, hiddenEssid);
			MHD_Response *response = MHD_create_response_from_buffer (page.size(), (void *) page.c_str(), MHD_RESPMEM_MUST_COPY);
			MHD_add_response_header (response, "Content-Type", "text/html");
			ret = MHD_queue_response (reqInfo->connection, MHD_HTTP_OK, response);
			MHD_destroy_response (response);
		}
		else if (reqInfo->url.find("/mounts") == 0)
		{
			// Save the settings if this was a post
			std::string cmd = reqInfo->arguments["cmd"];
			std::string message = "";

			if (reqInfo->method == RequestInfo::POST && cmd == "mount")
			{
				message = MountNetworkPath(reqInfo);
			}
			else if (reqInfo->method == RequestInfo::POST && cmd == "unmount")
			{
				message = UnmountPath(reqInfo);
			}

			std::string page = WebContent::mounts(message, MountManager::get()->getValidMounts());
			MHD_Response *response = MHD_create_response_from_buffer (page.size(), (void *) page.c_str(), MHD_RESPMEM_MUST_COPY);
			MHD_add_response_header (response, "Content-Type", "text/html");
			ret = MHD_queue_response (reqInfo->connection, MHD_HTTP_OK, response);
			MHD_destroy_response (response);
		}
		else if (reqInfo->url.find("/security") == 0)
		{
			// Save the settings if this was a post
			std::string cmd = reqInfo->arguments["cmd"];
			std::string message = "";

			if (reqInfo->method == RequestInfo::POST && cmd == "save")
			{
				message = SaveAuthenticationSettings(reqInfo);
			}

			std::string page = WebContent::security(message);
			MHD_Response *response = MHD_create_response_from_buffer (page.size(), (void *) page.c_str(), MHD_RESPMEM_MUST_COPY);
			MHD_add_response_header (response, "Content-Type", "text/html");
			ret = MHD_queue_response (reqInfo->connection, MHD_HTTP_OK, response);
			MHD_destroy_response (response);
		}
		else if (reqInfo->url.find("/writePhotos") == 0)
		{
			std::string cmd = reqInfo->arguments["cmd"];

			if (reqInfo->method == RequestInfo::POST && cmd == "writePhotos")
			{
				WritePhotoSequence(reqInfo);
			}
			else
			{
				std::string page = WebContent::writePhotos(MountManager::get()->getValidMounts());
				MHD_Response *response = MHD_create_response_from_buffer (page.size(), (void *) page.c_str(), MHD_RESPMEM_MUST_COPY);
				MHD_add_response_header (response, "Content-Type", "text/html");
				ret = MHD_queue_response (reqInfo->connection, MHD_HTTP_OK, response);
				MHD_destroy_response (response);
			}

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
					InfoLog << "Took " << numSteps << " steps." << Logger::ENDL;
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
				server->getScanner()->setTask(Scanner::GENERATE_DEBUG);

				Preset& preset = PresetManager::get()->getActivePreset();

				server->getScanner()->generateDebugInfo(preset.laserSide);
				ret = RetrieveFile(reqInfo, "/dbg/5.png");

				responded = true;
			}

			else if (reqInfo->method == RequestInfo::POST && cmd == "calibrateLasers")
			{
				message = AutocorrectLaserMisalignment(reqInfo);
			}
			else if (reqInfo->method == RequestInfo::POST && cmd == "setLightIntensity")
			{
				std::string intensityStr = reqInfo->arguments["intensity"];
				Lighting::get()->setIntensity(ToInt(intensityStr));
				message = "Set light intensity to " + intensityStr + ".";
			}
			else if (reqInfo->method == RequestInfo::POST && cmd == "setCameraExposureTime")
			{
				message = SetCameraExposureTime(reqInfo);
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

				server->getScanner()->setRange(ToReal(degrees.c_str()));
				server->getScanner()->setTask(Scanner::GENERATE_SCAN);
				server->getScanner()->execute();
				InfoLog << "Starting scan..." << Logger::ENDL;

				// Give the scanner time to start
				Thread::usleep(500000);
			}
			else
			{
				std::string page = WebContent::scan(server->getScanner()->getPastScanResults());
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
				cmd << "rm -f " << GetScanOutputDir() << "/" << ToInt(id.c_str()) << ".*";

				// Execute the command
				if (system(cmd.str().c_str()) != -1)
				{
					std::string page = WebContent::scan(server->getScanner()->getPastScanResults());
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
			if (system("reboot&") == -1)
			{
				ErrorLog << "Error rebooting the system." << Logger::ENDL;
			}

			std::string page = "Rebooting...";
			MHD_Response *response = MHD_create_response_from_buffer (page.size(), (void *) page.c_str(), MHD_RESPMEM_MUST_COPY);
			MHD_add_response_header (response, "Content-Type", "text/html");
			ret = MHD_queue_response (reqInfo->connection, MHD_HTTP_OK, response);
			MHD_destroy_response (response);
		}
		else if (GetFileContents(reqInfo, fileData, fileSize))
		{
			MHD_Response *response = MHD_create_response_from_buffer (*fileSize, (void *) fileData, MHD_RESPMEM_MUST_COPY);
			MHD_add_response_header (response, "Content-Type", "text/plain");
			ret = MHD_queue_response (reqInfo->connection, MHD_HTTP_OK, response);
			MHD_destroy_response (response);
		}
		else if (reqInfo->url == "/view")
		{
			int id = ToInt(reqInfo->arguments["id"]);
			std::string rotation = reqInfo->arguments[WebContent::ROTATION];
			std::string pixelRadius = reqInfo->arguments[WebContent::PIXEL_RADIUS];

			std::string page = WebContent::viewScanRender(id, reqInfo->url, rotation, pixelRadius);
			MHD_Response *response = MHD_create_response_from_buffer (page.size(), (void *) page.c_str(), MHD_RESPMEM_MUST_COPY);
			MHD_add_response_header (response, "Content-Type", "text/html");
			ret = MHD_queue_response (reqInfo->connection, MHD_HTTP_OK, response);
			MHD_destroy_response (response);
		}
		else if (reqInfo->url == "/viewPly")
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
		else if (reqInfo->url == "/onMount")
		{
			std::string page;

			// Perform network setup via USB
			if (Setup::get()->enableUsbNetworkConfig)
			{
				page = HandleUsbMount(reqInfo);
			}

			MHD_Response *response = MHD_create_response_from_buffer (page.size(), (void *) page.c_str(), MHD_RESPMEM_MUST_COPY);
			MHD_add_response_header (response, "Content-Type", "text/html");
			ret = MHD_queue_response (reqInfo->connection, MHD_HTTP_OK, response);
			MHD_destroy_response (response);
		}
		else if (reqInfo->url == "/restoreDefaults")
		{
			std::string page = WebContent::restoreDefaults();
			MHD_Response *response = MHD_create_response_from_buffer (page.size(), (void *) page.c_str(), MHD_RESPMEM_MUST_COPY);
			MHD_add_response_header (response, "Content-Type", "text/html");
			ret = MHD_queue_response (reqInfo->connection, MHD_HTTP_OK, response);
			MHD_destroy_response (response);
		}
		else if (reqInfo->url.find("/renderImage") == 0)
		{
			ret = GetRenderImage(reqInfo);
		}
		else
		{
			std::string message = std::string("Not Found: ") + reqInfo->url;
			ret = BuildError(reqInfo->connection, message, MHD_HTTP_NOT_FOUND);
		}
	}

	// Show the scan progress
	if (server->getScanner()->isRunning())
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
		else if (reqInfo->url.find("/renderImage") == 0)
		{
			ret = GetRenderImage(reqInfo);
		}
		else if (GetFileContents(reqInfo, fileData, fileSize))
		{
			MHD_Response *response = MHD_create_response_from_buffer (*fileSize, (void *) fileData, MHD_RESPMEM_MUST_COPY);
			MHD_add_response_header (response, "Content-Type", "text/plain");
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

	Setup * setup = Setup::get();

	// HTTP Basic Auth

	bool authenticated = false;

	if (setup->enableAuthentication)
	{
		char * pass = NULL;
		char * user = MHD_basic_auth_get_username_password (reqInfo->connection, &pass);
		try
		{
			if (pass != NULL)
			{
				// Generate hash for the password
				std::string password = pass;
				unsigned char hash[SHA_DIGEST_LENGTH];
				memset(hash, 0, sizeof(hash));
				SHA1((const unsigned char *)password.c_str(), password.size(), hash);

				// Test against the stored hash
				authenticated = ToHexString(hash, SHA_DIGEST_LENGTH) == setup->passwordHash;
			}
		}
		catch (...)
		{
			free(pass);
			free(user);
			throw;
		}

		free(pass);
		free(user);
	}
	else
	{
		authenticated = true;
	}

	if (authenticated)
	{
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
	}
	else
	{
		std::string page = "<html><body>UNAUTHORIZED</body></html>";
		struct MHD_Response * response = MHD_create_response_from_buffer (page.size(), (void *) page.c_str(), MHD_RESPMEM_PERSISTENT);
		ret = MHD_queue_basic_auth_fail_response (reqInfo->connection, "ATLAS 3D", response);
		MHD_destroy_response (response);
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
		ErrorLog << "!! " << ex << Logger::ENDL;
		ret = BuildError(connection, ex.c_str());
	}
	catch (...)
	{
		ErrorLog << "!! Unknown Error" << Logger::ENDL;
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

	InfoLog << "Reinitialized." << Logger::ENDL;
}

Scanner * HttpServer::getScanner()
{
	return  m_scanner;
}

}
