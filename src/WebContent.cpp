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
#include "WebContent.h"
#include "PresetManager.h"
#include "Setup.h"
#include "Laser.h"
#include "Camera.h"

namespace freelss
{

const std::string WebContent::CSS = "\
<style type=\"text/css\">\
.menu2 {\
	float: right;\
	width: 350px;\
	height: 20px;\
	padding-bottom: 5px;\
	margin-right: 20px;\
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
#vLine {\
	position: absolute;\
	background-color: red; \
	margin-left: 648px; \
	width: 1px; \
	height: 972px;\
}\
#hLine {\
	position: absolute;\
	background-color: red;\
	margin-top: 486px;\
	width: 1296px;\
	height: 1px;\
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
#cal1GenerateDebugDiv {\
	padding-bottom: 50px;\
}\
#cal1ImageDiv {\
	float: none;\
	margin-top: 75px;\
}\
#outerImageDiv {\
	float: none;\
	margin-top: 75px;\
	width: 1296px;\
	height: 972px;\
	background-size: contain;\
	background-repeat: no-repeat;\
	background-image: url('skull.jpg');\
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
</style>";

const std::string WebContent::JAVASCRIPT= "\
<script type=\"text/javascript\">\
var imageCount = 0;\
\
function endsWith(str, suffix) {\
    return str.indexOf(suffix, str.length - suffix.length) !== -1;\
}\
\
function startup1(){\
	setTimeout(updateImgImage, 1000);\
}\
function startup2(){\
	setTimeout(updateDivImage, 1000);\
}\
function updateImgImage(){\
	var img = document.getElementById('image');\
	img.src = 'camImage_' + Math.random() + '.jpg';\
	imageCount = imageCount + 1;\
}\
function updateDivImage(){\
	var div = document.getElementById('outerImageDiv');\
	div.style.backgroundImage = \"url('camImage_\" + Math.random() + \".jpg')\";\
	imageCount = imageCount + 1;\
	setTimeout(updateDivImage, 4000);\
}\
function centerTableClick(event)\
{\
	var pos_x = event.offsetX?(event.offsetX):event.pageX-document.getElementById('outerImageDiv').offsetLeft;\
	var pos_y = event.offsetY?(event.offsetY):event.pageY-document.getElementById('outerImageDiv').offsetTop;\
	window.location.href = 'cal3?y=' + pos_y;\
}\
function rightLaserClick(event) {\
    var pos_x = event.offsetX?(event.offsetX):event.pageX-document.getElementById('outerImageDiv').offsetLeft;\
    var pos_y = event.offsetY?(event.offsetY):event.pageY-document.getElementById('outerImageDiv').offsetTop;\
	window.location.href = 'cal4?rightLaserX=' + pos_x + '&rightLaserY=' + pos_y;\
}\
function leftLaserClick(event) {\
	var pos_x = event.offsetX?(event.offsetX):event.pageX-document.getElementById('outerImageDiv').offsetLeft;\
	var pos_y = event.offsetY?(event.offsetY):event.pageY-document.getElementById('outerImageDiv').offsetTop;\
	window.location.href = 'cal5?leftLaserX=' + pos_x + '&leftLaserY=' + pos_y;\
}\
</script>";

const std::string WebContent::PROFILE_NAME = "PROFILE_NAME";
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
const std::string WebContent::LASER_DELAY = "LASER_DELAY";
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
const std::string WebContent::LASER_DELAY_DESCR = "The time to delay after turning the laser on or off";
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

std::string WebContent::scan(const std::vector<ScanResult>& pastScans)
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
</div>\
<p>Click the button to start the scan </p>\
<form action=\"/\" method=\"POST\" enctype=\"application/x-www-form-urlencoded\">\
<div><div class=\"settingsText\">Degrees</div><input name=\"degrees\" class=\"settingsInput\" value=\"360\"> degrees</div>\
	<input type=\"hidden\" name=\"cmd\" value=\"startScan\">\
	<input class=\"submit\" type=\"submit\" value=\"Start Scan\">\
</form>";

	for (size_t iRt = 0; iRt < pastScans.size(); iRt++)
	{
		sstr << WebContent::scanResult(iRt + 1, pastScans[iRt]);
	}

	sstr << "</body></html>";

	return sstr.str();
}

std::string WebContent::scanResult(size_t index, const ScanResult& result)
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
	}

	sstr << "<br><br>\
	<form action=\"" << deleteUrl.str() << "#\" method=\"POST\">\
	<input type=\"Submit\" value=\"Delete\">\
	</form>\
	<div style=\"font-size: 12px\">" << dateStr << "\
	</div></div>\
	</span>\
	<br>";

	return sstr.str();
}

std::string WebContent::scanRunning(real progress, real remainingTime)
{
	std::stringstream sstr;
	sstr << "<!DOCTYPE html><html><head>"
		 << "<meta http-equiv=\"refresh\" content=\"5\">"
		 << CSS
		 << std::endl
		 << JAVASCRIPT
		 << std::endl
		 << "</head>"
	     << "<body><p>Scan is "
	     << progress
	     << "% complete with "
	     << (remainingTime / 60.0)
	     << " minutes remaining.</p>"
	     << "<form action=\"/\" method=\"POST\" enctype=\"application/x-www-form-urlencoded\">"
	     << "<input type=\"hidden\" name=\"cmd\" value=\"stopScan\">"
	     << "<input type=\"submit\" value=\"Stop Scan\">"
	     << "</form>"
	     << "</body></html>";

	     return sstr.str();
}

std::string WebContent::cal1(const std::string& inMessage)
{
	std::string message = inMessage.empty() ? std::string("") : ("<h2>" + inMessage + "</h2>");

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
</div>"
	     << message
         << "\
<div id=\"cal1ContentDiv\">\
  <div id=\"cal1ControlsDiv\">\
    <div id=\"cal1GenerateDebugDiv\">\
		<form action=\"/cal1\" method=\"POST\" enctype=\"application/x-www-form-urlencoded\">\
			<input name=\"cmd\" value=\"generateDebug\" type=\"hidden\">\
			<input style=\"height: 60px\" class=\"controlSubmit\" value=\"Test\" type=\"submit\">\
		</form>\
    </div>\
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
		<div class=\"settingsDescr\">Rotates the turntable in degrees.</div>\
  </div>\
<div style=\"position: relative\">\
	<div>\
		<img style=\"position: absolute\" onload=\"setTimeout(updateImgImage, 0);\" id=\"image\" width=\"1296\" src=\"camImage_\""
	<< time(NULL)
	<< ".jpg\">\
	</div>\
	<div style=\"margin-left: 215px;\">\
	  <div id=\"vLine\"> </div>\
	  <div id=\"hLine\"> </div>\
    </div>\
</div>\
</div></body></html>";

	     return sstr.str();
}
std::string WebContent::setup(const std::string& message)
{
	const Setup * setup = Setup::get();

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
</div>";

	if (!message.empty())
	{
		sstr << "<h2>" << message << "</h2>";
	}


	sstr << "<form action=\"/setup\" method=\"POST\" enctype=\"application/x-www-form-urlencoded\">";
	sstr << "<p><input class=\"submit\" type=\"submit\" value=\"Save\"></p>";

	sstr << setting(WebContent::CAMERA_X, "Camera X", setup->cameraLocation.x / 25.4f, CAMERA_X_DESCR, "in.", true);
	sstr << setting(WebContent::CAMERA_Y, "Camera Y", setup->cameraLocation.y / 25.4f, CAMERA_Y_DESCR,  "in.");
	sstr << setting(WebContent::CAMERA_Z, "Camera Z", setup->cameraLocation.z / 25.4f, CAMERA_Z_DESCR,  "in.");
	sstr << setting(WebContent::RIGHT_LASER_X, "Right Laser X", setup->rightLaserLocation.x / 25.4f, RIGHT_LASER_X_DESCR,  "in.");
	sstr << setting(WebContent::RIGHT_LASER_Y, "Right Laser Y", setup->rightLaserLocation.y / 25.4f, RIGHT_LASER_Y_DESCR,  "in.", true);
	sstr << setting(WebContent::RIGHT_LASER_Z, "Right Laser Z", setup->rightLaserLocation.z / 25.4f, RIGHT_LASER_Z_DESCR,  "in.", true);
	sstr << setting(WebContent::LEFT_LASER_X, "Left Laser X", setup->leftLaserLocation.x / 25.4f, LEFT_LASER_X_DESCR,  "in.");
	sstr << setting(WebContent::LEFT_LASER_Y, "Left Laser Y", setup->leftLaserLocation.y / 25.4f, LEFT_LASER_Y_DESCR,  "in.", true);
	sstr << setting(WebContent::LEFT_LASER_Z, "Left Laser Z", setup->leftLaserLocation.z / 25.4f, LEFT_LASER_Z_DESCR,  "in.", true);

	sstr << setting(WebContent::RIGHT_LASER_PIN, "Right Laser Pin", setup->rightLaserPin, RIGHT_LASER_PIN_DESCR);
	sstr << setting(WebContent::LEFT_LASER_PIN, "Left Laser Pin", setup->leftLaserPin, LEFT_LASER_PIN_DESCR);
	sstr << setting(WebContent::LASER_ON_VALUE, "Laser On Value", setup->laserOnValue, LASER_ON_VALUE_DESCR);

	sstr << setting(WebContent::STEPS_PER_REVOLUTION, "Steps Per Revolution", setup->stepsPerRevolution, STEPS_PER_REVOLUTION_DESCR);
	sstr << setting(WebContent::ENABLE_PIN, "Motor Enable Pin", setup->motorEnablePin, ENABLE_PIN_DESCR);
	sstr << setting(WebContent::STEP_PIN, "Motor Step Pin", setup->motorStepPin, STEP_PIN_DESCR);
	sstr << setting(WebContent::STEP_DELAY, "Motor Step Delay", setup->motorStepDelay, STEP_DELAY_DESCR, "&mu;s");
	sstr << setting(WebContent::DIRECTION_PIN, "Motor Direction Pin", setup->motorDirPin, DIRECTION_PIN_DESCR);
	sstr << setting(WebContent::RESPONSE_DELAY, "Motor Response Delay", setup->motorResponseDelay, RESPONSE_DELAY_DESCR, "&mu;s");

	sstr << "<p><input type=\"hidden\" name=\"cmd\" value=\"save\">\
<input class=\"submit\" type=\"submit\" value=\"Save\">\
</p></form>\
</body></html>";

	     return sstr.str();
}

std::string WebContent::settings(const std::string& message)
{

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
  <div class=\"menu\"><a href=\"/settings\">SETTINGS</a></div>\
  <div class=\"menu2\"><a href=\"/setup\"><small><small>Setup</small></small></a></div>\
</div>";

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
	Camera::CameraMode cameraMode = preset.cameraMode;
	std::string still5Sel   = cameraMode == Camera::CM_STILL_5MP  ? " SELECTED" : "";
	std::string video5Sel   = cameraMode == Camera::CM_VIDEO_5MP ? " SELECTED" : "";
	std::string videoHDSel  = cameraMode == Camera::CM_VIDEO_HD ? " SELECTED" : "";
	std::string video1P2Sel = cameraMode == Camera::CM_VIDEO_1P2MP ? " SELECTED" : "";
	std::string videoVGASel = cameraMode == Camera::CM_VIDEO_VGA ? " SELECTED" : "";

	sstr << "<div><div class=\"settingsText\">Camera Mode</div>";
	sstr << "<select name=\"" << WebContent::CAMERA_MODE<< "\">";
	sstr << "<option value=\"" << (int)Camera::CM_STILL_5MP << "\"" << still5Sel << ">5 Megapixel (still mode, 2592x1944)</option>\r\n";
	sstr << "<option value=\"" << (int)Camera::CM_VIDEO_5MP << "\"" << video5Sel << ">5 Megapixel (video mode, 2592x1944)</option>\r\n";
	sstr << "<option value=\"" << (int)Camera::CM_VIDEO_HD << "\"" << videoHDSel << ">1.9 Megapixel (video mode, 1600x1200)</option>\r\n";
	sstr << "<option value=\"" << (int)Camera::CM_VIDEO_1P2MP << "\"" << video1P2Sel << ">1.2 Megapixel (video mode, 1280x960)</option>\r\n";
	sstr << "<option value=\"" << (int)Camera::CM_VIDEO_VGA << "\"" << videoVGASel << ">0.3 Megapixel (video mode, 640x480)</option>\r\n";
	sstr << "</select></div>";
	sstr << "<div class=\"settingsDescr\">The camera mode to use. Video mode is faster but Still mode results in higher quality scans.</div>\n";

	sstr << setting(WebContent::FRAMES_PER_REVOLUTION, "Frames Per Revolution", preset.framesPerRevolution, FRAMES_PER_REVOLUTION_DESCR);
	sstr << setting(WebContent::LASER_MAGNITUDE_THRESHOLD, "Laser Threshold", preset.laserThreshold, LASER_MAGNITUDE_THRESHOLD_DESCR);
	sstr << setting(WebContent::LASER_DELAY, "Laser Delay", preset.laserDelay, LASER_DELAY_DESCR, "&mu;s");
	sstr << setting(WebContent::STABILITY_DELAY, "Stability Delay", preset.stabilityDelay, STABILITY_DELAY_DESCR, "&mu;s");
	sstr << setting(WebContent::MAX_LASER_WIDTH, "Max Laser Width", preset.maxLaserWidth, MAX_LASER_WIDTH_DESCR, "px.");
	sstr << setting(WebContent::MIN_LASER_WIDTH, "Min Laser Width", preset.minLaserWidth, MIN_LASER_WIDTH_DESCR, "px.");
	sstr << checkbox(WebContent::GENERATE_PLY, "Generate PLY File", preset.generatePly, GENERATE_PLY_DESCR);
	sstr << checkbox(WebContent::GENERATE_STL, "Generate STL File", preset.generateStl, GENERATE_STL_DESCR);
	sstr << checkbox(WebContent::GENERATE_XYZ, "Generate XYZ File", preset.generateXyz, GENERATE_XYZ_DESCR);
	sstr << checkbox(WebContent::SEPARATE_LASERS_BY_COLOR, "Separate the Lasers", preset.laserMergeAction == Preset::LMA_SEPARATE_BY_COLOR, SEPARATE_LASERS_BY_COLOR_DESCR);

	sstr << "</form>\
<form action=\"/settings\" method=\"POST\" enctype=\"application/x-www-form-urlencoded\">\
<p><br><br><br><form> <input type=\"hidden\" name=\"cmd\" value=\"shutdown\">\
<input class=\"submit\" type=\"submit\" value=\"Shutdown\"></p></form>\
</body></html>";

	     return sstr.str();
}

std::string WebContent::setting(const std::string& name, const std::string& label,
			const std::string& value, const std::string& description, const std::string& units, bool readOnly)
{
	std::stringstream sstr;
	sstr << "<div><div class=\"settingsText\">"
		 << label
		 << "</div><input class=\"settingsInput\" value=\""
		 << value
		 << "\" name=\""
		 << name
		 << "\" ";

	if (readOnly)
	{
		sstr << "readonly=\"true\"";
	}

	sstr << "> ";

	if (!units.empty())
	{
		sstr << units;
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

}
