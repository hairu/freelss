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
#include "WebContent.h"
#include "PresetManager.h"
#include "Setup.h"
#include "Laser.h"
#include "Camera.h"
#include "Progress.h"
#include "PlyWriter.h"
#include "Lighting.h"
#include "WifiConfig.h"
#include "BootConfigManager.h"

namespace freelss
{

// A warning is displayed if we have less than this amount of disk space available
const int WebContent::LOW_DISK_SPACE_MB = 1000;

const std::string WebContent::CSS = "\
<style type=\"text/css\">\
.menu2 {\
	float: left;\
	width: 650px;\
	height: 20px;\
    text-align: right;\
}\
body {\
	font-size: 20px;\
	font-family: Verdana, serif, Arial, Helvetica;\
	background-color: #aaa;\
	color: #ffffff;\
	text-shadow: #000000 2px 2px 2px;\
}\
A:link    {color: #ffffff; font-size: 22px; font-weight: bold; text-shadow: #000000 2px 2px 2px; text-decoration: none;}\
A:visited {color: #ffffff; font-size: 22px; font-weight: bold; text-shadow: #000000 2px 2px 2px; text-decoration: none;}\
A:hover   {color: #ffffff; font-size: 22px; font-weight: bold; text-shadow: #000000 2px 2px 2px; text-decoration: underline;}\
A:active  {color: #ffffff; font-size: 22px; font-weight: bold; text-shadow: #000000 2px 2px 2px; text-decoration: underline;}\
.submit {\
	height: 40px;\
	margin-bottom: 10px;\
	font-weight: bold;\
	font-size: 18px;\
	margin-right: 15px;\
}\
.submitSmall {\
	height: 30px;\
}\
.controlSubmit {\
	width: 200px;\
	height: 40px;\
	margin-bottom: 10px;\
	font-weight: bold;\
	font-size: 18px;\
	margin-right: 15px;\
}\
.message {\
	text: rgb(255, 0, 0);\
	font-weight: bold;\
	font-size: 18px;\
}\
.submitContainer {\
	float: left;\
}\
.menu {\
	float: left;\
	width: 200px;\
	height: 30px;\
	background-color: #cae9ff;\
	border: solid 2px #1d5fa9;\
	padding-top: 10px;\
	padding-bottom: 5px;\
	margin-right: 20px;\
}\
.settingsText {\
	float: left;\
	width: 250px;\
}\
.settingsDescr{\
	font-size: 10px;\
	padding-bottom: 20px;\
}\
.settingsInput {\
	width: 150px;\
}\
.result {\
	margin-top: 10px;\
	padding-top: 10px;\
	padding-left: 10px;\
	display: inline-block;\
	vertical-align: middle;\
	background-color: rgb(200, 200, 200);\
	border: solid 2px #1d5fa9;\
	width: 625px;\
}\
.calDescr{\
	font-size: 10px;\
	padding-bottom: 20px;\
	width: 200px;\
}\
.cal1GenerateDebugDiv {\
	padding-bottom: 50px;\
}\
#deleteButton {\
	float: left;\
	padding-left: 10px;\
}\
#viewButton {\
	padding-left: 250px;\
}\
#error1 {\
	font-size: 40px;\
	font-family: Verdana, serif, Arial, Helvetica;\
	color: #ff0000;\
	text-shadow: #000000 2px 2px 2px;\
}\
#menuContainer {\
	width: 800px;\
	text-align: center;\
	padding-bottom: 50px;\
}\
#cal1ContentDiv {\
    margin-top: 75px;\
}\
#cal1ControlsDiv {\
	float: left;\
}\
#cal1ImageDiv {\
	float: none;\
	margin-top: 75px;\
}\
#outerPresetDiv {\
	margin-top: 75px;\
	background-color: rgb(130, 130, 130);\
	width: 650px;\
	margin-bottom: 10px;\
	padding-top: 10px;\
}\
#presetTextDiv {\
	float: left;\
	padding-left: 25px;\
	padding-right: 12px;\
	padding-top: 5px;\
}\
#restoreDefaultsA {\
	padding-top: 15px;\
}\
</style>";

const std::string WebContent::JAVASCRIPT= "\
<script type=\"text/javascript\">\
\
function endsWith(str, suffix) {\
    return str.indexOf(suffix, str.length - suffix.length) !== -1;\
}\
\
function startup1(){\
	setTimeout(updateImgImage, 1000);\
}\
function updateImgImage(){\
	var img = document.getElementById('image');\
	img.src = 'camImageL_' + Math.random() + '.jpg';\
}\
</script>";

const std::string WebContent::PROFILE_NAME = "PROFILE_NAME";
const std::string WebContent::SERIAL_NUMBER = "SERIAL_NUMBER";
const std::string WebContent::CAMERA_X = "CAMERA_X";
const std::string WebContent::CAMERA_Y = "CAMERA_Y";
const std::string WebContent::CAMERA_Z = "CAMERA_Z";
const std::string WebContent::CAMERA_MODE = "CAMERA_MODE";
const std::string WebContent::RIGHT_LASER_X = "RIGHT_LASER_X";
const std::string WebContent::RIGHT_LASER_Y = "RIGHT_LASER_Y";
const std::string WebContent::RIGHT_LASER_Z = "RIGHT_LASER_Z";
const std::string WebContent::RIGHT_LASER_PIN = "RIGHT_LASER_PIN";
const std::string WebContent::LEFT_LASER_X = "LEFT_LASER_X";
const std::string WebContent::LEFT_LASER_Y = "LEFT_LASER_Y";
const std::string WebContent::LEFT_LASER_Z = "LEFT_LASER_Z";
const std::string WebContent::LEFT_LASER_PIN = "LEFT_LASER_PIN";
const std::string WebContent::LASER_MAGNITUDE_THRESHOLD = "LASER_MAGNITUDE_THRESHOLD";
const std::string WebContent::LASER_ON_VALUE = "LASER_ON_VALUE";
const std::string WebContent::LASER_SELECTION = "LASER_SELECTION";
const std::string WebContent::STABILITY_DELAY = "STABILITY_DELAY";
const std::string WebContent::MAX_LASER_WIDTH = "MAX_LASER_WIDTH";
const std::string WebContent::MIN_LASER_WIDTH = "MIN_LASER_WIDTH";
const std::string WebContent::STEPS_PER_REVOLUTION = "STEPS_PER_REVOLUTION";
const std::string WebContent::ENABLE_PIN = "ENABLE_PIN";
const std::string WebContent::STEP_PIN = "STEP_PIN";
const std::string WebContent::DIRECTION_PIN = "DIRECTION_PIN";
const std::string WebContent::DIRECTION_VALUE = "DIRECTION_VALUE";
const std::string WebContent::RESPONSE_DELAY = "RESPONSE_DELAY";
const std::string WebContent::STEP_DELAY = "STEP_DELAY";
const std::string WebContent::FRAMES_PER_REVOLUTION = "FRAMES_PER_REVOLUTION";
const std::string WebContent::GENERATE_XYZ = "GENERATE_XYZ";
const std::string WebContent::GENERATE_STL = "GENERATE_STL";
const std::string WebContent::GENERATE_PLY = "GENERATE_PLY";
const std::string WebContent::SEPARATE_LASERS_BY_COLOR = "SEPARATE_LASERS_BY_COLOR";
const std::string WebContent::UNIT_OF_LENGTH = "UNIT_OF_LENGTH";
const std::string WebContent::VERSION_NAME = "VERSION_NAME";
const std::string WebContent::GROUND_PLANE_HEIGHT = "GROUND_PLANE_HEIGHT";
const std::string WebContent::PLY_DATA_FORMAT = "PLY_DATA_FORMAT";
const std::string WebContent::FREE_DISK_SPACE = "FREE_DISK_SPACE";
const std::string WebContent::ENABLE_BURST_MODE = "ENABLE_BURST_MODE";
const std::string WebContent::ENABLE_LIGHTING = "ENABLE_LIGHTING";
const std::string WebContent::LIGHTING_PIN = "LIGHTING_PIN";
const std::string WebContent::CREATE_BASE_FOR_OBJECT = "CREATE_BASE_FOR_OBJECT";
const std::string WebContent::WIFI_ESSID = "WIFI_ESSID";
const std::string WebContent::WIFI_ESSID_HIDDEN = "WIFI_ESSID_HIDDEN";
const std::string WebContent::WIFI_PASSWORD = "WIFI_PASSWORD";
const std::string WebContent::KERNEL_VERSION = "KERNEL_VERSION";
const std::string WebContent::ENABLE_AUTHENTICATION = "ENABLE_AUTHENTICATION";
const std::string WebContent::AUTH_USERNAME = "AUTH_USERNAME";
const std::string WebContent::AUTH_PASSWORD1 = "AUTH_PASSWORD1";
const std::string WebContent::AUTH_PASSWORD2 = "AUTH_PASSWORD2";
const std::string WebContent::NOISE_REMOVAL_SETTING = "NOISE_REMOVAL_SETTING";
const std::string WebContent::IMAGE_THRESHOLD_MODE = "IMAGE_THRESHOLD_MODE";
const std::string WebContent::CAMERA_EXPOSURE_TIME = "CAMERA_EXPOSURE_TIME";
const std::string WebContent::ENABLE_USB_NETWORK_CONFIG = "ENABLE_USB_NETWORK_CONFIG";
const std::string WebContent::MOUNT_SERVERPATH = "MOUNT_SERVERPATH";
const std::string WebContent::MOUNT_USERNAME = "MOUNT_USERNAME";
const std::string WebContent::MOUNT_PASSWORD = "MOUNT_PASSWORD";
const std::string WebContent::MOUNT_WORKGROUP = "MOUNT_WORKGROUP";
const std::string WebContent::PHOTO_MOUNT = "PHOTO_MOUNT";
const std::string WebContent::PHOTO_WRITE_LASER_IMAGES = "PHOTO_WRITE_LASER_IMAGES";
const std::string WebContent::MOUNT_INDEX = "MOUNT_INDEX";
const std::string WebContent::GPU_MEMORY = "GPU_MEMORY";
const std::string WebContent::DISABLE_CAMERA_LED = "DISABLE_CAMERA_LED";
const std::string WebContent::ENABLE_EXPERIMENTAL = "ENABLE_EXPERIMENTAL";
const std::string WebContent::RESTORE_DEFAULTS = "RESTORE_DEFAULTS";
const std::string WebContent::WIDTH = "WIDTH";
const std::string WebContent::PIXEL_RADIUS = "PIXEL_RADIUS";
const std::string WebContent::ROTATION = "ROTATION";
const std::string WebContent::ENABLE_WEBGL = "ENABLE_WEBGL";
const std::string WebContent::CAMERA_SENSOR = "CAMERA_SENSOR";
const std::string WebContent::ENABLE_POINT_CLOUD_RENDERER = "ENABLE_POINT_CLOUD_RENDERER";
const std::string WebContent::OVERRIDE_FOCAL_LENGTH = "OVERRIDE_FOCAL_LENGTH";
const std::string WebContent::OVERRIDDEN_FOCAL_LENGTH = "OVERRIDDEN_FOCAL_LENGTH";
const std::string WebContent::FLIP_RED_BLUE = "FLIP_RED_BLUE";
const std::string WebContent::MAX_OBJECT_SIZE = "MAX_OBJECT_SIZE";

const std::string WebContent::ID = "id";
const std::string WebContent::MENU2 = "<div class=\"menu2\"><a href=\"/checkUpdate\"><small><small>Check for Update</small></small></a>&nbsp;&nbsp;&nbsp;&nbsp;<a href=\"/network\"><small><small>Network</small></small></a>&nbsp;&nbsp;&nbsp;&nbsp;<a href=\"/security\"><small><small>Security</small></small></a>&nbsp;&nbsp;&nbsp;&nbsp;<a href=\"/setup\"><small><small>Setup</small></small></a></div>";
const std::string WebContent::MENU2_EXPERIMENTAL = "<div class=\"menu2\"><a href=\"/checkUpdate\"><small><small>Check for Update</small></small></a>&nbsp;&nbsp;&nbsp;&nbsp;<a href=\"/mounts\"><small><small>Mounts</small></small></a>&nbsp;&nbsp;&nbsp;&nbsp;<a href=\"/network\"><small><small>Network</small></small></a>&nbsp;&nbsp;&nbsp;&nbsp;<a href=\"/security\"><small><small>Security</small></small></a>&nbsp;&nbsp;&nbsp;&nbsp;<a href=\"/setup\"><small><small>Setup</small></small></a></div>";
const std::string WebContent::MENU3 = "<div class=\"menu2\"></a>&nbsp;&nbsp;&nbsp;&nbsp;<a href=\"/writePhotos\"><small><small>Capture Photos</small></small></a></div>";


const std::string WebContent::SERIAL_NUMBER_DESCR = "The serial number or license key of the ATLAS 3D scanner";
const std::string WebContent::CAMERA_X_DESCR = "X-compoment of camera location. ie: The camera is always at X = 0.";
const std::string WebContent::CAMERA_Y_DESCR = "Y-component of camera location. ie: The distance from camera center to the XZ plane";
const std::string WebContent::CAMERA_Z_DESCR = "Z-component of camera location. ie: The distance from camera center to origin";
const std::string WebContent::RIGHT_LASER_X_DESCR = "X coordinate of the right laser.  This is how far the laser is from the camera.";
const std::string WebContent::RIGHT_LASER_Y_DESCR = "Y coordinate of the right laser";
const std::string WebContent::RIGHT_LASER_Z_DESCR = "Z coordinate of the right laser.  The laser is parallel with the camera focal point";
const std::string WebContent::LEFT_LASER_X_DESCR = "X coordinate of the right laser.  This is how far the laser is from the camera.";
const std::string WebContent::LEFT_LASER_Y_DESCR = "Y coordinate of the right laser";
const std::string WebContent::LEFT_LASER_Z_DESCR = "Z coordinate of the right laser.  The laser is parallel with the camera focal point";
const std::string WebContent::LASER_MAGNITUDE_THRESHOLD_DESCR = "How bright the difference between a laser on and laser off pixel must be in order to be detected as part of the laser";
const std::string WebContent::RIGHT_LASER_PIN_DESCR = "The wiringPi pin number for controlling the right laser";
const std::string WebContent::LEFT_LASER_PIN_DESCR = "The wiringPi pin number for controlling the left laser";
const std::string WebContent::LASER_ON_VALUE_DESCR = "Set to 1 if setting the laser pin HIGH turns if on and 0 if LOW turns it on";
const std::string WebContent::STABILITY_DELAY_DESCR = "The time in microseconds to delay after moving the turntable and before taking a picture";
const std::string WebContent::MAX_LASER_WIDTH_DESCR = "Maximum laser width in pixels";
const std::string WebContent::MIN_LASER_WIDTH_DESCR = "Minimum laser width in pixels";
const std::string WebContent::STEPS_PER_REVOLUTION_DESCR = "The number of motor steps before the turntable spins 360 degrees";
const std::string WebContent::ENABLE_PIN_DESCR = "The wiringPi pin number for enabling the stepper motor";
const std::string WebContent::STEP_PIN_DESCR = "The wiringPi pin number for stepping the stepper motor";
const std::string WebContent::STEP_DELAY_DESCR = "The amount of time between steps in microseconds";
const std::string WebContent::DIRECTION_PIN_DESCR = "The wiringPi pin number for the stepper motor direction or rotation";
const std::string WebContent::RESPONSE_DELAY_DESCR = "The time it takes for the stepper controller to recognize a pin value change in microseconds";
const std::string WebContent::FRAMES_PER_REVOLUTION_DESCR = "The number of frames that should be taken for a scan. Default is 800.";
const std::string WebContent::GENERATE_XYZ_DESCR = "Whether to generate an XYZ point cloud file from the scan.";
const std::string WebContent::GENERATE_STL_DESCR = "Whether to generate an STL mesh from the scan.";
const std::string WebContent::GENERATE_PLY_DESCR = "Whether to generate a PLY point clould from the scan.";
const std::string WebContent::SEPARATE_LASERS_BY_COLOR_DESCR = "Calibration debugging option to separate the results from different lasers by color (requires PLY).";
const std::string WebContent::GROUND_PLANE_HEIGHT_DESCR = "Any scan data less than this height above the turntable will not be included in the output files.";
const std::string WebContent::PLY_DATA_FORMAT_DESCR = "Whether to generate binary or ASCII PLY files.";
const std::string WebContent::ENABLE_BURST_MODE_DESCR = "Enables the camera's burst mode when capturing in still mode";
const std::string WebContent::ENABLE_LIGHTING_DESCR = "Enables support for controlling a connected light.";
const std::string WebContent::LIGHTING_PIN_DESCR = "The wiringPi pin number for the light. Change will not go into effect until system is rebooted.";
const std::string WebContent::CREATE_BASE_FOR_OBJECT_DESCR = "Adds a flat base to the object for easier 3D printing preparation.";
const std::string WebContent::WIFI_ESSID_DESCR = "The wireless network to configure";
const std::string WebContent::WIFI_PASSWORD_DESCR = "The password for the wireless network";
const std::string WebContent::ENABLE_AUTHENTICATION_DESCR = "Enables password protection for accessing the scanner";
const std::string WebContent::AUTH_USERNAME_DESCR = "The username to login with";
const std::string WebContent::AUTH_PASSWORD1_DESCR = "The password to login with";
const std::string WebContent::AUTH_PASSWORD2_DESCR = "Repeat the password";
const std::string WebContent::NOISE_REMOVAL_SETTING_DESCR = "Controls how aggressively noise should be removed from the point cloud";
const std::string WebContent::IMAGE_THRESHOLD_MODE_DESCR = "Controls how the laser line is detected in the image.<br>Static is the old method and the threshold must be given below.<br>The new adaptive mode is enabled by selecting low, medium, or high and does not require a threshold.";
const std::string WebContent::CAMERA_EXPOSURE_TIME_DESCR = "Controls how long the shutter stays open for each picture. Default is Auto";
const std::string WebContent::ENABLE_USB_NETWORK_CONFIG_DESCR = "Enables the ability to configure the network via USB flash drives.";
const std::string WebContent::MOUNT_PASSWORD_DESCR = "The user's password on the server.";
const std::string WebContent::MOUNT_USERNAME_DESCR = "The user's username on the server.";
const std::string WebContent::MOUNT_SERVERPATH_DESCR = "The source SMB mount path. Ex: \\\\server\\files";
const std::string WebContent::MOUNT_WORKGROUP_DESCR = "The workgroup on the server.";
const std::string WebContent::PHOTO_MOUNT_DESCR = "The location to write the photos.";
const std::string WebContent::PHOTO_WRITE_LASER_IMAGES_DESCR = "Whether the laser on pictures should be taken or not.";
const std::string WebContent::GPU_MEMORY_DESCR = "Changes the amount of memory dedicated to the GPU.  Reboot is required.";
const std::string WebContent::DISABLE_CAMERA_LED_DESCR = "Disables the LED light on the camera.  Reboot is required.";
const std::string WebContent::ENABLE_EXPERIMENTAL_DESCR = "Allows access to the experimental features.";
const std::string WebContent::RESTORE_DEFAULTS_DESCR = "Restore the default settings for the scanner?";
const std::string WebContent::ENABLE_WEBGL_DESCR = "Enable WebGL when available.";
const std::string WebContent::ENABLE_POINT_CLOUD_RENDERER_DESCR = "Enables the 2D point cloud renderer";
const std::string WebContent::OVERRIDE_FOCAL_LENGTH_DESCR = "Overrides the camera's focal length with the value given below.";
const std::string WebContent::OVERRIDDEN_FOCAL_LENGTH_DESCR = "The value override the camera's focal length with.  This is always in millimters.";
const std::string WebContent::FLIP_RED_BLUE_DESCR = "Flips the red and blue channels in the image.";
const std::string WebContent::MAX_OBJECT_SIZE_DESCR = "The maximum size object that can be scanned.";

std::string WebContent::scan(const std::vector<ScanResult>& pastScans)
{
	const Preset& preset = PresetManager::get()->getActivePreset();
	Setup * setup = Setup::get();
	std::string presetName = preset.name;

	std::stringstream sstr;
	sstr << "<!DOCTYPE html><html><head>"
		 << CSS
		 << std::endl
		 << JAVASCRIPT
		 << std::endl
		 << "</head>"
	     << "\
<body>\
<div id=\"menuContainer\">\
  <div class=\"menu\"><a href=\"/\">SCAN</a></div>\
  <div class=\"menu\"><a href=\"/cal1\">CAMERA</a></div>\
  <div class=\"menu\"><a href=\"/settings\">SETTINGS</a></div>\
</div>\
<p>Click the button to start the scan </p>\
<form action=\"/\" method=\"POST\" enctype=\"application/x-www-form-urlencoded\">\
<div><div class=\"settingsText\">Preset</div><div><small>" << presetName << "</small></div></div>\
<div><div class=\"settingsText\">Range</div><input name=\"degrees\" class=\"settingsInput\" value=\"360\"> degrees</div>\
	<input type=\"hidden\" name=\"cmd\" value=\"startScan\">\
	<input class=\"submit\" type=\"submit\" value=\"Start Scan\">\
</form>";

	int diskSpaceMb = GetFreeSpaceMb();
	if (diskSpaceMb < WebContent::LOW_DISK_SPACE_MB)
	{
		sstr << "<div id=\"error1\">LOW DISK SPACE WARNING: " << diskSpaceMb << " MB</div>";
	}

	for (size_t iRt = 0; iRt < pastScans.size(); iRt++)
	{
		sstr << WebContent::scanResult(iRt + 1, pastScans[iRt], setup->enablePointCloudRenderer);
	}

	sstr << "</body></html>";

	return sstr.str();
}

std::string WebContent::writePhotos(const std::vector<std::string>& mountPaths)
{
	std::stringstream sstr;
	sstr << "<!DOCTYPE html><html><head>"
		 << CSS
		 << std::endl
		 << JAVASCRIPT
		 << std::endl
		 << "</head>"
		 << "\
<body>\
<div id=\"menuContainer\">\
  <div class=\"menu\"><a href=\"/\">SCAN</a></div>\
  <div class=\"menu\"><a href=\"/cal1\">CAMERA</a></div>\
  <div class=\"menu\"><a href=\"/settings\">SETTINGS</a></div>\
  </div><div style=\"padding-top: 50px\">";

	if (mountPaths.empty())
	{
		sstr << "<h2>No valid mount paths detected.</h2>";
		sstr << "</div> </body></html>";
		return sstr.str();
	}

	sstr << "<form action=\"/writePhotos\" method=\"POST\" enctype=\"application/x-www-form-urlencoded\">";

	sstr << "<div><div class=\"settingsText\">Mount Path</div>";
	sstr << "<select name=\"" << WebContent::PHOTO_MOUNT << "\">";
	for (size_t i = 0; i < mountPaths.size(); i++)
	{
		sstr << "<option value=\"" << i << "\">" << mountPaths[i] << "</option>\r\n";
	}
	sstr << "</select></div>";
	sstr << "<div class=\"settingsDescr\">" << PHOTO_MOUNT_DESCR << "</div>\n";

	sstr << checkbox(WebContent::PHOTO_WRITE_LASER_IMAGES, "Write Laser Images", false, PHOTO_WRITE_LASER_IMAGES_DESCR);
	sstr << setting(WebContent::PROFILE_NAME, "Profile", PresetManager::get()->getActivePreset().name, "The profile to generate the photos with", "", true);
	sstr << "<p><input type=\"hidden\" name=\"cmd\" value=\"writePhotos\">\
<input class=\"submit\" type=\"submit\" value=\"Generate Photos\">\
</p></form></div>\
</body></html>";

	return sstr.str();
}

std::string WebContent::scanResult(size_t index, const ScanResult& result, bool usePointCloudRenderer)
{
	std::stringstream sstr;

	time_t scanDate = result.getScanDate();

	struct tm * tp = localtime (&scanDate);
	std::string dateStr = asctime(tp);
	std::stringstream deleteUrl;

	std::stringstream thumbnail;
	thumbnail << "/dl/" << result.getScanDate() << ".png";
	deleteUrl << "/del?id=" << result.getScanDate();

	sstr << "<span>" << index << "\
	<div class=\"result\">\
	<div style=\"float: left\">\
	<img width=\"128\" border=\"0\" src=\"" << thumbnail.str() << "\">\
	</div>";

	bool hasPly = false;
	const std::vector<ScanResultFile>& files = result.files;
	for (size_t iFil = 0; iFil < files.size(); iFil++)
	{
		const ScanResultFile& file = files[iFil];
		if (file.extension != "png")
		{
			std::string upperExtension = file.extension;
			std::transform(upperExtension.begin(), upperExtension.end(), upperExtension.begin(), toupper);

			// Format the file size
			std::stringstream fileSize;
			if (file.fileSize > 1024 * 1024)
			{
				fileSize << file.fileSize / (1024 * 1024) << " MB";
			}
			else if (file.fileSize > 1024)
			{
				fileSize << file.fileSize / 1024 << " KB";
			}
			else
			{
				fileSize << file.fileSize << " B";
			}

			sstr << "&nbsp;<a href=\"/dl/" << result.getScanDate()
				 << "." << file.extension << "\">"
				 << upperExtension << "</a><small>&nbsp;["
				 << fileSize.str() << "]</small>";
		}

		if (file.extension == "ply")
		{
			hasPly = true;
		}
	}

	sstr << "<br><br>\
	<div id=\"deleteButton\"><form action=\"" << deleteUrl.str() << "#\" method=\"POST\">\
	<input type=\"Submit\" value=\"Delete\">\
	</form></div>";

	// Show the PLY view link
	if (hasPly)
	{
		std::string action = usePointCloudRenderer ? "/view" : "/viewPly";

		sstr << "<div id=\"viewButton\"><form action=\"" << action << "\" method=\"POST\" enctype=\"application/x-www-form-urlencoded\">"
			 << "<input type=\"Hidden\" name=\"id\" value=\"" << result.getScanDate() << "\">"
			 << "<input style=\"left-padding: 20px\" type=\"Submit\" value=\"View\">"
			 << "</form></div>";
	}

	sstr << "<div style=\"font-size: 12px\">" << dateStr << "\
	</div></div>\
	</span>\
	<br>";

	return sstr.str();
}

std::string WebContent::scanRunning(Progress& progress, real remainingTime, Scanner::Task task,
		                            const std::string& url, bool showPointCloudRenderer,
		                            const std::string& rotation, const std::string& pixelRadius)
{
	real minRemaining = remainingTime / 60.0;

	std::string taskNoun;
	if (task == Scanner::GENERATE_PHOTOS)
	{
		taskNoun = "Generating Photos";
	}
	else
	{
		taskNoun = "Scanning";
	}

	bool showPreview = (task == Scanner::GENERATE_SCAN);

	std::stringstream sstr;
	sstr << "<!DOCTYPE html><html><head>"
		 << "<meta http-equiv=\"refresh\" content=\"5\">"
		 << CSS
		 << std::endl
		 << JAVASCRIPT
		 << std::endl
		 << "</head>"
	     << "<body><p><div style=\"float: left\">"
	     << progress.getLabel()
	     << " is "
	     << progress.getPercent()
	     << "% complete";

		if (minRemaining > 0.01)
		{
			 sstr << " with "
				  << minRemaining
				  << " minutes remaining";
		}

	sstr << ".&nbsp;</div>";

	if (showPreview)
	{
		sstr << "<div style=\"padding-left: 50px\"><form action=\"/preview\" method=\"GET\" enctype=\"application/x-www-form-urlencoded\">"
			 << "<input type=\"submit\" value=\"Preview 3D\">"
			 << "</form></div><br><br>";
	}

	sstr << "<form action=\"/\" method=\"POST\" enctype=\"application/x-www-form-urlencoded\">"
	     << "<input type=\"hidden\" name=\"cmd\" value=\"stopScan\">"
	     << "<input type=\"submit\" value=\"Stop " << taskNoun << "\">"
	     << "</form>";

	if (showPreview && showPointCloudRenderer)
	{
		sstr << renderImageControls(url, rotation, pixelRadius);
	}

	sstr << "</body></html>";

	return sstr.str();
}


std::string WebContent::viewScanRender(int scanId, const std::string& url, const std::string& rotation, const std::string& pixelRadius)
{
	std::stringstream sstr;
	sstr << "<!DOCTYPE html><html><head>"
		 << CSS
		 << std::endl
		 << JAVASCRIPT
		 << std::endl
		 << "\
		 </head><body>";

	sstr << "<div style=\"float: left\"><a href=\"/\">Back</a></div><div style=\"padding-left: 75px\"><form action=\"/viewPly\" method=\"GET\" enctype=\"application/x-www-form-urlencoded\">"
		 << "<input type=\"hidden\" name=\"id\" value=\"" << scanId << "\">"
		 << "<input type=\"submit\" value=\"View 3D\">"
		 << "</form></div><br><br>";

	sstr << renderImageControls(url, rotation, pixelRadius, scanId);

	sstr << "</body></html>";

	return sstr.str();
}

std::string WebContent::renderImageControls(const std::string& url, const std::string& inRotation, const std::string& inPixelRadius, int scanId)
{
	std::string rotation = inRotation.empty() ? "0" : inRotation;
	std::string pixelRadius = inPixelRadius.empty() ? "1" : inPixelRadius;

	std::stringstream sstr;
	sstr << "<div>";
	sstr << "<div style=\"float: left; height: 1000px\">";
	sstr << "<div style=\"padding-top: 20px\">View</div>";
	sstr << "<a href=\"" << url << "?" << WebContent::ROTATION << "=0&" << WebContent::PIXEL_RADIUS << "=" << pixelRadius << "&id=" << scanId << "\"> Front </a><br>";
	sstr << "<a href=\"" << url << "?" << WebContent::ROTATION << "=90&" << WebContent::PIXEL_RADIUS << "=" << pixelRadius << "&id=" << scanId << "\"> Right </a><br>";
	sstr << "<a href=\"" << url << "?" << WebContent::ROTATION << "=180&" << WebContent::PIXEL_RADIUS << "=" << pixelRadius << "&id=" << scanId << "\"> Back </a><br>";
	sstr << "<a href=\"" << url << "?" << WebContent::ROTATION << "=270&" << WebContent::PIXEL_RADIUS << "=" << pixelRadius << "&id=" << scanId << "\"> Left </a><br>";

	sstr << "<div style=\"padding-top: 20px\">Point Size</div>";
	sstr << "<a href=\"" << url << "?" << WebContent::ROTATION << "=" << rotation << "&" << WebContent::PIXEL_RADIUS << "=0&id=" << scanId << "\"> Small </a><br>";
	sstr << "<a href=\"" << url << "?" << WebContent::ROTATION << "=" << rotation << "&" << WebContent::PIXEL_RADIUS << "=1&id=" << scanId << "\"> Medium </a><br>";
	sstr << "<a href=\"" << url << "?" << WebContent::ROTATION << "=" << rotation << "&" << WebContent::PIXEL_RADIUS << "=2&id=" << scanId << "\"> Large </a><br>";
	sstr << "<a href=\"" << url << "?" << WebContent::ROTATION << "=" << rotation << "&" << WebContent::PIXEL_RADIUS << "=3&id=" << scanId << "\"> Very Large </a><br>";
	sstr << "</div>";

	sstr << "<div><img id=\"image\" width=\"800\" src=\"renderImage_" << time(NULL) << ".jpg?a=0";

	if (scanId != -1)
	{
		sstr << "&" << WebContent::ID << "=" << scanId;
	}

	if (!rotation.empty())
	{
		sstr << "&" << WebContent::ROTATION << "=" << rotation;
	}

	if (!pixelRadius.empty())
	{
		sstr << "&" << WebContent::PIXEL_RADIUS << "=" << pixelRadius;
	}

	sstr << "\"></div>";

	return sstr.str();
}

std::string WebContent::viewScan(const std::string& plyFilename)
{
	int canvasVertexSkip = 50;
	int detail = 100 / canvasVertexSkip;
	bool useWebGL = Setup::get()->enableWebGLWhenAvailable;

	std::stringstream sstr;
	sstr << "<!DOCTYPE html><html><head>"
		 << CSS
		 << std::endl
		 << JAVASCRIPT
		 << std::endl
		 << "\
		 </head><body>\
		 <div style=\"position: absolute; left: 10px; top: 10px\"> <a href=\"/\">Back</a>&nbsp;&nbsp;<a href=\"/preview\">Refresh</a><div id=\"detail\"></div></div>\
		 <script>";

	sstr << "var canvasVertexSkip = " << canvasVertexSkip << ";";
	sstr << "var plyFilename = '" << plyFilename << "';";
	sstr << "var useWebGL = " << (useWebGL ? "true" : "false") << ";";

	sstr << "</script>\
		 <script src=\"three.min.js\"></script>\
		 <script src=\"OrbitControls.js\"></script>\
		 <script src=\"PLYLoader.js\"></script>\
		 <script src=\"Projector.js\"></script>\
	     <script src=\"CanvasRenderer.js\"></script>\
	     <script src=\"SoftwareRenderer.js\"></script>\
		 <script src=\"showScan.js\"></script>";

	sstr << "<script>"
		 << "if (!useWebGL) { document.getElementById('detail').innerHTML = 'Displaying " << detail << "% detail';}"
		 << "</script>";
	sstr << "</body></html>";

	return sstr.str();
}

std::string WebContent::cal1(const std::string& inMessage)
{
	std::string message = inMessage.empty() ? std::string("") : ("<h2>" + inMessage + "</h2>");
	Setup * setup = Setup::get();
	const Preset& preset = PresetManager::get()->getActivePreset();

	std::string exposureStr;
	switch (preset.cameraExposureTime)
	{
	case CET_AUTO:
		exposureStr = "Auto";
		break;

	case CET_VERYSHORT:
		exposureStr = "Very Short";
		break;

	case CET_SHORT:
		exposureStr = "Short";
		break;

	case CET_MEDIUM:
		exposureStr = "Medium";
		break;

	case CET_LONG:
		exposureStr = "Long";
		break;

	case CET_VERYLONG:
		exposureStr = "Very Long";
		break;

	default:
		exposureStr = "UNSUPPORTED EXPOSURE SETTING";
		break;
	}

	std::stringstream sstr;
	sstr << "<!DOCTYPE html><html><head>"
		 << CSS
		 << std::endl
		 << JAVASCRIPT
		 << std::endl
		 << "</head>"
	     << "\
<body onload=\"startup1();\">\
<div id=\"menuContainer\">\
  <div class=\"menu\"><a href=\"/\">SCAN</a></div>\
  <div class=\"menu\"><a href=\"/cal1\">CAMERA</a></div>\
  <div class=\"menu\"><a href=\"/settings\">SETTINGS</a></div>\
</div>";

	if (setup->enableExperimental)
	{
	     sstr << WebContent::MENU3;
	}

	sstr << message
         << "\
<div id=\"cal1ContentDiv\">\
  <div id=\"cal1ControlsDiv\">\
    <div style=\"margin-bottom: 50px\">\
         <div><small>Exposure: " << exposureStr << "</small></div>";

	sstr << exposureDiv((int)CET_AUTO, "Auto", false);
	sstr << exposureDiv((int)CET_VERYSHORT, "Very Short", false);
	sstr << exposureDiv((int)CET_SHORT, "Short", false);
	sstr << exposureDiv((int)CET_MEDIUM, "Medium", false);
	sstr << exposureDiv((int)CET_LONG, "Long", false);
	sstr << exposureDiv((int)CET_VERYLONG, "Very Long", false);

    sstr << "</div>\
    <div class=\"cal1GenerateDebugDiv\">\
		<form action=\"/cal1\" method=\"POST\" enctype=\"application/x-www-form-urlencoded\">\
			<input name=\"cmd\" value=\"generateDebug\" type=\"hidden\">\
			<input style=\"height: 60px\" class=\"controlSubmit\" value=\"Test\" type=\"submit\">\
		</form>\
    </div>\
    <div class=\"cal1GenerateDebugDiv\">\
        <form action=\"/cal1\" method=\"POST\" enctype=\"application/x-www-form-urlencoded\"><input name=\"cmd\" value=\"calibrateLasers\" type=\"hidden\"><input class=\"controlSubmit\" value=\"Calibrate Lasers\" type=\"submit\"></form>\
        <div class=\"calDescr\">Line the front wall of the calibration item over the center of the turntable hole and click this button to calibrate the lasers.</div>\
    </div>";

	if (setup->enableLighting)
	{
		sstr << "\
		<div>\
		<form action=\"/cal1\" method=\"POST\" enctype=\"application/x-www-form-urlencoded\">\
		<input name=\"cmd\" value=\"setLightIntensity\" type=\"hidden\">\
		<input name=\"intensity\" style=\"width: 75px\" class=\"settingsInput\" value=\"" << Lighting::get()->getIntensity() << "\" type=\"text\">\
		<input class=\"submit\" style=\"width: 110px\" value=\"Set Light\" type=\"submit\">\
		</form>\
		</div>\
		<div class=\"calDescr\">Sets the lighting intensity - 0 (off) to 100 (full).</div>";
	}

	sstr << "\
	<form action=\"/cal1\" method=\"POST\" enctype=\"application/x-www-form-urlencoded\"><input name=\"cmd\" value=\"toggleLeftLaser\" type=\"hidden\"><input class=\"controlSubmit\" value=\"Toggle Left Laser\" type=\"submit\"></form>\
	<form action=\"/cal1\" method=\"POST\" enctype=\"application/x-www-form-urlencoded\"> <input name=\"cmd\" value=\"toggleRightLaser\" type=\"hidden\"> <input class=\"controlSubmit\" value=\"Toggle Right Laser\" type=\"submit\"> </form>\
	<form action=\"/cal1\" method=\"POST\" enctype=\"application/x-www-form-urlencoded\">  <input name=\"cmd\" value=\"disableMotor\" type=\"hidden\">  <input class=\"controlSubmit\" value=\"Disable Motor\" type=\"submit\">  </form>\
	<div>\
		<form action=\"/cal1\" method=\"POST\" enctype=\"application/x-www-form-urlencoded\">\
		<input name=\"cmd\" value=\"rotateTable\" type=\"hidden\">\
		<input name=\"degrees\" style=\"width: 75px\" class=\"settingsInput\" value=\"360\" type=\"text\">\
		<input class=\"submit\" style=\"width: 110px\" value=\"Rotate\" type=\"submit\">\
		</form>\
	</div>\
	<div class=\"calDescr\">Rotates the turntable in degrees.</div>\
  </div>\
<div style=\"position: relative\">\
	<div>\
		<img style=\"position: absolute\" onload=\"setTimeout(updateImgImage, 1000);\" id=\"image\" width=\"1296\" src=\"camImageL_"
	<< time(NULL)
	<< ".jpg\">\
	</div>\
</div>\
</div></body></html>";

	     return sstr.str();
}

std::string WebContent::menu2()
{
	return Setup::get()->enableExperimental ? WebContent::MENU2_EXPERIMENTAL : WebContent::MENU2;
}

std::string WebContent::showUpdate(SoftwareUpdate * update, const std::string& message)
{
	std::stringstream sstr;
	sstr << "<!DOCTYPE html><html><head>"
		 << CSS
		 << std::endl
		 << JAVASCRIPT
		 << std::endl
		 << "</head>"
		 << "\
<body>\
<div id=\"menuContainer\">\
  <div class=\"menu\"><a href=\"/\">SCAN</a></div>\
  <div class=\"menu\"><a href=\"/cal1\">CAMERA</a></div>\
  <div class=\"menu\"><a href=\"/settings\">SETTINGS</a></div>"
		 << menu2()
		 << "\
</div><div style=\"padding-top: 50px\">";

	if (update != NULL)
	{
		sstr << "<p>Update Available: " << update->name << "</p>";
		sstr << "<div><form action=\"/applyUpdate\" method=\"POST\" enctype=\"application/x-www-form-urlencoded\">";
		sstr <<" <input type=\"submit\" name=\"applyUpdate\" value=\"Apply Update\">";
		sstr <<" <input type=\"hidden\" name=\"majorVersion\" value=\"" << update->majorVersion << "\">";
		sstr <<" <input type=\"hidden\" name=\"minorVersion\" value=\"" << update->minorVersion << "\">";
		sstr << "</form></div>";
	}
	else if (!message.empty())
	{
		sstr << "<h2>" << message << "</h2>";
	}
	else
	{
		sstr << "<h2>No updates available. You are already running the latest version.</h2>";
	}

	sstr << "</div></body></html>";

	return sstr.str();
}

std::string WebContent::updateApplied(SoftwareUpdate * update, const std::string& message, bool success)
{
	std::stringstream sstr;
	sstr << "<!DOCTYPE html><html><head>"
		 << CSS
		 << std::endl
		 << JAVASCRIPT
		 << std::endl
		 << "</head>"
		 << "\
<body>\
<div id=\"menuContainer\">\
  <div class=\"menu\"><a href=\"/\">SCAN</a></div>\
  <div class=\"menu\"><a href=\"/cal1\">CAMERA</a></div>\
  <div class=\"menu\"><a href=\"/settings\">SETTINGS</a></div>"
		 << menu2()
		 << "\
</div><div style=\"padding-top: 50px\">";

	sstr << "<h2>" << message << "</h2>";

	sstr << "</div></body></html>";

	return sstr.str();
}

std::string WebContent::restoreDefaults()
{
	std::stringstream sstr;
	sstr << "<!DOCTYPE html><html><head>"
			 << CSS
			 << std::endl
			 << JAVASCRIPT
			 << std::endl
			 << "</head>"
			 << "\
	<body>\
	<div id=\"menuContainer\">\
	  <div class=\"menu\"><a href=\"/\">SCAN</a></div>\
	  <div class=\"menu\"><a href=\"/cal1\">CAMERA</a></div>\
	  <div class=\"menu\"><a href=\"/settings\">SETTINGS</a></div>"
			 << menu2()
			 << "\
	</div><div style=\"padding-top: 50px\">";

		sstr << "<h2>" << RESTORE_DEFAULTS_DESCR << "</h2>";
		sstr << "<form action=\"/setup\" method=\"POST\" enctype=\"application/x-www-form-urlencoded\">";
		sstr << "<input type=\"hidden\" name=\"cmd\" value=\"" << WebContent::RESTORE_DEFAULTS << "\">";
		sstr << "<input class=\"submit\" type=\"submit\" value=\"Restore Defaults\">";
		sstr << "</form>";
		sstr << "</div></body></html>";
	return sstr.str();
}

std::string WebContent::security(const std::string& message)
{
	Setup * setup = Setup::get();

	std::stringstream sstr;
	sstr << "<!DOCTYPE html><html><head>"
		 << CSS
		 << std::endl
		 << JAVASCRIPT
		 << std::endl
		 << "</head>"
		 << "\
<body>\
<div id=\"menuContainer\">\
  <div class=\"menu\"><a href=\"/\">SCAN</a></div>\
  <div class=\"menu\"><a href=\"/cal1\">CAMERA</a></div>\
  <div class=\"menu\"><a href=\"/settings\">SETTINGS</a></div>"
		 << menu2()
		 << "\
</div><div style=\"padding-top: 50px\">";

	if (!message.empty())
	{
		sstr << "<h2>" << message << "</h2>";
	}

	sstr << "<form action=\"/security\" method=\"POST\" enctype=\"application/x-www-form-urlencoded\">";
	sstr << checkbox(WebContent::ENABLE_AUTHENTICATION, "Require Password", setup->enableAuthentication, ENABLE_AUTHENTICATION_DESCR);
	sstr << setting(WebContent::AUTH_USERNAME, "Username", "pi", AUTH_USERNAME_DESCR,  "", true);
	sstr << setting(WebContent::AUTH_PASSWORD1, "Password", "", AUTH_PASSWORD1_DESCR,  "", false, true);
	sstr << setting(WebContent::AUTH_PASSWORD2, "Repeat Password", "", AUTH_PASSWORD2_DESCR,  "", false, true);
	sstr << "<p><input type=\"hidden\" name=\"cmd\" value=\"save\">\
<input class=\"submit\" type=\"submit\" value=\"Save\">\
</p></form></div>\
</body></html>";

	return sstr.str();
}

std::string WebContent::mounts(const std::string& message, const std::vector<std::string>& mountPaths)
{
	std::stringstream sstr;
	sstr << "<!DOCTYPE html><html><head>"
		 << CSS
		 << std::endl
		 << JAVASCRIPT
		 << std::endl
		 << "</head>"
		 << "\
<body>\
<div id=\"menuContainer\">\
  <div class=\"menu\"><a href=\"/\">SCAN</a></div>\
  <div class=\"menu\"><a href=\"/cal1\">CAMERA</a></div>\
  <div class=\"menu\"><a href=\"/settings\">SETTINGS</a></div>"
		 << menu2()
		 << "\
</div><div style=\"padding-top: 50px\">";

	if (!message.empty())
	{
		sstr << "<h2>" << message << "</h2>";
	}

	if (mountPaths.empty())
	{
		sstr << "<p>No mounted paths detected.</p><ol>";
	}
	else
	{
		sstr << "<p>Mounted Paths</p><ol>";
	}

	for (size_t i = 0; i < mountPaths.size(); i++)
	{
		sstr << "<form action=\"/mounts\" method=\"POST\" enctype=\"application/x-www-form-urlencoded\">"
			 << "<div style=\"float: left\"><li>" << mountPaths[i]
		     << "<input type=\"hidden\" name=\"cmd\" value=\"unmount\">"
		     << "<input type=\"hidden\" name=\"" << WebContent::MOUNT_INDEX << "\" value=\"" << i << "\">"
		     << "</li></div><div>"
		     << "<input class=\"submitSmall\" type=\"submit\" value=\"Unmount\">"
		     << "</div></form>";
	}
	sstr << "</ol>";

	sstr << "<form action=\"/mounts\" method=\"POST\" enctype=\"application/x-www-form-urlencoded\">";

	sstr << setting(WebContent::MOUNT_SERVERPATH, "Server Path", "", MOUNT_SERVERPATH_DESCR);
	sstr << setting(WebContent::MOUNT_USERNAME, "Username", "", MOUNT_USERNAME_DESCR);
	sstr << setting(WebContent::MOUNT_PASSWORD, "Password", "", MOUNT_PASSWORD_DESCR, "", false, true);
	sstr << setting(WebContent::MOUNT_WORKGROUP, "Workgroup", "", MOUNT_WORKGROUP_DESCR);


	sstr << "<p><input type=\"hidden\" name=\"cmd\" value=\"mount\">\
<input class=\"submit\" type=\"submit\" value=\"Mount\">\
</p></form></div>\
</body></html>";

	return sstr.str();
}

std::string WebContent::network(const std::string& message, bool hiddenEssidInput)
{
	WifiConfig * wifi = WifiConfig::get();

	//  Perform a scan
	if (!hiddenEssidInput)
	{
		wifi->scan();
	}

	std::vector<std::string> interfaces = wifi->getAllInterfaces();
	std::vector<WifiConfig::AccessPoint> accessPoints = wifi->getAccessPoints();

	std::stringstream sstr;
	sstr << "<!DOCTYPE html><html><head>"
		 << CSS
		 << std::endl
		 << JAVASCRIPT
		 << std::endl
		 << "</head>"
		 << "\
<body>\
<div id=\"menuContainer\">\
  <div class=\"menu\"><a href=\"/\">SCAN</a></div>\
  <div class=\"menu\"><a href=\"/cal1\">CAMERA</a></div>\
  <div class=\"menu\"><a href=\"/settings\">SETTINGS</a></div>"
		 << menu2()
		 << "\
</div><div style=\"padding-top: 50px\">";

	if (!message.empty())
	{
		sstr << "<h2>" << message << "</h2>";
	}

	sstr << "<form action=\"/network\" method=\"POST\" enctype=\"application/x-www-form-urlencoded\">";

	for (size_t iNt = 0; iNt < interfaces.size(); iNt++)
	{
		std::string interface = interfaces[iNt];
		if (interface != "lo")
		{
			sstr << setting(interface, interface, wifi->getIpAddress(interface), "IP address for " + interface, "", true);
		}
	}

	if (!hiddenEssidInput)
	{
		sstr << "<div><div class=\"settingsText\">Wireless Network</div>";
		sstr << "<select name=\"" << WebContent::WIFI_ESSID << "\">";
		for (size_t iAp = 0; iAp < accessPoints.size(); iAp++)
		{
			sstr << "<option value=\"" << iAp << "\">" << accessPoints[iAp].essid << "</option>\r\n";
		}

		sstr << "</select>&nbsp;&nbsp;<a href=\"?hidden=true\"><small><small>Hidden</small></small></a></div>";

		sstr << "";
		sstr << "<div class=\"settingsDescr\">" << WIFI_ESSID_DESCR << "</div>\n";
	}
	else
	{
		sstr << setting(WebContent::WIFI_ESSID_HIDDEN, "Wireless Network", "", WIFI_ESSID_DESCR);
	}

	sstr << setting(WebContent::WIFI_PASSWORD, "Wireless Password", "", WIFI_PASSWORD_DESCR, "", false, true);
	sstr << "<p><input type=\"hidden\" name=\"cmd\" value=\"connect\">\
<input class=\"submit\" type=\"submit\" value=\"Connect\">\
</p></form></div>\
</body></html>";

	return sstr.str();
}

std::string WebContent::setup(const std::string& message)
{
	const Setup * setup = Setup::get();
	BootConfigManager::Settings bootConfig = BootConfigManager::get()->readSettings();

	UnitOfLength srcUnit = UL_MILLIMETERS; // Lengths are always represented in millimeters internally
	UnitOfLength dstUnit = setup->unitOfLength;

	// Detect the amount of free space available
	std::string freeSpaceMb = ToString(GetFreeSpaceMb());

	std::stringstream kernelVersion;
	struct utsname uts;

	if (uname(&uts) == 0)
	{
		kernelVersion << uts.sysname << " " << uts.release << " " << uts.version << " " << uts.machine;
	}

	std::stringstream sstr;
	sstr << "<!DOCTYPE html><html><head>"
		 << CSS
		 << std::endl
		 << JAVASCRIPT
		 << std::endl
		 << "</head>"
	     << "\
<body>\
<div id=\"menuContainer\">\
  <div class=\"menu\"><a href=\"/\">SCAN</a></div>\
  <div class=\"menu\"><a href=\"/cal1\">CAMERA</a></div>\
  <div class=\"menu\"><a href=\"/settings\">SETTINGS</a></div>"
	     << menu2()
	     << "\
</div><div style=\"padding-top: 50px\">";

	if (!message.empty())
	{
		sstr << "<h2>" << message << "</h2>";
	}

	sstr << "<form action=\"/setup\" method=\"POST\" enctype=\"application/x-www-form-urlencoded\">";
	sstr << "<p><input class=\"submit\" type=\"submit\" value=\"Save\"></p>";

	sstr << setting(WebContent::SERIAL_NUMBER, "Serial Number", setup->serialNumber, SERIAL_NUMBER_DESCR);

	std::string mmSel = setup->unitOfLength == UL_MILLIMETERS ? " SELECTED" : "";
	std::string cmSel = setup->unitOfLength == UL_CENTIMETERS ? " SELECTED" : "";
	std::string inSel = setup->unitOfLength == UL_INCHES ? " SELECTED" : "";

	sstr << "<div><div class=\"settingsText\">Unit of Length</div>";
	sstr << "<select name=\"" << WebContent::UNIT_OF_LENGTH << "\">";
	sstr << "<option value=\"1\"" << mmSel << ">Millimeters</option>\r\n";
	sstr << "<option value=\"3\"" << cmSel << ">Centimeters</option>\r\n";
	sstr << "<option value=\"2\"" << inSel << ">Inches</option>\r\n";
	sstr << "</select></div>";
	sstr << "<div class=\"settingsDescr\">The unit of length for future values entered on the setup page.</div>\n";

	sstr << setting(WebContent::CAMERA_SENSOR, "Camera Sensor", Camera::getInstance()->getName(), "The name of the detected camera","", true);
	sstr << setting(WebContent::CAMERA_X, "Camera X", ConvertUnitOfLength(setup->cameraLocation.x, srcUnit, dstUnit), CAMERA_X_DESCR, ToString(dstUnit) + ".", true);
	sstr << setting(WebContent::CAMERA_Y, "Camera Y", ConvertUnitOfLength(setup->cameraLocation.y, srcUnit, dstUnit), CAMERA_Y_DESCR,  ToString(dstUnit) + ".");
	sstr << setting(WebContent::CAMERA_Z, "Camera Z", ConvertUnitOfLength(setup->cameraLocation.z, srcUnit, dstUnit), CAMERA_Z_DESCR,  ToString(dstUnit) + ".");
	sstr << setting(WebContent::RIGHT_LASER_X, "Right Laser X", ConvertUnitOfLength(setup->rightLaserLocation.x, srcUnit, dstUnit), RIGHT_LASER_X_DESCR,  ToString(dstUnit) + ".");
	sstr << setting(WebContent::RIGHT_LASER_Y, "Right Laser Y", ConvertUnitOfLength(setup->rightLaserLocation.y, srcUnit, dstUnit), RIGHT_LASER_Y_DESCR,  ToString(dstUnit) + ".", true);
	sstr << setting(WebContent::RIGHT_LASER_Z, "Right Laser Z", ConvertUnitOfLength(setup->rightLaserLocation.z, srcUnit, dstUnit), RIGHT_LASER_Z_DESCR,  ToString(dstUnit) + ".", true);
	sstr << setting(WebContent::LEFT_LASER_X, "Left Laser X", ConvertUnitOfLength(setup->leftLaserLocation.x, srcUnit, dstUnit), LEFT_LASER_X_DESCR,  ToString(dstUnit) + ".");
	sstr << setting(WebContent::LEFT_LASER_Y, "Left Laser Y", ConvertUnitOfLength(setup->leftLaserLocation.y, srcUnit, dstUnit), LEFT_LASER_Y_DESCR,  ToString(dstUnit) + ".", true);
	sstr << setting(WebContent::LEFT_LASER_Z, "Left Laser Z", ConvertUnitOfLength(setup->leftLaserLocation.z, srcUnit, dstUnit), LEFT_LASER_Z_DESCR,  ToString(dstUnit) + ".", true);

	sstr << setting(WebContent::MAX_OBJECT_SIZE, "Max Object Size", ConvertUnitOfLength(setup->maxObjectSize, srcUnit, dstUnit), MAX_OBJECT_SIZE_DESCR,  ToString(dstUnit) + ".");

	sstr << setting(WebContent::RIGHT_LASER_PIN, "Right Laser Pin", setup->rightLaserPin, RIGHT_LASER_PIN_DESCR);
	sstr << setting(WebContent::LEFT_LASER_PIN, "Left Laser Pin", setup->leftLaserPin, LEFT_LASER_PIN_DESCR);
	sstr << setting(WebContent::LASER_ON_VALUE, "Laser On Value", setup->laserOnValue, LASER_ON_VALUE_DESCR);

	sstr << setting(WebContent::STEPS_PER_REVOLUTION, "Steps Per Revolution", setup->stepsPerRevolution, STEPS_PER_REVOLUTION_DESCR);
	sstr << setting(WebContent::ENABLE_PIN, "Motor Enable Pin", setup->motorEnablePin, ENABLE_PIN_DESCR);
	sstr << setting(WebContent::STEP_PIN, "Motor Step Pin", setup->motorStepPin, STEP_PIN_DESCR);
	sstr << setting(WebContent::STEP_DELAY, "Motor Step Delay", setup->motorStepDelay, STEP_DELAY_DESCR, "&mu;s");
	sstr << setting(WebContent::DIRECTION_PIN, "Motor Direction Pin", setup->motorDirPin, DIRECTION_PIN_DESCR);
	sstr << setting(WebContent::RESPONSE_DELAY, "Motor Response Delay", setup->motorResponseDelay, RESPONSE_DELAY_DESCR, "&mu;s");
	sstr << checkbox(WebContent::ENABLE_LIGHTING, "Enable Lighting", setup->enableLighting, ENABLE_LIGHTING_DESCR);
	sstr << setting(WebContent::LIGHTING_PIN, "Lighting Pin", setup->lightingPin, LIGHTING_PIN_DESCR);

	std::set<std::string> memoryValues;
	memoryValues.insert("128");
	memoryValues.insert("256");
	memoryValues.insert(ToString(bootConfig.gpuMemoryMb));
	sstr << "<div><div class=\"settingsText\">GPU Memory</div>";
	sstr << "<select name=\"" << WebContent::GPU_MEMORY << "\">";

	std::set<std::string>::iterator it = memoryValues.begin();
	while (it != memoryValues.end())
	{
		std::string memString = * it;
		std::string selString = ToString(bootConfig.gpuMemoryMb) == memString ? "SELECTED" : "";

		sstr << "<option value=\"" << memString << "\" " << selString << ">" << memString << " MB</option>\r\n";

		it++;
	}

	sstr << "</select></div>";
	sstr << "<div class=\"settingsDescr\">" << GPU_MEMORY_DESCR << "</div>\n";

	sstr << checkbox(WebContent::DISABLE_CAMERA_LED, "Disable Camera LED", bootConfig.ledDisabled, DISABLE_CAMERA_LED_DESCR);
	sstr << checkbox(WebContent::ENABLE_USB_NETWORK_CONFIG, "Setup Network via USB", setup->enableUsbNetworkConfig, ENABLE_USB_NETWORK_CONFIG_DESCR);
	sstr << checkbox(WebContent::ENABLE_WEBGL, "Enable WebGL", setup->enableWebGLWhenAvailable, ENABLE_WEBGL_DESCR);
	sstr << checkbox(WebContent::ENABLE_POINT_CLOUD_RENDERER, "Enable Point Cloud Renderer", setup->enablePointCloudRenderer, ENABLE_POINT_CLOUD_RENDERER_DESCR);
	sstr << checkbox(WebContent::OVERRIDE_FOCAL_LENGTH, "Override Focal Length", setup->overrideFocalLength, OVERRIDE_FOCAL_LENGTH_DESCR);
	sstr << setting(WebContent::OVERRIDDEN_FOCAL_LENGTH, "Overridden Focal Length", setup->overriddenFocalLength, OVERRIDDEN_FOCAL_LENGTH_DESCR, "mm");
	sstr << checkbox(WebContent::ENABLE_EXPERIMENTAL, "Enable Experimental", setup->enableExperimental, ENABLE_EXPERIMENTAL_DESCR);
	sstr << checkbox(WebContent::FLIP_RED_BLUE, "Swap Red and Blue", setup->mmalFlipRedBlue, FLIP_RED_BLUE_DESCR);


	sstr << setting(WebContent::VERSION_NAME, "Firmware Version", FREELSS_VERSION_NAME, "The version of FreeLSS the scanner is running", "", true);
	sstr << setting(WebContent::FREE_DISK_SPACE, "Free Space", freeSpaceMb, "The amount of free disk space available", "MB", true);
	sstr << setting(WebContent::KERNEL_VERSION, "Kernel", kernelVersion.str(), "The version of the kernel", "", true);


	sstr << "<p><input type=\"hidden\" name=\"cmd\" value=\"save\">\
<input class=\"submit\" type=\"submit\" value=\"Save\">\
</p></form><p id=\"restoreDefaultsA\"><a href=\"/restoreDefaults\">Restore Factory Defaults</a></p></div>\
</body></html>";

	     return sstr.str();
}

std::string WebContent::settings(const std::string& message)
{
	const Setup * setup = Setup::get();
	UnitOfLength srcUnit = UL_MILLIMETERS; // Lengths are always represented in millimeters internally
	UnitOfLength dstUnit = setup->unitOfLength;

	const Preset& preset = PresetManager::get()->getActivePreset();

	std::stringstream sstr;
	sstr << "<!DOCTYPE html><html><head>"
		 << CSS
		 << std::endl
		 << JAVASCRIPT
		 << std::endl
		 << "</head>"
	     << "\
<body>\
<div id=\"menuContainer\">\
  <div class=\"menu\"><a href=\"/\">SCAN</a></div>\
  <div class=\"menu\"><a href=\"/cal1\">CAMERA</a></div>\
  <div class=\"menu\"><a href=\"/settings\">SETTINGS</a></div>"
	     << menu2()
	     << "</div>";

	if (!message.empty())
	{
		sstr << "<h2>" << message << "</h2>";
	}


	sstr << "<form action=\"/settings\" method=\"POST\" enctype=\"application/x-www-form-urlencoded\">";

	sstr << "\
	<div id=\"outerPresetDiv\">\
		<div id=\"presetTextDiv\">Preset</div>\
		<div style=\"float: left\"><input name=\""
		<< WebContent::PROFILE_NAME
		<< "\" value=\""
		<< preset.name
		<< "\" type=\"text\"></div>\
<div style=\"padding-left: 10px; float: left\"><input name=\"cmd\" class=\"submit\" value=\"Save\" type=\"submit\"></div>\
<div style=\"padding-left: 0px\"><input name=\"cmd\" class=\"submit\" value=\"Delete\" type=\"submit\"></div>";

	const std::vector<Preset>& profs = PresetManager::get()->getPresets();
	for (size_t iProf = 0; iProf < profs.size(); iProf++)
	{
		const Preset& prof = profs[iProf];

		sstr << "<div style=\"padding-left: 10px; padding-bottom: 10px\">"
		     << "<div style=\"width: 500px\">"
		     << (iProf + 1)
		     << ".<a href=\"/settings?cmd=activate&id="
		     << prof.id
		     << "\">&nbsp;&nbsp;"
		     << prof.name
		     << "</a></div>"
		     << "</div>";
	}

	sstr << "</div>";

	//
	// Laser selection UI
	//
	Laser::LaserSide laserSel = preset.laserSide;
	std::string leftSel  = laserSel == Laser::LEFT_LASER  ? " SELECTED" : "";
	std::string rightSel = laserSel == Laser::RIGHT_LASER ? " SELECTED" : "";
	std::string bothSel  = laserSel == Laser::ALL_LASERS  ? " SELECTED" : "";

	sstr << "<div><div class=\"settingsText\">Laser Selection</div>";
	sstr << "<select name=\"" << WebContent::LASER_SELECTION << "\">";
	sstr << "<option value=\"0\"" << leftSel << ">Left Laser</option>\r\n";
	sstr << "<option value=\"1\"" << rightSel << ">Right Laser</option>\r\n";
	sstr << "<option value=\"2\"" << bothSel << ">Both Lasers</option>\r\n";
	sstr << "</select></div>";
	sstr << "<div class=\"settingsDescr\">The laser(s) that will be used when scanning.</div>\n";


	//
	// Camera Mode UI
	//
	std::vector<CameraResolution> resolutions = Camera::getInstance()->getSupportedResolutions();

	sstr << "<div><div class=\"settingsText\">Camera Mode</div>";
	sstr << "<select name=\"" << WebContent::CAMERA_MODE<< "\">";
	for (size_t iRes = 0; iRes < resolutions.size(); iRes++)
	{
		CameraResolution& res = resolutions[iRes];

		std::string sel = res.cameraMode == preset.cameraMode ? " SELECTED" : "";
		sstr << "<option value=\"" << (int)res.cameraMode << "\"" << sel << ">" << res.name << "</option>\r\n";
	}

	sstr << "</select></div>";
	sstr << "<div class=\"settingsDescr\">The camera mode to use. Video mode is faster but Still mode results in higher quality scans.</div>\n";

	// Frames per revolution
	sstr << setting(WebContent::FRAMES_PER_REVOLUTION, "Frames Per Revolution", preset.framesPerRevolution, FRAMES_PER_REVOLUTION_DESCR);

	//
	// Exposure Setting
	//
	CameraExposureTime cameraExposureTime = preset.cameraExposureTime;
	std::string cetAutoSel      = cameraExposureTime == CET_AUTO ? " SELECTED" : "";
	std::string cetShortSel     = cameraExposureTime == CET_SHORT ? " SELECTED" : "";
	std::string cetVeryShortSel = cameraExposureTime == CET_VERYSHORT ? " SELECTED" : "";
	std::string cetMediumSel    = cameraExposureTime == CET_MEDIUM ? " SELECTED" : "";
	std::string cetLongSel      = cameraExposureTime == CET_LONG ? " SELECTED" : "";
	std::string cetVeryLongSel  = cameraExposureTime == CET_VERYLONG ? " SELECTED" : "";

	sstr << "<div><div class=\"settingsText\">Camera Exposure Time</div>";
	sstr << "<select name=\"" << WebContent::CAMERA_EXPOSURE_TIME<< "\">";
	sstr << "<option value=\"" << (int)CET_AUTO << "\"" << cetAutoSel << ">Auto</option>\r\n";
	sstr << "<option value=\"" << (int)CET_VERYSHORT << "\"" << cetVeryShortSel << ">Very Short</option>\r\n";
	sstr << "<option value=\"" << (int)CET_SHORT << "\"" << cetShortSel << ">Short</option>\r\n";
	sstr << "<option value=\"" << (int)CET_MEDIUM << "\"" << cetMediumSel << ">Medium</option>\r\n";
	sstr << "<option value=\"" << (int)CET_LONG << "\"" << cetLongSel << ">Long</option>\r\n";
	sstr << "<option value=\"" << (int)CET_VERYLONG << "\"" << cetVeryLongSel << ">Very Long</option>\r\n";
	sstr << "</select></div>";
	sstr << "<div class=\"settingsDescr\">" << CAMERA_EXPOSURE_TIME_DESCR << "</div>\n";

	//
	// Noise Removal Setting
	//
	NoiseRemover::Setting nrsSetting = preset.noiseRemovalSetting;
	std::string nrsDisabledSel = nrsSetting == NoiseRemover::NRS_DISABLED  ? " SELECTED" : "";
	std::string nrsLowSel      = nrsSetting == NoiseRemover::NRS_LOW  ? " SELECTED" : "";
	std::string nrsMediumSel   = nrsSetting == NoiseRemover::NRS_MEDIUM  ? " SELECTED" : "";
	std::string nrsHighSel     = nrsSetting == NoiseRemover::NRS_HIGH  ? " SELECTED" : "";

	sstr << "<div><div class=\"settingsText\">Noise Removal</div>";
	sstr << "<select name=\"" << WebContent::NOISE_REMOVAL_SETTING<< "\">";
	sstr << "<option value=\"" << (int)NoiseRemover::NRS_DISABLED << "\"" << nrsDisabledSel << ">Disabled</option>\r\n";
	sstr << "<option value=\"" << (int)NoiseRemover::NRS_LOW << "\"" << nrsLowSel << ">Low</option>\r\n";
	sstr << "<option value=\"" << (int)NoiseRemover::NRS_MEDIUM << "\"" << nrsMediumSel << ">Medium</option>\r\n";
	sstr << "<option value=\"" << (int)NoiseRemover::NRS_HIGH << "\"" << nrsHighSel << ">High</option>\r\n";
	sstr << "</select></div>";
	sstr << "<div class=\"settingsDescr\">" << NOISE_REMOVAL_SETTING_DESCR << "</div>\n";


	//
	// Image Threshold Mode
	//
	ImageProcessor::ThresholdMode imageThresholdMode = preset.imageThresholdMode;
	std::string itmStaticSel = imageThresholdMode == ImageProcessor::THM_STATIC  ? " SELECTED" : "";
	std::string itmLowSel    = imageThresholdMode == ImageProcessor::THM_LOW  ? " SELECTED" : "";
	std::string itmMediumSel = imageThresholdMode == ImageProcessor::THM_MEDIUM  ? " SELECTED" : "";
	std::string itmHighSel   = imageThresholdMode == ImageProcessor::THM_HIGH  ? " SELECTED" : "";

	sstr << "<div><div class=\"settingsText\">Image Threshold Mode</div>";
	sstr << "<select name=\"" << WebContent::IMAGE_THRESHOLD_MODE << "\">";
	sstr << "<option value=\"" << (int)ImageProcessor::THM_STATIC << "\"" << itmStaticSel << ">Static</option>\r\n";
	sstr << "<option value=\"" << (int)ImageProcessor::THM_LOW << "\"" << itmLowSel << ">Low</option>\r\n";
	sstr << "<option value=\"" << (int)ImageProcessor::THM_MEDIUM << "\"" << itmMediumSel << ">Medium</option>\r\n";
	sstr << "<option value=\"" << (int)ImageProcessor::THM_HIGH << "\"" << itmHighSel << ">High</option>\r\n";
	sstr << "</select></div>";
	sstr << "<div class=\"settingsDescr\">" << IMAGE_THRESHOLD_MODE_DESCR << "</div>\n";


	sstr << setting(WebContent::LASER_MAGNITUDE_THRESHOLD, "Laser Threshold", preset.laserThreshold, LASER_MAGNITUDE_THRESHOLD_DESCR);
	sstr << setting(WebContent::GROUND_PLANE_HEIGHT, "Ground Plane Height", ConvertUnitOfLength(preset.groundPlaneHeight, srcUnit, dstUnit), GROUND_PLANE_HEIGHT_DESCR,  ToString(dstUnit) + ".", false);
	sstr << setting(WebContent::STABILITY_DELAY, "Stability Delay", preset.stabilityDelay, STABILITY_DELAY_DESCR, "&mu;s");
	sstr << setting(WebContent::MAX_LASER_WIDTH, "Max Laser Width", preset.maxLaserWidth, MAX_LASER_WIDTH_DESCR, "px.");
	sstr << setting(WebContent::MIN_LASER_WIDTH, "Min Laser Width", preset.minLaserWidth, MIN_LASER_WIDTH_DESCR, "px.");


	//
	// PLY Data Format UI
	//
	PlyDataFormat plyDataFormat = preset.plyDataFormat;
	std::string plyAsciiSel  = plyDataFormat == PLY_ASCII  ? " SELECTED" : "";
	std::string plyBinarySel = plyDataFormat == PLY_BINARY ? " SELECTED" : "";

	sstr << checkbox(WebContent::GENERATE_PLY, "Generate PLY File", preset.generatePly, GENERATE_PLY_DESCR);

	sstr << "<div><div class=\"settingsText\">PLY Data Format</div>";
	sstr << "<select name=\"" << WebContent::PLY_DATA_FORMAT << "\">";
	sstr << "<option value=\"0\"" << plyAsciiSel << ">ASCII</option>\r\n";
	sstr << "<option value=\"1\"" << plyBinarySel << ">Binary</option>\r\n";
	sstr << "</select></div>";
	sstr << "<div class=\"settingsDescr\">" << WebContent::PLY_DATA_FORMAT_DESCR << "</div>\n";

	sstr << checkbox(WebContent::GENERATE_STL, "Generate STL File", preset.generateStl, GENERATE_STL_DESCR);
	sstr << checkbox(WebContent::GENERATE_XYZ, "Generate XYZ File", preset.generateXyz, GENERATE_XYZ_DESCR);
	sstr << checkbox(WebContent::SEPARATE_LASERS_BY_COLOR, "Separate the Lasers", preset.laserMergeAction == Preset::LMA_SEPARATE_BY_COLOR, SEPARATE_LASERS_BY_COLOR_DESCR);
	sstr << checkbox(WebContent::ENABLE_BURST_MODE, "Enable Burst Mode", preset.enableBurstModeForStillImages, ENABLE_BURST_MODE_DESCR);
	sstr << checkbox(WebContent::CREATE_BASE_FOR_OBJECT, "Create Base for Object", preset.createBaseForObject, CREATE_BASE_FOR_OBJECT_DESCR);

	sstr << "<p><br><br><a target=\"_\" href=\"/licenses.txt\">Licenses</a></p>";
	sstr << "</form>\
	<form action=\"/reboot\" method=\"POST\" enctype=\"application/x-www-form-urlencoded\">\
	<p><br><br><br><input class=\"submit\" type=\"submit\" value=\"Reboot\"></p></form>";

	sstr << "</form>\
<form action=\"/settings\" method=\"POST\" enctype=\"application/x-www-form-urlencoded\">\
<p><br><br><br><form> <input type=\"hidden\" name=\"cmd\" value=\"shutdown\">\
<input class=\"submit\" type=\"submit\" value=\"Shutdown\"></p></form>";

	sstr << "</body></html>";

	     return sstr.str();
}

std::string WebContent::setting(const std::string& name, const std::string& label,
			const std::string& value, const std::string& description, const std::string& units, bool readOnly, bool password)
{
	std::stringstream sstr;
	sstr << "<div><div class=\"settingsText\">"
		 << label
		 << "</div>";

	if (!readOnly)
	{
		sstr << "<input class=\"settingsInput\" value=\""
			 << value
			 << "\" name=\""
			 << name
			 << "\" ";

		if (password)
		{
			sstr << " type=\"password\"";
		}

		sstr << "> ";

		if (!units.empty())
		{
			sstr << units;
		}
	}
	else
	{
		sstr << "<div><small>" << value << "</small> ";

		if (!units.empty())
		{
			sstr << units;
		}
		sstr << "</div>";
	}

	sstr << "</div><div class=\"settingsDescr\">" << description << "</div>\n";


	return sstr.str();
}

std::string WebContent::checkbox(const std::string& name, const std::string& label, bool checked, const std::string& description)
{
	std::stringstream sstr;
	sstr << "<div><div class=\"settingsText\">"
		 << label
		 << "</div><input class=\"settingsInput\" name=\""
		 << name
		 << "\" type=\"checkbox\"";

	if (checked)
	{
		sstr << " checked";
	}

	sstr << "> ";

	sstr << "</div><div class=\"settingsDescr\">" << description << "</div>\n";


	return sstr.str();
}

std::string WebContent::setting(const std::string& name, const std::string& label,
		int value, const std::string& description, const std::string& units, bool readOnly)
{
	std::stringstream sstr;
	sstr << value;
	return setting(name, label, sstr.str(), description, units, readOnly);
}

std::string WebContent::setting(const std::string& name, const std::string& label,
		real value, const std::string& description, const std::string& units, bool readOnly)
{
	std::stringstream sstr;
	sstr << value;
	return setting(name, label, sstr.str(), description, units, readOnly);
}

std::string WebContent::exposureDiv(int value, const std::string& label, bool floatLeft)
{
	std::stringstream sstr;

	sstr << "<div style=\"float: " << (floatLeft ? "left" : "none") << "\">"
		 << "<form action=\"/cal1\" method=\"POST\" enctype=\"application/x-www-form-urlencoded\">"
	     << "<input style=\"width: 90px\" type=\"Submit\" value=\"" << label << "\">"
         << "<input type=\"hidden\" name=\"cmd\" value=\"setCameraExposureTime\">"
         << "<input type=\"hidden\" name=\"" << WebContent::CAMERA_EXPOSURE_TIME << "\" value=\"" << value << "\">"
         << "</form></div>";

	return sstr.str();
}
}
