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
#include "Settings.h"
#include "Laser.h"
#include "Camera.h"

namespace scanner
{

const std::string WebContent::CSS = "\
<style type=\"text/css\">\
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
	width: 100%;\
	margin: auto;\
	text-align: center;\
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
    <div><div class=\"settingsText\">Detail</div><input name=\"detail\" class=\"settingsInput\" value=\"800\"> samples per revolution</div>\
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
	     << "<p><a href=\"/\">Refresh</a></p>"
	     << "<form action=\"/\" method=\"POST\" enctype=\"application/x-www-form-urlencoded\">"
	     << "<input type=\"hidden\" name=\"cmd\" value=\"stopScan\">"
	     << "<input type=\"submit\" value=\"Stop Scan\">"
	     << "</form>"
	     << "</body></html>";

	     return sstr.str();
}

std::string WebContent::cal1()
{
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
</div>\
<div id=\"cal1ImageDiv\">\
	<div style=\"position: absolute\">\
		<img onload=\"setTimeout(updateImgImage, 0);\" id=\"image\" width=\"1296\" src=\"camImage_\""
	<< time(NULL)
	<< ".jpg\">\
	</div>\
	<div id=\"vLine\"> </div>\
	<div id=\"hLine\"> </div>\
</div></body></html>";

	     return sstr.str();
}

std::string WebContent::settings(const std::string& message, const std::string& rotationDegrees)
{

	Settings * settings = Settings::get();
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
<h1>Settings </h1>\
<form action=\"/settings\" method=\"POST\" enctype=\"application/x-www-form-urlencoded\">\
<input type=\"hidden\" name=\"cmd\" value=\"toggleLeftLaser\">\
<input class=\"submit\" type=\"submit\" value=\"Toggle Left Laser\">\
</form>\
 <form action=\"/settings\" method=\"POST\" enctype=\"application/x-www-form-urlencoded\">\
 <input type=\"hidden\" name=\"cmd\" value=\"toggleRightLaser\">\
 <input class=\"submit\" type=\"submit\" value=\"Toggle Right Laser\">\
 </form>\
 <form action=\"/settings\" method=\"POST\" enctype=\"application/x-www-form-urlencoded\">\
  <input type=\"hidden\" name=\"cmd\" value=\"disableMotor\">\
  <input class=\"submit\" type=\"submit\" value=\"Disable Motor\">\
  </form>\
 <form action=\"/settings\" method=\"POST\" enctype=\"application/x-www-form-urlencoded\">\
  <input type=\"hidden\" name=\"cmd\" value=\"autoCalibrate\">\
  <input class=\"submit\" type=\"submit\" value=\"Auto-Calibrate\">\
  </form>\
 <form action=\"/settings\" method=\"POST\" enctype=\"application/x-www-form-urlencoded\">\
  <input type=\"hidden\" name=\"cmd\" value=\"generateDebug\">\
  <input class=\"submit\" type=\"submit\" value=\"Generate Debug Data\">\
  </form>\
<div>\
  <a href=\"/dbg/1.jpg\">Base</a>\
  <a href=\"/dbg/2.jpg\">Laser On</a>\
  <a href=\"/dbg/3.png\">Difference</a>\
  <a href=\"/dbg/4.png\">Laser</a>\
  <a href=\"/dbg/5.png\">Overlay</a>\
  <a href=\"/dbg/0.csv\">Difference Values</a>\
</div><div><form action=\"/settings\" method=\"POST\" enctype=\"application/x-www-form-urlencoded\">\
<input type=\"hidden\" name=\"cmd\" value=\"rotateTable\">\
<input name=\"degrees\" class=\"settingsInput\" type=\"text\" value=\""
	     << rotationDegrees << "\"> degrees\
<input class=\"submit\" type=\"submit\" value=\"Rotate Table\">\
</form></div>\
	     ";
if (!message.empty())
	{
		sstr << "<h2>" << message << "</h2>";
	}


	sstr << "<form action=\"/settings\" method=\"POST\" enctype=\"application/x-www-form-urlencoded\">";
	sstr << "<p><input class=\"submit\" type=\"submit\" value=\"Save\"></p>";

	//
	// Laser selection UI
	//
	int laserSel = settings->readInt(Settings::GENERAL_SETTINGS, Settings::LASER_SELECTION);
	std::string leftSel  = laserSel == (int)Laser::LEFT_LASER  ? " SELECTED" : "";
	std::string rightSel = laserSel == (int)Laser::RIGHT_LASER ? " SELECTED" : "";
	std::string bothSel  = laserSel == (int)Laser::ALL_LASERS  ? " SELECTED" : "";

	sstr << "<div><div class=\"settingsText\">Laser Selection</div>";
	sstr << "<select name=\"" << Settings::LASER_SELECTION << "\">";
	sstr << "<option value=\"0\"" << leftSel << ">Left Laser</option>\r\n";
	sstr << "<option value=\"1\"" << rightSel << ">Right Laser</option>\r\n";
	sstr << "<option value=\"2\"" << bothSel << ">Both Lasers</option>\r\n";
	sstr << "</select></div>";
	sstr << "<div class=\"settingsDescr\">The laser(s) that will be used when scanning.</div>\n";

	//
	// Camera Mode UI
	//
	int cameraMode = settings->readInt(Settings::GENERAL_SETTINGS, Settings::CAMERA_MODE);
	std::string still5Sel  = cameraMode == (int)Camera::CM_STILL_5MP  ? " SELECTED" : "";
	std::string video5Sel = cameraMode == (int)Camera::CM_VIDEO_5MP ? " SELECTED" : "";
	std::string videoHDSel = cameraMode == (int)Camera::CM_VIDEO_HD ? " SELECTED" : "";
	std::string video1P2Sel = cameraMode == (int)Camera::CM_VIDEO_1P2MP ? " SELECTED" : "";
	std::string videoVGASel = cameraMode == (int)Camera::CM_VIDEO_VGA ? " SELECTED" : "";

	sstr << "<div><div class=\"settingsText\">Camera Mode</div>";
	sstr << "<select name=\"" << Settings::CAMERA_MODE<< "\">";
	sstr << "<option value=\"" << (int)Camera::CM_STILL_5MP << "\"" << still5Sel << ">5 Megapixel (still mode, 2592x1944)</option>\r\n";
	sstr << "<option value=\"" << (int)Camera::CM_VIDEO_5MP << "\"" << video5Sel << ">5 Megapixel (video mode, 2592x1944)</option>\r\n";
	sstr << "<option value=\"" << (int)Camera::CM_VIDEO_HD << "\"" << videoHDSel << ">1.9 Megapixel (video mode, 1600x1200)</option>\r\n";
	sstr << "<option value=\"" << (int)Camera::CM_VIDEO_1P2MP << "\"" << video1P2Sel << ">1.2 Megapixel (video mode, 1280x960)</option>\r\n";
	sstr << "<option value=\"" << (int)Camera::CM_VIDEO_VGA << "\"" << videoVGASel << ">0.3 Megapixel (video mode, 640x480)</option>\r\n";
	sstr << "</select></div>";
	sstr << "<div class=\"settingsDescr\">The camera mode to use. Video mode is faster but Still mode results in higher quality scans.</div>\n";

	sstr << setting(Settings::CAMERA_X, "Camera X", (real)(settings->readReal(Settings::GENERAL_SETTINGS, Settings::CAMERA_X) / 25.4f), CAMERA_X_DESCR, "in.", true);
	sstr << setting(Settings::CAMERA_Y, "Camera Y", (real)(settings->readReal(Settings::GENERAL_SETTINGS, Settings::CAMERA_Y) / 25.4f), CAMERA_Y_DESCR,  "in.");
	sstr << setting(Settings::CAMERA_Z, "Camera Z", (real)(settings->readReal(Settings::GENERAL_SETTINGS, Settings::CAMERA_Z) / 25.4f), CAMERA_Z_DESCR,  "in.");
	sstr << setting(Settings::RIGHT_LASER_X, "Right Laser X", (real)(settings->readReal(Settings::GENERAL_SETTINGS, Settings::RIGHT_LASER_X) / 25.4f), RIGHT_LASER_X_DESCR,  "in.");
	sstr << setting(Settings::RIGHT_LASER_Y, "Right Laser Y", (real)(settings->readReal(Settings::GENERAL_SETTINGS, Settings::RIGHT_LASER_Y) / 25.4f), RIGHT_LASER_Y_DESCR,  "in.", true);
	sstr << setting(Settings::RIGHT_LASER_Z, "Right Laser Z", (real)(settings->readReal(Settings::GENERAL_SETTINGS, Settings::RIGHT_LASER_Z) / 25.4f), RIGHT_LASER_Z_DESCR,  "in.", true);
	sstr << setting(Settings::LEFT_LASER_X, "Left Laser X", (real)(settings->readReal(Settings::GENERAL_SETTINGS, Settings::LEFT_LASER_X) / 25.4f), LEFT_LASER_X_DESCR,  "in.");
	sstr << setting(Settings::LEFT_LASER_Y, "Left Laser Y", (real)(settings->readReal(Settings::GENERAL_SETTINGS, Settings::LEFT_LASER_Y) / 25.4f), LEFT_LASER_Y_DESCR,  "in.", true);
	sstr << setting(Settings::LEFT_LASER_Z, "Left Laser Z", (real)(settings->readReal(Settings::GENERAL_SETTINGS, Settings::LEFT_LASER_Z) / 25.4f), LEFT_LASER_Z_DESCR,  "in.", true);
	sstr << setting(Settings::LASER_MAGNITUDE_THRESHOLD, "Laser Threshold", (real)settings->readReal(Settings::GENERAL_SETTINGS, Settings::LASER_MAGNITUDE_THRESHOLD), LASER_MAGNITUDE_THRESHOLD_DESCR);
	sstr << setting(Settings::LASER_DELAY, "Laser Delay", settings->readInt(Settings::GENERAL_SETTINGS, Settings::LASER_DELAY), LASER_DELAY_DESCR, "&mu;s");
	sstr << setting(Settings::RIGHT_LASER_PIN, "Right Laser Pin", settings->readInt(Settings::GENERAL_SETTINGS, Settings::RIGHT_LASER_PIN), RIGHT_LASER_PIN_DESCR);
	sstr << setting(Settings::LEFT_LASER_PIN, "Left Laser Pin", settings->readInt(Settings::GENERAL_SETTINGS, Settings::LEFT_LASER_PIN), LEFT_LASER_PIN_DESCR);
	sstr << setting(Settings::LASER_ON_VALUE, "Laser On Value", settings->readInt(Settings::GENERAL_SETTINGS, Settings::LASER_ON_VALUE), LASER_ON_VALUE_DESCR);
	sstr << setting(Settings::STABILITY_DELAY, "Stability Delay", settings->readInt(Settings::GENERAL_SETTINGS, Settings::STABILITY_DELAY), STABILITY_DELAY_DESCR, "&mu;s");
	sstr << setting(Settings::MAX_LASER_WIDTH, "Max Laser Width", settings->readInt(Settings::GENERAL_SETTINGS, Settings::MAX_LASER_WIDTH), MAX_LASER_WIDTH_DESCR, "px.");
	sstr << setting(Settings::MIN_LASER_WIDTH, "Min Laser Width", settings->readInt(Settings::GENERAL_SETTINGS, Settings::MIN_LASER_WIDTH), MIN_LASER_WIDTH_DESCR, "px.");
	sstr << setting(Settings::STEPS_PER_REVOLUTION, "Steps Per Revolution", settings->readInt(Settings::GENERAL_SETTINGS, Settings::STEPS_PER_REVOLUTION), STEPS_PER_REVOLUTION_DESCR);
	sstr << setting(Settings::ENABLE_PIN, "Motor Enable Pin", settings->readInt(Settings::A4988_SETTINGS, Settings::ENABLE_PIN), ENABLE_PIN_DESCR);
	sstr << setting(Settings::STEP_PIN, "Motor Step Pin", settings->readInt(Settings::A4988_SETTINGS, Settings::STEP_PIN), STEP_PIN_DESCR);
	sstr << setting(Settings::STEP_DELAY, "Motor Step Delay", settings->readInt(Settings::A4988_SETTINGS, Settings::STEP_DELAY), STEP_DELAY_DESCR, "&mu;s");
	sstr << setting(Settings::DIRECTION_PIN, "Motor Direction Pin", settings->readInt(Settings::A4988_SETTINGS, Settings::DIRECTION_PIN), DIRECTION_PIN_DESCR);
	sstr << setting(Settings::RESPONSE_DELAY, "Motor Response Delay", settings->readInt(Settings::A4988_SETTINGS, Settings::RESPONSE_DELAY), RESPONSE_DELAY_DESCR, "&mu;s");

	sstr << "<p><input type=\"hidden\" name=\"cmd\" value=\"save\">\
<input class=\"submit\" type=\"submit\" value=\"Save\">\
</p></form>\
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
