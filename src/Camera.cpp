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
#include "Camera.h"
#include "MockCamera.h"
#include "PresetManager.h"
#include "Thread.h"
#include "Logger.h"
#include "Setup.h"

#ifndef MOCK
#include "MmalStillCamera.h"
#include "MmalVideoCamera.h"
#endif

namespace freelss
{

#ifdef MOCK
typedef MockCamera MmalStillCamera;
typedef MockCamera MmalVideoCamera;
#endif

CriticalSection Camera::m_cs;
Camera * Camera::m_instance = NULL;

CameraAcquisitionType GetCameraType(CameraMode cameraMode)
{
	CameraAcquisitionType type = CT_UNKNOWN;
	switch (cameraMode)
	{
	case CM_STILL_5MP:
		type = CT_MMALSTILL;
		break;

	case CM_VIDEO_5MP:
		type = CT_MMALVIDEO;
		break;

	case CM_VIDEO_HD:
		type = CT_MMALVIDEO;
		break;

	case CM_VIDEO_1P2MP:
		type = CT_MMALVIDEO;
		break;

	case CM_VIDEO_VGA:
		type = CT_MMALVIDEO;
		break;

	case CM_STILL_8MP:
		type = CT_MMALSTILL;
		break;

	case CM_STILL_VGA:
		type = CT_MMALSTILL;
		break;

	case CM_STILL_HD:
		type = CT_MMALSTILL;
		break;

	default:
		throw Exception("Unsupported Camera Mode");
	}

	return type;
}

CameraResolution CreateResolution(int width, int height, int frameRate, CameraAcquisitionType cameraType, CameraMode cameraMode, const std::string& name)
{
	CameraResolution res;

	std::string modeName;

	switch (cameraType)
	{
	case CT_UNKNOWN:
		modeName = "unknown mode";
		break;

	case CT_MMALSTILL:
		modeName = "still mode";
		break;

	case CT_MMALVIDEO:
		modeName = "video mode";
		break;

	default:
		throw Exception("Unsupported camera mode");
	}

	std::stringstream fullName;
	fullName << name << "(" << modeName << "," << width << "x" << height << ")";

	res.width = width;
	res.height = height;
	res.frameRate = frameRate;
	res.cameraType = cameraType;
	res.cameraMode = cameraMode;
	res.name = fullName.str();

	return res;
}
// 5MP Video - 360000 laser delay
// 5MP Still -  90000 laser delay
// 1.9MP     -  90000 laser delay
// 1.2MP     -  90000 laser delay
// 0.3MP     -  90000 laser delay

Camera::Camera() :
	m_name("UNKNOWN"),
	m_supportedResolutions(),
	m_resolution(),
	m_nextAcquisitionTimeSec(0),
	m_sensorWidth(1.0),
	m_sensorHeight(1.0),
	m_focalLength(1.0)
{
	// Do nothing
}

Camera::~Camera()
{
	// Do nothing
}

Camera * Camera::getInstance()
{
	Preset preset = PresetManager::get()->getActivePreset();

	// Read the camera mode from the preset
	CameraMode cameraMode = preset.cameraMode;

	bool initialized = true;
	m_cs.enter();

	try
	{
		if (m_instance == NULL)
		{
			CameraAcquisitionType cameraType = GetCameraType(cameraMode);

			if (cameraType == CT_MMALSTILL)
			{
				MmalStillCamera * camera = new MmalStillCamera();
				camera->setFlipRedBlue(Setup::get()->mmalFlipRedBlue);
				camera->initialize(cameraMode);
				preset.cameraMode = camera->getCameraResolution().cameraMode;
				camera->setBurstModeEnabled(preset.enableBurstModeForStillImages);
				m_instance = camera;

				CameraResolution resolution = m_instance->getCameraResolution();
				InfoLog << "Created camera: " << resolution.name << Logger::ENDL;
			}
			else if (cameraType == CT_MMALVIDEO)
			{
				MmalVideoCamera * camera = new MmalVideoCamera();
				camera->setFlipRedBlue(Setup::get()->mmalFlipRedBlue);
				camera->initialize(cameraMode);
				preset.cameraMode = camera->getCameraResolution().cameraMode;
				m_instance = camera;

				CameraResolution resolution = m_instance->getCameraResolution();
				InfoLog << "Created camera: " << resolution.name << Logger::ENDL;
			}
			else
			{
				throw Exception("Unsupported camera type");
			}

			// Set the exposure time
			m_instance->setShutterSpeed(convertToShutterSpeed(preset.cameraExposureTime));
		}
	}
	catch (Exception& ex)
	{
		ErrorLog << "Error creating camera, error=" << ex << ", cameraMode=" << (int) cameraMode << Logger::ENDL;
		initialized = false;
	}
	catch (...)
	{
		ErrorLog << "Error creating camera, cameraMode=" << (int) cameraMode << Logger::ENDL;
		initialized = false;
	}

	if (!initialized)
	{
		// Initialize the mock camera if there was a problem
		delete m_instance;
		m_instance = NULL;

		MockCamera * camera = new MockCamera();
		camera->initialize(CM_VIDEO_VGA);
		m_instance = camera;
	}

	m_cs.leave();

	return m_instance;
}

void Camera::release()
{
	m_cs.enter();
	try
	{
		delete m_instance;
		m_instance = NULL;
	}
	catch (...)
	{
		m_cs.leave();
		throw;
	}
	m_cs.leave();
}

void Camera::reinitialize()
{
	m_cs.enter();
	try
	{
		// Set the new type
		delete m_instance;
		m_instance = NULL;
	}
	catch (...)
	{
		m_cs.leave();
		throw;
	}
	m_cs.leave();

	// Create an instance of the new type
	getInstance();
}


real Camera::getSensorWidth() const
{
	return m_sensorWidth;
}

real Camera::getSensorHeight() const
{
	return m_sensorHeight;
}

real Camera::getFocalLength() const
{
	return m_focalLength;
}

std::string Camera::getName() const
{
	return m_name;
}

const std::vector<CameraResolution>& Camera::getSupportedResolutions() const
{
	return m_supportedResolutions;
}

void Camera::setExposureTime(CameraExposureTime exposureTime)
{
	unsigned shutterSpeed = convertToShutterSpeed(exposureTime);

	m_cs.enter();

	try
	{
		setShutterSpeed(shutterSpeed);
	}
	catch (...)
	{
		m_cs.leave();
		throw;
	}

	m_cs.leave();
}

const CameraResolution& Camera::getCameraResolution() const
{
	return m_resolution;
}

unsigned Camera::convertToShutterSpeed(CameraExposureTime exposureTime)
{
	unsigned shutterSpeedUs;

	switch (exposureTime)
	{
	case CET_AUTO:
		shutterSpeedUs = 0;
		break;

	case CET_VERYSHORT:
		shutterSpeedUs = 2500;
		break;

	case CET_SHORT:
		shutterSpeedUs = 5000;
		break;

	case CET_MEDIUM:
		shutterSpeedUs = 15000;
		break;

	case CET_LONG:
		shutterSpeedUs = 50000;
		break;

	case CET_VERYLONG:
		shutterSpeedUs = 150000;
		break;

	default:
		throw Exception("Unsupported camera exposure time: " + ToString(exposureTime));
	}

	return shutterSpeedUs;
}

bool Camera::acquireJpeg(byte* buffer, unsigned * size)
{
	// Acquire an image
	Image * image = NULL;
	bool retVal = false;

	try
	{
		image = acquireImage();

		if (image == NULL)
		{
			throw Exception("NULL image in Camera::acquireJpeg");
		}

		// Make sure the buffer is big enough
		if (* size >= image->getPixelBufferSize())
		{
			// Convert the image to a JPEG
			Image::convertToJpeg(* image, buffer, size);
			retVal = true;
		}
		else
		{
			* size = image->getPixelBufferSize();
			retVal = false;
		}

		releaseImage(image);
	}
	catch (...)
	{
		releaseImage(image);
		throw;
	}

	return retVal;
}

void Camera::setAcquisitionDelay(double acquisitionDelaySec)
{
	double seconds = GetTimeInSeconds() + acquisitionDelaySec;
	m_nextAcquisitionTimeSec = MAX(m_nextAcquisitionTimeSec, seconds);
}

void Camera::handleAcquisitionDelay()
{
	double sleepTime = m_nextAcquisitionTimeSec - GetTimeInSeconds();
	if (sleepTime > 0)
	{
		//InfoLog << "Camera::handleAcquisitionDelay() - Sleeping for " << sleepTime << " seconds" << std::endl;
		Thread::usleep((unsigned long) (sleepTime * 1000000.0));
		m_nextAcquisitionTimeSec = 0;
	}
}

void Camera::setSensorProperties(real sensorWidth, real sensorHeight, real focalLength)
{
	m_sensorWidth = sensorWidth;
	m_sensorHeight = sensorHeight;
	m_focalLength = focalLength;

	Setup * setup = Setup::get();
	std::string focalStr = TrimString(setup->overriddenFocalLength);
	if (setup->overrideFocalLength && focalStr.length() > 0)
	{
		m_focalLength = ToReal(focalStr);
		InfoLog << "Overriding focal length with " << m_focalLength << " mm." << Logger::ENDL;
	}
}
}
